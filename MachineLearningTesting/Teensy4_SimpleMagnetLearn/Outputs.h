
#ifndef Outputs_h
#define Outputs_h

#include "Arduino.h"
#include "GlobalVariables.h"

// Use the built in arduino plotter to plot a line graph of the CCD readings, with an overlaid
// spike where the computed centroid is, mostly for debugging purposes. Note it is a hack to use
// the arduino plotter like this, since it is designed to be a rolling time plotter. Since it always
// plots the last 500 points, we MUST plot exactly 500 points every time we update it, to hide this
// rolling effect. Still easier than implementing my own plotter.
void plotCCDScan(){
  // set a baseline, since the window autoscales otherwise.
  for (int i = 0; i < (numCCDS*1)-1; i ++){
    Serial.print(0);Serial.print(",");
  } Serial.println(0);
  
  // compute where to place the centroid spikes
  for (int i = 0; i < numCCDS; i ++){
    intDisplayCentroid[i] = ((int)fltCentroid[i]*(1024/intCCDResolution)-34)/2;
    if (intDisplayCentroid[i] < 0){intDisplayCentroid[i] = 0;}
    if (intDisplayCentroid[i] > 498){intDisplayCentroid[i] = 498;}
    intDisplayCentroidMath[i] = ((int)(fltCentroidMath[i]-20)*(1024/intCCDResolution)-14)/2;
    if (intDisplayCentroidMath[i] < 0){intDisplayCentroidMath[i] = 0;}
    if (intDisplayCentroidMath[i] > 498){intDisplayCentroidMath[i] = 498;}
  }

  for (int j = 0; j < 498; j ++){
    for (int i = 0; i < numCCDS; i ++){
      if (intCCDResolution == 128){ // plot the middle 124 points, DUPLICATED 4 times. (this is a rare enough
        // use case, we don't want people thinking it has more resolution than it has).
        Serial.print(INTccdRaw[i][22+j/4]);}
      else if (intCCDResolution == 256){ // plot the middle 249 points, with averaging between them
        if (j%2==0){Serial.print(INTccdRaw[i][23+j/2]);}
        else{Serial.print((INTccdRaw[i][23+ j/2] + INTccdRaw[i][24+j/2])/2);}
      }else if (intCCDResolution == 512){ // plot the middle 498 points
        Serial.print(INTccdRaw[i][27+j]);
      }else if (intCCDResolution == 1024){ // plot the 2 pixel average of the middle 996 points.
        Serial.print((INTccdRaw[i][34+j*2] + INTccdRaw[i][35+j*2])/2);
      }
      //Serial.print(",");
      //if (intDisplayCentroid[i] == j){Serial.print(256);} else{Serial.print(0);}
      //Serial.print(",");
      //if (intDisplayCentroidMath[i] == j){Serial.print(256);} else{Serial.print(0);}
      if (i != numCCDS-1){Serial.print(",");}
    }
    Serial.println();
  }

  // set a maximum, since the window autoscales otherwise.
  for (int i = 0; i < (numCCDS*1)-1; i ++){
    Serial.print(256);Serial.print(",");
  } Serial.println(256);
}

void printCalibrationPoints(){
  Serial.println("CALIBRATION TABLE:");
  for (int i = 0; i < numCCDS; i ++){
    for (int j = 0; j < 50; j ++){
      Serial.print("CCD"); Serial.print(i); Serial.print(": ");
      Serial.print("position "); Serial.print(j);Serial.print(": maps to centroid (pixel number) ");Serial.println(FLTcalibrationTable[i][j]);
    }
  }
}

void plotCalibrationPoints(){
  for (int j = 0; j < 500; j ++){
    for (int i = 0; i < numCCDS; i ++){
      Serial.print(FLTcalibrationTable[i][j/10]);
      if (i != numCCDS-1){Serial.print(",");}
    } Serial.println();
  }
}

void plotRollingCentroids(){
  for (int i = 0; i < numCCDS; i ++){
    //Serial.print(fltCentroid[i]);
    Serial.print(fltCentroidMath[i]);
    if (i != numCCDS-1){Serial.print(",");}
  } Serial.println();
}

// X5 is a hack to put it in real numbers- but only applies if this was calibrated
// to a 500 thou scale . . . probably should not be left like this
void plotRollingDistance(){
  for (int i = 0; i < numCCDS; i ++){
    Serial.print(fltDistance[i]*8);
    if (i != numCCDS-1){Serial.print(",");}
  } Serial.println();
}

void plotRollingPowerHighSpeed(){
  Serial.println(intMagnetPower);
}

void plotRollingStateHighSpeed(){
  Serial.print(fltAveDistance);Serial.print(",");
  Serial.print(fltVelocity*10);Serial.print(",");
  Serial.print(fltAccel*25);Serial.print(",");
  Serial.print(fltTilt*5);
  Serial.println();
}

#endif
