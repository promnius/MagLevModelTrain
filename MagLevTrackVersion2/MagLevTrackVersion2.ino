

#include <ADC.h>
#include <WS2812Serial.h>
#include "LEDManagement.h"
#include "AnalogSensing.h"
#include "GlobalVariables.h"
#include "PowerAndControls.h"



// ***************************************************************************************************************************************************

void setup() {
  Serial.begin(1000000);
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

  updateControlLoops(); // will not always run, or may run for some magnets only. Happens fast enough
  // that asynchronous updates won't matter.
  adjustMagnetPowerLevels();

  printMagnetOneSettings();

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

void printMagnetOneSettings(){
  //Serial.print(micros());Serial.print(",");Serial.print(intSensorsNow[0]);Serial.print(",");Serial.println(intPowerNow[0]);
}

 
