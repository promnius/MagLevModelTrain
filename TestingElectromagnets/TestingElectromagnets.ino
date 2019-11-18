


const int pinMAGNETPWM0 = 3;
const int pinMAGNETPWM1 = 4;
const int pinMAGNETPWM2 = 20;
const int pinMAGNETPWM3 = 21;
const int pinMAGNETPWM4 = 22;
const int pinMAGNETPWM5 = 23;
const int pinMAGNETPWM6 = 32;
const int pinMAGNETPWM7 = 25;
const int pinMAGNETDIRECTION0 = 5;
const int pinMAGNETDIRECTION1 = 6;
const int pinMAGNETDIRECTION2 = 14;
const int pinMAGNETDIRECTION3 = 19;
const int pinMAGNETDIRECTION4 = A10;
const int pinMAGNETDIRECTION5 = 33;
const int pinMAGNETDIRECTION6 = 24;
const int pinMAGNETDIRECTION7 = 27;
const int pinMAGNETENABLE = 28;

int counter = 0;

void setup() {
  pinMode(pinMAGNETPWM0, OUTPUT);
  pinMode(pinMAGNETPWM1, OUTPUT);
  pinMode(pinMAGNETPWM2, OUTPUT);
  pinMode(pinMAGNETPWM3, OUTPUT);
  pinMode(pinMAGNETPWM4, OUTPUT);
  pinMode(pinMAGNETPWM5, OUTPUT);
  pinMode(pinMAGNETPWM6, OUTPUT);
  pinMode(pinMAGNETPWM7, OUTPUT);
  
  pinMode(pinMAGNETDIRECTION0, OUTPUT);
  pinMode(pinMAGNETDIRECTION1, OUTPUT);
  pinMode(pinMAGNETDIRECTION2, OUTPUT);
  pinMode(pinMAGNETDIRECTION3, OUTPUT);
  pinMode(pinMAGNETDIRECTION4, OUTPUT);
  pinMode(pinMAGNETDIRECTION5, OUTPUT);
  pinMode(pinMAGNETDIRECTION6, OUTPUT);
  pinMode(pinMAGNETDIRECTION7, OUTPUT);
  
  pinMode(pinMAGNETENABLE, OUTPUT);
  
  digitalWrite(pinMAGNETPWM0, HIGH);
  digitalWrite(pinMAGNETPWM1, HIGH);
  digitalWrite(pinMAGNETPWM2, HIGH);
  digitalWrite(pinMAGNETPWM3, HIGH);
  digitalWrite(pinMAGNETPWM4, HIGH);
  digitalWrite(pinMAGNETPWM5, HIGH);
  digitalWrite(pinMAGNETPWM6, HIGH);
  digitalWrite(pinMAGNETPWM7, HIGH);
  
  digitalWrite(pinMAGNETDIRECTION0, HIGH);
  digitalWrite(pinMAGNETDIRECTION1, LOW);
  digitalWrite(pinMAGNETDIRECTION2, HIGH);
  digitalWrite(pinMAGNETDIRECTION3, LOW);
  digitalWrite(pinMAGNETDIRECTION4, LOW);
  digitalWrite(pinMAGNETDIRECTION5, LOW);
  digitalWrite(pinMAGNETDIRECTION6, LOW);
  digitalWrite(pinMAGNETDIRECTION7, HIGH);

  digitalWrite(pinMAGNETENABLE, HIGH);

  analogWriteResolution(10);
  analogWriteFrequency(pinMAGNETPWM0, 46875); // some of these are redundant, since multiple pins are on the same timer
  // magic number based on keeping frequency above 20khz for audible reasons and the filter of the inductor is around 20khz,
  // plus that is the number that maps perfectly to 10 bit resolution at 96Mhz processor speed
  analogWriteFrequency(pinMAGNETPWM1, 46875);
  analogWriteFrequency(pinMAGNETPWM2, 46875);
  analogWriteFrequency(pinMAGNETPWM3, 46875);
  analogWriteFrequency(pinMAGNETPWM4, 46875);
  analogWriteFrequency(pinMAGNETPWM5, 46875);
  analogWriteFrequency(pinMAGNETPWM6, 46875);
  analogWriteFrequency(pinMAGNETPWM7, 46875);
}

void loop() {
  analogWrite(pinMAGNETPWM0, 600);
  analogWrite(pinMAGNETPWM1, 600);
  analogWrite(pinMAGNETPWM2, 600);
  // put your main code here, to run repeatedly:
  for (counter = 0; counter < 512; counter ++){
    //analogWrite(pinMAGNETPWM0, 1024 - counter);
    //analogWrite(pinMAGNETPWM7, 1024 - counter);
    Serial.print("Power: "); Serial.println(counter);
    delay(300);
  }

}
