
import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import numpy as np
from AgentDQN_family import AgentDQN_family
import gym
import tensorflow as tf
from AgentDQN_basic import AgentDQN_basic
import matplotlib.pyplot as plt
from time import time
import random
random.seed()
import math


def TrainOnCartPole(numGames=500, isDueling = False,isDual = False,isPER = False,lr = .0001, gamma = 0.97, epsilon = .2, 
		epsilon_decay = 0.99, tau = .01, batchSize = 64,filename='model', complexModel=False, learnInterval=1, layerCount=2,layerUnits=64, usePruning=False):
	startTime = time()
	gameName = 'CartPole-v1'
	env=gym.make(gameName)
	trainCounter = 0

	action_dim = env.action_space.n
	state_dim = (np.squeeze(env.observation_space.shape) + 1,)

	# Create agent
	if complexModel:
		agent = AgentDQN_family(state_dim, action_dim, batchSize, lr, tau, gamma, epsilon, epsilon_decay, learnInterval, 
					isDual=isDual, isDueling=isDueling, isPER=isPER, filename=filename, mem_size=1000000, layerCount=layerCount, layerUnits=layerUnits, usePruning=usePruning)
	else:
		agent = AgentDQN_basic(state_dim, action_dim, batchSize, lr, tau, gamma, epsilon, epsilon_decay, learnInterval, 
						isDual=isDual, isDueling=isDueling, isPER=isPER, filename=filename, mem_size=1000000, layerCount=layerCount, layerUnits=layerUnits, usePruning=usePruning)

	scores = []
	customScores = []

	# game loop!
	for i in range(numGames):
		if numGames-i < 100: # for the final 100 games that give us an average score, stop exploring
			agent.epsilon = 0
		done = False
		score = 0
		customScore = 0
		old_state = env.reset()
		steps=0
		#targetPosition=0
		targetPosition=1-2*random.random()
		targetPosition=updateTargetPosition(targetPosition,steps)
		old_state = np.append(old_state,old_state[0]-targetPosition)

		while not done:
			steps+=1
			# UPDATE TARGET POSITION HERE
			targetPosition = updateTargetPosition(targetPosition,steps)
			# Actor picks an action (following the policy)
			a = agent.chooseAction(old_state)
			# Retrieve new state, reward, and whether the state is terminal
			# GAME HOUSEKEEPING
			new_state, r, done, _ = env.step(a)
			# UPDATE OBSERVATION TO INCLUDE TARGET
			new_state = np.append(new_state,new_state[0]-targetPosition)
			# UPDATE REWARD TO MEET CUSTOM REWARD
			custom_r = getCustomReward(new_state, targetPosition)
			# UPDATE DONE BASED ON CUSTOM DONE
			if new_state[0] < -2.4 or new_state[0] > 2.4 or new_state[2] < -.4 or new_state[2] > .4:
				done = 1
				r = 0
			elif steps > 500:
				done = 2
				r = 0
			else:
				done = 0
				r = 1

			# Memorize for experience replay
			#if steps%100 !=0: # try to avoid putting in cases where we've caused the sudden error?
			agent.saveMemory(old_state, a, custom_r, done, new_state)

			# MORE GAME HOUSEKEEPING
			# Update current state
			old_state = new_state
			score += r
			customScore += custom_r

			# Train DDQN and transfer weights to target network
			#agent.learn(numLearns=learnInterval)
			if agent.memory.mem_counter - learnInterval > trainCounter:
				trainCounter = agent.memory.mem_counter
				print("Learning for " + str(learnInterval) + " batches")
				for thisIsNotADuplicateVariableThatWillBreakTheRestOfTheCode in range(learnInterval):
					agent.learn()
				#print("Learning done")

		# PROGRESS TRACKING HOUSEKEEPING
		scores.append(score)
		customScores.append(customScore)
		ave_score=np.mean(scores[-100:])
		print('episode: ', i, '/', numGames, 'score %.2f' % score, 'average_score %.2f' % ave_score, 'custom score %.2f' % customScore)

	agent.save_model(filenameAppendage='_score' + str(ave_score), directory = 'results')

	plt.plot(scores)
	plt.plot(customScores)
	plt.savefig('results/' + agent.getFilename() + '_score' + str(ave_score) + '_SCORES' + '.png')
	endTime = time()
	print("Total run time in seconds: " + str(endTime - startTime))

def updateTargetPosition(targetPosition, steps):
	"""
	if steps%100 == 0:
		return(1-2*random.random())
	else:
		return(targetPosition)
	"""
	"""
	if steps%600 == 0:
		if random.random()>.5:
			return -1
		else:
			return 1
		return(1-2*random.random())
	"""
	#return(math.sin(steps/100))
	return(targetPosition)
	#return targetPosition
	#targetPosition=math.sin(steps/60.0)/2


