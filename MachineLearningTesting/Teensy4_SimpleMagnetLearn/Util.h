
#ifndef Util_h
#define Util_h

#include "Arduino.h"
#include "GlobalVariables.h"
#include <EEPROM.h>

// disabling all interrupts means micro and millis are broken, so instead we lock the frame rate
// by waiting for an externally generated timing signal.
void waitForTimingTrigger(){
  while (micros() - lngLoopTimer < 1000){} // lock the refresh rate, but smoothly handle overloads too. Doesn't correct for lost time.
  lngLoopTimer = micros();
}

// placeholder, currently lasers are always on.
void setLaserPower(int laserPower){
  
}

void initializePinStates(){
  
  pinMode(pinHEARTBEAT,OUTPUT);
  pinMode(pinSHUTTER,OUTPUT);
  pinMode(pinSTARTREAD,OUTPUT);
  pinMode(pinREADCLOCK,OUTPUT);
  pinMode(pinADCCLOCK,OUTPUT);
  pinMode(pinFAKEDAC,OUTPUT);
  pinMode(pinLASERCONTROL,OUTPUT);
  pinMode(pinCOILDRIVEA,OUTPUT);
  pinMode(pinCOILDRIVEB,OUTPUT);

  pinMode(pinADC_A0,INPUT);
  pinMode(pinADC_A1,INPUT);
  pinMode(pinADC_A2,INPUT);
  pinMode(pinADC_A3,INPUT);
  pinMode(pinADC_A4,INPUT);
  pinMode(pinADC_A5,INPUT);
  pinMode(pinADC_A6,INPUT);
  pinMode(pinADC_A7,INPUT); 

  pinMode(pinADC_B0,INPUT);
  pinMode(pinADC_B1,INPUT);
  pinMode(pinADC_B2,INPUT);
  pinMode(pinADC_B3,INPUT);
  pinMode(pinADC_B4,INPUT);
  pinMode(pinADC_B5,INPUT);
  pinMode(pinADC_B6,INPUT);
  pinMode(pinADC_B7,INPUT); 

  pinMode(pinADC_C0,INPUT);
  pinMode(pinADC_C1,INPUT);
  pinMode(pinADC_C2,INPUT);
  pinMode(pinADC_C3,INPUT);
  pinMode(pinADC_C4,INPUT);
  pinMode(pinADC_C5,INPUT);
  pinMode(pinADC_C6,INPUT);
  pinMode(pinADC_C7,INPUT); 

  // timing seems to be ok without fast slew rates, as we have 10nS of delay for slew to happen, and not a lot of 
  // capacitance on the bus. The lower slew rate helps the analog signal a lot. The faster slew rate could go faster, in theory.
  //CORE_PIN16_CONFIG = CORE_PIN16_CONFIG | PORT_PCR_DSE; // high strength
  //CORE_PIN16_CONFIG = CORE_PIN16_CONFIG & 0xFFFFFFFB; // fast slew
  //CORE_PIN21_CONFIG = CORE_PIN21_CONFIG | PORT_PCR_DSE; // high strength
  //CORE_PIN21_CONFIG = CORE_PIN21_CONFIG & 0xFFFFFFFB; // fast slew

  analogWriteResolution(12);
  analogWriteFrequency(pinFAKEDAC,36621.09);
  analogWriteFrequency(pinCOILDRIVEA,36621.09);
  analogWriteFrequency(pinCOILDRIVEB,36621.09);

  digitalWriteFast(pinHEARTBEAT,LOW);
  digitalWriteFast(pinSHUTTER,LOW);
  digitalWriteFast(pinSTARTREAD,LOW);
  digitalWriteFast(pinREADCLOCK,LOW);
  digitalWriteFast(pinADCCLOCK,HIGH);
  digitalWriteFast(pinFAKEDAC,LOW);
  digitalWriteFast(pinLASERCONTROL,LOW);
  digitalWriteFast(pinCOILDRIVEA,HIGH);
  digitalWriteFast(pinCOILDRIVEB,HIGH);
}

void initializeVariables(){
  for (int i = 0; i < numCCDS; i ++){
    for (int j = 0; j < ccdBUFFERLENGTH; j ++){
      INTccdRaw[i][j] = 0;
    }
    for (int j = 0; j < intHistoryLength; j ++){
      FLTcentroids[i][j] = 0;
      FLTdistances[i][j] = 0;
    }
  }
}

// calibration is currently hardcoded to 100 points
void loadCalibrationData(){
  intEEAddress = 0;
  for (int i = 0; i < numCCDS; i ++){
    for (int j = 0; j < 50; j ++){
      EEPROM.get(intEEAddress, FLTcalibrationTable[i][j]);
      intEEAddress += sizeof(float);    
    }
  }
}

