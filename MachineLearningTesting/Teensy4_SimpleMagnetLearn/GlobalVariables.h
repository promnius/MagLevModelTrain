
#ifndef GlobalVariables_h
#define GlobalVariables_h

#include "Arduino.h"

// --------------- PIN DEFINITIONS --------------------
#define pinSHUTTER 29
#define pinSTARTREAD 30
#define pinFAKEDAC 4
#define pinREADCLOCK 28
#define pinHEARTBEAT 31
#define pinADCCLOCK 33
#define pinLASERCONTROL 5
#define pinCOILDRIVEA 2
#define pinCOILDRIVEB 3

#define pinADC_A0 1  // GPIO6_02
#define pinADC_A1 0  // GPIO6_03
#define pinADC_A2 24 // GPIO6_12
#define pinADC_A3 25 // GPIO6_13
#define pinADC_A4 19 // GPIO6_16
#define pinADC_A5 18 // GPIO6_17
#define pinADC_A6 14 // GPIO6_18
#define pinADC_A7 15 // GPIO6_19

#define pinADC_B0 17 // GPIO6_22
#define pinADC_B1 16 // GPIO6_23
#define pinADC_B2 22 // GPIO6_24
#define pinADC_B3 23 // GPIO6_25
#define pinADC_B4 20 // GPIO6_26
#define pinADC_B5 21 // GPIO6_27
#define pinADC_B6 26 // GPIO6_30
#define pinADC_B7 27 // GPIO6_31

#define pinADC_C0 10 // GPIO7_00
#define pinADC_C1 12 // GPIO7_01
#define pinADC_C2 11 // GPIO7_02
#define pinADC_C3 13 // GPIO7_03
#define pinADC_C4 6 // GPIO7_10
#define pinADC_C5 9 // GPIO7_11
#define pinADC_C6 32 // GPIO7_12
#define pinADC_C7 8 // GPIO7_16

// ------------------ PIN DRIVE OPTION BITS -------------------------
#define PORT_PCR_ISF                    ((uint32_t)0x01000000)          // Interrupt Status Flag
#define PORT_PCR_IRQC(n)                ((uint32_t)(((n) & 15) << 16))  // Interrupt Configuration
#define PORT_PCR_IRQC_MASK              ((uint32_t)0x000F0000)
#define PORT_PCR_LK                     ((uint32_t)0x00008000)          // Lock Register
#define PORT_PCR_MUX(n)                 ((uint32_t)(((n) & 7) << 8))    // Pin Mux Control
#define PORT_PCR_MUX_MASK               ((uint32_t)0x00000700)
#define PORT_PCR_DSE                    ((uint32_t)0x00000040)          // Drive Strength Enable, 0 is low strength, 1 is high strength
#define PORT_PCR_ODE                    ((uint32_t)0x00000020)          // Open Drain Enable
#define PORT_PCR_PFE                    ((uint32_t)0x00000010)          // Passive Filter Enable
#define PORT_PCR_SRE                    ((uint32_t)0x00000004)          // Slew Rate Enable, 0 is fast slew, 1 is slow slew
#define PORT_PCR_PE                     ((uint32_t)0x00000002)          // Pull Enable
#define PORT_PCR_PS                     ((uint32_t)0x00000001)          // Pull Select

// ------------------ VARIABLES ---------------------------
#define numCCDS 3
#define ccdBUFFERLENGTH 1050 // array length is longer than the longest possible resolution because of the dead pulses at the beginning. This allows
  // reconfiguring resolution on the fly
#define PLOTFREQUENCYDIVIDER 100 // defines how often the plots are updated- number of loop iterations before update, so this is a divider based on loop speed.

#define maxEpisodeDuration 3000

