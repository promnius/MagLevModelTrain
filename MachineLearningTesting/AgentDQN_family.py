import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import sys
import numpy as np
import tensorflow.keras.backend as K

from tensorflow.keras.optimizers import Adam
from tensorflow.keras.models import Model
from tensorflow.keras.layers import Input, Dense, Flatten, Reshape, LSTM, Lambda
from tensorflow.keras.regularizers import l2
import tensorflow_model_optimization as tfmot

from memory_buffer import MemoryBuffer
from random import random, randrange
#import random

def generateDQN(action_dim, lr, state_dim, isDueling, layerCount, layerUnits, usePruning):
	""" Build Deep Q-Network
	"""
	#pruning_schedule = tfmot.sparsity.keras.PolynomialDecay(initial_sparsity=0, final_sparsity=.5, begin_step=)
	inp = Input(state_dim)

	x = Flatten()(inp)
	for j in range(layerCount):
		x = Dense(layerUnits, activation='relu')(x)

	if(isDueling):
		# Have the network estimate the Advantage function as an intermediate layer
		x = Dense(action_dim + 1, activation='linear')(x)
		x = Lambda(lambda i: K.expand_dims(i[:,0],-1) + i[:,1:] - K.mean(i[:,1:], keepdims=True), output_shape=(action_dim,))(x)
	else:
		x = Dense(action_dim, activation='linear')(x)

	model= Model(inp, x)
	model.compile(Adam(lr), 'mse')
	return(model)

