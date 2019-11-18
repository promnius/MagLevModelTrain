
#ifndef PowerAndControls_h
#define PowerAndControls_h

#include "Arduino.h"
#include "GlobalVariables.h"
#include "LEDManagement.h"
#include <PID_v1.h>

void configureMagnetDrives(){ // set up pinmodes and default states
	pinMode(pinMAGNETPWM0, OUTPUT);
  pinMode(pinMAGNETPWM1, OUTPUT);
  pinMode(pinMAGNETPWM2, OUTPUT);
  pinMode(pinMAGNETPWM3, OUTPUT);
  pinMode(pinMAGNETPWM4, OUTPUT);
  pinMode(pinMAGNETPWM5, OUTPUT);
  pinMode(pinMAGNETPWM6, OUTPUT);
  pinMode(pinMAGNETPWM7, OUTPUT);
  
  pinMode(pinMAGNETDIRECTION0, OUTPUT);
  pinMode(pinMAGNETDIRECTION1, OUTPUT);
  pinMode(pinMAGNETDIRECTION2, OUTPUT);
  pinMode(pinMAGNETDIRECTION3, OUTPUT);
  pinMode(pinMAGNETDIRECTION4, OUTPUT);
  pinMode(pinMAGNETDIRECTION5, OUTPUT);
  pinMode(pinMAGNETDIRECTION6, OUTPUT);
  pinMode(pinMAGNETDIRECTION7, OUTPUT);
  
  pinMode(pinMAGNETENABLE, OUTPUT);
  
  digitalWrite(pinMAGNETPWM0, LOW);
  digitalWrite(pinMAGNETPWM1, LOW);
  digitalWrite(pinMAGNETPWM2, LOW);
  digitalWrite(pinMAGNETPWM3, LOW);
  digitalWrite(pinMAGNETPWM4, LOW);
  digitalWrite(pinMAGNETPWM5, LOW);
  digitalWrite(pinMAGNETPWM6, LOW);
  digitalWrite(pinMAGNETPWM7, LOW);
  
  digitalWrite(pinMAGNETDIRECTION0, LOW);
  digitalWrite(pinMAGNETDIRECTION1, LOW);
  digitalWrite(pinMAGNETDIRECTION2, LOW);
  digitalWrite(pinMAGNETDIRECTION3, LOW);
  digitalWrite(pinMAGNETDIRECTION4, LOW);
  digitalWrite(pinMAGNETDIRECTION5, LOW);
  digitalWrite(pinMAGNETDIRECTION6, LOW);
  digitalWrite(pinMAGNETDIRECTION7, LOW);

  digitalWrite(pinMAGNETENABLE, HIGH);

  analogWriteResolution(10);
  analogWriteFrequency(pinMAGNETPWM0, 46875); // some of these are redundant, since multiple pins are on the same timer
  // magic number based on keeping frequency above 20khz for audible reasons and the filter of the inductor is around 20khz,
  // plus that is the number that maps perfectly to 10 bit resolution at 96Mhz processor speed
  analogWriteFrequency(pinMAGNETPWM1, 46875);
  analogWriteFrequency(pinMAGNETPWM2, 46875);
  analogWriteFrequency(pinMAGNETPWM3, 46875);
  analogWriteFrequency(pinMAGNETPWM4, 46875);
  analogWriteFrequency(pinMAGNETPWM5, 46875);
  analogWriteFrequency(pinMAGNETPWM6, 46875);
  analogWriteFrequency(pinMAGNETPWM7, 46875);
}

void setupControlLoops(){
  myPID_Magnet0.SetSampleTime(sampleTime);
  myPID_Magnet1.SetSampleTime(sampleTime);
  myPID_Magnet2.SetSampleTime(sampleTime);
  myPID_Magnet3.SetSampleTime(sampleTime);
  myPID_Magnet4.SetSampleTime(sampleTime);
  myPID_Magnet5.SetSampleTime(sampleTime);
  myPID_Magnet6.SetSampleTime(sampleTime); 
  myPID_Magnet7.SetSampleTime(sampleTime);

  myPID_Magnet0.SetOutputLimits(-maxPower, maxPower); 
  myPID_Magnet1.SetOutputLimits(-maxPower, maxPower); 
  myPID_Magnet2.SetOutputLimits(-maxPower, maxPower); 
  myPID_Magnet3.SetOutputLimits(-maxPower, maxPower); 
  myPID_Magnet4.SetOutputLimits(-maxPower, maxPower); 
  myPID_Magnet5.SetOutputLimits(-maxPower, maxPower); 
  myPID_Magnet6.SetOutputLimits(-maxPower, maxPower); 
  myPID_Magnet7.SetOutputLimits(-maxPower, maxPower); 
  
  myPID_Magnet0.SetMode(AUTOMATIC); // for now we want to play with a single magnet
  //myPID_Magnet1.SetMode(AUTOMATIC);
  //myPID_Magnet2.SetMode(AUTOMATIC);
  //myPID_Magnet3.SetMode(AUTOMATIC);
  //myPID_Magnet4.SetMode(AUTOMATIC);
  //myPID_Magnet5.SetMode(AUTOMATIC);
  //myPID_Magnet6.SetMode(AUTOMATIC);
  //myPID_Magnet7.SetMode(AUTOMATIC);
}

