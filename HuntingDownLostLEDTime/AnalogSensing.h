
#ifndef AnalogSensing_h
#define AnalogSensing_h

#include "Arduino.h"
#include "GlobalVariables.h"
#include "HelperFunctions.h"
#include "LookupTables.h"



void extractVerticalPosition(){
  long y0,y1,x0,x1;
  for (int i = 0; i < CTFluxHeightLength-1; i ++){
    // the long casts may be not useful?
    if (maxSensorValue_uT >= (long)pgm_read_dword(&CTFLuxHeightuTuI[i][0]) && maxSensorValue_uT < (long)pgm_read_dword(&CTFLuxHeightuTuI[i+1][0])){
      y0 = (long) pgm_read_dword(&CTFLuxHeightuTuI[i][1]);  //lower bound
      y1 = (long) pgm_read_dword(&CTFLuxHeightuTuI[i + 1][1]); //upper bound
      x0 = (long) pgm_read_dword(&CTFLuxHeightuTuI[i][0]);
      x1 = (long) pgm_read_dword(&CTFLuxHeightuTuI[i + 1][0]);
      intMaxHeight_uI = y0 + ((y1 - y0) * (maxSensorValue_uT - x0)) / (x1 - x0);
    }
  }
  intMaxHeight_uI = intMaxHeight_uI- 35000; // compensation for the die being lower than the part
  intMaxHeight_uI = 360000 - intMaxHeight_uI;
}

// allows a single function call for setting a mux channel. Note: this function currently
// only supports a single group of muxes (all using the same select lines).
// (may need to upgrade for the final project). Note there are OOP libraries for analog muxes,
// but this code is really simple.
void setMux(int channel){
  // if we are worried about speed, these digital writes need to be replaced. (and from hardware, pin selects
  // need to end up on one port!)
  if (channel%2 == 1){digitalWrite(pinANALOGSELECT0, HIGH);} else{digitalWrite(pinANALOGSELECT0, LOW);}
  if ((channel/2)%2 == 1){digitalWrite(pinANALOGSELECT1, HIGH);} else{digitalWrite(pinANALOGSELECT1, LOW);}
  if ((channel/4)%2 == 1){digitalWrite(pinANALOGSELECT2, HIGH);} else{digitalWrite(pinANALOGSELECT2, LOW);}
  // This hardware is only for an 8 to 1 mux
  //if ((channel/8)%2 == 1){digitalWrite(pinANALOGSELECT3, HIGH);} else{digitalWrite(pinANALOGSELECT3, LOW);}
  delayMicroseconds(1); // give settling/ capacitor charge shuffle equilization time.
}

int getIndexOfMaximumValue(long* myArray, int mySize){
 int maxIndex = 0;
 int maxValue = myArray[maxIndex];
 for (int i=1; i<mySize; i++){
   if (maxValue<myArray[i]){
     maxValue = myArray[i];
     maxIndex = i;
   }
 }
 return maxIndex;
}

// This is a function that is fairly application specific and rather hard coded, but enables
// moderately efficient scanning of many many analog signals (using external muxes).
void SampleTheSensors(){
  intAdcCounter = 0;
  for (intMuxCounter = 0; intMuxCounter < 8; intMuxCounter ++){ // current hardware assumptions include 3 bit muxes.
    setMux(intMuxCounter);
    delayMicroseconds(1); // some very small time is allowed for the muxes to switch, equilize, and settle. The
    // assumption is that the hardware is fully buffered and has sufficient drive capacitance that this delay can
    // be small.
    for (intAnalogPinCounter = 0; intAnalogPinCounter < NUMANALOGPINS; intAnalogPinCounter = intAnalogPinCounter + 2){
      // Actually Sample the ADCs
      ADC_DualResult = adc->analogSyncRead(ANALOGPINS[intAnalogPinCounter], ANALOGPINS[intAnalogPinCounter+1]);
      intSensorData_Raw[intMuxCounter + (intAnalogPinCounter/2)*8] = ADC_DualResult.result_adc0;
      intAdcCounter++;
      intSensorData_Raw[intMuxCounter + (intAnalogPinCounter/2 + 4)*8] = ADC_DualResult.result_adc1;
      intAdcCounter++;
    }
  }
}

// setting up both adcs to run reasonably fast
void configureADCs(){
  // Sampling speed is how long the ADC waits for the internal sample and hold cap to equilize. 
  // For a large, low impedance external cap (.1u or larger), this can be very small (very fast).
  // Conversion Speed is what base speed is used for the ADC clock. It influences sampling time 
  // (which is set in number of adc clocks), as well as how fast the conversion clock runs.
  // Resolution *MAY* influence how many adc clocks are needed to make a conversion. 
  // Averaging is how many samples to take and average.
  adc->setReference(ADC_REFERENCE::REF_3V3, ADC_0); // options are: ADC_REFERENCE::REF_3V3, ADC_REFERENCE::REF_EXT, or ADC_REFERENCE::REF_1V2.
  adc->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED); // options are VERY_LOW_SPEED, LOW_SPEED, MED_SPEED, HIGH_SPEED or VERY_HIGH_SPEED.
  adc->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_HIGH_SPEED); // options are VERY_LOW_SPEED, LOW_SPEED, MED_SPEED, HIGH_SPEED_16BITS, HIGH_SPEED or VERY_HIGH_SPEED. VERY_HIGH_SPEED may be out of specs
  adc->setAveraging(1); // set number of averages
  adc->setResolution(12); // set number of bits

  adc->setReference(ADC_REFERENCE::REF_3V3, ADC_1);
  adc->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED, ADC_1);
  adc->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_HIGH_SPEED, ADC_1);
  adc->setAveraging(1, ADC_1); 
  adc->setResolution(12, ADC_1);  
}

