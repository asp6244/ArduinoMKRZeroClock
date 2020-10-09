/*     Simple Stepper Motor Control Exaple Code
 *      
 *  by Dejan Nedelkovski, www.HowToMechatronics.com
 *  
 */

#include <RTCZero.h>

/* Create an rtc object */
RTCZero rtc;
 
// defines pins numbers
const int stepPin = 8; 
const int dirPin = 9;
const int sleepPin = 7;
const int resetPin = 6;
const int ledPin = 32;

const int numSteps = 200; // full rotation
const int timeSpeed = 600; 
byte thisTime = 0;
byte oldTime = 0;

/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 0;
const byte hours = 16;

/* Change these values to set the current initial date */
const byte day = 15;
const byte month = 6;
const byte year = 15;
 
void setup() {
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(ledPin,OUTPUT);
  pinMode(sleepPin,OUTPUT);
  pinMode(resetPin,OUTPUT);
  digitalWrite(dirPin,HIGH);
  digitalWrite(sleepPin, HIGH);
  digitalWrite(resetPin, HIGH);
  
  Serial.begin(9600);
  rtc.begin();
  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);
}

void loop() {
  thisTime = rtc.getSeconds();
  if(thisTime > oldTime || (oldTime == 59 && thisTime == 0)) {
    for(int x = 0; x < numSteps; x++) {
      digitalWrite(stepPin,HIGH);
      digitalWrite(ledPin,HIGH);
      delayMicroseconds(timeSpeed); 
      digitalWrite(stepPin,LOW);
      digitalWrite(ledPin,LOW);
      delayMicroseconds(timeSpeed);
    }
    oldTime = thisTime;
    Serial.println(thisTime);
  }
}
