/********************************************************
 * PID Basic Example
 * Reading analog input 0 to control analog PWM output 3
 ********************************************************/

#include <PID_v1.h>

#define PIN_INPUT 0
#define PIN_OUTPUT 3

int delayTime = 6000; // ms

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
double Kp=1, Ki=1, Kd=1;
double myArray[9][2] = {{0,0},{1,1},{2,2},{3,4},{4,8},{5,16},{6,32},{7,64},{8,128}};
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT, myArray, 9);

void setup()
{
  Serial.begin(9600);
  //initialize the variables we're linked to
  Input = analogRead(PIN_INPUT);
  Setpoint = 25;

  //turn the PID on
  myPID.SetSampleTime(1000);
  myPID.SetMode(AUTOMATIC);

  delay(delayTime);

  //Serial.print("UPDATE MAIN LOOP: Math check: "); Serial.print(sizeof(myArray[0])/sizeof(myArray[0][0])); ///sizeof(km[0])))-1);
  //Serial.print(","); Serial.println(sizeof(myArray)/sizeof(myArray[0]));
}

void loop()
{
  //Input = analogRead(PIN_INPUT);
  Input = 0;
  delay(delayTime);
  Serial.print("Main Loop: Input: "); Serial.println(Input);
  myPID.Compute();
  Serial.print("Main Loop: Output: "); Serial.println(Output); Serial.println();

  Input = .5;
  delay(delayTime);
  Serial.print("Main Loop: Input: "); Serial.println(Input);
  myPID.Compute();
  Serial.print("Main Loop: Output: "); Serial.println(Output); Serial.println();

  Input = 3.7;
  delay(delayTime);
  Serial.print("Main Loop: Input: "); Serial.println(Input);
  myPID.Compute();
  Serial.print("Main Loop: Output: "); Serial.println(Output); Serial.println();

  Input = 7.5;
  delay(delayTime);
  Serial.print("Main Loop: Input: "); Serial.println(Input);
  myPID.Compute();
  Serial.print("Main Loop: Output: "); Serial.println(Output); Serial.println();

  Input = 8.1;
  delay(delayTime);
  Serial.print("Main Loop: Input: "); Serial.println(Input);
  myPID.Compute();
  Serial.print("Main Loop: Output: "); Serial.println(Output); Serial.println();
  
  //analogWrite(PIN_OUTPUT, Output);
}