def getCustomReward(state, targetPosition):
	newPosition = state[0]
	newPoleAngle = state[2]
	customReward = 2-(newPosition-targetPosition)**2-(50*(newPoleAngle**2))
	customReward = 3-abs(newPosition-targetPosition)-(20*(newPoleAngle**2))
	return (customReward)


def visualizeCartPoleModel(filename, numGames=1, complexModel=False):
	gameName = 'CartPole-v1'
	env=gym.make(gameName)
	# gamma, lr, bs, etc make no difference since we're not learning. it might actually not even matter
	# if we use simple or complex, since we're just loading models and running inference
	if complexModel:
		agent=AgentDQN_family(state_dim=(np.squeeze(env.observation_space.shape)+1,), 
					action_dim=env.action_space.n, 
					epsilon=0,
					learnInterval=999999999,
					fname=filename)
	else:
		agent=AgentDQN_basic(state_dim=(np.squeeze(env.observation_space.shape)+1,), 
					action_dim=env.action_space.n, 
					epsilon=0,
					learnInterval=999999999,
					filename=filename)
	agent.load_model()
	agent.epsilon = 0

	for i in range(numGames):
		done = False
		score = 0
		customScore = 0
		old_state = env.reset()
		steps=0
		positions = []
		targetPositions = []
		poleAngles = []
		singleStepRewards = []
		#targetPosition=0
		targetPosition=1-2*random.random()
		targetPosition = updateTargetPosition(targetPosition,steps)
		old_state = np.append(old_state,old_state[0]-targetPosition)
		for j in range(2300): # more than training to prove it extends indefinitly
		#while not done:
			env.render()
			steps+=1
			targetPosition = updateTargetPosition(targetPosition,steps)
			action= agent.chooseAction(old_state)
			new_state, r, done, info = env.step(action)
			new_state = np.append(new_state,new_state[0]-targetPosition)
			score += r
			customReward = getCustomReward(new_state, targetPosition)
			customScore += customReward
			old_state=new_state
			positions.append(new_state[0])
			poleAngles.append(10*new_state[2]) # X10 just puts it on a good scale to plot next to position.
			targetPositions.append(targetPosition)
			singleStepRewards.append(customReward)

		print('episode: ', i, '/', numGames, 'score %.2f' % score, 'custom score %.2f' % customScore)
		plt.figure()
		plt.plot(targetPositions)
		plt.plot(positions)
		plt.plot(poleAngles)
		plt.figure()
		plt.plot(singleStepRewards)
		plt.show()

if __name__ == '__main__':
	gpus = tf.config.experimental.list_physical_devices('GPU')
	for gpu in gpus:
		tf.config.experimental.set_memory_growth(gpu, True)
	tf.compat.v1.disable_eager_execution()

	#visualizeCartPoleModel('CartPole-v0_lr0.0001_nGames1500_LIntervalSTEP_batchSize256_gamma0.95_epsilon0.1_run103_score187.59')
	#visualizeCartPoleModel('CartPole-v0_lr0.0001_nGames1500_LIntervalSTEP_batchSize256_gamma0.95_epsilon0.1_run202_score200.0')
	#visualizeCartPoleModel('CartPole-v1_lr0.0001_nGames1000_LIntervalSTEP_batchSize256_gamma0.97_epsilon0.2_run400_score196.14')
	#visualizeCartPoleModel('CartPole-v1_lr0.0001_nGames1000_LIntervalSTEP_batchSize256_gamma0.97_epsilon0.2_run600_score224.0')
	#visualizeCartPoleModel('CartPole-v1_lr0.0001_nGames1000_LIntervalSTEP_batchSize256_gamma0.97_epsilon0.2_run700_score499.0')
	#visualizeCartPoleModel('CartPole-v1_lr0.0001_nGames500_LIntervalSTEP_batchSize256_gamma0.97_epsilon0.2_run1000_score499.0')

	# benchmarks: the simple model is about 2x faster, since the complex one implements a DDQN with T = 1 to emulate a DQN
	# the simple one takes all the same arguements as the advanced one for compatibility, but ignores a bunch of them, so for exploring
	# all the features use the slow option (who wants a basic DQN anyways?)
	numGames=2000
	filename=('run103' + "_numGames" + str(numGames))
	TrainOnCartPole(numGames=numGames, isDueling = False,isDual = False,isPER = False,lr = .0001, gamma = 0.95, epsilon = 1, 
			epsilon_decay = 0.99, tau = .05, batchSize = 256,filename=filename, complexModel=False, learnInterval=1000, layerCount=4,layerUnits=16, usePruning=False)


	#visualizeCartPoleModel('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network4x16_run101_numGames1000_score486.8')
	#visualizeCartPoleModel('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network4x16_run100_numGames800_score500.0')

	# AWESOME TRAINED MODEL CAN DO 0 error position tracking and sine wave tracking up to sin(steps/100). Complex model though.
	#visualizeCartPoleModel('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network4x16_run102_numGames1000_score499.5')