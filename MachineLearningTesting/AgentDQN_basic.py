import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import numpy as np
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras.optimizers import Adam
from tensorflow.keras.models import load_model
from tensorflow.keras.layers import Dense
import gym
import matplotlib.pyplot as plt
import time
import math
from tinymlgen import port
import pickle
import shutil

# we are creating our own memory and agent classes rather than using tf agents to facilitate
# easy swapping between simulated and real environments, where the data would be read off of disk
# and the update rate would be much delayed from the aquisition of data

# for a real-world mag lev experiement, this would need to load test data from
# disk, after it had been downloaded from the micro. For simulations, it just
# collects and holds sim data.
class replayBuffer():
	def __init__(self, mem_size, input_dims):
		self.mem_size = mem_size
		self.mem_counter = 0 # where is our last entered memory, ie, where does the next one go?

		self.state_memory = np.zeros((self.mem_size, *input_dims), dtype=np.float32) # this keeps track of 
			# our state history. input_dims is how many inputs the state provides
		self.new_state_memory = np.zeros((self.mem_size, *input_dims), dtype=np.float32) # this keeps track
			# of what state we encountered next, as a result of the action we took. Seems like a lot of duplicated
			# data from state_memory, since they should mostly be just offset by one, but this allows multiple
			# discrete events to be recorded to the memory buffer, and makes it a little more straightforward when
			# we go to sample from the memory buffer for learning
		self.action_memory = np.zeros(self.mem_size, dtype=np.int32) # which action did we take? only 1 dimensional,
			# since actions are discrete and therefore fully encodable in an int.
		self.reward_memory = np.zeros(self.mem_size, dtype=np.float32) # what reward did we get for the exact action
			# we took?
		self.terminal_memory = np.zeros(self.mem_size, dtype=np.int32) # when did discrete events start and end?
			# this is super inefficent at storing this information and is mostly an array of ones (a 0 anytime it 
			# event ended), but makes computing the summed future rewards much simpler as we know when to terminate
			# the rewards, as we don't want to take credit for rewards from future events.

	# for real world data, this would read off of disk
	def store_transition(self, state, action, reward, new_state, done):
		if done != 2:
			index = self.mem_counter % self.mem_size # wrap around if we reach the end of the buffer.
			self.state_memory[index]= state
			self.new_state_memory[index] = new_state
			self.reward_memory[index] = reward
			self.action_memory[index] = action
			self.terminal_memory[index] = 1 - int(done)
			self.mem_counter += 1

	# sample the buffer for the agent to use a random subset for learning. Do
	# not grab data past the counter if this is the first time filling up the buffer.
	def sample_buffer(self, batchSize): 
		max_mem_location = min(self.mem_size,self.mem_counter) # since mem_counter doesn't ever reset,
			# we know we haven't filled up the buffer if it is less than mem_size
		batchIndexes = np.random.choice(max_mem_location, batchSize, replace=False)
			# generate an array of batchSize elements, where the elements are non-repeating integers between
				# zero and max_mem_location. Essentially, a list of indexes that we will pull for the sample buffer.
				# the replace=False ensures all indexes are unique.
		# now lets grab the samples from our memory
		sampleStates = self.state_memory[batchIndexes]
		sampleNewStates = self.new_state_memory[batchIndexes]
		sampleRewards = self.reward_memory[batchIndexes]
		sampleActions = self.action_memory[batchIndexes]
		sampleTerminations = self.terminal_memory[batchIndexes]

		return sampleStates, sampleNewStates, sampleRewards, sampleActions, sampleTerminations

