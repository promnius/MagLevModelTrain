
// for maximum speed GPIO operation: compile in fastest, define void yield, disable all interrupts, use digitalwritefast (and static pin numbers), and set slew rates and drive strengths to fast.
// also, for very short for loops the compiler will type it out longhand anyways, but for longer loops, ie, toggle a pin 1000 times, it's actually better to type it
// out 1000 times, or you lose 4 clock cycles everytime the loop refreshes. Obviously, this cannot continue forever as there is limited memory. And it looks kind of funny.
// also, manually index into arrays, where possible.
// this gets single clock cycle output control, which for a teensy 3.2 running at 100Mhz (ok, 96Mhz), this is roughly 10nS per pin toggle.

#ifndef CCDRead_h
#define CCDRead_h

#include "Arduino.h"
#include "GlobalVariables.h"

// DEFINING THIS FUNCTION IS CRITICAL TO MAXIMUM SPEED OPERATION!!! IT DOESN'T EVEN HAVE TO GET CALLED ANYWHERE
// doesn't matter on teensy 4, already fast enough.
//void yield () {} //Get rid of the hidden function that checks for serial input and such. 

// >> means shift down, << means shift up
FASTRUN void readADCsFast(int i){
  data0 = GPIO6_DR;
  data1 = GPIO7_DR;
  // for first CCD, register 6_2 maps to bit 0 (shift down 2), 6_12 maps to bit 2 (shift down 10), and 6_16 maps to bit 4 (shift down 12)
  INTccdRaw[0][i] = ((data0 >> 2)  & 0b00000011) | ((data0 >> 10) & 0b00001100) | ((data0 >> 12) & 0b11110000);
  // for second CCD, register 6_22 maps to bit 0 (shift down 22), and 6_30 maps to bit 6 (shift down 24)
  INTccdRaw[1][i] = ((data0 >> 22) & 0b00111111) | ((data0 >> 24) & 0b11000000);
  // for third CCD, register 7_0 maps to bit 0 (no shift), 7_10 maps to bit 4 (shift down 6), and 7_16 maps to bit 7 (shift down 9)
  INTccdRaw[2][i] = ((data1)       & 0b00001111) | ((data1 >> 6)  & 0b01110000) | ((data1 >> 9)  & 0b10000000);  
}

FASTRUN void readADCsSlow(int i){
  //INTccdRaw[0][i] = 0
  INTccdRaw[0][i] = (byte)(digitalReadFast(pinADC_A0) << 0)|(byte)(digitalReadFast(pinADC_A1) << 1)|(byte)(digitalReadFast(pinADC_A2) << 2)|
    (byte)(digitalReadFast(pinADC_A3) << 3)|(byte)(digitalReadFast(pinADC_A4) << 4)|(byte)(digitalReadFast(pinADC_A5) << 5)|
    (byte)(digitalReadFast(pinADC_A6) << 6)|(byte)(digitalReadFast(pinADC_A7) << 7);
  INTccdRaw[1][i] = (byte)(digitalReadFast(pinADC_B0) << 0)|(byte)(digitalReadFast(pinADC_B1) << 1)|(byte)(digitalReadFast(pinADC_B2) << 2)|
    (byte)(digitalReadFast(pinADC_B3) << 3)|(byte)(digitalReadFast(pinADC_B4) << 4)|(byte)(digitalReadFast(pinADC_B5) << 5)|
    (byte)(digitalReadFast(pinADC_B6) << 6)|(byte)(digitalReadFast(pinADC_B7) << 7);
  INTccdRaw[2][i] = (byte)(digitalReadFast(pinADC_C0) << 0)|(byte)(digitalReadFast(pinADC_C1) << 1)|(byte)(digitalReadFast(pinADC_C2) << 2)|
    (byte)(digitalReadFast(pinADC_C3) << 3)|(byte)(digitalReadFast(pinADC_C4) << 4)|(byte)(digitalReadFast(pinADC_C5) << 5)|
    (byte)(digitalReadFast(pinADC_C6) << 6)|(byte)(digitalReadFast(pinADC_C7) << 7);
}

