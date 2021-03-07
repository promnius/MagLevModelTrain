import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import gym
import numpy as np
#from AgentTD3 import Agent
from AgentTD3 import Agent
import tensorflow as tf
import matplotlib.pyplot as plt

def train():
	#env = gym.make('LunarLanderContinuous-v2')
	#env = gym.make('Pendulum-v0')
	env = gym.make('Pendulum-v0')
	
	agent = Agent(alpha=0.001, beta=0.001,
			input_dims=env.observation_space.shape, tau=0.005, game="Pendulum",
			batch_size=100, layerSizeList=[128,128,64,64,32],
			n_actions=env.action_space.shape[0])
	
	#agent = Agent(uniqueID=11)
	
	n_games = 200
	allActions = []
		
	for i in range(n_games):
		observation = env.reset()
		done = False
		score = 0
		while not done:
			#env.render()
			action = agent.choose_action(observation)
			action = (4*action)-2
			allActions.append(tf.squeeze(action))
			observation_, reward, done, info = env.step(action) #(action,))
			agent.remember(observation, action, reward, observation_, done)
			agent.learn()
			score += reward
			observation = observation_
		agent.logScore(score)

		print('episode ', i, 'score %.1f' % score,
				'average score %.1f' % agent.getAveScore())
	agent.summary()
	#agent.save_all()
	agent.archive()

	print("ALL ACTIONS: ")
	print(allActions)

	plt.figure()
	plt.hist(allActions, bins=20)
	plt.show()

def visualizeGymModel(uniqueID, numGames=1):
	gameName = 'Pendulum-v0'
	env=gym.make(gameName)
	# gamma, lr, bs, etc make no difference since we're not learning. it might actually not even matter
	# if we use simple or complex, since we're just loading models and running inference
	agent=Agent(uniqueID=uniqueID)

	for i in range(numGames):
		done = False
		score = 0
		customScore = 0
		old_state = env.reset()
		steps=0
		targetAngles = []
		forces = []
		pendulumAngles = []
		singleStepRewards = []
		targetAngle = 0
		#targetPosition=0
		#targetPosition=1-2*random.random()
		#targetPosition = updateTargetPosition(targetPosition,steps)
		#old_state = np.append(old_state,old_state[0]-targetPosition)
		for j in range(2000): # more than training to prove it extends indefinitly
		#while not done:
			env.render()
			steps+=1
			#targetPosition = updateTargetPosition(targetPosition,steps)
			action= agent.choose_action(old_state)
			action = (4*action)-2
			new_state, r, done, info = env.step(action)
			#new_state = np.append(new_state,new_state[0]-targetPosition)
			score += r
			#customReward = getCustomReward(new_state, targetPosition)
			#customScore += customReward
			old_state=new_state 
			targetAngles.append(targetAngle)
			pendulumAngles.append(new_state[0]) # X10 just puts it on a good scale to plot next to position.
			forces.append(action)
			singleStepRewards.append(r)

		print('episode: ', i, '/', numGames, 'score %.2f' % score)
		plt.figure()
		plt.plot(targetAngles)
		plt.plot(pendulumAngles)
		plt.plot(forces)
		plt.figure()
		plt.plot(singleStepRewards)
		plt.show()

if __name__ == '__main__':
	gpus = tf.config.experimental.list_physical_devices('GPU')
	for gpu in gpus:
		tf.config.experimental.set_memory_growth(gpu, True)
	#tf.compat.v1.disable_eager_execution()
	#train()
	visualizeGymModel(24)

	#timing on pendulum: default, 8 seconds per episode. without eager execution: . with standard sequential models: .