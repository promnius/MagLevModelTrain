
import pandas as pd
import numpy as np
import datetime
import tensorflow as tf
import createDataset
from tensorflow.keras import layers, Sequential

def main():
	# get data ready
	csv_path = 'jena_climate_2009_2016.csv'
	df = pd.read_csv(csv_path)
	df = df[5::6]
	date_time = pd.to_datetime(df.pop('Date Time'), format='%d.%m.%Y %H:%M:%S')

	print(df.head())

	print(df.describe().transpose())

	# fix bad wind data
	wv = df['wv (m/s)']
	bad_wv = wv == -9999.0
	wv[bad_wv] = 0.0

	max_wv = df['max. wv (m/s)']
	bad_max_wv = max_wv == -9999.0
	max_wv[bad_max_wv] = 0.0

	# The above inplace edits are reflected in the DataFrame
	df['wv (m/s)'].min()

	# convert wind to vectors, to be more useful as a continous numeric entry (0 and 360 degrees are the same)
	wv = df.pop('wv (m/s)')
	max_wv = df.pop('max. wv (m/s)')

	# Convert to radians.
	wd_rad = df.pop('wd (deg)')*np.pi / 180

	# Calculate the wind x and y components.
	df['Wx'] = wv*np.cos(wd_rad)
	df['Wy'] = wv*np.sin(wd_rad)

	# Calculate the max wind x and y components.
	df['max Wx'] = max_wv*np.cos(wd_rad)
	df['max Wy'] = max_wv*np.sin(wd_rad)

	# convert date-time to seconds, again, to be more useful as a continous numeric entry
	timestamp_s = date_time.map(datetime.datetime.timestamp)

	# create time of day and time of year signals, since seconds will be hard to pull periodic info from.
	# if you didn't know what signals would be useful, use an FFT on the data
	day = 24*60*60
	year = (365.2425)*day

	df['Day sin'] = np.sin(timestamp_s * (2 * np.pi / day))
	df['Day cos'] = np.cos(timestamp_s * (2 * np.pi / day))
	df['Year sin'] = np.sin(timestamp_s * (2 * np.pi / year))
	df['Year cos'] = np.cos(timestamp_s * (2 * np.pi / year))


	# do NOT scramble data, so results are purely from AFTER training set.
	# split it into train, val, and test
	column_indices = {name: i for i, name in enumerate(df.columns)}
	n = len(df)
	train_df = df[0:int(n*0.7)]
	val_df = df[int(n*0.7):int(n*0.9)]
	test_df = df[int(n*0.9):]
	num_features = df.shape[1]

	# normalizing data. This becomes an interesting topic, as you need to be sure
	# you don't use look-ahead at info you shouldn't have. to do this you could use a 
	# rolling window to compute mean and deviation, but we won't here. Note that it's 
	# VERY IMPORTANT that we pull these ONLY from the training set and use them for the 
	# val and test sets as well.

	train_mean = train_df.mean()
	train_std = train_df.std()

	train_df = (train_df - train_mean) / train_std
	val_df = (val_df - train_mean) / train_std
	test_df = (test_df - train_mean) / train_std

	myWindowGenerator = WindowGenerator(6,1,1,train_df=train_df, val_df=val_df, test_df=test_df,label_columns=['T (degC)'])

	print(myWindowGenerator)

	singleStep(train_df=train_df, val_df=val_df, test_df=test_df)
	#multiStepInputSingleStepOutput(train_df=train_df, val_df=val_df, test_df=test_df, CONV_WIDTH=3)