// FASTRUN loads this entire function into RAM, which may run faster if the cache is overflowing- FLASH access is only 24Mhz
// so there could be additional 3 cycle delays. Of course, it eats RAM. Also, the teensy only has 1 cycle access to the lower
// 50% of RAM, so if the sketch uses more than that, the bonus is reduced.
// NOTE: on Teensy 4.0 and above, all functions are run as FASTRUN unless explicitly declared as FLASHMEM. I guess thats what you
// can get away with when you have 1M of Ram :)
FASTRUN void scanCCDs(){
  //cli(); // not needed on teensy 4, already fast enough
  for (int i = 1; i < numCCDS; i++){
    lngTotalizer[i] = 0;
    lngPositionTotalizer[i] = 0;
  }

  digitalWriteFast(pinSTARTREAD, HIGH); // initial pulse to start the read
  __asm__ __volatile__ ("nop\n\t"); // initial delay for this first pulse
  
  // Pulse #0, CCD is 16 cycles behind for setup, and ADC is 3 cycles behind that
  digitalWriteFast(pinADCCLOCK,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
  digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value

  // for teensy 3.2, we can only read one sensor but its all on one port. See old code base for teensy 3.0, converting requires more than
  // just uncommenting this line!
  //INTccdRaw[0] = GPIOC_PDIR & 0xFF; // READ ADC old value (actually 3 cycles old, due to pipeline structure)

  // for teensy 4.0, 3 sensors across 2 ports, not so nicely arranged.
  readADCsFast(0);
  //readADCsSlow(0);
  
  lngTotalizer[0] += INTccdRaw[0][0]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  lngTotalizer[1] += INTccdRaw[1][0];
  lngTotalizer[2] += INTccdRaw[2][0];
  
  digitalWriteFast(pinADCCLOCK,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
  digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.
  digitalWriteFast(pinSTARTREAD,LOW); // end of initial pulse to start the read
  
  lngPositionTotalizer[0] += INTccdRaw[0][0]*0; // USE DEAD TIME while we don't have to interact with hardware to compute some math
  lngPositionTotalizer[1] += INTccdRaw[1][0]*0;
  lngPositionTotalizer[2] += INTccdRaw[2][0]*0;
  
  for (int i = 1; i < intCCDResolution+21; i++){
    digitalWriteFast(pinADCCLOCK,LOW); // TRIGGER ADC SAMPLE, old value still present on outputs
    digitalWriteFast(pinREADCLOCK,HIGH); // TRIGGER CCD SHIFT, analog value will begin heading to new value
    readADCsFast(i);
    //readADCsSlow(i);

    /*data0 = GPIO6_DR;
    data1 = GPIO7_DR;
    // for first CCD, register 6_2 maps to bit 0 (shift down 2), 6_12 maps to bit 2 (shift down 10), and 6_16 maps to bit 4 (shift down 12)
    INTccdRaw[0][i] = ((data0 >> 2)  & 0b00000011) | ((data0 >> 10) & 0b00001100) | ((data0 >> 12) & 0b11110000);
    // for second CCD, register 6_22 maps to bit 0 (shift down 22), and 6_30 maps to bit 6 (shift down 24)
    INTccdRaw[1][i] = ((data0 >> 22) & 0b00111111) | ((data0 >> 24) & 0b11000000);
    // for third CCD, register 7_0 maps to bit 0 (no shift), 7_10 maps to bit 4 (shift down 6), and 7_16 maps to bit 7 (shift down 9)
    INTccdRaw[2][i] = ((data1)       & 0b00001111) | ((data1 >> 6)  & 0b01110000) | ((data1 >> 9)  & 0b10000000); 
    */ 
    
    //INTccdRaw[i] = GPIOC_PDIR & 0xFF; // OLD Teensy 3.2 logic

    digitalWriteFast(pinADCCLOCK,HIGH); // TELL ADC TO CHANGE OUTPUTS, will begin moving the next sample to outputs. output is momentarily invalid.
    digitalWriteFast(pinREADCLOCK,LOW); // ADC CLOCK, no action happens, all events on rising edge.

    lngTotalizer[0] += INTccdRaw[0][i]; // USE DEAD TIME while we don't have to interact with hardware to compute some math
    lngTotalizer[1] += INTccdRaw[1][i];
    lngTotalizer[2] += INTccdRaw[2][i];
    
    lngPositionTotalizer[0] += INTccdRaw[0][i]*i; // USE DEAD TIME while we don't have to interact with hardware to compute some math
    lngPositionTotalizer[1] += INTccdRaw[1][i]*i;
    lngPositionTotalizer[2] += INTccdRaw[2][i]*i;

    __asm__ __volatile__ ("nop\n\t"); // additional delays to make waveform square
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");
    __asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");__asm__ __volatile__ ("nop\n\t");

    // we have lots more dead time here than above!

    // DEBUGGING: breaks timing obviously
    //Serial.print(i);Serial.print(":");
    //Serial.print(data0);Serial.print(",");Serial.print(data1);
    //Serial.print(",");
    //Serial.print(INTccdRaw[0][i]);
    //Serial.print(",");Serial.print(INTccdRaw[1][i]);
    //Serial.print(",");Serial.print(INTccdRaw[2][i]);
    //Serial.println();
  }

  //sei()
}


#endif
