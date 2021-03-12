
import random
random.seed(a=None, version=2)
from scipy import interpolate
import matplotlib.pyplot as plt

class MagLevEnvironment:
	# min mass: 25, max 100. measured in grams. timestep measured in ms, .001 recommended.
	def __init__(self, mass=25, timestep=.001, initCase=0, rewardFunction=0, targetFrequency = 1):
		self.mass=mass
		self.timestep=timestep
		self.upperRail = 80 # based on physics simulation, this is as close as you can get and still gurantee disengagement.
		# must be physically enforced in the real world
		self.lowerRail = 300 # maximum airgap that can be compensated for with electromagnet
		self.dynamicRange = self.lowerRail - self.upperRail
		self.initCase = initCase 
		self.position = self.lowerRail # measured in thou
		self.velocity = 0 # measured in thou per timestep, typically thou per mS (which is also inches per second)
		self.accel = 0 # measured in thou per mS^2 (for default timestep)
		self.targetState = self.lowerRail # will be initialized during a reset call
		# options:
		# 1= init on lower rail
		# 2= init on upper rail
		# 3= init in (exact) middle with no velocity/accel
		# 4= init randomly between upper and lower rail (bounded to avoid rails by a small margin), no velocity/accel
		# 5= init randomly between upper and lower rail (bounded to avoid rails by a small margin) with random velocity/accel
		# 6= init randomly between upper and lower rail (includes rails) with random velocity/accel
		self.rewardFunction = rewardFunction
		# options:
		# 0= target fixed levitation in the middle
		# 1= target fixed levitation close to the magnet (steeper derivatives, so harder to deal with system noise)
		# 2= target a sine wave (bounded to avoid rails by a small margin), useful to learn the environment across all operating locations
		# 3= target zero power
		# note that all rewards, in addition to penalizing position error or power, will also penalize absolute velocity (speed) and acceleration, quadratically.
		# this gets a little interesting in the sine wave case, as it is actually best to NOT follow the sine wave perfectly depending on frequency, to avoid
		# accel and velocity
		# there are also penalties for overshoot, and non-linear penalties for positions close to rails so it doesn't learn to stabilize accel/velocity
		# by bouncing into the rails. In the real world, we'd penalize angle, but there is no mechanism for fixing that here
		self.targetFrequency = targetFrequency # only used during a sine wave target

		# this block of text that matches airgaps and currents to forces is based on an ansys maxwell simulation of the 
		# geometry that is being simulated in this environment
		self.xAirgaps = [400,375,350,325,300,275,250,225,200,175,150,125,100,90,80,70,60,50,40,30,20,10,0]
		self.yCurrents = [-1.5,-1.25,-1,-.75,-.5,-.25,0,.25,.5,.75,1,1.25,1.5]
		self.zForces = [[0.183424470886832,0.153397925666468,0.123382888889795,0.0933836681904324,0.0633921631066783,0.0334259788993681,0.00347362590521476,-0.0264701936085039,-0.0564115183275098,-0.0863559646262982,-0.116315721500309,-0.146274424810812,-0.176240092744966],
				[0.21910143529203,0.184949582453043,0.150811058417318,0.116691042606166,0.0825804391626188,0.0484994418980065,0.014435266313452,-0.0196180177672568,-0.0536672755904933,-0.0877189921060066,-0.121787292410203,-0.155853819173416,-0.189927745237136],
				[0.258179838561267,0.219216825395238,0.180269316030352,0.14134357199696,0.10242945317304,0.0635501561977376,0.0246914945565655,-0.0141531823840316,-0.0529917119505025,-0.0918316268387913,-0.130689351768664,-0.169544342036843,-0.208407072407975],
				[0.303912524050038,0.259293069606313,0.21469173588964,0.170116054067304,0.125554833354198,0.0810347177078598,0.0365400861958598,-0.00793648728004276,-0.0524039452618166,-0.096871058993464,-0.141356951585617,-0.185838771723645,-0.230328452930395],
				[0.337660752691447,0.286349889620766,0.235060377912019,0.183801246913671,0.132560337874065,0.0813682537949624,0.0302079311248068,-0.0209289015811834,-0.0720524500858107,-0.123172950183977,-0.174312668349669,-0.225446429745332,-0.276587728355585],
				[0.412009812023448,0.352686668551897,0.293388900561552,0.234127260674536,0.174888860111179,0.115708737915695,0.0565686002015545,-0.00254070848073057,-0.0616308353755966,-0.120713788208825,-0.179815556408783,-0.238908795896361,-0.298008690804576],
				[0.479135175204418,0.410321592174013,0.341538403508711,0.272798198032335,0.204108252688979,0.135447478299007,0.066857686648664,-0.00169144344151118,-0.0702131597029328,-0.138721547076172,-0.207228854198907,-0.275759837661149,-0.344277640968226],
				[0.590705206840787,0.510395260196784,0.430122191864315,0.349900511827479,0.269740990971775,0.189619204462408,0.109585297119747,0.0296054568243427,-0.0503352650471654,-0.130253365239687,-0.210163755728364,-0.290096679989603,-0.370010944197277],
				[0.725703340620689,0.631607754740825,0.537557571128312,0.443568950123902,0.349657229250767,0.255824693128581,0.162042072266824,0.0683612464852529,-0.025263958500406,-0.118852771400676,-0.212423349590433,-0.306012189765497,-0.399574822820854],
				[0.892797477776976,0.782417879408169,0.672094772858265,0.561845648317701,0.451690896114715,0.341637091030019,0.231680813245666,0.121791402390188,0.0120125750450287,-0.0977099239888822,-0.207398077191016,-0.317095051349792,-0.426755591157208],
				[1.13801009897187,1.00764548194503,0.877352616061496,0.747149923171302,0.617061881993083,0.487103093489749,0.357272009382576,0.22755989810638,0.097937133668411,-0.0315692645645189,-0.161016199062474,-0.290453420034248,-0.419839551860262],
				[1.45864353566051,1.30427140123006,1.14999153687887,0.995823190909622,0.841792594484772,0.68792488614563,0.534224337339258,0.38068366025964,0.227287604526437,0.0740144229912016,-0.0791458721940866,-0.232260168258036,-0.385301991530242],
				[1.93892558974433,1.75543030118537,1.5720550004654,1.38881990117108,1.20575156701515,1.02288193718382,0.840229101018156,0.657790102363318,0.475551282043577,0.293490272987711,0.111579311714881,-0.0702096874898767,-0.251900159307561],
				[2.20720696645505,2.01011357575238,1.81315390185924,1.61634882597742,1.41972497272444,1.22331531705154,1.02714423961442,0.831213088928075,0.63550965751301,0.440012005535369,0.244691519436694,0.0495180521250475,-0.145534626257886],
				[2.55837172716632,2.34576473742921,2.13330718148288,1.92102076671527,1.70893208771192,1.49707463189686,1.28547803296507,1.0741502235736,0.86308148186911,0.652250961340031,0.441629724588068,0.231185802184212,0.0208900335483704],
				[2.92289468496959,2.69641369320959,2.47009782960298,2.24396974560951,2.0180562201553,1.79239090407425,1.56700681497533,1.34191846214771,1.11711954764781,0.892590727399956,0.668303204813835,0.444223705627098,0.220320612704581],
				[3.38771762567144,3.14571967444218,2.90390425891933,2.66229481517097,2.42091844553872,2.17980879474202,1.93900138484319,1.69851755000718,1.45835642746711,1.21850121573433,0.978923989223963,0.73959050891249,0.500466495337927],
				[3.99942396170297,3.74087455019613,3.48252795036111,3.22440708460781,2.96653954474458,2.70895898151532,2.45170293441841,2.19479890000001,1.93825295784155,1.68205207382478,1.42616987554305,1.17057154491683,0.915220279648275],
				[4.8277522574219,4.55081508369952,4.27410634554935,3.99764551495604,3.72146056241665,3.44558533772523,3.17005904492326,2.89491471959594,2.62016634787533,2.34580644816723,2.07181102483846,1.79814514412603,1.52476994636089],
				[5.92613855791995,5.63106555122257,5.33624830716387,5.04170097727639,4.74745173989456,4.45353548542382,4.15999282898298,3.86686094696937,3.57416052601245,3.28189019429805,2.99002903848319,2.69854315128721,2.40739272473977],
				[7.40297350651928,7.09443683990071,6.78617231528737,6.47819099984459,6.17052015340727,5.86319556277029,5.55625811027582,5.2497468332473,4.94368639483905,4.63808007370021,4.33291068117205,4.02814641589354,3.72374764506107],
				[9.77684807142577,9.45565185689713,9.13476006560065,8.81418287752954,8.49394414650663,8.17407771433474,7.85462147251183,7.53561259734672,7.21707775377379,6.89902532547334,6.58144407827609,6.264306611235,5.94757579689949],
				[2.3363755078511,12.2504375267808,11.9393741463785,11.6419665374911,11.4243574934185,11.2013254998009,10.9725487020242,10.7378394660804,10.4970939949939,10.2502781006446,9.99755715753438,9.73931932128784,9.47606155827266]]
		self.forceLookupFunction = interpolate.interp2d(self.yCurrents,self.xAirgaps,self.zForces,kind='linear')

	# generate random values for parameters that are typically fixed within a single 
	# simulation, but should change from incarnation to incarnation
	def randomizePhysicalParameters(self, updateMass=True, updateSensorErrors=True):
		pass
		if updateSensorErrors:
			self.sensor1Offset = (random.random()-.5)*20 # generates -10 to 10
			self.sensor2Offset = (random.random()-.5)*20
			self.sensor3Offset = (random.random()-.5)*20

	def reset(self):
		self.states = []
		self.actions = []
		self.rewards = []
		self.observedStates = []

		# options:
		# 1= init on lower rail
		# 2= init on upper rail
		# 3= init in (exact) middle with no velocity/accel
		# 4= init randomly between upper and lower rail (bounded to avoid rails by a small margin), no velocity/accel
		# 5= init randomly between upper and lower rail (bounded to avoid rails by a small margin) with random velocity/accel
		# 6= init randomly between upper and lower rail (includes rails) with random velocity/accel

		if self.initCase == 1:
			self.position = self.upperRail
		elif self.initCase == 2:
			self.position = self.lowerRail
		elif self.initCase == 3:
			self.position = (self.upperRail + self.lowerRail) / 2
		elif self.initCase == 4 or self.initCase == 5:
			self.position == self.upperRail + random.random()*.8*self.dynamicRange + .1*self.dynamicRange
		elif self.initCase == 6:
			# increase probability of spawning on one of the rails
			self.position == self.upperRail-.2*self.dynamicRange + 1.4*random.random()*self.dynamicRange
			if self.position < self.upperRail:
				self.position = self.upperRail
			if self.position > self.lowerRail:
				self.position = self.lowerRail

		if self.initCase == 1 or self.initCase == 2 or self.initCase == 3 or self.initCase == 4:
			self.accel = 0
			self.velocity = 0
		elif self.initCase == 5 or self.initCase == 6:
			#self.accel = 2*random.random()
			self.accel = 0 # accel doesn't matter, it has 1 step control
			self.velocity = 10*random.random()

		if self.rewardFunction == 0 or self.rewardFunction == 2:
			self.targetState = (self.upperRail + self.lowerRail) / 2
		elif self.rewardFunction == 1:
			#self.targetState = self.upperRail + .3*self.dynamicRange
			self.pickNewTarget()
		elif self.rewardFunction == 3:
			self.targetState = 0 # target state is now interpreted as power

		self.positionError = self.targetState-self.position
		self.states.append([self.position,self.velocity,self.accel,self.positionError,0,0])

		return(self.getReturnableState())

	# action is a current in Amps
	def step(self, action):
		if action > 1.5:
			print("ERROR, target current > 1.5A provided")
			action = 1.5
		if action < -1.5:
			print("ERROR, target current < -1.5A provided")
			action = -1.5
		self.action = action
		if self.rewardFunction == 2: # sine wave mode, need to update target state
			pass # not implemented yet
			#self.targetState
		# PHYSICS UPDATE
		self.accel = self.getAccel(current=action,airgap=self.position,mass=self.mass)
		for counter in range(5): # VERY crude ODE, split into multiple steps for higher accuracy
			self.position += self.velocity/5
			self.velocity += self.accel/5
			self.accel = self.getAccel(current=action,airgap=self.position,mass=self.mass)
			if self.position <= self.upperRail:
				self.position = self.upperRail
				if self.velocity < 0: # we are pushing into the rail
					self.velocity = 0
			if self.position >= self.lowerRail:
				self.position = self.lowerRail
				if self.velocity > 0: # we are pushing into the rail
					self.velocity = 0
			self.positionError = self.targetState-self.position
		# INJECT SENSOR NOISE (optional)
		# COMPUTE REWARD
		reward = self.getRewardIdeal()
		# LOG DATA
		self.states.append([self.position,self.velocity,self.accel,self.positionError,action,reward])
		# NORMALIZE VALUES
		# BUILD STATE VARIABLE, USING HISTORIC INFO IF DESIRED
		returnableState =  self.getReturnableState()		
		# RETURN new_state, r, done, info AS NORMALIZED VALUES (ALSO FROM RESET)
		return (returnableState, reward, False, "")
	
	# for now, just the most recent entries for position, velocity, accel, and position error. Could include
	# timeseries info here at some point.
	def getReturnableState(self):
		#normalizedPosition = self.position/self.lowerRail
		normalizedPosition = self.position
		#normalizedPositionError = self.positionError/self.dynamicRange
		normalizedPositionError = self.positionError
		return ([normalizedPosition,self.velocity,self.accel,normalizedPositionError])


	# compute acceleration from a lookup tabe generated by Ansys Maxwell based on our hardware model
	def getAccel(self,current,airgap,mass):

		force = self.forceLookupFunction(current,airgap) # in newtons
		force = force[0] # it comes back as a 1x1 array
		# lets keep our orientations correct: positive force means the unit is being pulled up toward the track,
		# but this shows up in decreasing position (so positive force is negative acceleration, we need a negative somewhere here)
		# gravity is trying to open up the airgap, so negative force. To keep units straight, we'll multiply out gravity with mass
		# to get a force in newtons and sum that now.
		#.001N = 1(m/s^2)*g
		gravityForce = (9.8*mass)/1000
		#print(force)
		force = force-gravityForce
		# .0254*F(N) = (M(g)*A(1000in/s^2))

		# 1 thou/ms^2 = 25.4m/s^2
		# 1 g = .001Kg
		# so N = .001g*25.4A
		# A = N/(.0254g)
		#print(force)
		acceleration = (force)/(.0254*mass)
		acceleration = - acceleration
		return(acceleration)

	def setMass(self, mass):
		self.mass=mass

	def setTimestep(self, timestep):
		self.timestep=timestep

	def plotRun(self):
		pass

	def plotObservedRun(self):
		pass

	# given all the quirks of our simulated real-world sensors, what are the states we actually see?
	def observeState(self):
		self.observedPosition1 = self.possition + self.sensor1Offset + self.generateSensorNoise()
		self.observedPosition2 = self.possition + self.sensor2Offset + self.generateSensorNoise()
		self.observedPosition3 = self.possition + self.sensor3Offset + self.generateSensorNoise()
		observedVelocity1 = (self.observedPosition1-self.observedPosition1_last)/self.timestep
		observedVelocity2 = (self.observedPosition2-self.observedPosition2_last)/self.timestep
		observedVelocity3 = (self.observedPosition3-self.observedPosition3_last)/self.timestep
		observedAccel1 = (self.observedVelocity1-self.observedVelocity1_last)/self.timestep
		observedAccel2 = (self.observedVelocity2-self.observedVelocity2_last)/self.timestep
		observedAccel3 = (self.observedVelocity3-self.observedVelocity3_last)/self.timestep

		# also create rolling averages of the states here? 

		self.observedStates.append[observedPosition1,observedPosition2,observedPosition3,observedVelocity1,observedVelocity2,observedVelocity3,observedAccel1,observedAccel2,observedAccel3]

	# this could be gaussian, and the values could depend on our sensors
	def generateSensorNoise(self):
		return((random.random()-.5)*4) # +/- 2 thou

	# calculate a very carefuly tailored reward function designed to shape
	# how the system learns to move. Tuning a and b will change the parameters.
	# kind of wierd to have them here instead of in the agent class, since they are hyper
	# parameters that influence how the problem is solved, but remember, they are describing
	# what it means to have a perfect solution, not how to find such a solution, and as such
	# they are part of the environment. 
	# Note that the rewards must be calculated off of the observed values, as in the real
	# world we have no way of collecting other info.
	def getRewardIdeal(self):
		# position error is already valued as absolute, so that sets the scale. ie, an error of 100 thou is a cost of 100.
		# 
		a=2 # velocity coefficient, 1 causes a velocity of 10 thou/mS to give error of 100 and velocity of 20 to give error of 200
		# this is a pretty aggressive anti-velocity term, but that may be a good thing. (may even need stronger). At 1 thou/ mS, we
		# could still bounce between the rails a few times per second, so step response targets should probably be closer to .2 thou
		# per mS, and at this point, it's barely penalized at all. 
		b=0 # acceleration penalty coefficient, which is similar but not identical to power penalty. For starters, let this be 0, ie,
		# we'll target whatever acceleration is needed to cancel out velocity, then let velocity determine path to 0 position error.
		RAIL_PENALTY = 500 # penalty for being within 10% of either rail. this should outweigh all other penalties under reasonably
		# normal operating conditions or else 

		reward = 0
		if self.rewardFunction == 0 or self.rewardFunction == 1 or self.rewardFunction == 2:
			# penalize position error
			#reward += abs(self.observedPosition1-self.targetState) + abs(self.observedPosition2-self.targetState) + abs(self.observedPosition3-self.targetState)
			reward += abs(self.position-self.targetState)
			# penalize overshoot? NOT IMPLEMENTED YET

		elif self.rewardFunction == 3:
			# penalize power error
			reward += abs(self.action) * 100 # *100 puts this on a similar scale to position, since position has dynamic range of 200-300 thou,
			# and current is +/- 1.5 amps
		# penalize velocity QUESTION SHOULD THIS BE INSTANTANEOUS VELOCITY OR AVERAGE VELOCITY??
		#reward += a*self.observedVelocity1_ave**2 + a*self.observedVelocity2_ave**2 + a*self.observedVelocity3_ave**2
		if abs(self.velocity) > 3: # allow small velocities so we can still correct position errors
			reward += a*self.velocity**2
		# penalize accel QUESTION SHOULD THIS BE INSTANTANEOUS ACCEL OR AVERAGE ACCEL??
		#reward += b*self.observedAccel1_ave**2 + a*self.observedAccel2_ave**2 + a*self.observedAccel3_ave**2
		reward += b*self.accel**2

		# penalize getting close to a rail
		"""
		if self.observedPosition1 > 480:
			reward += RAIL_PENALTY
		if self.observedPosition2 > 480:
			reward += RAIL_PENALTY
		if self.observedPosition3 > 480:
			reward += RAIL_PENALTY
		if self.observedPosition1 < 20:
			reward += RAIL_PENALTY
		if self.observedPosition2 > 20:
			reward += RAIL_PENALTY
		if self.observedPosition3 > 20:
			reward += RAIL_PENALTY
		"""	
		if self.upperRail + .1*self.dynamicRange >self.position:
			reward += RAIL_PENALTY
		if self.lowerRail - .1*self.dynamicRange < self.position:
			reward += RAIL_PENALTY
		
		# lets try something really simple
		reward = abs(self.position-self.targetState)
		#reward = reward/self.dynamicRange
		reward = -reward
		"""
		# reward for moving toward target, capped at some speed?
		if self.position > self.targetState: # we are below the target, want negative velocity (heading toward target)
			if self.velocity < 0:
				reward += min(abs(self.velocity),3) # negative velocity makes reward larger
			else:
				reward -= min(abs(self.velocity),3)
		else: # we are above the target, want positive velocity 
			if self.velocity > 0:
				reward += min(abs(self.velocity),3) # positive velocity makes reward larger
			else:
				reward -= min(abs(self.velocity),3)
		"""
		#self.velocity
		# make reward a penalty
		#return(-reward)
		return(reward)

	def pickNewTarget(self,target=None):
		if target == None:
			# use a random target, somewhere in the middle 60 percent
			self.targetState = self.upperRail + .2*self.dynamicRange + .6*self.dynamicRange*random.random()
		else:
			self.targetState = target

	def plotStates(self):
		positions = []
		velocitys = []
		accelerations = []
		errors = []
		currents = []
		costs = []
		for observation in self.states:
			positions.append(observation[0])
			velocitys.append(observation[1])
			accelerations.append(observation[2])
			errors.append(observation[3])
			currents.append(observation[4])
			costs.append(observation[5])
		plt.plot(positions)
		plt.plot(velocitys)
		plt.plot(accelerations)
		plt.plot(errors)
		plt.plot(currents)
		plt.plot(costs)
		plt.legend(['position (thou airgap)','velocity (thou/ms)','acceleration (thou/ms^2)','error', 'current(A) applied to \n get to current state \n (off by 1)',
					'cost/reward for getting \n to the current state \n (reward given for action=current)'])
		plt.show()


if __name__ == '__main__':
	env = MagLevEnvironment()
	
	env.reset()
	testCurrent = 1.5
	stepCurrent = -.05

	for counter in range(200):
		#obs,r,done,info = env.step((random.random()*3)-1.5)
		obs,r,done,info = env.step(testCurrent)
		testCurrent += stepCurrent
		if testCurrent < -1.5:
			testCurrent = -1.5
		if testCurrent > 1.5:
			testCurrent = 1.5
		if counter > 100:
			stepCurrent = .05
		print("observation: " + str(obs) + ", reward: " + str(r))

	env.plotStates()
	
	#print(env.getAccel(current=-1.5, airgap=120, mass=25))
	#env.getAccel(.6, 115, 25)



	# 1.55N on a 25gram mass, is 1.255N after gravity (roughly), which is f=ma 1.55=.025X = 62m/s^2, - gravity = 52.
	# convert 52m/s^2 to thou/ms^2: 
	# 2047 in/s^2
	# 2 thou per millisecond squared

	# 1 thou/ms^2 = 25.4 m/s^2
	# also checks out