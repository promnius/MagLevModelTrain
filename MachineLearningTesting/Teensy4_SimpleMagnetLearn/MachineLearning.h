
#ifndef MachineLearning_h
#define MachineLearning_h

#include "Arduino.h"
#include "GlobalVariables.h"
#include <EloquentTinyML.h>
#include "TeensyModel.h"

#define NUMBER_OF_INPUTS 5
#define NUMBER_OF_OUTPUTS 3
// in future projects you may need to tweek this value: it's a trial and error process
#define TENSOR_ARENA_SIZE 256*1024

DMAMEM Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml_model;

void initializeML(){
  ml_model.begin((unsigned char*)modelParams);
}

float MLInput[5] = {0,0,0,0,0};
float y_pred[3] = {0};
int bestAction = 0;
float fltMaxValue = 0;
int action = 0;
int intEpsilon = 1000;
int intEpsilonDecay = 10; // 70 episodes to reach eps. min
int intEpsilonMin = 300;

void computeMagnetPowerMLMethod(){
  MLInput[0]=fltAveDistance;
  MLInput[1]=fltVelocity;
  MLInput[2]=fltAccel;
  MLInput[3]=fltAveDistance-fltSetpoint;
  MLInput[4]=fltMagnetCurrent;
  ml_model.predict(MLInput,y_pred);

  if (random(1000) < intEpsilon){ // Choose random action
    action = random(3);
    //Serial.print("R");
  } else{ // choose ML action
    bestAction = 0;
    fltMaxValue = y_pred[bestAction];
    for (int i=1; i<3;i++){
      if (y_pred[i] > fltMaxValue){
        fltMaxValue = y_pred[i];
        bestAction = i;
      }
    }
    action = bestAction;
    //Serial.print("M");
  }
  // apply action
  if (action == 0){fltMagnetCurrent -= .1;}
  else if (action == 1){}
  else if (action == 2){fltMagnetCurrent += .1;}

  //Serial.print(action);

  // NOW actually set the PWM power, after some housekeeping
  // protect magnet by limiting power
  if(fltMagnetCurrent > 1.5){fltMagnetCurrent = 1.5;}
  if (fltMagnetCurrent < -1.5){fltMagnetCurrent = -1.5;}

  intMagnetPower = int(fltMagnetCurrent*1500);
}

// SHOULD NOT HARDCODE RESET THRESHOLD!!
void resetSimulation(){
  Serial.println("Reseting Simulation");
  intMagnetPower = 2250;
  setMagnetPower();
  delay(500);
  scanCCDs(); // time expensive, no way around this.
  scanCCDs(); // VERY crude way of flushing buffer . . . should really add in the shutter function to standardize timings
  computeCentroid(); // Fast, but uses the hardware background threshold.
  computeCentroidMath(); // computationally expensive. Maybe not a problem on the teensy
  computeDistance(); // CURRENTLY USES CENTROID MATH. Currently very slow, due to bad implementation.
  computeDerivatives();
  logHistoricalValues(); // CURRENTLY USES CENTROID MATH
  if (fltAveDistance < 10){ // we are stuck on the upper rail
    intMagnetPower= -2250;
    setMagnetPower();
    delay(1500);
    intMagnetPower = 2250; // momentarily force us to a rail. Later on this should be random between upper and lower rail
    setMagnetPower();
    delay(1000);
  }
  intMagnetPower = 0; // let the system cool down while data transfer is happening
  setMagnetPower();
  fltSetpoint = (float(random(1000))/76.92)+9; // dynamic range from 4.5 to 28.5. We want to utilize the middle 60% (extra empahsis on staying
    // off bottom rail), so we map to 9-22 with a dynamic range of 13
  fltSetpoint = 16.0;
  Serial.println("Reset Complete");
}

void logMarkovTransition(){
  FLTPositionLog[episodeCounter]=fltAveDistance;
  FLTVelocityLog[episodeCounter]=fltVelocity;
  FLTAccelLog[episodeCounter]=fltAccel;
  FLTPositionErrorLog[episodeCounter]=fltAveDistance-fltSetpoint;
  FLTCurrentLog[episodeCounter]=fltMagnetCurrent;
  INTActionLog[episodeCounter]=action;
  FLTTargetPositionLog[episodeCounter]=fltSetpoint;
}

// WARNING WE NEED TO SET A WAY TO MAKE PRINT RESOLUTION CONFIGURABLE, for some problems
// 2 decimal places will be unacceptable
void sendEpisodeData(){
  //delay(15000);
  Serial.println("DATA_START");
  
  for (long counter = 0; counter < maxEpisodeDuration; counter ++){
    Serial.print(counter);Serial.print(",");
    Serial.print(FLTPositionLog[counter]);Serial.print(",");
    Serial.print(FLTVelocityLog[counter]);Serial.print(",");
    Serial.print(FLTAccelLog[counter]);Serial.print(",");
    Serial.print(FLTPositionErrorLog[counter]);Serial.print(",");
    Serial.print(FLTCurrentLog[counter]);Serial.print(",");
    Serial.print(INTActionLog[counter]);Serial.print(",");
    Serial.print(FLTTargetPositionLog[counter]);
    Serial.println();
    delayMicroseconds(200); // allow CPU on computer to keep up, not sure how to find the best value here
      // or if its always needed based on implementation, etc. at 200uS, transmission is at 5x real time 
      // (assuming 1khz update rate).
  }
  Serial.println("DATA_END");
}

// WARNING!!! If this is used, NO OTHER SERIAL (input) SHOULD BE USED since every byte is interpreted
void checkForModelUpdate(){
  Serial.println("Blocking until we recieve any form of serial data");
  while (Serial.available() < 1){} // want this to be a blocking function since we know we need a new model
  if (Serial.available() > 0) { // for the sake of this test, the only serial comms will be a new model dump.
    Serial.println("Serial received, prepping for data download");
    // the PC will have sent 1 synce byte, content doesn't matter. Serial.flush() has changed over the 
    // years and doesn't seem to do what I'm looking for anymore
    //Serial.flush(); 
    while(Serial.available() > 0){Serial.read();} // clear serial buffer
    Serial.println("ReadyForDownload"); // This is the actual string that signals to the PC we are ready for download, this triggers
    // the incoming data
    lngModelTransferStart = micros();
    for (long dataTransferCounter = 0; dataTransferCounter < modelParams_len; dataTransferCounter ++){
      while(Serial.available() == 0){} // delay and wait for transmission to arrive
      serialByte = byte(Serial.read()); // do we need to cast this to byte?
      modelParams[dataTransferCounter] = serialByte;
    }
    lngModelTransferEnd = micros();
    Serial.print(modelParams_len); Serial.print(" bytes of data recieved in "); Serial.print(lngModelTransferEnd-lngModelTransferStart); Serial.println("us");
  }
}

void handleTerminalConditions(){
  if (episodeCounter >= maxEpisodeDuration){
    Serial.print("Episode finished, current epsilon: ");Serial.println(intEpsilon);
    Serial.println();
    episodeCounter = 0;
    resetSimulation();
    sendEpisodeData();
    checkForModelUpdate();
    //Serial.println("Hello1");
    intEpsilon -= intEpsilonDecay;
    if (intEpsilon < intEpsilonMin){intEpsilon = intEpsilonMin;}
    //Serial.println("Hello2");
  }
  else{ // game not over, log transition
    logMarkovTransition();
    episodeCounter ++;
  }
}

#endif
