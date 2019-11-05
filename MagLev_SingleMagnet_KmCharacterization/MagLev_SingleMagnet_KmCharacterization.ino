


// to use multiple (or different) magnets: in PowerAndControls/adjustMagnetPowerLevels(), uncomment
// desired magnets. in LEDManagement/plotAllGraphs(), uncomment corresponding bar graphs. in main
// code, add a way to resolve intSensorsNow[0] to initSensorValue and sensorValue (two lines). Easiest
// option, just average all the active sensors in both places (assuming we don't want to map each sensor
// indevidually?? maybe we have to??)

#include <ADC.h>
#include <WS2812Serial.h>
#include "LEDManagement.h"
#include "AnalogSensing.h"
#include "GlobalVariables.h"
#include "PowerAndControls.h"



// ***************************************************************************************************************************************************

void setup() {
  Serial.begin(9600);
  configureADCs(); // prep adcs for max speed
  configureMagnetDrives(); // set up pinmodes and default states
  // also set default magnet drive directions.
  beginAnalogSensors(); // set up ADCs
  
  leds.begin();
  lngLEDUpdateTime = millis();
  lngPowerUpdateTime = millis();
}

// ***************************************************************************************************************************************************



void loop() {
  // sample all analog inputs
  lngScanStartTime = micros();
  SampleTheSensors(); // this is currently a blocking function!! takes about 750us??
  ExtractUsefulData();
  lngScanEndTime = micros();

  if (!loopActive){ // check to see if the user is ready to reactivate
    if(debug && (millis() - lngPowerUpdateTime) > 1000){
      Serial.println("System idle, waiting for user input. send 'p' to start.");
      Serial.print("Calibrated Sensor Values: "); 
      for (int i = 0; i < 8; i ++){
        Serial.print(intSensorsNow[i]); Serial.print(",");
      } Serial.println();
      lngPowerUpdateTime = millis();
    }
    while(Serial.available() > 0){
      serialCommand = Serial.read();
      if(serialCommand == 'p'){
        if(debug){Serial.println("Recieved command to start new cycle");}
        powerLevel = 0;
        lngPowerUpdateTime = millis();
        loopActive = true;
        initSensorValue = intSensorsNow[0];
        if(debug){Serial.print("initial sensor value: "); Serial.println(initSensorValue);}
      }
    }
  }

  if(debug){cycleTimer = 1000;} else{cycleTimer = 100;}
  if ((millis() - lngPowerUpdateTime > cycleTimer)&& loopActive){
    sensorValue = intSensorsNow[0];
    
    lngPowerUpdateTime = millis();
    tmp = sensorValue - initSensorValue;
    if (abs(tmp) > sensorNoiseThreshold) { // the target has moved!!!
      if(debug){Serial.println("Target moved, cycle over!");}
      Serial.print(initSensorValue); Serial.print(","); Serial.println(powerLevel);
      powerLevel = 0;
      loopActive = false;
    }
    else if ((powerLevel+2)>maxPower){
      Serial.print(initSensorValue);Serial.println(", Could Not Lift!!");
      if(debug){Serial.println("Cycle over!");}
      powerLevel = 0;
      loopActive = false;
    }
    else { // target has not moved, lets increase power!
      powerLevel = powerLevel +2;
      if(debug){Serial.println("Target didn't move, increase power");}
      if(debug){Serial.print("New power: "); Serial.println(powerLevel);}
    }
    
    adjustMagnetPowerLevels();
  }
  
  if (millis() - lngLEDUpdateTime > 15){
    //Serial.println();
    lngLEDUpdateTime = millis();
    plotAllGraphs();
    correctForGamma();
    leds.show();
  }
  //delay(30);
}

 
