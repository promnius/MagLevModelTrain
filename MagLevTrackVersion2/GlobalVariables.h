
#ifndef GlobalVariables_h
#define GlobalVariables_h

#include "Arduino.h"
#include <ADC.h>
#include <WS2812Serial.h> 
#include <PID_v1.h>

// PIN DECLARATIONS
const int pinANALOGSELECT0 = 7; //(Rx3)
const int pinANALOGSELECT1 = 10; //(Tx2)
const int pinANALOGSELECT2 = 9; //(Rx2)
const int pinIRONOFF = 0;
const int pinLEDDATA = 1;
const int pinMAGNETPWM0 = 0;
const int pinMAGNETPWM1 = 0;
const int pinMAGNETPWM2 = 0;
const int pinMAGNETPWM3 = 0;
const int pinMAGNETPWM4 = 0;
const int pinMAGNETPWM5 = 0;
const int pinMAGNETPWM6 = 0;
const int pinMAGNETPWM7 = 0;
const int pinMAGNETDIRECTION0 = 0;
const int pinMAGNETDIRECTION1 = 0;
const int pinMAGNETDIRECTION2 = 0;
const int pinMAGNETDIRECTION3 = 0;
const int pinMAGNETDIRECTION4 = 0;
const int pinMAGNETDIRECTION5 = 0;
const int pinMAGNETDIRECTION6 = 0;
const int pinMAGNETDIRECTION7 = 0;
const int pinMAGNETENABLE = 0;

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
int intSensorsNow[8]= {0,0,0,0,0,0,0,0};
int intSensorsAvg[8] = {0,0,0,0,0,0,0,0};
int intPowerNow[8]= {0,0,0,0,0,0,0,0};
int intPowerAvg[8]= {0,0,0,0,0,0,0,0};

// Timing variables
int loopCounter = 0;
unsigned long lngScanStartTime = 0;
unsigned long lngScanEndTime = 0;
unsigned long lngLEDUpdateTime = 0;

// ADC variables
ADC *adc = new ADC();
ADC::Sync_result ADC_DualResult;

// PID variables
int maxPower = 512; // 1024 max, based on 10 bit PWM. Shouldn't exceed 512 until we have some thermal protection like
// joule tracking.
int sampleTime = 3; // ms
double Setpoint = 6000; // hopefully we can use the same setpoint for all of them!!
double PID_InputFromSensors[8]; // these are actually duplicate registers, just as doubles . . . could just cast in place
double PID_OutputMagnetCommand[8];
double Kp=2, Ki=0, Kd=1; // for now hopefully we can use the same for every magnet
PID myPID_Magnet0(&PID_InputFromSensors[0], &PID_OutputMagnetCommand[0], &Setpoint, Kp, Ki, Kd, DIRECT);
PID myPID_Magnet1(&PID_InputFromSensors[1], &PID_OutputMagnetCommand[1], &Setpoint, Kp, Ki, Kd, DIRECT);
PID myPID_Magnet2(&PID_InputFromSensors[2], &PID_OutputMagnetCommand[2], &Setpoint, Kp, Ki, Kd, DIRECT);
PID myPID_Magnet3(&PID_InputFromSensors[3], &PID_OutputMagnetCommand[3], &Setpoint, Kp, Ki, Kd, DIRECT);
PID myPID_Magnet4(&PID_InputFromSensors[4], &PID_OutputMagnetCommand[4], &Setpoint, Kp, Ki, Kd, DIRECT);
PID myPID_Magnet5(&PID_InputFromSensors[5], &PID_OutputMagnetCommand[5], &Setpoint, Kp, Ki, Kd, DIRECT);
PID myPID_Magnet6(&PID_InputFromSensors[6], &PID_OutputMagnetCommand[6], &Setpoint, Kp, Ki, Kd, DIRECT);
PID myPID_Magnet7(&PID_InputFromSensors[7], &PID_OutputMagnetCommand[7], &Setpoint, Kp, Ki, Kd, DIRECT);

#endif