def singleStep(train_df, val_df, test_df):
	val_performance = {}
	performance = {}
	single_step_window = WindowGenerator(input_width=1, label_width=1, shift=1,train_df=train_df, val_df=val_df, test_df=test_df,label_columns=['T (degC)'])

	#baseline = Baseline(label_index=column_indices['T (degC)'])

	#baseline.compile(loss=tf.losses.MeanSquaredError(), metrics=[tf.metrics.MeanAbsoluteError()])

	val_x,val_y = single_step_window.val
	#val_performance['Baseline'] = baseline.evaluate(val_x,val_y, batch_size=32, verbose=2)
	test_x, test_y = single_step_window.test
	#performance['Baseline'] = baseline.evaluate(test_x,test_y, batch_size=32, verbose=2)

	dense = tf.keras.Sequential([
			tf.keras.layers.Dense(units=64, activation='relu'),
			tf.keras.layers.Dense(units=64, activation='relu'),
			tf.keras.layers.Dense(units=1)
	])

	print("ValX shape: " + str(val_x.shape))
	squeezed = val_x.squeeze()
	print("ValX after squezzing: " + str(squeezed.shape))
	print("Squeezed, shape[1]: " + str(squeezed.shape[1]))
	
	dense2 = Sequential()
	dense2.add(layers.Dense(16, activation='relu', input_shape=(squeezed.shape[1],)))
	#model.add(layers.Dropout(0.25))
	dense2.add(layers.Dense(16, activation='relu'))
	#model.add(layers.Dropout(0.25))
	dense2.add(layers.Dense(1, activation='relu'))

	dense2.compile(loss=tf.losses.MeanSquaredError(),
			optimizer=tf.optimizers.Adam(),
			metrics=[tf.metrics.MeanAbsoluteError()])

	train_x, train_y = single_step_window.train
	train_x = train_x.squeeze()
	train_y = train_y.squeeze()
	val_x, val_y = single_step_window.val
	val_x = val_x.squeeze()
	val_y = val_y.squeeze()
	history2 = dense2.fit(train_x, train_y, epochs=20,
				validation_data=(val_x, val_y), batch_size=128, verbose=2)
	

	history = compile_and_fit(dense, single_step_window)

	val_performance['Dense'] = dense.evaluate(val_x,val_y,verbose=2)
	performance['Dense'] = dense.evaluate(test_x,test_y, verbose=2)

	dense.save('dense_single_weather_model.h5')
	dense2.save('dense2_single_weather_model.h5')

def multiStepInputSingleStepOutput(train_df, val_df, test_df,CONV_WIDTH = 3):
	val_performance = {}
	performance = {}
	conv_window = WindowGenerator(input_width=CONV_WIDTH,label_width=1,shift=1,train_df=train_df, val_df=val_df, test_df=test_df,label_columns=['T (degC)'])
	print(conv_window)

	multi_step_dense = tf.keras.Sequential([
		# Shape: (time, features) => (time*features)
		tf.keras.layers.Flatten(),
		tf.keras.layers.Dense(units=32, activation='relu'),
		tf.keras.layers.Dense(units=32, activation='relu'),
		tf.keras.layers.Dense(units=1),
		# Add back the time dimension.
		# Shape: (outputs) => (1, outputs)
		tf.keras.layers.Reshape([1, -1]),])

	val_x,val_y = conv_window.val
	test_x, test_y = conv_window.test

	history = compile_and_fit(multi_step_dense, conv_window)
	val_performance['Multi step dense'] = multi_step_dense.evaluate(val_x,val_y, verbose=2)
	performance['Multi step dense'] = multi_step_dense.evaluate(test_x, test_y, verbose=2)

	conv_model = tf.keras.Sequential([
		tf.keras.layers.Conv1D(filters=32,
								kernel_size=(CONV_WIDTH,),
								activation='relu'),
		tf.keras.layers.Dense(units=32, activation='relu'),
		tf.keras.layers.Dense(units=1),])

	lstm_model = tf.keras.models.Sequential([
		# Shape [batch, time, features] => [batch, time, lstm_units]
		tf.keras.layers.LSTM(32,),# return_sequences=True),
		# Shape => [batch, time, features]
		tf.keras.layers.Dense(units=1)])

	history = compile_and_fit(lstm_model, conv_window)
	val_performance['LSTM'] = lstm_model.evaluate(val_x,val_y, verbose=2)
	performance['LSTM'] = lstm_model.evaluate(test_x, test_y, verbose=2)

	print("Val performance:")
	for name, value in val_performance.items():
		print(f'{name:12s}: {value[1]:0.4f}')
	print("Test Performance")
	for name, value in performance.items():
		print(f'{name:12s}: {value[1]:0.4f}')

	lstm_model.save('lstm_weather_model.h5')
	multi_step_dense.save('dense_weather_model.h5')


def compile_and_fit(model, window):

	model.compile(loss=tf.losses.MeanSquaredError(),
				optimizer=tf.optimizers.Adam(),
				metrics=[tf.metrics.MeanAbsoluteError()])

	train_x, train_y = window.train
	val_x, val_y = window.val
	history = model.fit(train_x, train_y, epochs=20,
				validation_data=(val_x, val_y), batch_size=128, verbose=2)
	return history

