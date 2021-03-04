
import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import tensorflow as tf
from tinymlgen import port
import numpy as np



def main(tfModelName = "TeensyModel", modelName = None):
	if modelName == NONE:
		#model = tf.keras.models.load_model('dense2_single_weather_model.h5')
		#model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network2x256_run11_numGames800_score500.0.h5')
		#model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network4x16_run23_numGames800_score438.73.h5')
		#model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network2x32_run18_numGames800_score391.24.h5')
		
		#model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network3x64_run20_numGames800_score500.0.h5')
		#model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network4x32_run22_numGames800_score500.0.h5')
		model = tf.keras.models.load_model('results/DQNbasic_lr0.0001_LI1_bs256_g0.95_e1_t0.05_network4x64_run21_numGames800_score498.75.h5')
	else:
		model = tf.keras.models.load_model('results/' + modelName + '.h5')

	print(model.summary())

	with open('TeensyModels/' + tfModelName + '.h', 'w') as f:  # change path if needed
		f.write(port(model, optimize=False, pretty_print=True, variable_name='modelParams'))

	# show test outputs for cartpole to prove teensy model gives similar results
	#an_input = [[.05,.05,.05,.05,0]]
	#npData = np.array(an_input, dtype=np.float32)
	#print("Shape: " + str(npData.shape))
	#outputPrediction = model.predict(npData)
	#print("Output Prediction: " + str(outputPrediction))

if __name__ == '__main__':
	main()