# this only gets run at the VERY beginning of training. After that, training should save the partially trained model
# to disk and reload as needed, rather than generate new ones, if real world training takes more than one session (it will)
# because of this, we don't bother to make this fancy. It's hard coded and not an object.
def build_dqn(n_actions, lr, input_dims,layerSizeList):
	model= keras.Sequential()
	isFirstLayer = True
	for unitCount in layerSizeList:
		if isFirstLayer:
			isFirstLayer = False
			layer = Dense(unitCount, input_dim=np.squeeze(input_dims), activation='relu')
		else:
			layer = Dense(unitCount, activation='relu')
		model.add(layer)
	lastLayer = Dense(n_actions, activation=None)#tanh
	model.add(lastLayer)
	#return(model)

	"""
	# things to play with: dropout, l2 normalization, model structure (try a LSTM?)
	model= keras.Sequential()
	model.add(keras.layers.Dense(layerUnits, activation = 'relu', input_dim=np.squeeze(input_dims)))
	for j in range(layerCount-1):
		model.add(keras.layers.Dense(layerUnits, activation = 'relu'))
	
	#model.add(keras.layers.Dense(128, activation = 'relu', input_dim=np.squeeze(input_dims)))
	#model.add(keras.layers.Dense(128, activation = 'relu'))
	#model.add(keras.layers.Dense(128, activation = 'relu'))
	#model.add(keras.layers.Dense(64, activation = 'relu'))
	#model.add(keras.layers.Dense(64, activation = 'relu'))
	#model.add(keras.layers.Dense(32, activation = 'relu'))
	#model.add(keras.layers.Dense(32, activation = 'relu'))

	model.add(keras.layers.Dense(n_actions, activation = None)) # number of available actions
	"""

	model.compile(optimizer=Adam(learning_rate=lr), loss='mean_squared_error')
	# we use a very small learning rate because it is likely our sample is not well
	# distributed. We'd rather take many samples and lots of data for this problem
	return(model)

