


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

boolean testNegativePower = false;



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
  if (!loopActive){ // check to see if the user is ready to reactivate
    if(debug && (millis() - lngPowerUpdateTime) > 2000){
      Serial.println("System idle, waiting for user input. send 's' to start. send 'e' to end");
      lngPowerUpdateTime = millis();
    }
    while(Serial.available() > 0){
      serialCommand = Serial.read();
      if(serialCommand == 's'){
        if(debug){Serial.println("Recieved command to start new cycle");}
        delay(500);
        powerLevel = maxPower;
        lngPowerUpdateTime = millis();
        loopActive = true;
      }
    }
  }

  if(debug){cycleTimer = 300;} else{cycleTimer = 100;}
  if ((millis() - lngPowerUpdateTime > cycleTimer)&& loopActive){
    lngPowerUpdateTime = millis();
    
    if (Serial.available() > 0){
      serialCommand = Serial.read();
      if(serialCommand == 'e'){
        Serial.println("End command received, cycle stopped.");
        Serial.print("Final power level: "); Serial.println(powerLevel);
        powerLevel = 0;
        loopActive = false;
      }
    }
    else if ((powerLevel-2)<-maxPower){
      Serial.println("End of cycle reached, user did not terminate!");
      powerLevel = 0;
      loopActive = false;
    }
    else { // target has not moved, lets increase power!
      powerLevel = powerLevel -2;
      if(debug){Serial.print("Target didn't move, increase power. ");}
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

 
