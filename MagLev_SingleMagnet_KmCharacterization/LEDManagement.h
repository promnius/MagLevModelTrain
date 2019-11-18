
#ifndef LEDManagement_h
#define LEDManagement_h

#include "Arduino.h"
#include "GlobalVariables.h"
#include "AnalogSensing.h"

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

// fancy variables for LEDs
const int numled = 384;
byte drawingMemory[numled*3];         //  3 bytes per LED
DMAMEM byte displayMemory[numled*12]; // 12 bytes per LED
WS2812Serial leds(numled, displayMemory, drawingMemory, pinLEDDATA, WS2812_GRB);

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

// this is ugly . . .
/*void plotAllHeights(){
  updateBarGraph(extractData(0,1), 2000, 12000, 0, 16, 2, 1, 255);  
  updateBarGraph(extractData(0,6), 2000, 12000, 32, 16, 2, 1, 255);  
  updateBarGraph(extractData(0,11), 2000, 12000, 64, 16, 2, 1, 255);
  updateBarGraph(extractData(0,16), 2000, 12000, 96, 16, 2, 1, 255);
  updateBarGraph(extractData(0,21), 2000, 12000, 128, 16, 2, 1, 255);
  updateBarGraph(extractData(0,26), 2000, 12000, 160, 16, 2, 1, 255);
  updateBarGraph(extractData(0,31), 2000, 12000, 192, 16, 2, 1, 255);
  updateBarGraph(extractData(0,36), 2000, 12000, 224, 16, 2, 1, 255);
}*/

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

// this is ugly . . .
void plotAllGraphs(){
  // sensor data
  updateBarGraph(intSensorsAvg[0], 2000, 12000, 0, 16, 1, 1, 255);  
  updateBarGraph(intSensorsAvg[1], 2000, 12000, 32, 16, 1, 1, 255);  
  updateBarGraph(intSensorsAvg[2], 2000, 12000, 64, 16, 1, 1, 255);
  updateBarGraph(intSensorsAvg[3], 2000, 12000, 96, 16, 1, 1, 255);
  updateBarGraph(intSensorsAvg[4], 2000, 12000, 128, 16, 1, 1, 255);
  updateBarGraph(intSensorsAvg[5], 2000, 12000, 160, 16, 1, 1, 255);
  updateBarGraph(intSensorsAvg[6], 2000, 12000, 192, 16, 1, 1, 255);
  updateBarGraph(intSensorsAvg[7], 2000, 12000, 224, 16, 1, 1, 255);

  // power data -----------------------------------------^----- this is the color channel.
  if (powerLevel < 0){updateBarGraph(abs(powerLevel), 0, maxPower, 16, 16, 0, 1, 255);}  
  else{updateBarGraph(abs(powerLevel), 0, maxPower, 16, 16, 2, 1, 255);} 
  //updateBarGraph(powerLevel, 0, maxPower, 48, 16, 1, 1, 255);  
  //updateBarGraph(powerLevel, 0, maxPower, 80, 16, 1, 1, 255);
  //updateBarGraph(powerLevel, 0, maxPower, 112, 16, 1, 1, 255);
  //updateBarGraph(powerLevel, 0, maxPower, 144, 16, 1, 1, 255);
  //updateBarGraph(powerLevel, 0, maxPower, 176, 16, 1, 1, 255);
  //updateBarGraph(powerLevel, 0, maxPower, 208, 16, 1, 1, 255);
  //updateBarGraph(powerLevel, 0, maxPower, 240, 16, 1, 1, 255);
}

void plotPositionAve(){
  
}


#endif
