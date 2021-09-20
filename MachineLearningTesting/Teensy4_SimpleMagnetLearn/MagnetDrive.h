
#ifndef MagnetDrive_h
#define MagnetDrive_h

#include "Arduino.h"
#include "GlobalVariables.h"

// BASED ON THE MODE, this function will compute the desired magnet power for the next 'frame'.
// it could be calculated using a traditional control loop, a neural network (tf-lite), a
// random value (seeding a neural network training set), or received over serial (external control).
// for now, it is a placeholder.
int powerCounter = 0;
float fltKp = 100;
float fltKd = 0;
float fltKdd = 0;
unsigned long lngSystemOffTimer = 0;
void computeMagnetPower(){
  // set power to a fixed value for testing holding strength, magnet, etc.
  //intMagnetPower = -50;

  // set power to a random value. optionally, don't update value every loop
  //powerCounter ++;
  //if (powerCounter == 50){
  //  intMagnetPower = random(-3000, 3000);
  //  powerCounter = 0;
  //}

  // bang bang
  //if (fltAveDistance > fltSetpoint){intMagnetPower = 1000;}
  //else{intMagnetPower = -1000;}

  // PID
  intMagnetPower = (long)(fltKp*(fltAveDistance-fltSetpoint) + fltKd*fltVelocity + fltKdd*fltAccel);
  
  // protect magnet by turning it off with no target. Also update state variables so we don't get rough transitions
  for (int i = 0; i < numCCDS; i ++){
    if (fltDistance[i] == -1){
      intMagnetPower = 0;
      fltTilt = 0;
      fltVelocity = 0;
      fltLastDistance = 50;
      fltAccel = 0;
      fltLastVelocity = 0;  
      intLastStateValid = 0;
    }
  }

  // protect magnet by limiting power
  if(intMagnetPower > 3000){intMagnetPower = 3000;}
  if (intMagnetPower < -3000){intMagnetPower = -3000;}

  // shutdown under extreme tilt
  if (fltTilt > 500.0){
    intMagnetPower = 0;
    lngSystemOffTimer = millis();
  }

  // stay off for reset time
  if (millis() - lngSystemOffTimer < 3000){
    intMagnetPower = 0;
  }
  
  while (micros() - lngLoopTimer < 100){} // sync math time, whether we did hard math or not
}

// placeholder
void setMagnetPower(){
  if (intMagnetPower > 0){
    //digitalWriteFast(pinCOILDRIVEA, HIGH);
    analogWrite(pinCOILDRIVEA, 4096);
    analogWrite(pinCOILDRIVEB,4096 - intMagnetPower);
  }
  else{
    //digitalWriteFast(pinCOILDRIVEB, HIGH); // not sure why this doesn't work when interleaved with analog writes, but it doesn't.
    analogWrite(pinCOILDRIVEB, 4096);
    analogWrite(pinCOILDRIVEA,4096 + intMagnetPower);
  }
}

#endif