unsigned long lngLoopTimer = 0;
unsigned long lngFunctionTimerStart = 0;
unsigned long lngFunctionTimerEnd = 0;
unsigned long lngModelTransferStart = 0;
unsigned long lngModelTransferEnd = 0;
byte serialByte = 0;
long episodeCounter = 0;
long intMagnetPower = 0; // a PWM value, ranging from -4096 to 4096. actual current depends on temp, etc. but typcially 50% in either direction is a safe maximum
int intHeartbeatState = 0;
uint32_t data0 = 0; // holder variables for port reads for fast CCD IO.
uint32_t data1 = 0;
int intSerialByte = 0; // incoming serial byte for processing
int INTccdRaw[numCCDS][ccdBUFFERLENGTH]; // raw values from the CCD ADC. 
float fltCentroid[numCCDS]; // center of the blip, in pixels (or fractional pixels), using values calculated on the fly
float fltCentroidMath[numCCDS]; // center of the blip, in pixels or fractional pixels, using values calculated after the fact (includes background subtraction)
int intDisplayCentroid[numCCDS]; // the index to display the center bump at, used for plotting
int intDisplayCentroidMath[numCCDS]; // same as above, for the math centroid
long lngTotalizerMath[numCCDS]; // sum of all pixel values
long lngPositionTotalizerMath[numCCDS]; // sum of all pixel values multiplied by their position (x coordinate)
long lngTotalizer[numCCDS]; // sum of all pixel values calculated on the fly
long lngPositionTotalizer[numCCDS]; // sum of all pixel values multiplied by their position, calculated on the fly
//int intBackgroundThreshold = 22; // value in raw adc ticks to be ignored as 'background noise. Note this is a simpler method than actually taking 
// an empty frame and doing frame subtraction
int intBackgroundThreshold = 63;
int lngLoopCounter = 0;
int const intHistoryLength = 100; // how many historical values are stored? used for averaging/ digital signal processing on-board.
int intHistoryCounter = 0; // index pointer- where in the circular history array are we now?
float FLTcentroids[numCCDS][intHistoryLength]; // history of centroid values
float fltDistance[numCCDS]; // calibrated distance to target, in 'full range scale,' ie, 0 maps to the smallest value seen during calibration, maximum 
// maps to the maximum value seen during calibration. The actual units/ range of the device is based on the distances used during calibration,
// since the device doesn't actually know the real distances, only the calibration steps.
float FLTcalibrationTable[numCCDS][50];
float fltSumCentroid[numCCDS];
float fltMinCentroid[numCCDS];
float fltMaxCentroid[numCCDS];
float FLTdistances[numCCDS][intHistoryLength]; // history of distances. Redundant, as these could be calculated from the centroids hisory
int intCCDResolution = 256; // number of active pixels on the array. Acceptable values: 128,256,512,1024. Other numbers may not work.
int intEEAddress = 0; // location to read or write from eeprom.
int intMode = 6; // Modes are as follows:
// 0 analog output of distance only. This is also included in the further options. 
// 1 raw CCD on serial plotter for debugging
// 2 scrolling timeseries of centroid on serial monitor for assessing stability. Note currently only the math variant is used.
// 3 scrolling timeseries of distance on serial monitor for assessing stability and accuracy.
// 4 plot a single character every 1 million sensor reads, for timing how fast we are going, since timing functions are broken with our abuse of interrupts
// 5 RUN TRAINING ALGORITHEM: dump every state sample to the PC.
// 6 plot power
// 7 plot state variables
// 8 plot calibration values
// 9 calibration OBSOLETE: calibration no longer has a special mode, there are just serial commands reserved for
  // setting each cal point.
// more numbers: not implemented, but can include digital outputs of distance, or response to queries

int intLastStateValid = 0;
float fltTilt = 0;
float fltMaxDistance = 0;
float fltMinDistance = 0;
float fltAveDistance = 0;
float fltLastDistance = 0;
float fltLastVelocity = 0;
float fltAccel = 0;
float fltVelocity = 0;

float fltSetpoint = 40;

float FLTPositionLog[maxEpisodeDuration];
float FLTVelocityLog[maxEpisodeDuration];
float FLTAccelLog[maxEpisodeDuration];
float FLTPositionErrorLog[maxEpisodeDuration];
float FLTCurrentLog[maxEpisodeDuration];
int INTActionLog[maxEpisodeDuration];
float FLTTargetPositionLog[maxEpisodeDuration];
float fltMagnetCurrent = 0;

#endif