# set learnInterval to 0 to enable learning anytime you call learn. set it to a large value to simulate learning from a real environment,
# where you won't get data as often as you'd like
class AgentDQN_family:
	""" Agent Class (Network) for DDQN
	"""
	def __init__(self, state_dim, action_dim, batchSize=64, lr=.0001, tau=.05, gamma=.95, epsilon=1, eps_dec=.99, learnInterval=1,isDual=False, 
				isDueling=False, isPER=False,filename='model',mem_size=1000000,layerCount=2,layerUnits=64,usePruning=False):
		self.state_dim = state_dim
		self.action_dim = action_dim
		self.isDueling = isDueling
		self.isDual = isDual
		self.isPER = isPER
		self.lr = lr
		self.gamma = gamma
		self.epsilon = epsilon
		self.epsilon_decay = eps_dec
		self.batchSize = batchSize
		self.filename=filename
		self.learnInterval = learnInterval
		# Initialize Deep Q-Network
		self.model = generateDQN(action_dim, lr, state_dim, isDueling, layerCount, layerUnits, usePruning)
		# Build target Q-Network
		self.target_model = generateDQN(action_dim, lr, state_dim, isDueling, layerCount, layerUnits, usePruning)
		self.layerCount = layerCount
		self.layerUnits = layerUnits
		self.target_model.set_weights(self.model.get_weights())
		self.memory = MemoryBuffer(mem_size,isPER)
		self.epsilonInitial=epsilon
		self.minEpsilon = .1
		self.usePruning=usePruning

		if isDual:
			self.tau = tau
		else:
			self.tau = 1.0

		# load memory data from disk if needed
		self.lastLearnIndex = self.memory.totalMemCount

	def chooseAction(self, state):
		""" Apply an espilon-greedy policy to pick next action
		"""
		if random() <= self.epsilon:
			return randrange(self.action_dim)
		else:
			return np.argmax(self.model.predict(self.reshape(state,debug=False))) # state[0] just means first batch

	def learn(self, numLearns=1):
		""" Train Q-network on batch sampled from the buffer
		"""
		if(self.memory.getSize() < self.batchSize): # we'd get strange errors if we tried to train before we had enough entries in 
			# our memory to fill a batch
			return
		if self.memory.totalMemCount - self.lastLearnIndex < self.learnInterval:
			# print("THIS SHOULD NEVER HAPPEN ON NORMAL RUNS, UNLESS WE TERMINATED DUE TO COMPLETEING MISSION")
			return
		self.lastLearnIndex = self.memory.totalMemCount

		for localCounter in range(numLearns):
			# Sample experience from memory buffer (optionally with PER)
			s, a, r, d, new_s, idx = self.memory.sample_batch(self.batchSize)

			# Apply Bellman Equation on batch samples to train our DDQN
			#q = self.model.predict(self.reshape(s,debug=True))
			#next_q = self.model.predict(self.reshape(new_s))
			#q_targ = self.target_model.predict(self.reshape(new_s))
			q = self.model.predict(s)
			next_q = self.model.predict(new_s)
			q_targ = self.target_model.predict(new_s)

			batch_index = np.arange(self.batchSize, dtype=np.int32) # This creates a properly formated array of the indexes of the samples in the batch, ie,
				# [1,2,3,4,...]
			#q_target[batch_index, sampleActions] = sampleRewards + self.gamma*np.max(q_next_predictions, axis=1)*sampleTerminations

			#old_q = q[i, a[i]]
			#if d[i]:
			#	q[i, a[i]] = r[i]
			#else:
			#	next_best_action = np.argmax(next_q[i,:])
			#	q[i, a[i]] = r[i] + self.gamma * q_targ[i, next_best_action]
			#if(self.isPER):
				# Update PER Sum Tree
			#	self.buffer.update(idx[i], abs(old_q - q[i, a[i]]))

			q[batch_index,a] = r + self.gamma * d * q_targ[batch_index,np.argmax(next_q,axis=1)]

			"""
			for i in range(s.shape[0]):
				old_q = q[i, a[i]]
				if d[i]:
					q[i, a[i]] = r[i]
				else:
					next_best_action = np.argmax(next_q[i,:])
					q[i, a[i]] = r[i] + self.gamma * q_targ[i, next_best_action]
				if(self.isPER):
					# Update PER Sum Tree
					self.buffer.update(idx[i], abs(old_q - q[i, a[i]]))
			"""
			# Train on batch
			self.model.fit(s, q, epochs=1, verbose=0) # do we really reshape S here???
			# Decay epsilon
			self.updateWeights()
		if self.epsilon > self.minEpsilon:
			self.epsilon *= self.epsilon_decay


	def huber_loss(self, y_true, y_pred):
		return K.mean(K.sqrt(1 + K.square(y_pred - y_true)) - 1, axis=-1)

	def updateWeights(self):
		""" Transfer Weights from Model to Target at rate Tau
		"""
		W = self.model.get_weights()
		tgt_W = self.target_model.get_weights()
		for i in range(len(W)):
			tgt_W[i] = self.tau * W[i] + (1 - self.tau) * tgt_W[i]
		self.target_model.set_weights(tgt_W)

	def saveMemory(self, state, action, reward, done, new_state):
		if(self.isPER):
			q_val = self.self.model.predict(self.reshape(state))
			q_val_t = self.target_model.predict(self.reshape(new_state))
			next_best_action = np.argmax(self.model.predict(self.reshape(new_state)))
			new_val = reward + self.gamma * q_val_t[0, next_best_action]
			td_error = abs(new_val - q_val)[0]
		else:
			td_error = 0
		self.memory.saveMemory(state, action, reward, done, new_state, td_error)

	def reshape(self, x, debug=False):
		if debug: print("RESHAPING: x was: " + str(x.shape) + ", and has len(shape): " + str(len(x.shape)))
		if len(x.shape) < 4 and len(self.state_dim) > 2: return np.expand_dims(x, axis=0)
		elif len(x.shape) < 3: 
			if debug: y = np.expand_dims(x, axis=0)
			if debug: print("A: Now x is: " + str(y.shape) + ", and has len(shape): " + str(len(y.shape)))
			if debug: breakthis = idontexist
			return np.expand_dims(x, axis=0)
		else: 
			if debug: print("B: Now x is: " + str(x.shape) + ", and has len(shape): " + str(len(x.shape)))
			if debug: breakthis = idontexist
			return x

	def load_model(self):
		self.model = load_model(self.filename + '.h5')
		self.target_model = load_model(self.filename + '.h5')

	def getFilename(self, filename=None, filenameAppendage=None, intelligentFilename = True, directory = None):
		if directory != None:
			filename = directory + "/"
		else:
			filename = ""
		if intelligentFilename == True:
			if self.isDueling and self.isDual:
				filename+="D3QN"
			elif self.isDueling:
				filename += "DuelingDQN"
			elif self.isDual:
				filename += "DDQN"
			else:
				filename += "DQN"
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
			if self.isDueling:
				filename += "_dueling"
		return(filename)
	# are we supposed to save the target model or the training model?
	def save_model(self, filename=None, filenameAppendage=None, intelligentFilename = True, directory = None):
		filename = self.getFilename(filename=filename, filenameAppendage=filenameAppendage, intelligentFilename = intelligentFilename, directory = directory)
		self.model.save(filename + '.h5')

		

