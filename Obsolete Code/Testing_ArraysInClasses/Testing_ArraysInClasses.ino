
class Sonar {
 int pin;
public:
 Sonar(int pinIn) : pin(pinIn) {
 }
 int GetValue() { return 4; }
};
class Robot {
 Sonar* sonars;
 uint8_t numsonars;
 int* ints;
 int (*doubleArray)[2];
 int doubleArrayLength;
public:
 Robot(Sonar* sonarsIn, uint8_t numsonarsIn, int* integersIn, int (*doubleArrayIn)[2], int doubleArrayLengthIn){ 
  sonars = sonarsIn;
  numsonars = numsonarsIn;
  ints = integersIn;
  doubleArray = doubleArrayIn;
  doubleArrayLength = doubleArrayLengthIn;
 }
 void printInts() {
   for (uint8_t i=0; i<numsonars; i++) {
     int foo = ints[i];
     Serial.println(foo);
   }
 }
void printDoubleArray() {
  Serial.print("METHOD: Size of double array: "); Serial.println(sizeof(doubleArray)/sizeof(doubleArray[0]));
   for (uint8_t i=0; i< doubleArrayLength; i++) {
     //int *foo = doubleArray[i];
     //Serial.print("Array entry: "); Serial.print(foo[0]); Serial.print(","); Serial.println(foo[1]);
     Serial.print("Array entry: "); Serial.print(doubleArray[i][0]); Serial.print(","); Serial.println(doubleArray[i][1]);
     //Serial.println(foo);
   }
 }
 void method(){
   for (int i=0; i<numsonars; i++) {
     int foo = sonars[i].GetValue();
     Serial.println(foo);
   }
 }
};

Sonar mySonars[] = {3, 4, 2, 1};
int myInts[] = {3,11,2,1};
int myDoubleArray[][2] = {{1,7},{2,67},{3,11},{4,12},{6,12}};
Robot myRobot(mySonars, sizeof(mySonars)/sizeof(mySonars[0]), myInts, myDoubleArray, sizeof(myDoubleArray)/sizeof(myDoubleArray[0]));
void setup() {
  Serial.begin(9600);
  delay(2000);
  Serial.println("Begin:");
  myRobot.method();
  delay(1000);
  Serial.println("Integers:");
  myRobot.printInts();
  delay(1000);
  Serial.println("Double Array:");
  myRobot.printDoubleArray();
  Serial.print("Size of array: "); Serial.println(sizeof(myDoubleArray)/sizeof(myDoubleArray[0]));
}
void loop() {}
