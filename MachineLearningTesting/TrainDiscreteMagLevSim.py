
import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import numpy as np
from AgentDQN_family import AgentDQN_family
from MagLevSimulatedEnvironment_SimpleDiscrete import MagLevEnvironment as gym
import tensorflow as tf
from AgentDQN_basic import AgentDQN_basic
import matplotlib.pyplot as plt
from time import time
import random
random.seed()
import math


def train(numGames=500,lr = .0001, gamma = 0.97, epsilon = .2, 
		epsilon_decay = 0.99, batchSize = 64,filename='model', learnInterval=1,layerCount=4,layerUnits=64):
	startTime = time()
	env=gym()
	trainCounter = 0
	trainingSession = 0

	action_dim = 3
	state_dim = (5,)

	# Create agent
	agent = AgentDQN_basic(state_dim, action_dim, batchSize=batchSize, lr=lr, gamma=gamma, epsilon=epsilon, eps_dec=epsilon_decay,
						filename=filename, mem_size=1000000, layerCount=layerCount, layerUnits=layerUnits)

	scores = []

	# game loop!
	for i in range(numGames):
		if numGames-i < 100: # for the final 100 games that give us an average score, stop exploring
			agent.epsilon = 0
		elif numGames-i< 200:
			agent.epsilon = .1
		score = 0
		#env.initCase = 6 # hyper random
		#env.initCase = 1 # lower rail
		env.initCase = 7 # either lower or upper rail, randomly
		#env.rewardFunction = 0 # middle
		env.rewardFunction = 1 # random
		old_state = env.reset()

		for j in range(600):
			if j%300 == 0:
				env.pickNewTarget()
			# Actor picks an action (following the policy)
			a = agent.chooseAction(old_state)
			# Retrieve new state, reward, and whether the state is terminal
			# GAME HOUSEKEEPING
			new_state, r, done, _ = env.step(a)

			# Memorize for experience replay
			agent.saveMemory(old_state, a, r, done, new_state)

			# MORE GAME HOUSEKEEPING
			# Update current state
			old_state = new_state
			score += r

			# Train DDQN and transfer weights to target network
			"""
			if learnInterval == 1:
				agent.learn(numLearns=learnInterval)
			elif agent.memory.mem_counter - learnInterval > trainCounter:
				trainingSession += 1
				trainCounter = agent.memory.mem_counter
				print("Learning for " + str(learnInterval) + " batches, this is training session #" + str(trainingSession))
				for thisIsNotADuplicateVariableThatWillBreakTheRestOfTheCode in range(int(learnInterval)):
					agent.learn()
				#print("Learning done")
			"""
		print("learning")
		for j in range(600):
			agent.learn()
		print("learning done")

		# PROGRESS TRACKING HOUSEKEEPING
		scores.append(score)
		ave_score=np.mean(scores[-100:])
		print('episode: ', i, '/', numGames, 'score %.2f' % score, 'average_score %.2f' % ave_score)

	agent.save_model(filenameAppendage='_score' + str(ave_score), directory = 'results')

	plt.plot(scores)
	plt.savefig('results/' + agent.getFilename() + '_score' + str(ave_score) + '_SCORES' + '.png')
	endTime = time()
	print("Total run time in seconds: " + str(endTime - startTime))

def visualize(filename, numGames=1):
	env=gym()
	# gamma, lr, bs, etc make no difference since we're not learning. it might actually not even matter
	# if we use simple or complex, since we're just loading models and running inference
	agent=AgentDQN_basic(state_dim=(5,), 
				action_dim=5, 
				epsilon=0,
				learnInterval=999999999,
				filename=filename)
	agent.load_model()
	agent.epsilon = 0

	for i in range(numGames):
		score = 0
		old_state = env.reset()
		for j in range(20000): # more than training to prove it extends indefinitly
			if j%10000 == 0:
				env.pickNewTarget()
			env.pickNewTarget(-(math.sin(j/25)*75)+190)
			# env.pickNewTarget(-(math.sin(j/10)*75)+190) # Yikes, can't track this with physics, so rough
			#env.pickNewTarget(-(math.sin(j/10)*25)+190)
			# env.pickNewTarget(-(math.sin(j/5)*25)+190) # brutal
			action= agent.chooseAction(old_state)
			new_state, r, done, info = env.step(action)
			score += r
			old_state=new_state

		print('episode: ', i, '/', numGames, 'score %.2f' % score)
		env.plotStates()

