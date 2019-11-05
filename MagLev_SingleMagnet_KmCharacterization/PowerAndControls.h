
#ifndef PowerAndControls_h
#define PowerAndControls_h

#include "Arduino.h"
#include "GlobalVariables.h"
#include "LEDManagement.h"


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


void adjustMagnetPowerLevels(){
  analogWrite(pinMAGNETPWM0, powerLevel);
  //analogWrite(pinMAGNETPWM1, powerLevel);
  //analogWrite(pinMAGNETPWM2, powerLevel);
  //analogWrite(pinMAGNETPWM3, powerLevel);
  //analogWrite(pinMAGNETPWM4, powerLevel);
  //analogWrite(pinMAGNETPWM5, powerLevel);
  //analogWrite(pinMAGNETPWM6, powerLevel);
  //analogWrite(pinMAGNETPWM7, powerLevel);
}

#endif
