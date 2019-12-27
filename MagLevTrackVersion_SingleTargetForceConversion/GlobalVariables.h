
#ifndef GlobalVariables_h
#define GlobalVariables_h

#include "Arduino.h"
#include <ADC.h>
#include <WS2812Serial.h> 
#include <PID_v1.h>

// CALIBRATION TABLES
long intSensorCalibrationScalar[40] = {98,107,96,90,103,106,95,102,101,98,97,100,104,97,97,105,99,103,96,105,95,100,112,95,92,113,105,95,97,101,103,103,94,92,96,103,100,100,98,106};
long intSensorCalibrationOffset[40] = {4264,3386,2955,3249,3149,3251,4284,1969,3506,3241,3927,3914,3394,3109,3241,3868,2755,3982,3742,3211,4026,2730,3804,2986,3164,3484,4262,3458,3165,3526,2761,3519,2775,3710,4235,3809,3665,2293,4257,2285};


// PIN DECLARATIONS
const int pinANALOGSELECT0 = 7; //(Rx3)
const int pinANALOGSELECT1 = 10; //(Tx2)
const int pinANALOGSELECT2 = 9; //(Rx2)
const int pinIRONOFF = 0;
const int pinLEDDATA = 1;
const int pinMAGNETPWM0 = 3; const int pinMAGNETPWM1 = 4; const int pinMAGNETPWM2 = 20; const int pinMAGNETPWM3 = 21;
const int pinMAGNETPWM4 = 22; const int pinMAGNETPWM5 = 23; const int pinMAGNETPWM6 = 32; const int pinMAGNETPWM7 = 25;
const int MAGNETPWMPINS[] = {pinMAGNETPWM0, pinMAGNETPWM1, pinMAGNETPWM2, pinMAGNETPWM3, pinMAGNETPWM4, pinMAGNETPWM5, pinMAGNETPWM6, pinMAGNETPWM7};
const int pinMAGNETDIRECTION0 = 5; const int pinMAGNETDIRECTION1 = 6; const int pinMAGNETDIRECTION2 = 14; const int pinMAGNETDIRECTION3 = 19;
const int pinMAGNETDIRECTION4 = A10; const int pinMAGNETDIRECTION5 = 33; const int pinMAGNETDIRECTION6 = 24; const int pinMAGNETDIRECTION7 = 27;
const int MAGNETDIRECTIONPINS[] = {pinMAGNETDIRECTION0, pinMAGNETDIRECTION1, pinMAGNETDIRECTION2, pinMAGNETDIRECTION3,
    pinMAGNETDIRECTION4, pinMAGNETDIRECTION5, pinMAGNETDIRECTION6, pinMAGNETDIRECTION7};
const int pinMAGNETENABLE = 28;

// Variable Declarations
int intScanCounter = 0;
int intAdcCounter = 0;
int intAnalogPinCounter = 0;
int intMuxCounter = 0;
int intBrightnessScalar = 6;
long lngSum = 0;
int oneHotPosition = 0;

// fancy variables for the Analog
const int NUMANALOGPINS = 8;
const int ANALOGPINS [] = {A1, A15, A2, A18, A3, A19, A4, A20}; // these are set up in pairs, IE [0] is on adc0, and [1] is on adc1, and will be sampled together
// things will break if this is not an even number of entries, where every other entry
// is accessable by the appropriate ADC, and if NUMANALOGPINS is not the exact number
// of entries. There could be some improvement here
long intSensorData_Raw[64]; // 
long intSensorData_uV[64]; // these are all duplicated data in different formats, so a waste of RAM if that ever gets tight. For now, its kind of useful to have all values always available.
long intSensorData_uT[64];
long intSensorData_uT_Max[40];
long intSensorData_uT_Min[40];
long intSensorData_uT_Max_Avg[40];
long intSensorData_uT_Min_Avg[40];
long intSensorDataHeightBarGraph_uT[5];
long intBarGraphHeights[8][100];
long intSensorData_uT_Avg[40][100];
long intBarGraphPositions[100];
long intMaxHeight_uI = 0;
long maxSensorDataIndex = 0;
long maxSensorValueLocation = 0;
long maxSensorValue_uT = 0;
long minValidSensor = 0;
long maxValidSensor = 0;

