
#ifndef HelperFunctions_h
#define HelperFunctions_h

#include "Arduino.h"
#include "GlobalVariables.h"

int getAverageOfArray(long* myArray, int mySize){
  lngSum = 0;
  for (int i=1; i<mySize; i++){
    lngSum = lngSum + myArray[i];
  }
  return lngSum/mySize;
}

#endif