# for now, we hardcode a lot of the hyperparameters for transparency.
class AgentDQN_basic():
	def loadActor(self, uniqueID):
		self.uniqueID = uniqueID
		print('... loading entire actor/ RL state ...')
		self.getSaveDirectoryFromUniqueID()
		self.loadStoredData()
		self.load_models()
		self.summary()

	def __init__(self, state_dim=5, action_dim=3, batchSize= 64, lr=.0001, tau=.05, gamma=.95, epsilon=1, eps_dec=.99, learnInterval='STEP', isDual=False, 
				isDueling=False, isPER=False,gameName='mavLevSim',mem_size=1000000,layerSizeList=[64,64,64],usePruning=False, minEpsilon=.3,uniqueID=None):
		if uniqueID != None:
			self.loadActor(uniqueID)
		else:
			self.tau=tau # not used in simple model, just implemented for compatibility
			self.learnInterval = learnInterval
			self.epsilon=epsilon
			self.epsilon_decay = eps_dec
			self.action_space = [i for i in range(action_dim)] # a simple list [1,2,3,4,...] for num actions, to enable
				# easy swapping between one-hot, probabilities (values, remember this is DQN not PGN), and an integer for the action space.
				# we can also use this list for easy selecting of random actions
			self. gamma = gamma
			self.gameName = gameName
			self.memory = replayBuffer(mem_size, state_dim)
			self.q_network = build_dqn(action_dim, lr, state_dim,layerSizeList)
			self.n_actions = action_dim
			self.input_dims = state_dim
			self.layerSizeList = layerSizeList
			self.lr=lr
			self.batchSize=batchSize
			self.epsilonInitial = epsilon
			self.minEpsilon = minEpsilon
			self.usePruning=usePruning
			self.scores = []
			self.timeStepNumLearns = 0
			self.timeStepNumActionsChoosen = 0
			self.epsilonHistory = []

			# housekeeping for logging and saving models and progress
			self.getUniqueID()
			self.saveDirectory = "results/dqn/"
			if self.uniqueID < 10: self.saveDirectory += "00"
			elif self.uniqueID < 100: self.saveDirectory += "0"
			self.saveDirectory += str(self.uniqueID) + "_" + str(self.gameName) + ("_lr" + str(self.lr) +
					"_LI" + str(self.learnInterval) + '_bs' + str(self.batchSize) + '_g' +
					str(self.gamma) + "_network" + self.getNetworkAsString())
			os.mkdir(self.saveDirectory)

	def wipeMemory(self):
		memSize = self.memory.mem_size
		self.memory = replayBuffer(memSize, self.input_dims)

	def summary(self):
		print('... Information about DQN actor, network, and training progress ...')
		print('Game Name: ' + str(self.gameName))
		print('Actor Unique ID: ' + str(self.uniqueID))
		print('... Hyperparameters ...')
		print('lr: ' + str(self.lr))
		print('gamma: ' + str(self.gamma))
		print('batch size: ' + str(self.batchSize))
		print('learn interval (externally enforced, only used for naming conventions): ' + str(self.learnInterval))
		print('input dims: ' + str(self.input_dims))
		print('action dims: ' + str(self.n_actions))
		print('... Network Information ...')
		print('network design: ' + str(self.getNetworkAsString()))
		print('... Learning Progress ...')
		print('number of actions decided: ' + str(self.timeStepNumActionsChoosen))
		print('number of times agent has learned on a batch: ' + str(self.timeStepNumLearns))
		print('memory filled: ' + str(self.memory.mem_counter) + "/" + str(self.memory.mem_size) + 
				", " + str(int(100*self.memory.mem_counter/self.memory.mem_size)) + "%")
		print('number of epochs/ number of recorded game scores: ' + str(len(self.scores)))
		print('average score of last 100 games: ' + str(self.getAveScore()))

	def getSaveDirectoryFromUniqueID(self):
		list_subfolders = [f.name for f in os.scandir('results/dqn/') if f.is_dir()]
		searchForID  =-1
		for subfolder in list_subfolders:
			if int(subfolder[0:3]) == self.uniqueID:
				self.saveDirectory = 'results/dqn/' + str(subfolder)
				searchForID += 1
		if searchForID == -1:
			print("...MAJOR ERROR, SELECTED UNIQUE ID DOES NOT EXIST...")
		elif searchForID > 0:
			print("... ERROR WE FOUND MORE THAN 1 DIRECTORY WITH THE UNIQUE ID ...")

	def getUniqueID(self):
		list_subfolders = [f.name for f in os.scandir('results/dqn/') if f.is_dir()]
		uniqueIDs = []
		for subfolder in list_subfolders:
			if 'placeholder' not in subfolder:
				uniqueIDs.append(int(subfolder[0:3]))
		if uniqueIDs == []:
			self.uniqueID = 0
		else:
			self.uniqueID = max(uniqueIDs)+1

	def getNetworkAsString(self):
		networkString=""
		isFirstLayer = True
		for layer in self.layerSizeList:
			if isFirstLayer:
				isFirstLayer = False
			else:
				networkString += "-"
			networkString += str(layer)
		return(networkString)

	# for actual deployment on a micro, this is not really needed. This is forward inference (or random),
	# and this is the part that needs to be replicated on 
	def chooseAction(self, observation):
		if np.random.random() < self.epsilon: # hardcoded epsilon with no change. Does this do anything?
			# might be interesting to explore a case where we force a series of random numbers, or
			# use the same random number multiple steps in a row, to force putting us in a bad case.
			# otherwise, we force exploring the state space by injecting changes (sine waves, step functions)
			# in the target state, which is also part of the recorded state.
			action = np.random.choice(self.action_space)
		else:
			stateAsArray = np.array([observation]) # also adds a dimension as the NN expects to see a batch dimension
			# MONSTER SPEED DIFFERENCE: with eager execute OFF, predict is 8 seconds for some benchmark, 35 seconds with eager execute ON
			# with eager execute ON, () is 3 seconds for same benchmark. () won't run with eager execute OFF due to tensor/numpy conversions (I think)
			#actions = self.q_network.predict(stateAsArray)
			actions = self.q_network(stateAsArray)
			action=np.argmax(actions) # since the NN returns scores, we want the best score
		self.timeStepNumActionsChoosen += 1
		return(action)

	def learn(self, numLearns=1, customBatchSize=None):
		if customBatchSize == None:
			localBatchSize = self.batchSize
		else:
			localBatchSize = customBatchSize
			if self.memory.mem_counter < localBatchSize: # we'd get strange errors if we tried to train before we had enough entries in 
				# our memory to fill a batch
				return
		self.timeStepNumLearns += 1
		sampleStates, sampleNewStates, sampleRewards, sampleActions, sampleTerminations = self.memory.sample_buffer(localBatchSize)

		# for each of the states in our sample, how do we want the predictions we make to change? well, for any actions we didn't take,
		# we want them to stay the same since we don't have any more information. For the action we took, we want the prediction to 
		# adjust toward the real value.
		#q_predictions = self.q_network.predict(sampleStates)
		#q_next_predictions = self.q_network.predict(sampleNewStates)
		q_predictions = self.q_network(sampleStates)
		q_next_predictions = self.q_network(sampleNewStates)
		# so above are the predictions we would have made in our old state. If we were using a 2-network approach, we'd used the locked
		# network to generate these, while the active network is free to learn. depending on epsilon and random numbers, we may or may
		# not have followed our advice and taken the best action based on the above predictions
		q_target = np.copy(q_predictions)
		batch_index = np.arange(localBatchSize, dtype=np.int32) # This creates a properly formated array of the indexes of the samples in the batch, ie,
			# [1,2,3,4,...]
		q_target[batch_index, sampleActions] = sampleRewards + self.gamma*np.max(q_next_predictions, axis=1)*sampleTerminations
		# this is sort of clunky, sort of brilliant, and is at the heart of how this algorithem can work without summing up future rewards or being dependant on the policy.
			# IF the algorithem is working (which assuming it converges, eventually it will work), then the predictions are accurately predicting
			# the rewards of the actions for now + all future states (with decay) (assuming we follow the policy), so the ACTUAL reward we received (accounting for all
			# future gains) is just the real reward from now plus the predicted reward from next (depreciated by gamma)- we don't need any real future rewards! 
			# It's completely bootstrapped! no adding up future rewards, etc. In some ways, it's actually better because if future rewards were generated
			# by random actions, we don't really want heavy penalties to the now action. Of course, it leads to instabilities too/ makes early training data
			# unusual (dangerous?) to keep around, since action and reward are only half of the story. Our predicted reward is also how we guess at our real reward,
			# which is dependant on our policy, which may have changed since we first used the old training data (so this could make it oscillate).
			# the sampleTerminations means that if a state was our last one, then the reward does not include any future rewards, the game is over.

		# So how do we update our model? Well, we made some predicitons, we copy those predictions, we update the prediction for the choice we actually took based
			# on the reward we actually got and the reward we predict for the next state our choice took us to, then we compare the predictions with this new modified
			# target and perform gradient descent. Simple!

		# a version of this that computes reward sums by actually summing real rewards might be interesting. It won't work without a full greedy strategy (or at least
			# using reward expectations as relative chances) or future rewards aren't really policy dependant, but it might make things more stable? (still need to 
			# ditch early training data as we wouldn't have accurately followed the newest policy . . . in some ways this would make it start looking more like PGD)

		self.q_network.train_on_batch(sampleStates, q_target)
		if self.epsilon > self.minEpsilon:
			self.epsilon *= self.epsilon_decay

	def saveMemory(self, state, action, reward, done, new_state):
		self.memory.store_transition(state, action, reward, new_state, done)

	"""
	# are we supposed to save the target model or the training model?
	def getFilename(self, filename=None, filenameAppendage=None, intelligentFilename = True, directory = None):
		if directory != None:
			filename = directory + "/"
		else:
			filename = ""
		if intelligentFilename == True:
			filename += "DQNbasic"
			filename += ("_lr" + str(self.lr) + "_LI" + str(self.learnInterval) + '_bs' + str(self.batchSize) + '_g' +
				str(self.gamma) + '_e' + str(self.epsilonInitial) + '_t' + str(self.tau) + "_network" + str(self.layerCount) + "x" + str(self.layerUnits) + "_")
			filename += self.filename
			if self.usePruning:
				filename += "PRUNED_"
			if filenameAppendage!= None:
				filename += filenameAppendage

		else:
			if filename == None:
				if filenameAppendage == None:
					filename += self.filename				
				else:
					filename += self.filename + filenameAppendage
		return(filename)
	# are we supposed to save the target model or the training model?
	def save_model(self, filename=None, filenameAppendage=None, intelligentFilename = True, directory = None):
		filename = self.getFilename(filename=filename, filenameAppendage=filenameAppendage, intelligentFilename = intelligentFilename, directory = directory)
		self.q_network.save(filename + '.h5')

	def load_model(self):
		self.q_network = load_model(self.filename + '.h5')
	"""

	def save_models(self):
		print('... saving model ...')
		self.q_network.save(self.saveDirectory + '/q_model.h5')

	def load_models(self):
		print('... loading model ...')
		self.q_network = load_model(self.saveDirectory + '/q_model.h5')

	def logScore(self, score):
		self.scores.append(score)
		self.epsilonHistory.append(self.epsilon)

	def save_all(self):
		print('... saving entire actor/ RL state ...')
		afile = open(self.saveDirectory + '/hyperparams.pkl', 'wb')
		pickle.dump((self.layerSizeList,self.lr,self.input_dims,self.gamma,self.batchSize,self.timeStepNumLearns,
				self.timeStepNumActionsChoosen,self.n_actions,self.learnInterval, self.epsilon, self.epsilon_decay,
				self.minEpsilon, self.gameName,self.scores, self.epsilonHistory), afile)
		afile.close()		
		afile = open(self.saveDirectory + '/memory.pkl', 'wb')
		pickle.dump(self.memory,afile)
		afile.close()
		self.save_models()
		plt.figure()
		plt.plot(self.scores)
		if len(self.scores) > 0:
			minValue = min(self.scores)
			maxValue = max(self.scores)
			epsilonPlotting = []
			for epsilon in self.epsilonHistory:
				epsilonPlotting.append(epsilon*(maxValue-minValue)+minValue)
			plt.plot(epsilonPlotting)
		plt.savefig(self.saveDirectory + '/RESULTS_epochs' + str(len(self.scores)) + '_score' + str(self.getAveScore()) + '.png')
		self.saveModelForTFLite()

	def saveModelForTFLite(self):
		with open(self.saveDirectory + '/TeensyModel.h', 'w') as f:
			text = port(self.q_network, optimize=False, pretty_print=True, variable_name='modelParams')
			text = text.replace(' int ', ' long ') # model size may be too big to describe length in int
			text = text.replace('const', '') # for live updates, this array needs to be modifyable
			f.write(text)

	def getTFLiteModelPath(self):
		return(self.saveDirectory + '/TeensyModel.h')

	def getAveScore(self):
		return(np.mean(self.scores[-100:]))

	def loadStoredData(self):
		afile = open(self.saveDirectory + '/hyperparams.pkl', 'rb')
		self.layerSizeList,self.lr,self.input_dims,self.gamma,self.batchSize,self.timeStepNumLearns,\
				self.timeStepNumActionsChoosen,self.n_actions,self.learnInterval, self.epsilon, self.epsilon_decay,\
				self.minEpsilon, self.gameName,self.scores, self.epsilonHistory = pickle.load(afile)
		afile.close()
		afile = open(self.saveDirectory + '/memory.pkl', 'rb')
		self.memory = pickle.load(afile)
		afile.close()

	def archive(self):
		print("... Archiving Model At " + str(len(self.scores)) + " Epochs ...")
		self.save_all()
		archiveDir = "/ARCHIVE_epoch" + str(len(self.scores))
		os.mkdir(self.saveDirectory + archiveDir)
		#shutil.copytree(self.saveDirectory + "/modelWeights", self.saveDirectory + archiveDir + "/modelWeights")
		#shutil.copytree(self.saveDirectory + "/models", self.saveDirectory + archiveDir + "/models")
		shutil.copy2(self.saveDirectory + '/memory.pkl', self.saveDirectory + archiveDir)
		shutil.copy2(self.saveDirectory + '/hyperparams.pkl', self.saveDirectory + archiveDir)
		shutil.copy2(self.saveDirectory + '/RESULTS_epochs' + str(len(self.scores)) + '_score' + str(self.getAveScore()) + '.png', 
				self.saveDirectory + archiveDir)
		shutil.copy2(self.saveDirectory + '/q_model.h5', self.saveDirectory + archiveDir)
		shutil.copy2(self.saveDirectory + '/TeensyModel.h', self.saveDirectory + archiveDir)

