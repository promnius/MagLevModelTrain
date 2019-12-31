
#ifndef PowerAndControls_h
#define PowerAndControls_h

#include "Arduino.h"
#include "GlobalVariables.h"
#include "LEDManagement.h"

void configureMagnetDrives(){ // set up pinmodes and default states
  pinMode(pinMAGNETENABLE, OUTPUT);
  digitalWrite(pinMAGNETENABLE, LOW);
  
  analogWriteResolution(10);
  for (int i = 0; i < 8; i ++){
    pinMode(MAGNETPWMPINS[i], OUTPUT);
    pinMode(MAGNETDIRECTIONPINS[i], OUTPUT);
    digitalWrite(MAGNETPWMPINS[i], HIGH);
    digitalWrite(MAGNETDIRECTIONPINS[i], LOW);
    analogWriteFrequency(MAGNETPWMPINS[i], 46875); // some of these are redundant, since multiple pins are on the same timer
    // magic number based on keeping frequency above 20khz for audible reasons and the filter of the inductor is around 20khz,
    // plus that is the number that maps perfectly to 10 bit resolution at 96Mhz processor speed
    magnetActive[i] = false;

    digitalWrite(pinMAGNETENABLE, HIGH);
  }
}

void setupControlLoops(){
  // not much to do anymore :)
}

long targetCurrent_uA_atHeight0 = 0;
long targetCurrent_uA_atHeight1 = 0;
long savej0 = 0; long savej1 = 1;

void computeTargetCurrent(){
  // *****************************************************************************************************************************************************
  // 2 dimensional lookup table to get from position and target force to target current
  //intMaxHeight_UI
  //targetForce_uN
  //intMaxHeight_uI = 225000; // for testing control loop math.
  //targetForce_uN = 500000;
  //targetCurrent_uA
  //Serial.println("NewSample");
  long y0,y1,x0,x1;
  for (int i = 0; i < CTHeightForceCurrentLengthDimHeight-1; i ++){
    // the long casts may be not useful?
    if (intMaxHeight_uI >= (long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][0][0]) && intMaxHeight_uI < (long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][0][0])){
      for (int j = 0; j < CTHeightForceCurrentLengthDimForce-1; j ++){
        if (targetForce_uN >= (long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][j][1]) && targetForce_uN < (long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][j+1][1])){
          y0 = (long) pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][j][2]);  //lower bound
          y1 = (long) pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][j+1][2]); //upper bound
          x0 = (long) pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][j][1]);
          x1 = (long) pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][j+1][1]);
          //Serial.print("DEBUG: ");Serial.print("x0 (mN): "); Serial.print(x0/1000);Serial.print(", x1 (mN): "); Serial.print(x1/1000);
          //Serial.print(", y0 (mA): "); Serial.print(y0/1000);Serial.print(", y1 (mA): "); Serial.print(y1/1000);
          //Serial.println();
          targetCurrent_uA_atHeight0 = y0 + ((((y1 - y0)/1000) * ((targetForce_uN - x0))) / (x1 - x0))*1000; // counting on current y1 and y0 being round numbers that
          // can be decimated to prevent overflow. max input approx. 2N. may want to find a better way to do this in fixed point.
          //Serial.print("DEBUG: ((((y1 - y0)/1000) * ((targetForce_uN - x0))) / (x1 - x0))*1000: "); Serial.print(((((y1 - y0)/1000) * ((targetForce_uN - x0))) / (x1 - x0))*1000);Serial.println(); 
          //Serial.print("DEBUG: OUTPUT (mA): "); Serial.print(targetCurrent_uA_atHeight0/1000);Serial.println(); 
          savej0 = j;
        }
      }
      for (int j = 0; j < CTHeightForceCurrentLengthDimForce-1; j ++){
        if (targetForce_uN >= (long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][j][1]) && targetForce_uN < (long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][j+1][1])){
          y0 = (long) pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][j][2]);  //lower bound
          y1 = (long) pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][j+1][2]); //upper bound
          x0 = (long) pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][j][1]);
          x1 = (long) pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][j+1][1]);
          targetCurrent_uA_atHeight1 = y0 + ((((y1 - y0)/1000) * ((targetForce_uN - x0))) / (x1 - x0))*1000;
          savej1 = j;
        }
      }      

      y0 = (long) targetCurrent_uA_atHeight0;  //lower bound
      y1 = (long) targetCurrent_uA_atHeight1; //upper bound
      x0 = (long) pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][0][0]);
      x1 = (long) pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][0][0]);
      targetCurrent_uA = y0 + ((((y1 - y0)/1000) * ((intMaxHeight_uI - x0))) / (x1 - x0))*1000;

      /*
      Serial.print("DEBUG: ");Serial.print("x0 (mil): "); Serial.print(x0/1000);Serial.print(", x1 (mil): "); Serial.print(x1/1000);
      Serial.print(", y0 (mA): "); Serial.print(y0/1000);Serial.print(", y1 (mA): "); Serial.print(y1/1000);
      Serial.println();
          
      Serial.print("Height (mil): "); Serial.print(intMaxHeight_uI/1000);Serial.print(", Requested Force (mN): "); Serial.print(targetForce_uN/1000);Serial.println();
      Serial.print("i(H,mil): "); Serial.print((long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][0][0])/1000);Serial.print(" at i: ");Serial.print(i);Serial.println();
      Serial.print("i+1(H,mil): "); Serial.print((long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][0][0])/1000); Serial.println();
      Serial.print("j for i(F,mN): "); Serial.print((long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][savej0][1])/1000);Serial.print(" at j: ");Serial.print(savej0);
      Serial.print(", Current needed at this force (mA): "); Serial.print((long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][savej0][2])/1000);Serial.println();
      Serial.print("j+1 for i(F,mN): "); Serial.print((long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][savej0+1][1])/1000);
      Serial.print(", Current needed at this force (mA): "); Serial.print((long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i][savej0+1][2])/1000);Serial.println();
      Serial.print("j for i+1(F,mN): "); Serial.print((long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][savej1][1])/1000);Serial.print(" at j: ");Serial.print(savej1);
      Serial.print(", Current needed at this force (mA): "); Serial.print((long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][savej1][2])/1000);Serial.println();
      Serial.print("j+1 for i+1(F,mN): "); Serial.print((long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][savej1+1][1])/1000);
      Serial.print(", Current needed at this force (mA): "); Serial.print((long)pgm_read_dword(&CTHeightForceCurrent_uIuNuA[i+1][savej1+1][2])/1000);Serial.println();
      Serial.print("Current interpolation at Height 0 (mA): "); Serial.print(targetCurrent_uA_atHeight0/1000);Serial.println();
      Serial.print("Force interpolation at Height 1 (mA): "); Serial.print(targetCurrent_uA_atHeight1/1000);Serial.println();
      Serial.print("Final Current interpolation (mA): "); Serial.print(targetCurrent_uA/1000);Serial.println();
      Serial.println();
      */
    }
  }
}


