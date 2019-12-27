

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
  //configureMagnetDrives(); // set up pinmodes and default states
  // also set default magnet drive directions.
  beginAnalogSensors(); // set up ADCs
  calibrateSensors(); // get basic sensor offsets
  setupControlLoops(); // initialize control loops
  leds.begin();
  lngLEDUpdateTime = millis();
}

// ***************************************************************************************************************************************************

void loop() {
  // sample all analog inputs
  lngScanStartTime = micros();
  SampleTheSensors(); // this is currently a blocking function!! takes about 750us??
  lngScanEndTime = micros();
  ExtractUsefulData(); // looking for vertical and horizontal positions, in real units
  lngConversionMathEndTime = micros();

  updateControlLoops();
  lngControlLoopMathEndTime = micros();
  //adjustMagnetPowerLevels();

  loopCounter +=1;
  if (loopCounter > 99){loopCounter = 0; averagesValid = true;}
  
  if (millis() - lngLEDUpdateTime > 200){
    //Serial.print(maxSensorValueLocation);Serial.print(",");Serial.println(intMaxHeight_uI/1000);
    //Serial.println(455-(maxSensorValueLocation/3)-(intMaxHeight_uI/1000));
    //Serial.println(intMaxHeight_uI/1000);

    
    Serial.print("Maximums: ");
    for (int i = 0; i < 40; i ++){
      //Serial.print(intSensorData_uT_Max[i]);Serial.print(",");
      Serial.print(intSensorData_uT_Max_Avg[i]);Serial.print(",");
    } Serial.println(); Serial.print("Minimums: ");
    for (int i = 0; i < 40; i ++){
      //Serial.print(intSensorData_uT_Min[i]);Serial.print(",");
      Serial.print(intSensorData_uT_Min_Avg[i]);Serial.print(",");
    } Serial.println(); Serial.print("Averages: ");    
    for (int i = 0; i < 40; i ++){
      Serial.print(getAverageOfArray(intSensorData_uT_Avg[i],100));Serial.print(",");
    }Serial.println(); Serial.println(); 
    /*
    Serial.print(intSensorData_uT[2]);Serial.print("-");Serial.print(intSensorCalibrationOffset[2]);Serial.print("=");Serial.println(intSensorData_uT[2]-intSensorCalibrationOffset[2]);
    for (int i = 0; i < 100; i ++){
      Serial.print(intSensorData_uT_Avg[2][i]);Serial.print(",");
    }Serial.println();Serial.println(); 
    */
    lngLEDUpdateTime = millis();
    plotAllGraphs();
    correctForGamma();
    leds.show();
  }
  //delay(1000);
}

 