class Baseline(tf.keras.Model):
	def __init__(self, label_index=None):
		super().__init__()
		self.label_index = label_index

	def call(self, inputs):
		if self.label_index is None:
			return inputs
		result = inputs[:, :, self.label_index]
		return result[:, :, tf.newaxis]

class WindowGenerator():
	def __init__(self, input_width, label_width, shift,
				   train_df, val_df, test_df,
				   label_columns=None):
		# Store the raw data.
		self.train_df = train_df
		self.val_df = val_df
		self.test_df = test_df

		# Work out the label column indices.
		self.label_columns = label_columns
		if label_columns is not None:
		  self.label_columns_indices = {name: i for i, name in
										enumerate(label_columns)}
		self.column_indices = {name: i for i, name in
							   enumerate(train_df.columns)}

		# Work out the window parameters.
		self.input_width = input_width
		self.label_width = label_width
		self.shift = shift

		self.total_window_size = input_width + shift

		self.input_slice = slice(0, input_width)
		#print("input slicer object: " + str(self.input_slice))
		self.input_indices = np.arange(self.total_window_size)[self.input_slice]
		#print("input indices: " + str(self.input_indices))

		self.label_start = self.total_window_size - self.label_width
		self.labels_slice = slice(self.label_start, None)
		#print("output (label) slicer object: " + str(self.labels_slice))
		self.label_indices = np.arange(self.total_window_size)[self.labels_slice]
		#print("output (label) indices: " + str(self.label_indices))

	def __repr__(self):
		return '\n'.join([
			f'Total window size: {self.total_window_size}',
			f'Input indices: {self.input_indices}',
			f'Label indices: {self.label_indices}',
			f'Label column name(s): {self.label_columns}'])

	def split_window(self, features):
		inputs = features[:, self.input_slice, :]
		labels = features[:, self.labels_slice, :]
		if self.label_columns is not None:
			labels = tf.stack(
				[labels[:, :, self.column_indices[name]] for name in self.label_columns],
				axis=-1)

		# Slicing doesn't preserve static shape information, so set the shapes
		# manually. This way the `tf.data.Datasets` are easier to inspect.
		inputs.set_shape([None, self.input_width, None])
		labels.set_shape([None, self.label_width, None])

		return inputs, labels

	def make_dataset_from_continous_pandas_timeseries(self, data,verbose=2):
		npData = np.array(data, dtype=np.float32)
		if verbose>0: print("npData Shape: " + str(npData.shape))
		numRows = len(data.index)
		if verbose>0: print("number of rows: " + str(numRows))
		expandedDataInput = []
		expandedDataOutput = []
		for counter in range(numRows+1-self.total_window_size):
			expandedDataInput.append(npData[counter:counter+self.input_width,0:npData.shape[1]+1])
			# outputs must be consecutive! that's the only limit here, since we use basic slicing
			# note the "None" is required if using only 1 output variable to maintain dimensionality
			expandedDataOutput.append(npData[counter+self.input_width:counter+self.total_window_size,1,None]) # NEED TO MAKE THIS AUTO DETECT FROM label_column_indices
		if verbose>0: print("Number of discrete timeseries: " + str(len(expandedDataInput)))
		if verbose>0: print("Input timeseries shape: " + str(expandedDataInput[0].shape))
		if verbose>0: print("Output timeseries shape: " + str(expandedDataOutput[0].shape))
		npExpandedDataInput = np.stack(expandedDataInput, axis=0 )
		npExpandedDataOutput = np.stack(expandedDataOutput, axis=0 )
		if verbose>0: print("Input full dataset shape: " + str(npExpandedDataInput.shape))
		if verbose>0: print("Output full dataset shape: " + str(npExpandedDataOutput.shape))
		if verbose > 1: print("first input: " + str(expandedDataInput[0]))
		if verbose > 1: print("first output: " + str(expandedDataOutput[0]))

		return npExpandedDataInput,npExpandedDataOutput

	# simple ways to get built datasets of the data contained in this class
	@property
	def train(self):
		return self.make_dataset_from_continous_pandas_timeseries(self.train_df)

	@property
	def val(self):
		return self.make_dataset_from_continous_pandas_timeseries(self.val_df)

	@property
	def test(self):
		return self.make_dataset_from_continous_pandas_timeseries(self.test_df)



if __name__ == '__main__':
	main()