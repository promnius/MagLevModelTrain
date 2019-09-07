
#ifndef AnalogSensing_h
#define AnalogSensing_h

#include "Arduino.h"
#include "GlobalVariables.h"

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

// This is a function that is fairly application specific and rather hard coded, but enables
// moderately efficient scanning of many many analog signals (using external muxes). Due to the 
// external muxes, DMA and PDB are both more difficult to use (not impossible probably), so we 
// don't use them here. We use syncronized scans, not because we care about identical timestamps,
// but because it is the simplest way to utilize both ADCs at once (we can call blocking functions
// this way without needing to set up any of our own interrupts, and yet we don't need to worry about
// one adc sitting idle). Because of these design choices, this function is a fully blocking function
// that returns when everything has been sampled. It has been designed for the MAG LEV research in specific,
// and blocks for about 750uS. This enables a control loop to run at 1khz (1ms per loop), with some time 
// left over for processing. It also enables some level of oversampling/averaging, which is handled here.
// due to the additional artifacts of the muxes, we prefer to manually scan through the analog pins multiple
// times, and average the results ourselves, than to use the built in oversampleing functions.
// that way artifacts from the mux switching are also replicated on every scan. We have also opted to
// directly manipulate global arrays corresponding to the appropriate data (again, hardcoded application
// specific), rather than return the information. This is a poor architectural choice, but allows greater
// planning when it comes to memory allocation.
void SampleTheSensors(){
  intAdcCounter = 0;
  // currently set up for: 4 scans for averaging, 3 bit muxes (so 8 inputs per mux), and 8 pins used on chip (64 signals, 256 samples). This may be too slow.
  // currently set up to only average one read.
  for (intScanCounter = 0; intScanCounter < 4; intScanCounter++){
    //Serial.print("SCAN: "); Serial.println(intScanCounter);
    for (intMuxCounter = 0; intMuxCounter < 8; intMuxCounter ++){ // current hardware assumptions include 3 bit muxes.
      //Serial.print("MUX CHANNEL: "); Serial.println(intMuxCounter);
      setMux(intMuxCounter);
      delayMicroseconds(1); // some very small time is allowed for the muxes to switch, equilize, and settle. The
      // assumption is that the hardware is fully buffered and has sufficient drive capacitance that this delay can
      // be small.
  
      for (intAnalogPinCounter = 0; intAnalogPinCounter < NUMANALOGPINS; intAnalogPinCounter = intAnalogPinCounter + 2){
        //Serial.print("ANALOG DUAL READ: "); Serial.println(intAnalogPinCounter);
        // Actually Sample the ADCs
        ADC_DualResult = adc->analogSyncRead(ANALOGPINS[intAnalogPinCounter], ANALOGPINS[intAnalogPinCounter+1]);
        intRawAnalogReadDump[intAdcCounter] = ADC_DualResult.result_adc0;
        //Serial.print(intAdcCounter); Serial.print(":"); Serial.println(ADC_DualResult.result_adc0);
        intAdcCounter++;
        intRawAnalogReadDump[intAdcCounter] = ADC_DualResult.result_adc1;
        //Serial.print(intAdcCounter); Serial.print(":"); Serial.println(ADC_DualResult.result_adc1);
        intAdcCounter++;
        // in theory a single adc conversion should take the same amount of time as the dual conversion, but we can try that here too.
        // intRawAnalogReadDump[intAdcCounter] = adc->analogRead(readPin); // can also supply which adc to use, so the function doesn't waste time figuring out which is more available
        // intAdcCounter++;
      }
    }
  }
}

// when the sensors are scanned, they are done so in the most convineint fashion, and dropped into a long array
// with no concern for where. This finds out where a particular sensor's first read is in the array (each consecutive
// read will be multiples of 64 plus the number this function finds).
// NO ERROR CHECKING!!
int computeDataLocation(int intSensorType, int intSensorNumber){
  int intDataLocation = 0;
  int intSensorNumberExpanded = 0; // lets convert sensor number into 0 to 63, rather
  // than broken out by sensor type
  if (intSensorType == 0) {intSensorNumberExpanded = intSensorNumber;}
  else if (intSensorType == 1) {intSensorNumberExpanded = intSensorNumber + 40;}
  else {intSensorNumberExpanded = intSensorNumber + 48;} // assuming intSensorType == 2
  
  intDataLocation = (intSensorNumberExpanded%8)*8; // which mux was it sampled on
  if (intSensorNumberExpanded/8<4) {intDataLocation = intDataLocation + (intSensorNumberExpanded/8)*2;} // was sampled on ADC0
  else {intDataLocation = intDataLocation + (((intSensorNumberExpanded/8)-4)*2)+1;} // was sampled on ADC1
  return intDataLocation;
}

// This function takes the long array of raw analog reads and extracts an average for the selected sample.
// note that it returns a sum rather than a true average, so for now we can keep everything as raw integers
// intSensorType is 0 for IR breakbeams, 1 for IR reflectivity, and 2 for Hall sensors.
// eventually we can do this while sampling, which will save a lot of memory (ie, just add the new sample
// to the existing bin), but for now it is also interesting to have access to the raw reads for the sake
// of data analysis.
int extractData(int intSensorType, int intSensorNumber){
  int intFirstLocation = computeDataLocation(intSensorType, intSensorNumber);
  return intRawAnalogReadDump[intFirstLocation]+intRawAnalogReadDump[intFirstLocation+64]+intRawAnalogReadDump[intFirstLocation+128]+intRawAnalogReadDump[intFirstLocation+192];
}


// same as extractData, except instead of returning a sum, it prints all the raw reads
void printRawData(int intSensorType, int intSensorNumber){
  int intFirstLocation = computeDataLocation(intSensorType,intSensorNumber);
  //Serial.print("Sensor type "); Serial.print(intSensorType); Serial.print(", Number "); Serial.print(intSensorNumber); Serial.print(", Raw Reads: ");
  Serial.print(intRawAnalogReadDump[intFirstLocation]); Serial.print(",");
  Serial.print(intRawAnalogReadDump[intFirstLocation+64]); Serial.print(",");
  Serial.print(intRawAnalogReadDump[intFirstLocation+128]); Serial.print(",");
  Serial.println(intRawAnalogReadDump[intFirstLocation+192]);
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
  pinMode(pinIRONOFF, OUTPUT);
  digitalWrite(pinIRONOFF, HIGH);
}

#endif
