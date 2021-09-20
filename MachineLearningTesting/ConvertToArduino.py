
import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import tensorflow as tf
from tinymlgen import port
import numpy as np



def main(tfModelName = "TeensyModel", modelName = None, debug=False):
	if modelName == None:
		#model = tf.keras.models.load_model('dense2_single_weather_model.h5')
		#model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network2x256_run11_numGames800_score500.0.h5')
		#model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network4x16_run23_numGames800_score438.73.h5')
		#model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network2x32_run18_numGames800_score391.24.h5')
		
		#model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network3x64_run20_numGames800_score500.0.h5')
		#model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network4x32_run22_numGames800_score500.0.h5')
		#model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network4x64_run21_numGames800_score498.75.h5')

		#model = tf.keras.models.load_model('results/td3/011_Pendulum_lr0.001-0.001_LI1_bs100_g0.99_t0.005_n0.1_network128-128-64-64-32/models/actor.h5')
		#model = tf.keras.models.load_model('results/td3/012_Pendulum_lr0.001-0.001_LI1_bs100_g0.99_t0.005_n0.1_network128-128-64-64-32/models/actor.h5')
		#model = tf.keras.models.load_model('results/td3/013_Pendulum_lr0.001-0.001_LI1_bs100_g0.99_t0.005_n0.1_network128-64-64-32/models/actor.h5')
		#model = tf.keras.models.load_model('results/td3/014_Pendulum_lr0.001-0.001_LI1_bs100_g0.99_t0.005_n0.1_network128-64-64-32/models/actor.h5')
		#model = tf.keras.models.load_model('break_me.h5')
		#model = tf.keras.models.load_model('results/td3/022_Pendulum_lr0.001-0.001_LI1_bs100_g0.99_t0.005_n0.05_network128-128-64-64-32/models/actor.h5')
		#model = tf.keras.models.load_model('results/td3/024_Pendulum_lr0.001-0.001_LI1_bs100_g0.99_t0.005_n0.05_network128-128-64-64-32/models/actor.h5')

		model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LISTEP_bs256_g0.95_e1_t0.05_network4x64_run1100_numGames500_score-11669.970607377209.h5')
	else:
		model = tf.keras.models.load_model('results/' + modelName + '.h5')

	print(model.summary())

	with open('TeensyModels/' + tfModelName + '.h', 'w') as f:  # change path if needed
		text = port(model, optimize=False, pretty_print=True, variable_name='modelParams')
		text = text.replace(' int ', ' long ') # model size may be too big to describe length in int
		text = text.replace('const', '') # for live updates, this array needs to be modifyable
		f.write(text)
		#f.write(port(model, optimize=False))

	#afile = open('TeensyModels/' + tfModelName + '.h', 'rw')

	# show test outputs to prove teensy model gives similar results
	if debug:
		an_input = [[.05,.05,.05,]]
		npData = np.array(an_input, dtype=np.float32)
		#print("Shape: " + str(npData.shape))
		outputPrediction = model.predict(npData)
		print("Output Prediction: " + str(outputPrediction))

if __name__ == '__main__':
	main()