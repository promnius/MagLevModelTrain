 
#include <EloquentTinyML.h>
#include "TeensyModel.h"

#define NUMBER_OF_INPUTS 3
#define NUMBER_OF_OUTPUTS 1
// in future projects you may need to tweek this value: it's a trial and error process
#define TENSOR_ARENA_SIZE 256*1024

DMAMEM Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml_model;

unsigned long lngInferenceStart = 0;
unsigned long lngInferenceEnd = 0;
unsigned long lngModelTransferStart = 0;
unsigned long lngModelTransferEnd = 0;

float test_input[3] = {.05,.05,.05};
float predicted = 0;
float y_pred[2] = {0}; // if the model has output_dims>1, use an array like this
byte serialByte = 0;

void setup() {
  // delays mostly just to let printing finish before we crash, if part of the ML process causes us to hang. 
  // This lets us get more useful debugging info.
  delay(2000);
  Serial.begin(115200);
  Serial.println("Start");
  delay(50);
  Serial.print("Model 1 loaded");      
  delay(50);
  ml_model.begin((unsigned char*)modelParams);
  delay(50);
  Serial.println("All Models Loaded");
  delay(50);      
}

void loop() {
  Serial.println("About to predict");
  delay(50);
  // hardcoded print statements from previous tests, just a convinient way to be able to compare previous runs to the current one.
  Serial.println("Network Weather: size 19/16/16/1, 609 trainable parameters, execution time: 10us ");
  Serial.println("Network Cartpole: size 5/16/16/16/16/2, 946 trainable parameters, execution time: 17us. Uses XX RAM (bechmarked against Network Weather)");
  Serial.println("Network Cartpole: size 5/32/32/2, 1314 trainable parameters, execution time: 20us. Uses XX RAM (bechmarked against Network Weather)");
  Serial.println("Network Cartpole: size 5/64/64/64/64/2, 12,994 trainable parameters, execution time: 165us. Uses 69% RAM");
  Serial.println("Network Cartpole: size 5/128/128/2, 17,538 trainable parameters, execution time: 217us. Uses 71% RAM");
  Serial.println("Network Cartpole: size 3/128/128/64/64/32/1, 31,553 trainable parameters, execution time: 390us. Uses 81% RAM, with tensor arena moved to DMAMEM");
  Serial.println("Network Cartpole: size 5/256/256/2, 67,842 trainable parameters, execution time: CAN NOT COMPILE, not enough ram (only because program runs in memory and mem is fragmented)");

  lngInferenceStart = micros();
  predicted = ml_model.predict(test_input);
  //ml_cartpole.predict(test_cartpole,y_pred); // If there are multiple outputs, use this instead
  lngInferenceEnd = micros(); delay(50);
  Serial.print("execution time: "); Serial.print(lngInferenceEnd-lngInferenceStart); Serial.print("uS");
  Serial.print(".   Predicted Output: ");  
  //Serial.print(y_pred[0],7);Serial.print(",");Serial.println(y_pred[1],7); // for multiple outputs
  Serial.println(predicted,7);
  Serial.println("Finished Predictions!");
  Serial.println();

  checkForModelUpdate();
  delay(4000);
}

void checkForModelUpdate(){
  if (Serial.available() > 0) { // for the sake of this test, the only serial comms will be a new model dump.
    Serial.println("Serial received, prepping for data download");
    // the PC will have sent 1 synce byte, content doesn't matter. Serial.flush() has changed over the 
    // years and doesn't seem to do what I'm looking for anymore
    //Serial.flush(); 
    while(Serial.available() > 0){Serial.read();} // clear serial buffer
    Serial.println("ReadyForDownload"); // This is the actual string that signals to the PC we are ready for download, this triggers
    // the incoming data
    lngModelTransferStart = micros();
    for (long dataTransferCounter = 0; dataTransferCounter < modelParams_len; dataTransferCounter ++){
      while(Serial.available() == 0){} // delay and wait for transmission to arrive
      serialByte = byte(Serial.read()); // do we need to cast this to byte?
      modelParams[dataTransferCounter] = serialByte;
    }
    lngModelTransferEnd = micros();
    Serial.print(modelParams_len); Serial.print(" bytes of data recieved in "); Serial.print(lngInferenceEnd-lngInferenceStart); Serial.println("us");
  }
}