boolean averagesValid = false;

int intSensorsNow[8]= {0,0,0,0,0,0,0,0};
int intSensorsAvg[8] = {0,0,0,0,0,0,0,0};
int intPowerNow[8]= {0,0,0,0,0,0,0,0};
int intPowerAvg[8]= {0,0,0,0,0,0,0,0};

// Timing variables
int loopCounter = 0;
unsigned long lngScanStartTime = 0;
unsigned long lngScanEndTime = 0;
unsigned long lngLEDUpdateTime = 0;
unsigned long lngConversionMathEndTime = 0;
unsigned long lngControlLoopMathEndTime = 0;

// ADC variables
ADC *adc = new ADC();
ADC::Sync_result ADC_DualResult;

// PID variables
int maxResolution = 1024;
int maxPower = 512; // 1024 max, based on 10 bit PWM. Shouldn't exceed 512 until we have some thermal protection like
// joule tracking.
int sampleTime = 1; // ms
// setpoint for round double Setpoint = 6000; // hopefully we can use the same setpoint for all of them!!
double Setpoint = 8000;
double PID_InputFromSensors[8]; // these are actually duplicate registers, just as doubles . . . could just cast in place
double PID_OutputMagnetCommand[8];
// controls for round, no Km table double Kp=.2, Ki=0, Kd=.01; // for now hopefully we can use the same for every magnet
// controls for round, Km table double Kp=.05, Ki=0, Kd=.006;
double Kp=.1, Ki=0, Kd=.1;
//double KmTable[4][2] = {{0,350},{7300,0},{8000,-80},{12000,-800}};
double KmTable[4][2] = {{0,0},{1,0},{2,0},{3,0}};
double KmTableLength = 4; // Make this calculated soon!!
PID myPID_Magnet0(&PID_InputFromSensors[0], &PID_OutputMagnetCommand[0], &Setpoint, Kp, Ki, Kd, DIRECT, KmTable, KmTableLength);
PID myPID_Magnet1(&PID_InputFromSensors[1], &PID_OutputMagnetCommand[1], &Setpoint, Kp, Ki, Kd, DIRECT, KmTable, KmTableLength);
PID myPID_Magnet2(&PID_InputFromSensors[2], &PID_OutputMagnetCommand[2], &Setpoint, Kp, Ki, Kd, DIRECT, KmTable, KmTableLength);
PID myPID_Magnet3(&PID_InputFromSensors[3], &PID_OutputMagnetCommand[3], &Setpoint, Kp, Ki, Kd, DIRECT, KmTable, KmTableLength);
PID myPID_Magnet4(&PID_InputFromSensors[4], &PID_OutputMagnetCommand[4], &Setpoint, Kp, Ki, Kd, DIRECT, KmTable, KmTableLength);
PID myPID_Magnet5(&PID_InputFromSensors[5], &PID_OutputMagnetCommand[5], &Setpoint, Kp, Ki, Kd, DIRECT, KmTable, KmTableLength);
PID myPID_Magnet6(&PID_InputFromSensors[6], &PID_OutputMagnetCommand[6], &Setpoint, Kp, Ki, Kd, DIRECT, KmTable, KmTableLength);
PID myPID_Magnet7(&PID_InputFromSensors[7], &PID_OutputMagnetCommand[7], &Setpoint, Kp, Ki, Kd, DIRECT, KmTable, KmTableLength);
boolean MAGNET0NORTH = LOW;
boolean MAGNET1NORTH = HIGH;
boolean MAGNET2NORTH = HIGH;
boolean MAGNET3NORTH = HIGH;
boolean MAGNET4NORTH = LOW;
boolean MAGNET5NORTH = LOW;
boolean MAGNET6NORTH = LOW;
boolean MAGNET7NORTH = LOW;

#endif
