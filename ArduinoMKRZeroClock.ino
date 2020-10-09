#include <RTCZero.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27,20,4);  

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
const byte seconds = 30;
const byte minutes = 21;
const byte hours = 19;

/* Change these values to set the current initial date */
const byte day = 26;
const byte month = 2;
const byte year = 20;

const String months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
 
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

  lcd.init();
  lcd.backlight();
}

void loop() {
  thisTime = rtc.getSeconds();
  if(thisTime > oldTime || (oldTime == 59 && thisTime == 0)) {
    updateTime();
    
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

void updateTime() {
    lcd.setCursor(1,0);
    lcd.print(months[rtc.getMonth()-1] + " " + String(rtc.getDay()) + ", 20" + 
              String(rtc.getYear()));
    lcd.setCursor(1,1);
    int hours = rtc.getHours();
    String sunlight = "am";
    if(hours > 12) {
      hours -= 12;
      sunlight = "pm";
    } else if(hours == 0) {
      hours = 12;
    }
    lcd.print(formatTime(hours, true) + ":" + formatTime(rtc.getMinutes(), false) + ":" + 
              formatTime(rtc.getSeconds(), false) + " " + sunlight);
}

String formatTime(int theTime, boolean hours) {
  if(theTime < 10) {
    if(hours) {
      return " " + String(theTime);
    } else {
      return "0" + String(theTime);
    }
  } else {
    return String(theTime);
  }
}
