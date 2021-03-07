
import serial
import array
import time

def sendLiveModelUpdateToArduino(comPort = 'COM6'):
	# can we go faster? it's a USB serial port on the other end, and seems to work regardless
	# of wether or not this speed matches their speed.
	ser = serial.Serial(comPort, 1000000, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE)

	dataFile = open('TeensyModel_good.h')
	modelDataText = dataFile.read()
	dataFile.close()

	#print(modelDataText)

	# strip off all the header information
	modelDataText = modelDataText[modelDataText.index('{')+1:]
	modelDataText = modelDataText[:modelDataText.index('}')]
	modelDataText= modelDataText.replace('\n', '')
	modelDataText= modelDataText.replace('\t', '')

	# separate the text into a list
	modelDataList = modelDataText.split(',')

	# convert hexidecimal strings to integers
	modelDataListAsInts = []
	for entry in modelDataList:
		modelDataListAsInts.append(int(entry,0))

	# another option for sending data, encoded as strings?
	#modelData = array.array('B', [0x00, 0x01, 0x02]).tostring()
	#ser.write(modelData.encode())

	modelDataAsByteArray = bytearray(modelDataListAsInts)

	print("Number of bytes to send: " + str(len(modelDataListAsInts)))
	print("At 1Mega baud, this should take " + str(int((len(modelDataListAsInts)*8.0)/1000)) + "ms")

	# NEED TO FLUSH SERIAL BUFFER HERE
	ser.write(bytes([99])) # send a single byte to Teensy to indicate we have data for it. Value doesn't matter.
		# there is no error checking, we just have to stay synced on number of bytes transmitted (yikes!). This
		# is the least effort and the least overhead, and so far seems to work, but also a bit ugly. If we ever
		# de-sync, we might resync at some point as both sides flush the buffer when beginning.
	waiting = True
	print("Waiting for serial confirmation from teensy before sending data")
	while waiting:
		incomingText = ser.readline()
		if incomingText == b'ReadyForDownload\r\n':
			print("Received go-ahead text. Getting ready to send data!")
			waiting = False
		else:
			print("Received the following: " + str(incomingText))
		time.sleep(1)
	#ser.read()
	
	ser.write(modelDataAsByteArray)

	while(1):
		incomingText = ser.readline()
		print(incomingText)
		#time.sleep(1)

if __name__ == '__main__':
	sendLiveModelUpdateToArduino()