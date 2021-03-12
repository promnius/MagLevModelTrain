import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
from MagLevSimulatedEnvironment_Simple import MagLevEnvironment as gym
import numpy as np
#from AgentTD3 import Agent
from AgentTD3 import Agent
import tensorflow as tf
import matplotlib.pyplot as plt

def train():
	#env = gym.make('LunarLanderContinuous-v2')
	#env = gym.make('Pendulum-v0')
	env = gym()
	"""
	agent = Agent(alpha=0.001, beta=0.001,
			input_dims=(4,), tau=0.005, game="MagLevSim",
			batch_size=256, layerSizeList=[64,64,64,32],
			n_actions=1)
	"""
	agent = Agent(alpha=0.0001, beta=0.0001,
			input_dims=(4,), tau=0.005, game="MagLevSim",
			batch_size=256, layerSizeList=[128,128,64,64,32],
			n_actions=1,noise=.2)
	
	#agent = Agent(uniqueID=11)
	
	n_games = 50
	allActions = []
	
	print("Game loaded and set up")

	for i in range(n_games):
		#if i > 50
		#env.initCase = 6 # hyper random
		env.initCase = 1 # lower rail
		#env.rewardFunction = 0 # middle
		#env.rewardFunction = 1 # random
		observation = env.reset()
		score = 0
		for stepCounter in range(500): # let each sim run for 1 second, knowing we can probably stabalize in < 200ms
			#if stepCounter%300 == 0:
			#	env.pickNewTarget()
			#env.render()
			action = agent.choose_action(observation)
			action = (3*action)-1.5 # convert 0-1 to -1.5 to 1.5
			allActions.append(tf.squeeze(action))
			observation_, reward, done, info = env.step(action) #(action,))
			agent.remember(observation, action, reward, observation_, done)
			#if agent.timeStepNumActionsChoosen % 600 == 0:
			#	for localCounter in range(600):
			#		agent.learn()
			agent.learn()
			score += reward
			observation = observation_
		agent.logScore(score)

		print('episode ', i, 'score %.1f' % score,
				'average score %.1f' % agent.getAveScore())
	agent.summary()
	#agent.save_all()
	agent.archive()

	#print("ALL ACTIONS: ")
	#print(allActions)

	plt.figure()
	plt.hist(allActions, bins=20)
	plt.show()

def visualizeGymModel(uniqueID, numGames=1):
	env=gym()
	# gamma, lr, bs, etc make no difference since we're not learning. it might actually not even matter
	# if we use simple or complex, since we're just loading models and running inference
	agent=Agent(uniqueID=uniqueID)

	for i in range(numGames):
		done = False
		score = 0
		customScore = 0
		env.initCase = 6
		env.rewardFunction = 1
		old_state = env.reset()

		for j in range(5000): # more than training to prove it extends indefinitly
			if j%800 == 0:
				env.pickNewTarget()
		#while not done:
			#targetPosition = updateTargetPosition(targetPosition,steps)
			# env. update target position
			action= agent.choose_action(old_state)
			action = (3*action)-1.5
			new_state, r, done, info = env.step(action)
			print("State: " + str(new_state) + ", reward: " + str(r))
			score += r
			old_state=new_state

		print('episode: ', i, '/', numGames, 'score %.2f' % score)
		env.plotStates()

if __name__ == '__main__':
	gpus = tf.config.experimental.list_physical_devices('GPU')
	for gpu in gpus:
		tf.config.experimental.set_memory_growth(gpu, True)
	#tf.compat.v1.disable_eager_execution()
	train()
	#visualizeGymModel(29)
	#visualizeGymModel(33)
	#visualizeGymModel(34)

	#visualizeGymModel(43)
	#visualizeGymModel(44)

	#visualizeGymModel(46)
	#timing on maglev: default (1000 step sim), 40 seconds per episode. 1hr for 100 run sim