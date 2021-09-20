
import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import numpy as np
from AgentDQN_family import AgentDQN_family
from MagLevSimulatedEnvironment_SimpleDiscrete import MagLevEnvironment as gym
import tensorflow as tf
from AgentDQN_basic import AgentDQN_basic
import matplotlib.pyplot as plt
from time import time
import time as sleeper
import random
random.seed()
import math
from SendLiveModelUpdateToArduino import sendLiveModelUpdateToArduino
import serial
import matplotlib.pyplot as plt


def train(numGames=500,lr = .0001, gamma = 0.97, epsilon = .2, 
		epsilon_decay = 0.99, batchSize = 64, learnInterval=1,layerSizeList = [64,64,64,64],uniqueID=None, comPort="COM7", dummyStates=[]):

	comPort = serial.Serial(comPort, 1000000, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE)
	comPort.set_buffer_size(rx_size = 1000000, tx_size = 1000000)
	sleeper.sleep(1) # let port initialize
	
	startTime = time()

	action_dim = 3
	state_dim = (5,)

	# Create agent
	if uniqueID == None:
		agent = AgentDQN_basic(state_dim, action_dim, batchSize=batchSize, lr=lr, gamma=gamma, epsilon=epsilon, eps_dec=epsilon_decay,
						mem_size=1000000, layerSizeList = layerSizeList)
		# WTF I cannot explain for the life of me what is going on here. If the model is archived before it is trained at all, the TFLite version
		# is a different size and breaks things. Ok, that's not that crazy, the first train must initialize some stuff, right? But it DOESN'T
		# react any different if I train it on random generated states, but it DOES if I use a recorded set of real states . . . HMMM, that's not
		# how math works.
		for dummyState in dummyStates:
			#dummyState1 = [random.random(),random.random(),random.random(),random.random(),random.random()]
			#dummyState2 = [random.random(),random.random(),random.random(),random.random(),random.random()]
			#agent.saveMemory(dummyState, float(random.randint(-1,1)), random.random(), False, dummyState)
			agent.saveMemory(dummyState[0], dummyState[1], dummyState[2], dummyState[3], dummyState[4])
		#for counter in range(600):
		#	agent.learn()
		agent.learn(customBatchSize=3)
		agent.wipeMemory()
	else:
		agent = AgentDQN_basic(uniqueID=uniqueID) # use this instead to continue training a previous agent

	aFirst = agent.chooseAction([[0,0,0,0,0]])

	# game loop!
	for i in range(numGames):

		agent.save_all() # get teensy file generated
		#agent.archive()

		sendLiveModelUpdateToArduino(comPort=comPort, filename=agent.getTFLiteModelPath())
		print()

		# collect data from teensy, which may not be ready for a few seconds as it runs the real world test
		waiting = True
		print("Waiting for indication from Teensy that serial data is ready")
		receiveCounter = 0
		while waiting:
			incomingText = comPort.readline()
			receiveCounter += 1
			if incomingText == b'DATA_START\r\n':
				print("Received go-ahead text. Getting ready to ingest data!")
				waiting = False
			else:
				#print("Received the following: " + str(incomingText))
				print(str(receiveCounter) + "R: " + str(incomingText))
			sleeper.sleep(.001)
		dataComing = True
		dataDump = []
		while dataComing:
			incomingText = comPort.readline()
			incomingText = incomingText.replace(b'\r\n',b'').decode("utf-8")
			#print(incomingText)
			if incomingText == 'DATA_END': # NOT a byte string here since we've converted
				dataComing = False
				print("All Data received! number of entries: " + str(len(dataDump)))
			else:
				textAsList = list(map(float, incomingText.split(','))) # split the line of data (formatted like a csv) into a list of entries, and convert them from strings to floats
				dataDump.append(textAsList)
		print()
		# convert dataDump into markov elements and insert into agent memory
		score = 0
		positions = []
		for counter in range(len(dataDump)-1): # since we always need next state for markov, the last state/action pair is not useful
			nowStatus = dataDump[counter]
			nextStatus = dataDump[counter + 1]
			# dataDump entries: [inex, position, velocity, accel, position error, current, action]
			# old state and action come from nowStatus, new state and reward come from nextStatus. Done is not used in this mode
			a = nowStatus[6]
			old_state = nowStatus[1:6]
			#print(old_state,",")
			new_state = nextStatus[1:6]
			position = nextStatus[1]
			targetPosition = nowStatus[7] # use now status instead of next status because if it changed, we want the reward relative to what we were trying to accomplish
			# simulated system goes from 80 to 300, dynamic range of 220. New system has dynamic range of 20-30, so we adjust rewards
			if abs(position-targetPosition) < 2:
				reward = abs(position-targetPosition) * 25
			else: # position over 10, get the penalty for that plus the delta penalty at a slower rate
				reward = (abs(position-targetPosition)-2)*5 + 50
			reward = -reward
			done = False
			#print("[",old_state,",",a,",",reward,",",done,",",new_state,"],")
			agent.saveMemory(old_state, a, reward, done, new_state)
			score += reward
			positions.append(position)

		#plt.plot(positions)
		#plt.show()
		print("learning")
		for j in range(2000):
			agent.learn()
		print("learning done")

		# PROGRESS TRACKING HOUSEKEEPING
		agent.logScore(score)
		ave_score=agent.getAveScore()
		print(".................................................................................................................")
		print('finished episode: ', i, '/', numGames, 'score %.2f' % score, 'average_score %.2f' % ave_score)
		print(".................................................................................................................")
		print()


	agent.archive() # archive will also save second copies of everything, so if you retrain this model in the future, it will still have this version available
	#agent.save_all()
	print()

	endTime = time()
	print("Total run time in seconds: " + str(endTime - startTime))

if __name__ == '__main__':
	gpus = tf.config.experimental.list_physical_devices('GPU')
	for gpu in gpus:
		tf.config.experimental.set_memory_growth(gpu, True)
	#tf.compat.v1.disable_eager_execution()
	dummyStates = [[ [28.5, -0.01, -0.01, 6.62, -0.8] , 1.0 , -73.10000000000001 , False , [28.5, 0.0, 0.01, 6.62, -0.7] ],
		[ [28.5, 0.0, 0.01, 6.62, -0.7] , 2.0 , -73.10000000000001 , False , [28.5, -0.01, -0.01, 6.61, -0.7] ],
		[ [28.5, -0.01, -0.01, 6.61, -0.7] , 1.0 , -73.10000000000001 , False , [28.5, 0.0, 0.01, 6.62, -0.6] ],
		[ [28.5, 0.0, 0.01, 6.62, -0.6] , 2.0 , -73.10000000000001 , False , [28.5, -0.0, -0.01, 6.61, -0.5] ],
		[ [28.5, -0.0, -0.01, 6.61, -0.5] , 2.0 , -73.15 , False , [28.51, 0.01, 0.01, 6.62, -0.5] ],
		[ [28.51, 0.01, 0.01, 6.62, -0.5] , 1.0 , -73.10000000000001 , False , [28.5, -0.01, -0.01, 6.62, -0.6] ],
		[ [28.5, -0.01, -0.01, 6.62, -0.6] , 0.0 , -73.05 , False , [28.49, -0.01, 0.0, 6.61, -0.7] ]]

	numGames=300
	train(numGames=numGames,lr = .0001, gamma = 0.95, epsilon = 1, 
			epsilon_decay = 0.99997, batchSize = 256,learnInterval=1, layerSizeList=[64,64,64,64], uniqueID=None, dummyStates=dummyStates)
	# epsilon_decay makes no difference, we implement that on the Teensy