# OBSOLETE Only being kept around for comments and reference. See TrainOnCartpoleFile
def TrainOnCartPole(LearnInterval='STEP',runNumber=0, showFigures=False, lr=.0001,n_games=150,batchSize = 256,gamma = .95,epsilon = .1):
	gameName = 'CartPole-v1'
	env=gym.make(gameName)
	
	
	load_model=False
	numTrainsPerReplay = 3000
	
	
	
	gameStartNumber = 0
	filename = (gameName + "_lr" + str(lr) + "_nGames" + str(n_games) + "_LInterval" + LearnInterval + '_batchSize' + str(batchSize) + '_gamma' +
			str(gamma) + '_epsilon' + str(epsilon) + '_run' + str(runNumber))
	agent=AgentDQN(input_dims=(np.squeeze(env.observation_space.shape)+1,), # + 1 for commanded location
				n_actions=env.action_space.n, # WTF is the .n all about?
				gamma=gamma, lr=lr, batchSize= batchSize, mem_size=1000000, fname=filename, epsilon=epsilon)
	
	scores = []
	customScores = []
	lastLearnIndex = 0
	numberOfGamesPerTrainingLoop = [0]
	episodeNumbers = []

	# game loop!
	for i in range(gameStartNumber, n_games):
		if n_games-i < 100: # for the final 100 games that give us an average score, stop exploring
			agent.epsilon = 0
		done = False
		score = 0
		customScore = 0
		observation = env.reset()
		steps=0
		targetPosition=math.sin(steps/60.0)/2
		targetPosition = -.5
		observation = np.append(observation,observation[0]-targetPosition)
		
		#targetPosition = .5
		while not done:
			#env.render()
			#time.sleep(.100)
			steps+=1
			#if steps > 100:
			#	targetPosition = 0
			#else:
			#	targetPosition=math.sin(steps/60.0)
			#targetPosition=math.sin(steps/60.0)/2
			
			action = agent.choose_action(observation)
			nextObservation, reward, done, info = env.step(action)
			nextObservation = np.append(nextObservation,nextObservation[0]-targetPosition)
			#if done == 1 and steps>199:
			#	done=2
			#print(nextObservation)
			newPosition = nextObservation[0] #-2.4 to 2.4, target 0
			newVelocity = nextObservation[1] # -inf to inf, target 0
			newPoleAngle = nextObservation[2] # -.2 to .2 rad, target 0
			newPoleVelocity = nextObservation[3]
			poleAngle = observation[2]
			if newPosition < -2.4 or newPosition > 2.4 or newPoleAngle < -.4 or newPoleAngle > .4:
				done = 1
				reward = 0
			elif steps > 499:
				done = 2
				reward = 0
			else:
				done = 0
				reward = 1
			# track a unique setpoint
			#cost = (1.2-abs(abs(newPosition)-targetPosition))**2+(12*(.1-abs(newPoleAngle)))**2 # THIS IS HORRIBLY BROKEN IT GIVES WRONG REWARDS MATH IS BAD
			cost = 2-(newPosition-targetPosition)**2-(50*(newPoleAngle**2)) # position error of 1.4 out of 4.8 (+/-2.4) zeros out positive reward,
					# angle error of 12 degrees (.2 rad) zeros out positive reward (either or, together they would make it negative), so if one of these
					# conditions is reached and there is no clear path to returning to stability, it would be concidered advantageous to crash to get the 
					# 0 reward of ending. So maybe we want a larger positive reward per cycle?
			#cost = (1.2-abs(newPosition))+20*(.1-abs(newPoleAngle)) # priority is keep pole upright, after that look for centering the cart. 
				# in a game with fixed episodes, reward must be positive, or agent learns to end the game to avoid further penalty
				# 12* would make them evenly valid targets, so 20 makes pole angle more valuable. The offset allows a very very bad value
				# to be slightly negative. Eliminating healthy game ends from the dataset allows this to work.
			customReward = cost
			#reward = -100*(abs(newPoleAngle) - abs(poleAngle)) # this was provided by someone online. It penalizes delta angle (angle change speed, not velocity)
			#customReward = reward # use the default
			# we train on our custom reward, so feature engineering can allow faster training/
			# learning specific behavior (like keeping the carpole centered in addition to keeping)
			# it alive, but we also track our progress on the original score to see how well we play
			# the game
			agent.memory.store_transition(observation,action,customReward,nextObservation,done)
			score += reward
			customScore += customReward
			observation=nextObservation # this gets us ready to choose an action based on the new environment

			if LearnInterval == 'STEP':
				agent.learn() # if we want to learn EVERY STEP
		if LearnInterval == 'GAME':
			agent.learn() # if we want to learn EVERY GAME. Probably want to learn multiple times if this is the case.
		if LearnInterval == 'EPISODE':
			if agent.memory.mem_counter-3000>lastLearnIndex: 
				# learn only inbetween games, and only if sufficient data has been collected.
					# but when this case occurs, learn many times. this is more similar to how real world learning will go
					# on a real system, since it's impractical to learn every timestep- you must collect data then report 
					# back and learn in a block
				lastLearnIndex=agent.memory.mem_counter
				for counter in range(numTrainsPerReplay): # 100*256 is 25.6k, so on average each sample gets pulled 2.5 times (assuming we learn
					# only from most recent samples, which, of course, we don't). This is a little on the low side.
					agent.learn()
				numberOfGamesPerTrainingLoop.append(i)

		scores.append(score)
		customScores.append(customScore)
		episodeNumbers.append(i)
		ave_score=np.mean(scores[-100:])
		print('episode: ', i, '/', n_games, 'score %.2f' % score, 'average_score %.2f' % ave_score, 'custom score %.2f' % customScore)

	print('max number of entries in replay buffer: ' + str(agent.memory.mem_counter))
	agent.save_model(filenameAppendage='_score' + str(ave_score))

	plt.plot(episodeNumbers,scores)
	plt.savefig(filename + '_score' + str(ave_score) + '_SCORES' + '.png')
	plt.figure()
	plt.plot(numberOfGamesPerTrainingLoop)
	plt.savefig(filename + '_score' + str(ave_score) + '_TRAININTERVALS' + '.png')
	plt.figure()
	plt.plot(episodeNumbers,customScores)
	plt.savefig(filename + '_score' + str(ave_score) + '_CUSTOMSCORES' + '.png')
	
	if showFigures==True:
		plt.show()