void beginAnalogSensors(){ // set default states
  pinMode(pinANALOGSELECT0, OUTPUT);
  pinMode(pinANALOGSELECT1, OUTPUT);
  pinMode(pinANALOGSELECT2, OUTPUT);
  digitalWrite(pinANALOGSELECT0, LOW);
  digitalWrite(pinANALOGSELECT1, LOW);
  digitalWrite(pinANALOGSELECT2, LOW);
  //pinMode(pinIRONOFF, OUTPUT);
  //digitalWrite(pinIRONOFF, HIGH);
}

// a very crude calibration, but we just record each sensors's zero position at power on, assuming
// all magnets are off and no magnets are on the track. Someday this will not be good enough (if the track
// is on for a long time sensors may drift? or a train may be on the track during a turn on event)
// we're also doing a single sample . . . during control loop execution this is important for speed
// but for calibration we could afford to do some averaging.
void calibrateSensors(){
  for (int i = 0; i < 40; i ++){
    intSensorData_uT_Max[i] = 0;
    intSensorData_uT_Min[i] = 1000000;
    intSensorData_uT_Max[i] = 0;
    intSensorData_uT_Min[i] = 1000000;
  }
  delay(1000); // be sure everything has powered up and stabalized.
}

void ApplySensorCalibration(int i){
  intSensorData_uT[i] = intSensorData_uT[i] - intSensorCalibrationOffset[i];
  intSensorData_uT[i] = (intSensorData_uT[i]*100)/intSensorCalibrationScalar[i];
  intSensorData_uT[i] = (intSensorData_uT[i] * 1100)/1000;
}
  
void ExtractUsefulData(){
  for (int i = 0; i < 40; i ++){
    intSensorData_uV[i] = intSensorData_Raw[i]; // abs(intSensorCalibrationOffset[i] - intSensorData_Raw[i]); // using abs here has the advantage that we don't care what
    // direction the sensing magnet is pointing . . . but possibly ruins our data in some setups too!!
    intSensorData_uV[i] = intSensorData_uV[i] * 805; // conversion to get from raw ADC bits to microVolts (rounded) from 3.3V reference and 12bit adc read
    intSensorData_uV[i] = intSensorData_uV[i] - 1000000; // adjust for 0 offset, since sensor data is bidirectional
    intSensorData_uT[i] = intSensorData_uV[i] / 11; // sensors have 11mV/mT, so convert to uT
    ApplySensorCalibration(i);
    intSensorData_uT[i] = abs(intSensorData_uT[i]); // this is crude and gives up information.
    if (intSensorData_uT_Max[i] < intSensorData_uT[i]){intSensorData_uT_Max[i] = intSensorData_uT[i];}
    if (intSensorData_uT_Min[i] > intSensorData_uT[i]){intSensorData_uT_Min[i] = intSensorData_uT[i];}
  }
  maxSensorDataIndex = getIndexOfMaximumValue(intSensorData_uT, 40);

  // compute display values
  for (int barGraphCounter = 0; barGraphCounter < 8; barGraphCounter ++){ // 8 bar graphs, this could be parameterized someday
    for (int i = 0; i < 5; i ++){ // 5 values per bar graph
      //intSensorDataHeightBarGraph_uT[i] = abs(intSensorData_uT[(5*barGraphCounter) + i]);
      intSensorDataHeightBarGraph_uT[i] = abs(intSensorData_uT[(barGraphCounter)]);
    }
    intBarGraphHeights[barGraphCounter][loopCounter] = intSensorDataHeightBarGraph_uT[getIndexOfMaximumValue(intSensorDataHeightBarGraph_uT, 5)];
  }

  for (int i = 0; i < 40; i ++){
    intSensorData_uT_Avg[i][loopCounter] = intSensorData_uT[i];
    if ((intSensorData_uT_Max_Avg[i] < getAverageOfArray(intSensorData_uT_Avg[i],100)) && (averagesValid == true)){intSensorData_uT_Max_Avg[i] = getAverageOfArray(intSensorData_uT_Avg[i],100);}
    if ((intSensorData_uT_Min_Avg[i] > getAverageOfArray(intSensorData_uT_Avg[i],100)) && (averagesValid == true)){intSensorData_uT_Min_Avg[i] = getAverageOfArray(intSensorData_uT_Avg[i],100);}
  }

    
  
  // find horizontal position. This is VERY crude, we are looking for the center of a symetric waveform, which may not be centered on
  // a sensor. For now, however, using integer math, we are just going to find the center of a region that is above a threshold.
  maxSensorValue_uT = intSensorData_uT[maxSensorDataIndex];
  minValidSensor = 40;
  maxValidSensor = 0;
  if (maxSensorValue_uT > 10000){ // if no magnet exists, just go to one end. using threshold 10mT.
    for (int i = 0; i < 40; i ++){
      if (intSensorData_uT[i] > (maxSensorValue_uT*50)/100){ // hard coded to within 50 percent
        maxValidSensor = i;
        if (minValidSensor == 40){
          minValidSensor = i;
        }
      }
    }
    maxSensorValueLocation = minValidSensor + maxValidSensor; // out of 80. Not dividing by 2 to avoid rounding error.    
  } else {
    maxSensorValueLocation = 0;
  }
  intBarGraphPositions[loopCounter] = maxSensorValueLocation*10; // the times 10 gives us some buffer for integer math

  // update max sensor value based on the known central position, to eradicate the 'horns' effect
  // NOT IMPLEMENTED YET- IT ONLY MATTERS FOR HEIGHTS WE CANT CONTROL ANYWAYS
  //maxSensorValue_uT = intSensorData_uT[maxSensorValueLocation/2];
  extractVerticalPosition();

}



#endif