if __name__ == '__main__':
	gpus = tf.config.experimental.list_physical_devices('GPU')
	for gpu in gpus:
		tf.config.experimental.set_memory_growth(gpu, True)
	tf.compat.v1.disable_eager_execution()

	numGames=500
	filename=('run1100' + "_numGames" + str(numGames))
	#train(numGames=numGames,lr = .0001, gamma = 0.95, epsilon = 1, 
	#		epsilon_decay = 0.99997, batchSize = 256,filename=filename, learnInterval=1, layerCount=4,layerUnits=128)
	# epsilon_decay = .9999 means 20,000 steps to reach .1, or 33 runs of semi-random (assuming numGames >133)


	# best model trained with intermittent training so far, still not perfect
	#visualize('results/DQNbasic_lr0.0001_LISTEP_bs256_g0.95_e1_t0.05_network6x64_run0_numGames100_score-15817.255384265703')

	# RUN 100: trained on step functions, random inits, using cost: linear position error. state normalization: normalized position and error, NOT normalized cost
	# PRETTY GOOD: tracks step functions and sine wave well, but has some slight steady state error and some ripple on position holding
	#visualize('results/DQNbasic_lr0.0001_LISTEP_bs256_g0.95_e1_t0.05_network6x64_run102_numGames500_score-2852.052229682106')

	# NOTE ONLY TRAINED 100 SIMS, SO NO EPSILON
	# RUN 200: trained on step functions, random inits, using cost: linear position error. state normalization: NO NORMALIZATION
	# PRETTY GOOD, not as good as above, but less training and no epsilon?
	#visualize('results/DQNbasic_lr0.0001_LISTEP_bs256_g0.95_e1_t0.05_network6x64_run200_numGames100_score-11607.336754116066')

	# NOTE ONLY TRAINED 100 SIMS, SO NO EPSILON
	# RUN 300: trained on step functions, random inits, using cost: linear position and extreme linear velocity after V>2. No normalization
	# PRETTY BAD
	#visualize('results/DQNbasic_lr0.0001_LISTEP_bs256_g0.95_e1_t0.05_network6x64_run301_numGames100_score-38327.59825021052')

	# NOTE ONLY TRAINED 100 SIMS, SO NO EPSILON
	# RUN 400: trained on step functions, random inits, using cost: linear position with extreme slope close to target, extreme linear velocity after V>2 with shallow slope at extremes. No Normalization
	# TOTAL FAIL
	#visualize('results/DQNbasic_lr0.0001_LISTEP_bs256_g0.95_e1_t0.05_network6x64_run400_numGames100_score-90894.19478480882')

	# ok, so other reward functions didn't work . . . but what about JUST a double V on position?
	# can we lightly penalize velocity/accel for more stability without shaping approach?
	# may need to repeat this on normalized values?
	# 

	# RUN 500: cost doubleV position only
	#visualize('results/DQNbasic_lr0.0001_LISTEP_bs256_g0.95_e1_t0.05_network6x64_run500_numGames200_score-6378.516541752138')
	# RUN 600: cost doubleV position, linear accel
	#visualize('results/DQNbasic_lr0.0001_LISTEP_bs256_g0.95_e1_t0.05_network6x64_run602_numGames200_score-11997.582142012101')
	# RUN 700: cost doubleV position, linear accel, cost to using big current swings
	#visualize('results/DQNbasic_lr0.0001_LISTEP_bs256_g0.95_e1_t0.05_network6x64_run702_numGames200_score-12424.867032432283')

	# RUN 800: doubleV position, penalize big current swings. INTERMITTENT TRAINING!!!

	# RUN 900: same as 800, but min epsilon .3 and epsilon decay extended and action space reduced to x3. num games was increased as well
	# Very good performance. can destabalize with maximum position change request (caused by starting on the top rail and trying to track
		# a very high speed sine wave), but restabilizes very quickly. Minimal steady state offset (<1-2 thou max, often less), and minimal
		# steady state wiggle (because it only has X3 action space). Can avoid the instability by feeding ramp generators for maximum position
		# changes. Note that in this instability case, it doesn't even try to turn around- I think that without a penalty for hitting the rail,
		# it actually learned to use the rail to cancel out significant acceleration for nearby targets
	#visualize('results/DQNbasic_lr0.0001_LISTEP_bs256_g0.95_e1_t0.05_network6x64_run902_numGames500_score-11840.546835957235')

	# RUN 1000: using the new, slower, higher epsilon training from 900, add back in velocity penalty (at lower value). Also increase
	# run length so it has more time to stabalize

	# if that doesn't work, getting stabile velocity performance may require adding a state variable which is deviation from target
	# velocity, penalizing that, and having target velocity approach 0 as position approaches 0 (maybe even fully drop position control?)
	# in this case, do we provide position state information, or is it irrelevant?

	# note we should also be able to control velocity by feeding a ramp generator to position control

	# RUN 1100: same settings as 900, using just 3X actions, doubleV position penalty, 500 epoch, slow esp dec to .3,
	# ALL INTERMITTENT TRAINING. Looking at network shape
	# 16x anything failed (16x8 is the only one tried)
	# 32x anything was messy
	# 64x2 was beautiful. 64x3 wasn't. All 64x4 and above worked well
	# 128x2 and 128x3 worked, 128x4 had slightly more instability but still generally found a solution
	visualize('results/DQNbasic_lr0.0001_LISTEP_bs256_g0.95_e1_t0.05_network2x64_run1100_numGames500_score-12333.222940476711')
	visualize('results/DQNbasic_lr0.0001_LISTEP_bs256_g0.95_e1_t0.05_network3x64_run1100_numGames500_score-21594.4939838892')


	#python TrainDiscreteMagLevSim.py
