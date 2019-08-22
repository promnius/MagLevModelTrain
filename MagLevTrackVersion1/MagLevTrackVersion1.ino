

#include <ADC.h>
#include <WS2812Serial.h>

#define RED    0x160000
#define GREEN  0x001600
#define BLUE   0x000016
#define YELLOW 0x101400
#define PINK   0x120009
#define ORANGE 0x100400

const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };


// PIN DECLARATIONS
const int pinANALOGSELECT0 = 7; //(Rx3)
const int pinANALOGSELECT1 = 10; //(Tx2)
const int pinANALOGSELECT2 = 9; //(Rx2)
const int pinIRONOFF = 0;
const int pinLEDDATA = 1;

// Variable Declarations
int intScanCounter = 0;
int intAdcCounter = 0;
int intAnalogPinCounter = 0;
int intMuxCounter = 0;
int intBrightnessScalar = 6;

// fancy variables for the analog reads
const int NUMANALOGPINS = 8;
const int ANALOGPINS [] = {A1, A15, A2, A18, A3, A19, A4, A20}; // these are set up in pairs, IE [0] is on adc0, and [1] is on adc1, and will be sampled together
// things will break if this is not an even number of entries, where every other entry
// is accessable by the appropriate ADC, and if NUMANALOGPINS is not the exact number
// of entries. There could be some improvement here
int intRawAnalogReadDump[500]; // This will need to be unpacked based on what it actually contains. Currently no initialization. Could add.

// fancy variables for LEDs
const int numled = 384;
byte drawingMemory[numled*3];         //  3 bytes per LED
DMAMEM byte displayMemory[numled*12]; // 12 bytes per LED
WS2812Serial leds(numled, displayMemory, drawingMemory, pinLEDDATA, WS2812_GRB);

// Timing variables
unsigned long lngScanStartTime = 0;
unsigned long lngScanEndTime = 0;


// ADC variables
ADC *adc = new ADC();
ADC::Sync_result ADC_DualResult;

// ***************************************************************************************************************************************************

void setup() {
  Serial.begin(9600);
  configureADCs(); // prep adcs for max speed

  
  pinMode(pinANALOGSELECT0, OUTPUT);
  pinMode(pinANALOGSELECT1, OUTPUT);
  pinMode(pinANALOGSELECT2, OUTPUT);
  digitalWrite(pinANALOGSELECT0, LOW);
  digitalWrite(pinANALOGSELECT1, LOW);
  digitalWrite(pinANALOGSELECT2, LOW);
  
  pinMode(pinIRONOFF, OUTPUT);
  digitalWrite(pinIRONOFF, HIGH);

  leds.begin();

  //updateBarGraph(435, 0, 500, 0, 16, 2, 1, 255);
  //updateBarGraph(303, 0, 500, 0, 16, 0, 1, 255);
  //updateBarGraph(150, 0, 500, 0, 16, 2, 0, 255);
  //correctForGamma();
  //leds.show();
  
}

// ***************************************************************************************************************************************************

void loop() {
 
  // sample all analog inputs
  lngScanStartTime = micros();
  SampleTheSensors(); // this is currently a blocking function!! takes about 750us??
  lngScanEndTime = micros();

  // update display for the real world:
  //Serial.print("Scan Finished in: ");
  //Serial.print(lngScanEndTime-lngScanStartTime);
  //Serial.println("uS");
  //Serial.print("Number of samples taken: ");
  //Serial.println(intAnalogPinCounter);

  //Serial.print("Sample results for sensor number 10: ");
  //Serial.print(intRawAnalogReadDump[10]); Serial.print(",");
  //Serial.print(intRawAnalogReadDump[10+64]); Serial.print(",");
  //Serial.print(intRawAnalogReadDump[10+128]); Serial.print(",");
  //Serial.println(intRawAnalogReadDump[10+192]);
  //printRawData(0,7);
  //extractData(0,10);
  //Serial.println();

  Serial.print(extractData(0,6)); Serial.print(",");
  Serial.print(extractData(0,7)); Serial.print(",");
  Serial.print(extractData(0,8)); Serial.print(",");
  Serial.print(extractData(0,31)); Serial.print(",");
  Serial.print(extractData(0,32)); Serial.print(",");
  Serial.println(extractData(0,33));

  plotAllHeights();
  correctForGamma();
  leds.show();
  
  delay(30);
}
// ***************************************************************************************************************************************************