void saveCalibrationData(){
  intEEAddress = 0;
  for (int i = 0; i < numCCDS; i ++){
    for (int j = 0; j < 50; j ++){
      EEPROM.put(intEEAddress, FLTcalibrationTable[i][j]);
      intEEAddress += sizeof(float);    
    }
  }
}

// Sum the stored history of CCD centroid readings, for calculating an average. Too computationally
// expensive to do on the fly, but useful for extracting better calibration data.
void sumCentroids(){
  for (int i = 0; i < numCCDS; i ++){
    int intMissingData = 0;
    fltSumCentroid[i] = 0;
    fltMinCentroid[i] = 255;
    fltMaxCentroid[i] = 0;
    for (int j = 0; j < intHistoryLength; j++){
      fltSumCentroid[i] += FLTcentroids[i][j];
      if (FLTcentroids[i][j]<fltMinCentroid[i]){fltMinCentroid[i] = FLTcentroids[i][j];}
      if (FLTcentroids[i][j]>fltMaxCentroid[i]){fltMaxCentroid[i] = FLTcentroids[i][j];}
      if (FLTcentroids[i][j]==0){intMissingData = 1;}
    }
    if (intMissingData == 1){fltSumCentroid[i] = 0;}
  }  
}

void processSerial(){
  intSerialByte = Serial.read();
  if (intSerialByte >=128 && intSerialByte < 178){ // set calibration point
    sumCentroids();
    for (int i = 0; i < numCCDS; i ++){
      FLTcalibrationTable[i][intSerialByte-128] = fltSumCentroid[i]/(float)intHistoryLength;
      Serial.print("CCD");Serial.print(i);Serial.print(": ");
      Serial.print("Successfully calibrated point "); Serial.print(intSerialByte-128); Serial.print(". Min centroid: ");
      Serial.print(fltMinCentroid[i]); Serial.print(", Max centroid: "); Serial.print(fltMaxCentroid[i]); 
      Serial.print(", Ave Centroid (recorded calibration): "); Serial.println(FLTcalibrationTable[i][intSerialByte-128]);
    }
  }
  else if (intSerialByte == 228){saveCalibrationData();Serial.println("Calibration Data Saved!");}
  // ok . . . there's probably a better way to do this. But I like having the ascii numbers map to the
  // mode commands
  else if (intSerialByte == '0'){intMode=0;}
  else if (intSerialByte == '1'){intMode=1;}
  else if (intSerialByte == '2'){intMode=2;}
  else if (intSerialByte == '3'){intMode=3;}
  else if (intSerialByte == '4'){intMode=4;}
  else if (intSerialByte == '5'){intMode=5;}
  else if (intSerialByte == '6'){intMode=6;}
  else if (intSerialByte == '7'){intMode=7;}
  else if (intSerialByte == '8'){intMode=8;}
  else if (intSerialByte == '9'){intMode=9;}
}

void logHistoricalValues(){
  for (int i = 0; i < numCCDS; i ++){
    FLTcentroids[i][intHistoryCounter] = fltCentroidMath[i];
    FLTdistances[i][intHistoryCounter] = fltDistance[i];
  }
  intHistoryCounter++;
  if (intHistoryCounter >= intHistoryLength){intHistoryCounter = 0;}
}

void forceGenerateCalTable(){
  for (int i = 0; i < numCCDS; i ++){
    for (int j = 0; j < 50; j ++){
      FLTcalibrationTable[i][j] = 150;
    }
  }
  saveCalibrationData();
}

void computeDerivatives(){
  fltAveDistance = 0;
  fltMinDistance = 50;
  fltMaxDistance = 0;
  // compute average
  // compute max and min so we can compute range
  for (int i = 0; i < numCCDS; i ++){
    fltAveDistance += fltDistance[i];
    if (fltDistance[i] < fltMinDistance){fltMinDistance = fltDistance[i];}
    if (fltDistance[i] > fltMaxDistance){fltMaxDistance = fltDistance[i];}
  }
  fltAveDistance = fltAveDistance/numCCDS;
  
  //Serial.print(fltAveDistance);Serial.print(",");
  //Serial.print(fltDistance[0]);Serial.print(",");
  //Serial.print(fltDistance[1]);Serial.print(",");
  //Serial.print(fltDistance[2]);Serial.print(",");
  //Serial.print(fltMinDistance);Serial.print(",");
  //Serial.print(fltMaxDistance);
  //Serial.println();
  
  
  fltTilt = fltMaxDistance - fltMinDistance;
  fltVelocity = fltAveDistance - fltLastDistance;
  fltLastDistance = fltAveDistance;
  fltAccel = fltVelocity - fltLastVelocity;
  fltLastVelocity = fltVelocity;
  if (intLastStateValid == 0){
    fltVelocity = 0;
    fltLastVelocity = 0;
    fltAccel = 0;
  }

  intLastStateValid = 1;
}

#endif
