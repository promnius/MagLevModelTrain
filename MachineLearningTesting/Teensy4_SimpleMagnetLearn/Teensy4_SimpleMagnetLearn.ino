
// TODO: make mode variable persistent (and make calibration routine preserve it)
// move all or most of math to fixed point (long) math, and/or make look up table faster
// general speed boosts; reduction of unneeded math, timing stuff, etc.

#include "GlobalVariables.h"
#include "CCDRead.h"
#include "Util.h"
#include "Math.h"
#include "Outputs.h"
#include "MagnetDrive.h"
#include "MachineLearning.h"
#include <Entropy.h>


void setup() {
  //randomSeed(analogRead())
  initializePinStates();
  initializeVariables();
  loadCalibrationData();
  initializeML();
  Serial.begin(1000000);
  delay(1000);
  lngLoopTimer = micros();
  lngSystemOffTimer = millis();

  Entropy.Initialize(); // entropy is good but slow, so we just use it to seed random
  randomSeed(Entropy.random());

  resetSimulation();
  delay(100);
  ml_model.predict(MLInput,y_pred);
  ml_model.predict(MLInput,y_pred);
  ml_model.predict(MLInput,y_pred);
  delay(8000); // for testing, give us time to get things into place, and to start the other script since we don't want to miss any serial
  checkForModelUpdate(); //this would ensure we always have the most up to date model, but needs to be 
  // synced with the sending script
  
}

void loop() {
  // WAIT FOR THE START OF A NEW FRAME, SO TIMING IS ALWAYS SYNCED.
  //Serial.println("Hello5");
  waitForTimingTrigger();

  lngFunctionTimerStart = micros();
  // SAMPLE THE SENSOR AND DO ALL RELEVANT MATH TO GET THE NUMBERS WE WANT
  scanCCDs(); // time expensive, no way around this.
  scanCCDs(); // VERY crude way of flushing buffer . . . should really add in the shutter function to standardize timings
  computeCentroid(); // Fast, but uses the hardware background threshold.
  computeCentroidMath(); // computationally expensive. Maybe not a problem on the teensy
  computeDistance(); // CURRENTLY USES CENTROID MATH. Currently very slow, due to bad implementation.
  computeDerivatives();
  logHistoricalValues(); // CURRENTLY USES CENTROID MATH
  // ALL OF THE ABOVE COMPUTATIONS: currently 91uS, INCLUDING the second scan
  lngFunctionTimerEnd = micros();

  // DO MATH BASED ON OUR SENSOR RESULTS AND UPDATE THE ELECTROMAGNETS
  //computeMagnetPower();
  //Serial.println("We made it here");
  //Serial.println("Hello3");
  //delay(25);
  computeMagnetPowerMLMethod();
  //Serial.println("Hello4");
  //intMagnetPower = -2000;
  // with our current winding and magnet orientation, positive intMagnetPower means negative force, ie, repel the target
  // FOR DEBUGGING: force maximum or minimum power
  //intMagnetPower = -2250; // 2250 is max based on our current algo
  setMagnetPower();
  //delay(5000);
  
  // FOR DEBUGGING ONLY, MUST DISABLE OTHER SERIAL FUNCTIONS!
  //if (lngLoopCounter%PLOTFREQUENCYDIVIDER == 0){plotRollingDistance();}

  // FOR DEBUGGING ONLY, MUST DISABLE OTHER SERIAL FUNCTIONS!
  //Serial.print("function time: ");Serial.print(lngFunctionTimerEnd-lngFunctionTimerStart);Serial.print("uS");Serial.println();

  // HANDLE SERIAL REQUESTS TO CHANGE MODE OR INITIATE CALIBRATION ROUTINE
  //if (Serial.available()>0){processSerial();} // NOTE since any serial is used for transfering ML stuff,
    // we can't use this code to calibrate. load the baseline rangefinder code first and get a calibration
    // saved into eeprom.

  handleTerminalConditions();

  // UPDATE HEARTBEAT
  if (lngLoopCounter%10000 == 0){
    if (intHeartbeatState == 0){
      intHeartbeatState = 1;
      digitalWriteFast(pinHEARTBEAT,LOW);
    }else{
      intHeartbeatState = 0;
      digitalWriteFast(pinHEARTBEAT,HIGH);
    }
  }
}