if __name__ == '__main__':
	gpus = tf.config.experimental.list_physical_devices('GPU')
	for gpu in gpus:
		tf.config.experimental.set_memory_growth(gpu, True)
	tf.compat.v1.disable_eager_execution()
	#TrainOnCartPole()

	#TrainOnCartPole(LearnInterval='STEP',runNumber=1100, showFigures=False, lr=.0001,n_games=500,batchSize = 256,gamma = .97,epsilon = .2)

	#visualizeCartPoleModel('CartPole-v0_lr0.0001_nGames1500_LIntervalSTEP_batchSize256_gamma0.95_epsilon0.1_run103_score187.59')
	#visualizeCartPoleModel('CartPole-v0_lr0.0001_nGames1500_LIntervalSTEP_batchSize256_gamma0.95_epsilon0.1_run202_score200.0')
	#visualizeCartPoleModel('CartPole-v1_lr0.0001_nGames1000_LIntervalSTEP_batchSize256_gamma0.97_epsilon0.2_run400_score196.14')
	#visualizeCartPoleModel('CartPole-v1_lr0.0001_nGames1000_LIntervalSTEP_batchSize256_gamma0.97_epsilon0.2_run600_score224.0')
	#visualizeCartPoleModel('CartPole-v1_lr0.0001_nGames1000_LIntervalSTEP_batchSize256_gamma0.97_epsilon0.2_run700_score499.0')
	#visualizeCartPoleModel('CartPole-v1_lr0.0001_nGames500_LIntervalSTEP_batchSize256_gamma0.97_epsilon0.2_run1000_score499.0')
# 

# Python dqn_V0.py