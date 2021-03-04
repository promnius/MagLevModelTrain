
// ToDo:
// --remove if true statements/ artificial height samples
// evaluate magnetActive[]
// --finish 2D LUT.
// --verify 2D LUT is working.
// --test timing
// double check magnet power polarity
// tune kd and kdd
// test levitation
// evaluate if the requested force is available or not, and update the final bar graph showing requested torque vs. real torque.
// --why leds.show() is blocking
// --can sensor math go faster?



#include <ADC.h>
#include <WS2812Serial.h>
#include "GlobalVariables.h"
#include "LookupTables.h"
#include "HelperFunctions.h"
#include "LEDManagement.h"
#include "AnalogSensing.h"
#include "PowerAndControls.h"


IntervalTimer myTimer;

void controlLoop(){
  SampleTheSensors(); // this is currently a blocking function!! takes about 250uS
  ExtractUsefulData(); // looking for vertical and horizontal positions, in real units. Takes about 50uS, with averaging disabled.
  updateControlLoops(); // calculates new current values and applies them. takes about 10uS
  adjustMagnetPowerLevels();
  loopCounter +=1;
  if (loopCounter > 99){loopCounter = 0; averagesValid = true;}
}

// ***************************************************************************************************************************************************

void setup() {
  Serial.begin(1000000);
  configureADCs(); // prep adcs for max speed
  configureMagnetDrives(); // set up pinmodes and default states
  // also set default magnet drive directions.
  beginAnalogSensors(); // set up ADCs
  calibrateSensors(); // get basic sensor offsets
  setupControlLoops(); // initialize control loops
  leds.begin();
  lngLEDUpdateTime = millis();
  /*
  Serial.println(CTHeightForceCurrent_uIuNuA[0][0][1]);
  Serial.println(CTHeightForceCurrent_uIuNuA[0][1][1]);
  Serial.println(CTHeightForceCurrent_uIuNuA[8][0][1]);
  Serial.println(CTHeightForceCurrent_uIuNuA[8][1][1]);
  Serial.println(CTHeightForceCurrent_uIuNuA[0][5][1]);
  Serial.println(CTHeightForceCurrent_uIuNuA[0][6][1]);
  Serial.println(CTHeightForceCurrent_uIuNuA[8][5][1]);
  Serial.println(CTHeightForceCurrent_uIuNuA[8][6][1]);
  Serial.println();
  delay(4000);
  */
  /*
  targetPWM = 500;
  magnetActive[0] = true;
  magnetActive[1] = true;
  delay(2000);
  analogWrite(MAGNETPWMPINS[0], maxResolution - targetPWM);
  analogWrite(MAGNETPWMPINS[1], maxResolution - targetPWM);
  */
  myTimer.priority(200); // lower than most timers so micros and millis still work.
  myTimer.begin(controlLoop, 400); // 2.5khz
}

// ***************************************************************************************************************************************************

void printDataSensors(){
  Serial.print("Maximums: ");
  for (int i = 0; i < 40; i ++){
    //Serial.print(intSensorData_uT_Max[i]);Serial.print(",");
    Serial.print(intSensorData_uT_Max_Avg[i]);Serial.print(",");
  } Serial.println(); Serial.print("Minimums: ");
  for (int i = 0; i < 40; i ++){
    //Serial.print(intSensorData_uT_Min[i]);Serial.print(",");
    Serial.print(intSensorData_uT_Min_Avg[i]);Serial.print(",");
  } Serial.println(); Serial.print("Averages: ");    
  for (int i = 0; i < 40; i ++){
    Serial.print(getAverageOfArray(intSensorData_uT_Avg[i],100));Serial.print(",");
  }Serial.println(); Serial.print("True Location/ Height: ");
  Serial.print(maxSensorValueLocation);Serial.print(",");Serial.println(intMaxHeight_uI/1000);
  //Serial.print(455-(maxSensorValueLocation/3)-(intMaxHeight_uI/1000));
  Serial.println(); 
}

void printDataTiming(){
  Serial.print("Analog Samples (uS): "); Serial.println(lngScanEndTime-lngScanStartTime);
  Serial.print("Sensor Math (uS): "); Serial.println(lngConversionMathEndTime-lngScanEndTime);
  Serial.print("Control Loop Math (uS): "); Serial.println(lngControlLoopMathEndTime-lngConversionMathEndTime);
  Serial.print("Update LEDs Part 1 (uS): "); Serial.println(lngLEDUpdatesEndTime0-lngControlLoopMathEndTime);
  Serial.print("Update LEDs Part 2 (uS): "); Serial.println(lngLEDUpdatesEndTime1-lngLEDUpdatesEndTime0);
  Serial.print("Update LEDs Part 3 (uS): "); Serial.println(lngLEDUpdatesEndTime2-lngLEDUpdatesEndTime1);
  Serial.println();
}

void printDataControlLoop(){
  // not using averaged variables because there would be a lot of extra stuff to track, and we are more interested in whether or not the math is 
  // accurate than what the real state of the system is.
  Serial.print("kK: ");Serial.print(kk);Serial.print(", kP: ");Serial.print((float)kpT/(float)kpB);Serial.print(", kD: ");
  Serial.print((float)kdT/(float)kdB);Serial.print(", kDD: ");Serial.print((float)kddT/(float)kddB);Serial.println();
  Serial.print("Setpoint: ");Serial.print(setpoint_uI);Serial.print(", Position: ");Serial.print(intMaxHeight_uI);Serial.print(", Error: ");Serial.print(error_uI);Serial.print(", Dterm: ");
  Serial.print(dterm);Serial.print(", ddterm: ");Serial.print(ddterm); Serial.println();
  Serial.print("Target Force (mN): ");Serial.print(targetForce_uN/1000);Serial.print(", Target Current (mA): ");Serial.print(targetCurrent_uA/1000); Serial.println();
  Serial.print(", TargetPWM (x/1024): ");Serial.print(targetPWM);Serial.print(", Polarity: ");Serial.print(targetPolarity); Serial.println();
  Serial.println();  
}

// ***************************************************************************************************************************************************




void loop() {
  // sample all analog inputs
  /*
  lngScanStartTime = micros();
  SampleTheSensors(); // this is currently a blocking function!! takes about 750us??
  lngScanEndTime = micros();
  ExtractUsefulData(); // looking for vertical and horizontal positions, in real units
  lngConversionMathEndTime = micros();

  updateControlLoops();
  lngControlLoopMathEndTime = micros();
  ///////adjustMagnetPowerLevels(); // currently update control loops calls this as well . . . not sure if i want this broken out or not.

  loopCounter +=1;
  if (loopCounter > 99){loopCounter = 0; averagesValid = true;}
  */
  
  if (millis() - lngLEDUpdateTime > 50){
    //updateControlLoops(); // only in here so we can control run speed
    //printDataSensors();
    printDataControlLoop();
    plotAllGraphs();
    lngLEDUpdatesEndTime0 = micros();
    correctForGamma();
    lngLEDUpdatesEndTime1 = micros();
    leds.show();
    lngLEDUpdatesEndTime2 = micros();
    //printDataTiming();
    lngLEDUpdateTime = millis(); // the two different LED timers are confusing, please rename!
  }
  //delay(1000);
}

 
