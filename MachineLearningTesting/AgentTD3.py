
import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import numpy as np
import tensorflow as tf
import tensorflow.keras as keras
from tensorflow.keras.layers import Dense
from tensorflow.keras.optimizers import Adam
import os
from tensorflow.keras.models import load_model
import matplotlib.pyplot as plt
import pickle
import shutil

class ReplayBuffer():
	def __init__(self, max_size, input_shape, n_actions):
		self.mem_size = max_size
		self.mem_cntr = 0
		self.state_memory = np.zeros((self.mem_size, *input_shape))
		self.new_state_memory = np.zeros((self.mem_size, *input_shape))
		self.action_memory = np.zeros((self.mem_size, n_actions))
		self.reward_memory = np.zeros(self.mem_size)
		self.terminal_memory = np.zeros(self.mem_size, dtype=np.bool)

	def store_transition(self, state, action, reward, state_, done):
		index = self.mem_cntr % self.mem_size
		self.state_memory[index] = state
		self.new_state_memory[index] = state_
		self.terminal_memory[index] = done
		self.reward_memory[index] = reward
		self.action_memory[index] = action

		self.mem_cntr += 1

	def sample_buffer(self, batch_size):
		max_mem = min(self.mem_cntr, self.mem_size)

		batch = np.random.choice(max_mem, batch_size)

		states = self.state_memory[batch]
		states_ = self.new_state_memory[batch]
		actions = self.action_memory[batch]
		rewards = self.reward_memory[batch]
		dones = self.terminal_memory[batch]

		return states, actions, rewards, states_, dones


class CriticNetwork(keras.Model):
	def __init__(self, layerSizeList, n_stateVariables, n_actions, name, save_dir='tmp/td3'):
		super(CriticNetwork, self).__init__()
		self.layerSizeList = layerSizeList
		self.n_stateVariables = n_stateVariables
		self.model_name = name
		self.save_dir = save_dir
		self.save_file = os.path.join(self.save_dir, name+'_td3')
		self.modelLayers = []

		isFirstLayer = True
		for unitCount in self.layerSizeList:
			if isFirstLayer:
				isFirstLayer = False
				layer = Dense(unitCount, input_dim=np.squeeze(self.n_stateVariables)+np.squeeze(n_actions), activation='relu')
			else:
				layer = Dense(unitCount, activation='relu')
			self.modelLayers.append(layer)
		finalLayer = Dense(1, activation=None)
		self.modelLayers.append(finalLayer)

	def call(self, state, action):
		state_action_value = tf.concat([state, action], axis=1)

		for layer in self.modelLayers:
			state_action_value = layer(state_action_value)
		return state_action_value # note the bad naming convention here- since state_action
			# has gone through the network, it's actually value now


class ActorNetwork(keras.Model):
	def __init__(self, layerSizeList, n_stateVariables, n_actions, name, save_dir='tmp/td3'):
		super(ActorNetwork, self).__init__()

		self.modelLayers = []
		self.layerSizeList = layerSizeList

		self.n_actions = n_actions
		self.n_stateVariables = n_stateVariables

		self.model_name = name
		self.save_dir = save_dir
		self.save_file = os.path.join(self.save_dir, name+'_td3')

		isFirstLayer = True
		for unitCount in self.layerSizeList:
			if isFirstLayer:
				isFirstLayer = False
				layer = Dense(unitCount, input_dim=np.squeeze(self.n_stateVariables), activation='relu')
			else:
				layer = Dense(unitCount, activation='relu')
			self.modelLayers.append(layer)
		lastLayer = Dense(self.n_actions, activation='tanh')
		self.modelLayers.append(lastLayer)

	# execute forward inference
	def call(self, state):
		for layer in self.modelLayers:
			state = layer(state)
		return state # note the bad naming convention here- since state has gone through the network, it's actually action now