long inputVoltage_uV = 12000000;
long coilResistance_uOhms = 3700000;
void adjustMagnetPowerLevels(){
  // *****************************************************************************************************************************************************
  // somewhere we need to assess magnetactive as well
    // *****************************************************************************************************************************************************
  for (int i = 0; i < 8; i ++){
    if (((i*10)+5 > maxSensorValueLocation-18)&&((i*10)+5 < maxSensorValueLocation+18)){
      magnetActive[i]=true;
      //Serial.println(i);
    } else{
      magnetActive[i]=false;
    }
  }
  //maxResolution = 1024;
  for (int i = 0; i < 8; i ++){
    //if (false){ // use this to disable actual magnet output while leaving the rest of the code untouched.
    if (magnetActive[i] == true){ // magnet is on, because it is close to the train position
      digitalWrite(MAGNETDIRECTIONPINS[i], targetPolarity); // does true false turn into high low correctly? Is this backwards?
      analogWrite(MAGNETPWMPINS[i], maxResolution - targetPWM); // is this max resolution - targetpwm, or does the math that evaluates targetpwm sort that out?
    }
    else{ // magnet is off because it is not close to the train position
      digitalWrite(MAGNETPWMPINS[i], HIGH); 
    }
  }
}
long targetOpenLoopVoltage = 0;
void updateControlLoops(){
  error_uI = setpoint_uI - intMaxHeight_uI;
  dterm = intMaxHeight_uI - lastMaxHeight;
  ddterm = dterm - lastDterm;
  lastDterm = dterm; // these happen whether the loop was on or off so there isn't a harsh jump when the loop transitions
  lastMaxHeight = intMaxHeight_uI; // from on to off.
  //Serial.println(intMaxHeight_uI/1000);
  if (intMaxHeight_uI > 50000 && intMaxHeight_uI < 300000) { // max usable range, will force control loop off when magnet is not present
  //if(true){ // for testing control loop math
    targetForce_uN = kk - (kpT * error_uI)/kpB + (kdT*dterm)/kdB + (kddT*ddterm)/kddB; // here's where the magic happens!
    if (targetForce_uN > maxForce_uN){targetForce_uN = maxForce_uN;}
    if (targetForce_uN < minForce_uN){targetForce_uN = minForce_uN;}
    computeTargetCurrent();
  } else{ // control loop is off, set all current to 0.
    targetCurrent_uA = 0;
    //Serial.println('x');
  }

  targetOpenLoopVoltage = ((abs(targetCurrent_uA))/1000)*(coilResistance_uOhms/1000)/1000;
  //Serial.println(intermediateMath);
  targetPWM = (targetOpenLoopVoltage*maxResolution)/(inputVoltage_uV/1000); // careful of overflow!!
  //Serial.println(targetPWM);
  if (targetCurrent_uA > 0) {targetPolarity = false;} // attraction
  else {targetPolarity = true;} // repulsion
  if (targetPWM > maxPower){targetPWM = maxPower;}
  
  //adjustMagnetPowerLevels(); // not sure this belongs here.
  intBarGraphCurrents[loopCounter] = targetCurrent_uA; // data logging
}



#endif
