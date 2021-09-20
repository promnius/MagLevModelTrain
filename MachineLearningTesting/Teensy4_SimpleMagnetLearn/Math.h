
#ifndef Math_h
#define Math_h

#include "Arduino.h"
#include "GlobalVariables.h"

void computeCentroidMath(){
  for (int i = 0; i < numCCDS; i ++){
    lngTotalizerMath[i] = 0;
    lngPositionTotalizerMath[i] = 0;
    for (int j = 20; j<intCCDResolution+20; j++){
      if (INTccdRaw[i][j]>intBackgroundThreshold){
        lngTotalizerMath[i] += INTccdRaw[i][j]-intBackgroundThreshold;
        lngPositionTotalizerMath[i] += (INTccdRaw[i][j]-intBackgroundThreshold)*j;
      }
    }
    if (lngTotalizerMath[i] == 0){
      fltCentroidMath[i] = 0;
    } else {
      fltCentroidMath[i] = (float)lngPositionTotalizerMath[i]/(float)lngTotalizerMath[i];
    }
  }
}

void computeCentroid(){
  for (int i = 0; i < numCCDS; i ++){
    fltCentroid[i] = (float)lngPositionTotalizer[i]/(float)lngTotalizer[i];
  }
}

// this could be faster, and may be a significant portion of wasted time. It's also
// non-deterministic as it stands. It also doesn't handle the case where the input
// is greater than the maximum value in the table. Distance will just be unassigned, so
// it will stay as whatever it was from before. Maybe the calibration table needs
// to be reconsidered, or the case for 'no signal detected' needs to be something other
// than an output of 0 - or maybe the table needs to cal from 1-100 so 0 is reserved.
// as it stands, the table is not even checked to know that it is continously increasing. 
void computeDistance(){
  for (int i = 0; i < numCCDS; i ++){
    if (fltCentroidMath[i] == 0){fltDistance[i] = -1;} // no response
    else if (fltCentroidMath[i] < FLTcalibrationTable[i][0]){fltDistance[i] = 0;} // response below scale, not necessarily an error, since the hope is minimum calibration IS at 0
    else if (fltCentroidMath[i] > FLTcalibrationTable[i][49]){fltDistance[i] = 50;}
    else{
      for (int j = 0; j < 49; j ++){
        if (fltCentroidMath[i] > FLTcalibrationTable[i][j] && fltCentroidMath[i] < FLTcalibrationTable[i][j+1]){
          fltDistance[i] = (fltCentroidMath[i]-FLTcalibrationTable[i][j])/(FLTcalibrationTable[i][j+1] - FLTcalibrationTable[i][j])+(float)(j);
        }
      }
    }
  }
}

#endif
