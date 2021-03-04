

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
  beginAnalogSensors(); // set up pinmodes and default states
  setupControlLoops();
  leds.begin();
  lngLEDUpdateTime = millis();
}

// ***************************************************************************************************************************************************

void loop() {
  // sample all analog inputs
  lngScanStartTime = micros();
  SampleTheSensors(); // this is currently a blocking function!! takes about 750us??
  lngScanEndTime = micros();

  updateControlLoops();
  
  recordAverageSamples();
  recordAveragePowers();
  loopCounter +=1;
  if (loopCounter == 10){
    Serial.println(micros()-lngLEDUpdateTime);
    lngLEDUpdateTime = micros();
    loopCounter = 0;
    plotHeightsAve();
    plotPositionAve();
    plotPowerAve();
    correctForGamma();
    leds.show();
  }
  //delay(30);
}

 
