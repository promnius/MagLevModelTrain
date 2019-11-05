

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
  setupControlLoops(); // initialize control loops
  leds.begin();
  lngLEDUpdateTime = millis();
}

// ***************************************************************************************************************************************************

void loop() {
  // sample all analog inputs
  lngScanStartTime = micros();
  SampleTheSensors(); // this is currently a blocking function!! takes about 750us??
  ExtractUsefulData();
  lngScanEndTime = micros();

  updateControlLoops();
  adjustMagnetPowerLevels();

  loopCounter +=1;
  
  if (millis() - lngLEDUpdateTime > 15){
    //Serial.println();
    lngLEDUpdateTime = millis();
    plotAllGraphs();
    correctForGamma();
    leds.show();
  }
  //delay(30);
}

 