class Agent():
	def loadActor(self, uniqueID):
		self.uniqueID = uniqueID
		print('... loading entire actor/ RL state ...')
		self.getSaveDirectoryFromUniqueID()
		self.loadStoredData()
		#self.createModels()
		self.load_models()
		self.summary()

	def __init__(self, uniqueID = None, alpha=.001, beta=.001, input_dims=5, tau=.01, game='game',
			gamma=0.99, update_actor_interval=2, learnInterval = 1,
			n_actions=2, max_size=1000000, layerSizeList=[128,128], batch_size=128, noise=0.05):
		if uniqueID != None:
			self.loadActor(uniqueID)
		else:
			# save the hyperparameters
			self.alpha = alpha
			self.beta = beta
			self.input_dims = input_dims
			self.gamma = gamma
			self.tau = tau
			self.memory = ReplayBuffer(max_size, input_dims, n_actions)
			self.batch_size = batch_size
			self.timeStepNumLearns = 0
			self.timeStepNumActionsChoosen = 0
			self.n_actions = n_actions
			self.update_actor_iter = update_actor_interval
			self.learnInterval = learnInterval
			self.game = game
			self.noise = noise
			self.scores=[]
			self.customScores = []
			self.layerSizeList = layerSizeList

			# housekeeping for logging and saving models and progress
			self.getUniqueID()
			self.saveDirectory = "results/TD3/"
			if self.uniqueID < 10: self.saveDirectory += "00"
			elif self.uniqueID < 100: self.saveDirectory += "0"
			self.saveDirectory += str(self.uniqueID) + "_" + str(self.game) + ("_lr" + str(self.alpha) + "-" + 
					str(self.beta) + "_LI" + str(self.learnInterval) + '_bs' + str(self.batch_size) + '_g' +
					str(self.gamma) + '_t' + str(self.tau) + "_n" + str(self.noise) + "_network" + self.getNetworkAsString())
			os.mkdir(self.saveDirectory)
			#os.mkdir(self.saveDirectory + "/modelWeights")
			os.mkdir(self.saveDirectory + "/models") 

			self.createModels()
			# force hard copy
			self.update_network_parameters(tau=1)

	def summary(self):
		print('... Information about TD3 actor, networks, and training progress ...')
		print('Game Name: ' + str(self.game))
		print('Actor Unique ID: ' + str(self.uniqueID))
		print('... Hyperparameters ...')
		print('alpha: ' + str(self.alpha))
		print('beta: ' + str(self.beta))
		print('gamma: ' + str(self.gamma))
		print('tau: ' + str(self.tau))
		print('batch size: ' + str(self.batch_size))
		print('noise: ' + str(self.noise))
		print('update actor every n trains: ' + str(self.update_actor_iter))
		print('learn interval (externally enforced, only used for naming conventions): ' + str(self.learnInterval))
		print('input dims: ' + str(self.input_dims))
		print('action dims: ' + str(self.n_actions))
		print('... Network Information ...')
		print('network design: ' + str(self.getNetworkAsString()))
		print('... Learning Progress ...')
		print('number of actions decided: ' + str(self.timeStepNumActionsChoosen))
		print('number of times agent has learned on a batch: ' + str(self.timeStepNumLearns))
		print('memory filled: ' + str(self.memory.mem_cntr) + "/" + str(self.memory.mem_size) + 
				", " + str(int(100*self.memory.mem_cntr/self.memory.mem_size)) + "%")
		print('number of epochs/ number of recorded game scores: ' + str(len(self.scores)))
		print('average score of last 100 games: ' + str(self.getAveScore()))

	def createModels(self):
		self.actor = generateSequentialActorNetwork(self.layerSizeList, n_stateVariables=self.input_dims, n_actions=self.n_actions)
		self.target_actor = generateSequentialActorNetwork(self.layerSizeList, n_stateVariables=self.input_dims, n_actions=self.n_actions)
		self.critic_1 = generateSequentialCriticNetwork(self.layerSizeList, n_stateVariables=self.input_dims, n_actions=self.n_actions)
		self.target_critic_1 = generateSequentialCriticNetwork(self.layerSizeList, n_stateVariables=self.input_dims, n_actions=self.n_actions)
		self.critic_2 = generateSequentialCriticNetwork(self.layerSizeList, n_stateVariables=self.input_dims, n_actions=self.n_actions)
		self.target_critic_2 = generateSequentialCriticNetwork(self.layerSizeList, n_stateVariables=self.input_dims, n_actions=self.n_actions)

		"""
		self.actor = ActorNetwork(self.layerSizeList, n_stateVariables=self.input_dims,
									n_actions=self.n_actions, name='actor', save_dir=self.saveDirectory + "/modelWeights/")
		self.critic_1 = CriticNetwork(self.layerSizeList, n_stateVariables=self.input_dims,
									n_actions=self.n_actions, name='critic_1', save_dir=self.saveDirectory + "/modelWeights")
		self.critic_2 = CriticNetwork(self.layerSizeList, n_stateVariables=self.input_dims,
									n_actions=self.n_actions, name='critic_2', save_dir=self.saveDirectory + "/modelWeights")
		self.target_actor = ActorNetwork(self.layerSizeList, n_stateVariables=self.input_dims,
									n_actions=self.n_actions, name='target_actor', save_dir=self.saveDirectory + "/modelWeights")
		self.target_critic_1 = CriticNetwork(self.layerSizeList, n_stateVariables=self.input_dims,
									n_actions=self.n_actions, name='target_critic_1', save_dir=self.saveDirectory + "/modelWeights")
		self.target_critic_2 = CriticNetwork(self.layerSizeList, n_stateVariables=self.input_dims,
									n_actions=self.n_actions, name='target_critic_2', save_dir=self.saveDirectory + "/modelWeights")
		"""
		self.actor.compile(optimizer=Adam(learning_rate=self.alpha), loss='mean_absolute_error')
		self.critic_1.compile(optimizer=Adam(learning_rate=self.beta), loss='mean_squared_error')
		self.critic_2.compile(optimizer=Adam(learning_rate=self.beta), loss='mean_squared_error')
		self.target_actor.compile(optimizer=Adam(learning_rate=self.alpha), loss='mean_absolute_error')
		self.target_critic_1.compile(optimizer=Adam(learning_rate=self.beta), loss='mean_squared_error')
		self.target_critic_2.compile(optimizer=Adam(learning_rate=self.beta), loss='mean_squared_error')

	def getSaveDirectoryFromUniqueID(self):
		list_subfolders = [f.name for f in os.scandir('results/TD3/') if f.is_dir()]
		searchForID  =-1
		for subfolder in list_subfolders:
			if int(subfolder[0:3]) == self.uniqueID:
				self.saveDirectory = 'results/TD3/' + str(subfolder)
				searchForID += 1
		if searchForID == -1:
			print("...MAJOR ERROR, SELECTED UNIQUE ID DOES NOT EXIST...")
		elif searchForID > 0:
			print("... ERROR WE FOUND MORE THAN 1 DIRECTORY WITH THE UNIQUE ID ...")

	def getUniqueID(self):
		list_subfolders = [f.name for f in os.scandir('results/TD3/') if f.is_dir()]
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

	def choose_action(self, observation, chooseRandom = False):
		if chooseRandom:
			#mu = np.random.normal(scale=self.noise, size=(self.n_actions,))
			#mu = 1-(2*np.random.rand(size=(self.n_actions,)))
			mu = np.random.rand(size=(self.n_actions,))
		else:
			state = tf.convert_to_tensor([observation], dtype=tf.float32)
			mu = self.actor(state)[0] # returns a batch size of 1, want a scalar array
		mu_prime = mu + np.random.normal(scale=self.noise)

		mu_prime = tf.clip_by_value(mu_prime, 0, 1)
		self.timeStepNumActionsChoosen += 1

		return mu_prime

	def remember(self, state, action, reward, new_state, done):
		self.memory.store_transition(state, action, reward, new_state, done)

	def learn(self):
		if self.memory.mem_cntr < self.batch_size:
			return 

		states, actions, rewards, new_states, dones = \
				self.memory.sample_buffer(self.batch_size)

		states = tf.convert_to_tensor(states, dtype=tf.float32)
		actions = tf.convert_to_tensor(actions, dtype=tf.float32)
		rewards = tf.convert_to_tensor(rewards, dtype=tf.float32)
		states_ = tf.convert_to_tensor(new_states, dtype=tf.float32)

		with tf.GradientTape(persistent=True) as tape:
			target_actions = self.target_actor(states_)
			target_actions = target_actions + \
					tf.clip_by_value(np.random.normal(scale=0.1), -.25, .25)            # WHY HARD CODED? ----------------------------

			target_actions = tf.clip_by_value(target_actions, 0, 1) # min and max action
		
			#q1_ = self.target_critic_1(states_, target_actions)
			q1_ = self.target_critic_1(tf.concat([states_, target_actions], axis=1))
			#q2_ = self.target_critic_2(states_, target_actions)
			q2_ = self.target_critic_2(tf.concat([states_, target_actions], axis=1))

			#q1 = tf.squeeze(self.critic_1(states, actions), 1)
			#q2 = tf.squeeze(self.critic_2(states, actions), 1)
			q1 = tf.squeeze(self.critic_1(tf.concat([states, actions], axis=1)), 1)
			q2 = tf.squeeze(self.critic_2(tf.concat([states, actions], axis=1)), 1)

			# shape is [batch_size, 1], want to collapse to [batch_size]
			q1_ = tf.squeeze(q1_, 1)
			q2_ = tf.squeeze(q2_, 1)

			critic_value_ = tf.math.minimum(q1_, q2_)
			# in tf2 only integer scalar arrays can be used as indices
			# and eager exection doesn't support assignment, so we can't do
			# q1_[dones] = 0.0
			target = rewards + self.gamma*critic_value_*(1-dones)
			#critic_1_loss = tf.math.reduce_mean(tf.math.square(target - q1))
			#critic_2_loss = tf.math.reduce_mean(tf.math.square(target - q2))
			critic_1_loss = keras.losses.MSE(target, q1)
			critic_2_loss = keras.losses.MSE(target, q2)


		critic_1_gradient = tape.gradient(critic_1_loss, 
										  self.critic_1.trainable_variables)
		critic_2_gradient = tape.gradient(critic_2_loss, 
										  self.critic_2.trainable_variables)

		self.critic_1.optimizer.apply_gradients(
					   zip(critic_1_gradient, self.critic_1.trainable_variables))
		self.critic_2.optimizer.apply_gradients(
					   zip(critic_2_gradient, self.critic_2.trainable_variables))

		
		self.timeStepNumLearns += 1

		if self.timeStepNumLearns % self.update_actor_iter != 0:
			return

		with tf.GradientTape() as tape:
			new_actions = self.actor(states)
			critic_1_value = self.critic_1(tf.concat([states, new_actions], axis=1))
			# critic_1_value . . . I deleted a line here for the new style
			actor_loss = -tf.math.reduce_mean(critic_1_value)

		actor_gradient = tape.gradient(actor_loss, self.actor.trainable_variables)
		self.actor.optimizer.apply_gradients(
						zip(actor_gradient, self.actor.trainable_variables))

		self.update_network_parameters()

	def update_network_parameters(self, tau=None):
		if tau is None:
			tau = self.tau

		weights = []
		targets = self.target_actor.weights
		for i, weight in enumerate(self.actor.weights):
			weights.append(weight * tau + targets[i]*(1-tau))

		self.target_actor.set_weights(weights)

		weights = []
		targets = self.target_critic_1.weights
		for i, weight in enumerate(self.critic_1.weights):
			weights.append(weight * tau + targets[i]*(1-tau))

		self.target_critic_1.set_weights(weights)

		weights = []
		targets = self.target_critic_2.weights
		for i, weight in enumerate(self.critic_2.weights):
			weights.append(weight * tau + targets[i]*(1-tau))

		self.target_critic_2.set_weights(weights)

	def save_models(self):
		print('... saving models ...')
		"""
		self.actor.save_weights(self.actor.save_file)
		self.critic_1.save_weights(self.critic_1.save_file)
		self.critic_2.save_weights(self.critic_2.save_file)
		self.target_actor.save_weights(self.target_actor.save_file)
		self.target_critic_1.save_weights(self.target_critic_1.save_file)
		self.target_critic_2.save_weights(self.target_critic_2.save_file)
		"""
		self.actor.save(self.saveDirectory + '/models/actor.h5')
		self.target_actor.save(self.saveDirectory + '/models/target_actor.h5')
		self.critic_1.save(self.saveDirectory + '/models/critic_1.h5')
		self.critic_2.save(self.saveDirectory + '/models/critic_2.h5')
		self.target_critic_1.save(self.saveDirectory + '/models/target_critic_1.h5')
		self.target_critic_2.save(self.saveDirectory + '/models/target_critic_2.h5')

	def load_models(self):
		
		print('... loading models ...')
		"""
		self.actor.load_weights(self.actor.save_file)
		self.critic_1.load_weights(self.critic_1.save_file)
		self.critic_2.load_weights(self.critic_2.save_file)
		self.target_actor.load_weights(self.target_actor.save_file)
		self.target_critic_1.load_weights(self.target_critic_1.save_file)
		self.target_critic_2.load_weights(self.target_critic_2.save_file)
		"""
		self.actor = load_model(self.saveDirectory + '/models/actor.h5')
		self.target_actor = load_model(self.saveDirectory + '/models/target_actor.h5')
		self.critic_1 = load_model(self.saveDirectory + '/models/critic_1.h5')
		self.critic_2 = load_model(self.saveDirectory + '/models/critic_2.h5')
		self.target_critic_1 = load_model(self.saveDirectory + '/models/target_critic_1.h5')
		self.target_critic_2 = load_model(self.saveDirectory + '/models/target_critic_2.h5')

	def logScore(self, score, customScore=None):
		self.scores.append(score)
		if customScore == None:
			self.customScores.append(score)
		else:
			self.customScores.append(customScore)

	def save_all(self):
		print('... saving entire actor/ RL state ...')
		afile = open(self.saveDirectory + '/hyperparams.pkl', 'wb')
		pickle.dump((self.layerSizeList,self.alpha,self.beta,self.input_dims,self.gamma,self.tau,self.batch_size,self.timeStepNumLearns,
				self.timeStepNumActionsChoosen,self.n_actions,self.update_actor_iter,self.learnInterval,
				self.game,self.noise,self.scores,self.customScores), afile)
		afile.close()		
		afile = open(self.saveDirectory + '/memory.pkl', 'wb')
		pickle.dump(self.memory,afile)
		afile.close()
		self.save_models()
		plt.plot(self.scores)
		plt.plot(self.customScores)
		plt.savefig(self.saveDirectory + '/RESULTS_epochs' + str(len(self.scores)) + '_score' + str(self.getAveScore()) + '.png')
		#self.saveActorModelAsSequentialForTFLite()

	def getAveScore(self):
		return(np.mean(self.scores[-100:]))

	def loadStoredData(self):
		afile = open(self.saveDirectory + '/hyperparams.pkl', 'rb')
		self.layerSizeList,self.alpha,self.beta,self.input_dims,self.gamma,self.tau,self.batch_size,self.timeStepNumLearns,\
				self.timeStepNumActionsChoosen,self.n_actions,self.update_actor_iter,self.learnInterval,\
				self.game,self.noise,self.scores,self.customScores = pickle.load(afile)
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
		shutil.copytree(self.saveDirectory + "/models", self.saveDirectory + archiveDir + "/models")
		shutil.copy2(self.saveDirectory + '/memory.pkl', self.saveDirectory + archiveDir)
		shutil.copy2(self.saveDirectory + '/hyperparams.pkl', self.saveDirectory + archiveDir)
		shutil.copy2(self.saveDirectory + '/RESULTS_epochs' + str(len(self.scores)) + '_score' + str(self.getAveScore()) + '.png', 
				self.saveDirectory + archiveDir)

