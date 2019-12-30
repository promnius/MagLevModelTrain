
#ifndef GlobalVariables_h
#define GlobalVariables_h

#include "Arduino.h"
#include <ADC.h>
#include <WS2812Serial.h> 

// PIN DECLARATIONS
const int pinANALOGSELECT0 = 7; //(Rx3)
const int pinANALOGSELECT1 = 10; //(Tx2)
const int pinANALOGSELECT2 = 9; //(Rx2)
//const int pinIRONOFF = 0;
const int pinLEDDATA = 1;
const int pinMAGNETPWM0 = 3; const int pinMAGNETPWM1 = 4; const int pinMAGNETPWM2 = 20; const int pinMAGNETPWM3 = 21;
const int pinMAGNETPWM4 = 22; const int pinMAGNETPWM5 = 23; const int pinMAGNETPWM6 = 32; const int pinMAGNETPWM7 = 25;
const int MAGNETPWMPINS[] = {pinMAGNETPWM0, pinMAGNETPWM1, pinMAGNETPWM2, pinMAGNETPWM3, pinMAGNETPWM4, pinMAGNETPWM5, pinMAGNETPWM6, pinMAGNETPWM7};
const int pinMAGNETDIRECTION0 = 5; const int pinMAGNETDIRECTION1 = 6; const int pinMAGNETDIRECTION2 = 14; const int pinMAGNETDIRECTION3 = 19;
const int pinMAGNETDIRECTION4 = A10; const int pinMAGNETDIRECTION5 = 33; const int pinMAGNETDIRECTION6 = 24; const int pinMAGNETDIRECTION7 = 27;
const int MAGNETDIRECTIONPINS[] = {pinMAGNETDIRECTION0, pinMAGNETDIRECTION1, pinMAGNETDIRECTION2, pinMAGNETDIRECTION3,
    pinMAGNETDIRECTION4, pinMAGNETDIRECTION5, pinMAGNETDIRECTION6, pinMAGNETDIRECTION7};
const int pinMAGNETENABLE = 28;
const int NUMANALOGPINS = 8;
const int ANALOGPINS [] = {A1, A15, A2, A18, A3, A19, A4, A20}; // these are set up in pairs, IE [0] is on adc0, and [1] is on adc1, and will be sampled together
// things will break if this is not an even number of entries, where every other entry
// is accessable by the appropriate ADC, and if NUMANALOGPINS is not the exact number
// of entries. There could be some improvement here

// Variable Declarations, some are obsolete?
int intScanCounter = 0;
int intAdcCounter = 0;
int intAnalogPinCounter = 0;
int intMuxCounter = 0;
int intBrightnessScalar = 6;
long lngSum = 0;
int oneHotPosition = 0;
boolean averagesValid = false;

// fancy variables for the Analog inputs
long intSensorData_Raw[64]; // 
long intSensorData_uV[64]; // these are all duplicated data in different formats, so a waste of RAM if that ever gets tight. For now, its kind of useful to have all values always available.
long intSensorData_uT[64];
long maxSensorValue_uT = 0;
long intMaxHeight_uI = 0;

long maxSensorDataIndex = 0;
long maxSensorValueLocation = 0;
long minValidSensor = 0;
long maxValidSensor = 0;

// recording variables for display and data tracking (not striktly necessary if we run out of ram)
long intBarGraphCurrents[100];
long intSensorDataHeightBarGraph_uT[5];
long intBarGraphHeights[8][100];
long intSensorData_uT_Avg[40][100];
long intSensorData_uT_Max[40];
long intSensorData_uT_Min[40];
long intSensorData_uT_Max_Avg[40];
long intSensorData_uT_Min_Avg[40];
long intBarGraphPositions[100];


// Timing variables
int loopCounter = 0;
unsigned long lngScanStartTime = 0;
unsigned long lngScanEndTime = 0;
unsigned long lngLEDUpdateTime = 0;
unsigned long lngConversionMathEndTime = 0;
unsigned long lngControlLoopMathEndTime = 0;
unsigned long lngLEDUpdatesEndTime = 0;
unsigned long lngLEDUpdatesEndTime0 = 0;
unsigned long lngLEDUpdatesEndTime1 = 0;
unsigned long lngLEDUpdatesEndTime2 = 0;

// ADC variables
ADC *adc = new ADC();
ADC::Sync_result ADC_DualResult;

// fancy variables for LEDs
const int numled = 384;
byte drawingMemory[numled*3];         //  3 bytes per LED
DMAMEM byte displayMemory[numled*12]; // 12 bytes per LED
WS2812Serial leds(numled, displayMemory, drawingMemory, pinLEDDATA, WS2812_GRB);

// PID variables
long maxResolution = 1024;
long maxPower = 512; // 1024 max, based on 10 bit PWM. Shouldn't exceed 512 until we have some thermal protection like
// joule tracking.

long setpoint_uI = 200000;
long error_uI = 0;
long kk = 50000; // uN, based on mass and gravity
long kpT = 1; // kpT/kpB forms kp, in integer math. Units of uN per uI of error (or Newtons per Inch of error)
long kpB = 1;
long kdT = 0; // Units of uN per (uI/timebase), so for 1khz this is , for 10khz it is 
long kdB = 1;
long kddT = 0;
long kddB = 1;
long dterm = 0;
long lastDterm = 0;
long ddterm = 0;
long lastMaxHeight = 0;

boolean magnetActive[8];
long targetForce_uN = 0;
long targetCurrent_uA = 0;
long targetPWM = 0;
boolean targetPolarity = 0;


#endif