// this is ugly . . .
void plotAllHeights(){
  updateBarGraph(extractData(0,1), 2000, 12000, 0, 16, 2, 1, 255);
  updateBarGraph(extractData(0,2), 2000, 12000, 0, 16, 0, 1, 255);
  updateBarGraph(extractData(0,3), 2000, 12000, 0, 16, 2, 0, 255);
  
  updateBarGraph(extractData(0,6), 2000, 12000, 32, 16, 2, 1, 255);
  updateBarGraph(extractData(0,7), 2000, 12000, 32, 16, 0, 1, 255);
  updateBarGraph(extractData(0,8), 2000, 12000, 32, 16, 2, 0, 255);
  
  updateBarGraph(extractData(0,11), 2000, 12000, 64, 16, 2, 1, 255);
  updateBarGraph(extractData(0,12), 2000, 12000, 64, 16, 0, 1, 255);
  updateBarGraph(extractData(0,13), 2000, 12000, 64, 16, 2, 0, 255);

  updateBarGraph(extractData(0,16), 2000, 12000, 96, 16, 2, 1, 255);
  updateBarGraph(extractData(0,17), 2000, 12000, 96, 16, 0, 1, 255);
  updateBarGraph(extractData(0,18), 2000, 12000, 96, 16, 2, 0, 255);

  updateBarGraph(extractData(0,21), 2000, 12000, 128, 16, 2, 1, 255);
  updateBarGraph(extractData(0,22), 2000, 12000, 128, 16, 0, 1, 255);
  updateBarGraph(extractData(0,23), 2000, 12000, 128, 16, 2, 0, 255);

  updateBarGraph(extractData(0,26), 2000, 12000, 160, 16, 2, 1, 255);
  updateBarGraph(extractData(0,27), 2000, 12000, 160, 16, 0, 1, 255);
  updateBarGraph(extractData(0,28), 2000, 12000, 160, 16, 2, 0, 255);

  updateBarGraph(extractData(0,31), 2000, 12000, 192, 16, 2, 1, 255);
  updateBarGraph(extractData(0,32), 2000, 12000, 192, 16, 0, 1, 255);
  updateBarGraph(extractData(0,33), 2000, 12000, 192, 16, 2, 0, 255);

  updateBarGraph(extractData(0,36), 2000, 12000, 224, 16, 2, 1, 255);
  updateBarGraph(extractData(0,37), 2000, 12000, 224, 16, 0, 1, 255);
  updateBarGraph(extractData(0,38), 2000, 12000, 224, 16, 2, 0, 255);
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

// ********************************************************************************************************************************************************************************8
// GRAPHIC FUNCTIONS

// WARNING: for long bar graphs, ints may overflow. No worries for 16 bit though.
// This function takes in a value inside a range, and prints it out to a bar graph composed of addressable LEDs.
// It takes a color channel (RGB), and an AddOrSubtract variable that dictates if the graph is positive (traditional)
// or negative (lights turning off as the value gets greater). It does NOT allow the user to specifiy a specific hex color,
// instead only allowing the color channels to be specified. This is because of the way that overlayed bar graph addition
// allows multiple values to be superimposed on a single set of LEDs. If a non-standard color is requested, you can simply
// print the same value to the bar graph twice (or three times) for each color channel, with a different MaxBrightness set
// for each. Note that MaxBrightness is converted to an integer scale factor, so some distortion is possible.
// When used in subtraction mode, the bar graph will NOT illuminate lights past the final value. It will only set 
// any lights that should be all on (which in subtract mode means all off) to be off, and set the final LED to the 
// appropriate dimmed value. The user must pre-seed the bar graph with all on LEDs before calling the subtract bar graph
// if they want a traditional (but inverted) bar graph. This allows two bar graphs on the same set of LEDs on the same color
// channel, where as long as the values are different enough that their 'final' LED is not the same LED, the bar graph will 
// display a sliding window of color where it is off below the minimum and off above the maximum and on inbetween. This can be used with alternating
// positive negative graphs on separate color channels to display many (up to 7) stacked bar graphs. example: if you add green, then
// add red, then subtract green, you'll get a POSITIVE green, POSITIVE yellow, and then a POSITIVE red all superimposed. (and possibly
// all shy of maxed out). make sure to manually order them from highest to lowest, or strange results can occur. (if each color belongs
// to a defined variable, and you can't gurantee that the variables will be ordered at all times, then stacked bar graphs is not a 
// good way to visually represent them).
void updateBarGraph(int intBarGraphValue, int intBarGraphMinimum, int intBarGraphMaximum, int intFirstLEDIndex, int intBarGraphNumLEDs, int intColorChannel, boolean booAddOrSubtract, int intMaxBrightness){
  int mappedBarGraphValue = map(intBarGraphValue, intBarGraphMinimum, intBarGraphMaximum, 0, 255*intBarGraphNumLEDs); // map(value, fromLow, fromHigh, toLow, toHigh)
  if (intMaxBrightness > 255){
    intMaxBrightness = 255;
  }
  else if (intMaxBrightness < 1){ // divide by zero would be bad
    intMaxBrightness = 1;
  }
  int intScaleFactor = (255/intMaxBrightness); // would be better as a float
  
  for (int i = intFirstLEDIndex; i < intFirstLEDIndex + intBarGraphNumLEDs; i++){
    if (mappedBarGraphValue < 0){ // ERROR. For very long bar graph, integer addition may wrap around. For now, this is unhandled.
      mappedBarGraphValue = 0;
    }
    if (mappedBarGraphValue == 0){ // beyond the final LED: the light should be off, but only in a positive graph
      if (booAddOrSubtract){ // add
        setPixelChannel(i, 0, intColorChannel);
      } else {} // do nothing, since subtract does not set the high pixels
    }
    else if (mappedBarGraphValue < 255){ // final LED in bar graph, exhibit dimming.
      if (booAddOrSubtract){ // add, set brightness
        setPixelChannel(i, (uint8_t)(mappedBarGraphValue/intScaleFactor), intColorChannel);
      }
      else { // subtract, set pixel value to . . . max brightness - scaled value, IF the pixel was already at max brightness.
        // otherwise . . . kind of undefined. for now we are doing current value - scaled value, making sure this is at least 0.
        // this enables a positive and negative graph on the same color channel to have the same last pixel, and the brightness
        // will represent the gap between them.
        reducePixelChannel(i,(uint8_t)(mappedBarGraphValue/intScaleFactor), intColorChannel);
        
      }
      mappedBarGraphValue = 0;
    }
    else { // not final LED in bar graph, set to full brightness (or full off), then subtract the value of one pixel
      if (booAddOrSubtract){ // add, set to max brightness
        setPixelChannel(i, 255/intScaleFactor, intColorChannel);
      }
      else { // subtract, set to off
        setPixelChannel(i, 0, intColorChannel);
      }
      mappedBarGraphValue -= 255;
    }
  }
}

// may be faster to pass around uint8s instead of ints.
void setPixelChannel(int num, uint8_t value, int channel) {
  //if (num >= numled) return;
  //if (value > 255){ value = 255;}
  //if (value < 0){ value = 0;}
  num *= 3;
  if (channel == 0){
    drawingMemory[num+0] = value;
  }
  else if (channel == 1){
    drawingMemory[num+1] = value;
  }
  else if (channel == 2){
    drawingMemory[num+2] = value;
  }
  else{} // error, not handled
}

void reducePixelChannel(int num, uint8_t value, int channel) {
  //if (num >= numled) return;
  num *= 3;
  if (channel == 0){
    if (value > drawingMemory[num+0]){ // the subtraction would be negative, so just set the pixel value to zero
      drawingMemory[num+0] = 0;
    } else { // time to subtract
      drawingMemory[num+0] = drawingMemory[num+0] - value;
    }
  }
  else if (channel == 1){
    if (value > drawingMemory[num+1]){ // the subtraction would be negative, so just set the pixel value to zero
      drawingMemory[num+1] = 0;
    } else { // time to subtract
      drawingMemory[num+1] = drawingMemory[num+1] - value;
    }
  }
  else if (channel == 2){
    if (value > drawingMemory[num+2]){ // the subtraction would be negative, so just set the pixel value to zero
      drawingMemory[num+2] = 0;
    } else { // time to subtract
      drawingMemory[num+2] = drawingMemory[num+2] - value;
    }
  }
  else{} // error, not handled
}

// this function corrects all LED values for proper gamma. At some point we may need a selective gamma correction
// function, as this will cause strange effects if called multiple times without redrawing a pixel to its nominal
// value. Right now the usage would be to redraw every pixel (nominally), then call this function before spitting
// the data out into hardware.
void correctForGamma(){
  for (int i = 0; i < numled*3; i++){
    drawingMemory[i] = pgm_read_byte(&gamma8[drawingMemory[i]])/intBrightnessScalar;
  }
}

// basic display function to test LEDs
void colorWipe(int color, int wait) {
  for (int i=0; i < leds.numPixels(); i++) {
    leds.setPixel(i, color);
    leds.show();
    delayMicroseconds(wait);
  }
}

// *************************************************************************************************************************************


 