"""
	def saveActorModelAsSequentialForTFLite(self):
		sequentialActor = generateSequentialActorNetwork(self.layerSizeList, self.input_dims, self.n_actions)
		sequentialActor.compile(optimizer=Adam(learning_rate=self.alpha), loss='mean')
		print(sequentialActor.summary())
		print(self.actor.summary())
		print(self.actor.weights)
		sequentialActor.set_weights(self.actor.weights)
		sequentialActor.save(self.saveDirectory + 'actorModel.h5')

		print("Comparing custom class to sequential model:")
		testInput = [[.5,.5,.5]]
		print("sequential model output:")
		print(sequentialActor.predict(testInput))
		print("custom class output:")
		print(self.actor(testInput))
"""

def generateSequentialActorNetwork(layerSizeList, n_stateVariables, n_actions):
	model= keras.Sequential()
	isFirstLayer = True
	for unitCount in layerSizeList:
		if isFirstLayer:
			isFirstLayer = False
			layer = Dense(unitCount, input_dim=np.squeeze(n_stateVariables), activation='relu')
		else:
			layer = Dense(unitCount, activation='relu')
		model.add(layer)
	lastLayer = Dense(n_actions, activation='sigmoid')#tanh
	model.add(lastLayer)
	return(model)

# takes state-action input, maps to value output (dim=1)
def generateSequentialCriticNetwork(layerSizeList, n_stateVariables, n_actions):
	model= keras.Sequential()
	isFirstLayer = True
	for unitCount in layerSizeList:
		if isFirstLayer:
			isFirstLayer = False
			layer = Dense(unitCount, input_dim=np.squeeze(n_stateVariables)+np.squeeze(n_actions), activation='relu')
		else:
			layer = Dense(unitCount, activation='relu')
		model.add(layer)
	finalLayer = Dense(1, activation=None)
	model.add(finalLayer)
	return(model)


	# MOVE EPISODE PRINT STATEMENT TO AGENT EPISODE log