void updateControlLoops(){

  // check if the control loops should be on or off!! ----------------------------------------------------------------------------------------------------------
  
  myPID_Magnet0.Compute(); // this may not update every time through as the sensors are scanned faster than
  // the control loops update, and depending on delays, etc. they may not all update in the same
  // callback, causeing asynchronous updates. This shouldn't hurt anything though. The largest downside
  // is they are all assuming a fixed interval since their last call (of 3ms), when in reality it is within
  // the nearest time of the sample speed (so +- 700us)
  myPID_Magnet1.Compute();
  myPID_Magnet2.Compute();
  myPID_Magnet3.Compute();
  myPID_Magnet4.Compute();
  myPID_Magnet5.Compute();
  myPID_Magnet6.Compute();
  myPID_Magnet7.Compute();

  // handling case where some magnets are off
  if (myPID_Magnet0.GetMode() == MANUAL){PID_OutputMagnetCommand[0] = 0;}
  if (myPID_Magnet1.GetMode() == MANUAL){PID_OutputMagnetCommand[1] = 0;}
  if (myPID_Magnet2.GetMode() == MANUAL){PID_OutputMagnetCommand[2] = 0;}
  if (myPID_Magnet3.GetMode() == MANUAL){PID_OutputMagnetCommand[3] = 0;}
  if (myPID_Magnet4.GetMode() == MANUAL){PID_OutputMagnetCommand[4] = 0;}
  if (myPID_Magnet5.GetMode() == MANUAL){PID_OutputMagnetCommand[5] = 0;}
  if (myPID_Magnet6.GetMode() == MANUAL){PID_OutputMagnetCommand[6] = 0;}
  if (myPID_Magnet7.GetMode() == MANUAL){PID_OutputMagnetCommand[7] = 0;}
  
  // update visual elements
  // weak averaging function
  for (int i = 0; i < 8; i ++){
    intPowerNow[i] = (int)PID_OutputMagnetCommand[i];
    //intPowerAvg[i] = ((intPowerAvg[i]*3)/4)+ (intPowerNow[i]/4);
    intPowerAvg[i] = intPowerNow[i]; // no averaging
  }
}

void adjustMagnetPowerLevels(){
  if (intPowerNow[0]<0){digitalWrite(pinMAGNETDIRECTION0, MAGNET0NORTH);}else{digitalWrite(pinMAGNETDIRECTION0, !MAGNET0NORTH);}
  analogWrite(pinMAGNETPWM0, maxResolution - abs(intPowerNow[0]));
  if (intPowerNow[1]<0){digitalWrite(pinMAGNETDIRECTION1, MAGNET1NORTH);}else{digitalWrite(pinMAGNETDIRECTION1, !MAGNET1NORTH);}
  analogWrite(pinMAGNETPWM1, maxResolution - abs(intPowerNow[1]));
  if (intPowerNow[2]<0){digitalWrite(pinMAGNETDIRECTION2, MAGNET2NORTH);}else{digitalWrite(pinMAGNETDIRECTION2, !MAGNET2NORTH);}
  analogWrite(pinMAGNETPWM2, maxResolution - abs(intPowerNow[2]));
  if (intPowerNow[3]<0){digitalWrite(pinMAGNETDIRECTION3, MAGNET3NORTH);}else{digitalWrite(pinMAGNETDIRECTION3, !MAGNET3NORTH);}
  analogWrite(pinMAGNETPWM3, maxResolution - abs(intPowerNow[3]));
  if (intPowerNow[4]<0){digitalWrite(pinMAGNETDIRECTION4, MAGNET4NORTH);}else{digitalWrite(pinMAGNETDIRECTION4, !MAGNET4NORTH);}
  analogWrite(pinMAGNETPWM4, maxResolution - abs(intPowerNow[4]));
  if (intPowerNow[5]<0){digitalWrite(pinMAGNETDIRECTION5, MAGNET5NORTH);}else{digitalWrite(pinMAGNETDIRECTION5, !MAGNET5NORTH);}
  analogWrite(pinMAGNETPWM5, maxResolution - abs(intPowerNow[5]));
  if (intPowerNow[6]<0){digitalWrite(pinMAGNETDIRECTION6, MAGNET6NORTH);}else{digitalWrite(pinMAGNETDIRECTION6, !MAGNET6NORTH);}
  analogWrite(pinMAGNETPWM6, maxResolution - abs(intPowerNow[6]));
  if (intPowerNow[7]<0){digitalWrite(pinMAGNETDIRECTION7, MAGNET7NORTH);}else{digitalWrite(pinMAGNETDIRECTION7, !MAGNET7NORTH);}
  analogWrite(pinMAGNETPWM7, maxResolution - abs(intPowerNow[7]));
  /*analogWrite(pinMAGNETPWM0, intPowerNow[0]);
  analogWrite(pinMAGNETPWM1, intPowerNow[1]);
  analogWrite(pinMAGNETPWM2, intPowerNow[2]);
  analogWrite(pinMAGNETPWM3, intPowerNow[3]);
  analogWrite(pinMAGNETPWM4, intPowerNow[4]);
  analogWrite(pinMAGNETPWM5, intPowerNow[5]);
  analogWrite(pinMAGNETPWM6, intPowerNow[6]);
  analogWrite(pinMAGNETPWM7, intPowerNow[7]);*/
}

#endif
