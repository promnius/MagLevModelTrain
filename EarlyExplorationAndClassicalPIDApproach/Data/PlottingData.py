
import csv
import matplotlib.pyplot as plt

#with open('SampleDataDump.txt', 'rb') as f:
with open('SampleDataDump.txt', 'rt') as f:
    reader = csv.reader(f)
    myData = list(reader)

timestamps = []
positions = []
powerLevels = []
for entry in myData:
	timestamps.append(int(entry[0]))
	positions.append(int(entry[1]))
	powerLevels.append(int(entry[2]))

velocitys = []
previousPosition = positions[0]
for position in positions:
	velocitys.append(position-previousPosition)
	previousPosition=position

accelerations = []
previousVelocity = velocitys[0]
for velocity in velocitys:
	accelerations.append(velocity-previousVelocity)
	previousVelocity = velocity


# crude scaling to put everything on the same axis
for a in velocitys:
	a = a*5
for a in powerLevels:
	a = a *2
for a in positions:
	a = a -1000


plt.plot(positions)
plt.plot(powerLevels)
plt.plot(velocitys)
plt.plot(accelerations)
#plt.ylabel('some numbers')
plt.show()

#print (myData[0])