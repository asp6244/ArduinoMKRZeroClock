#include <RTCZero.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#include <SD.h>
#include <AudioZero.h>

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
const byte seconds = 0;
const byte minutes = 42;
const byte hours = 2;

/* Change these values to set the current initial date */
const byte day = 11;
const byte month = 3;
const byte year = 20;

const String months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

const String files[] = {"7395", "2682209", "alaska", "alask-im", "amer-nev", "anch-lib",
                        "chi-fall", "comdeat1", "comdeat2", "comdetec", "comengag",
                        "comfailu", "comlie", "comsetba", "comtarga", "compos-1",
                        "compos-2", "compos-3", "compos-4", "compos-5", "death-1", "death-3",
                        "death-4", "death-5", "death-co", "death-no",
                        "dem-neve", "dem-nonn", "dest-co1", "dest-co2",
                        "embrc-de", "enggchin", "enggred", "eradictd", "freedom",
                        "futile", "imposs", "inad", "lastfall", "libonln1", "libonln2",
                        "libonln3", "libonln4", "notfearr", "ovrchrg1", "ovrchrg2",
                        "ovrchrg3", "primtrg1", "primtrg2", "primtrg3",
                        "primtrg4", "reddetct", "redimpos", "titan", "voicemd1",
                        "voicemd2", "voicemd3", "voicemd4", "warning1", "warning2",
                        "zeroper"};
const int numFiles = 61;

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

  pinMode(32,OUTPUT);
  digitalWrite(32,HIGH);

  // disable shutdown pin
  pinMode(5,OUTPUT);
  digitalWrite(5,LOW);

  // setup SD-card
  Serial.print("Initializing SD card");
  if (!SD.begin(28)) {
    Serial.println("Initialization failed!");
    return;
  }

  // 24000 sample rate
  AudioZero.begin(24000);

  randomSeed(analogRead(6));
}

void loop() {
  thisTime = rtc.getSeconds();
  if(thisTime > oldTime || (oldTime == 59 && thisTime == 0)) {
    updateTime();
    
    rotateMotor();
    
    oldTime = thisTime;
    Serial.println(thisTime);
  }
}

void rotateMotor() {
    for(int x = 0; x < numSteps; x++) {
    digitalWrite(stepPin,HIGH);
    digitalWrite(ledPin,HIGH);
    delayMicroseconds(timeSpeed); 
    digitalWrite(stepPin,LOW);
    digitalWrite(ledPin,LOW);
    delayMicroseconds(timeSpeed);
  }
}

void updateTime() {
  lcd.setCursor(1,0);
  lcd.print(months[rtc.getMonth()-1] + " " + String(rtc.getDay()) + ", 20" + 
            String(rtc.getYear()));
  
  int hours = rtc.getHours();
  String sunlight = "am";
  if(hours > 12) {
    hours -= 12;
    sunlight = "pm";
  } else if(hours == 0) {
    hours = 12;
  }
  
  int minutes = rtc.getMinutes();
  int seconds = rtc.getSeconds();
  if(minutes == 0 && seconds == 0) {
    updateLCD(hours, minutes, seconds, sunlight);
              
    int oldSecs = seconds;
    hourlyAlarm();
    seconds = rtc.getSeconds();

    while(oldSecs < seconds) {
      for(int i = 0; i < seconds-oldSecs; i++) {
        rotateMotor();
        updateLCD(hours, minutes, seconds, sunlight);
        Serial.println(oldSecs+i);
      }
      oldSecs = seconds;
      seconds = rtc.getSeconds();
    }
    delay(500);
  }

  updateLCD(hours, minutes, seconds, sunlight);
}

void updateLCD(int hours, int minutes, int seconds, String sunlight) {
    lcd.setCursor(1,1);
    lcd.print(formatTime(hours, true) + ":" + formatTime(minutes, false) + ":" + 
              formatTime(seconds, false) + " " + sunlight);
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

void hourlyAlarm() {
  String file = files[random(61)];
  File myFile = SD.open("sounds/" + file + ".wav");
  if (!myFile) {
    // if the file didn't open, print an error and stop
    Serial.println("error opening " + file);
  } else {
    Serial.println("Playing " + file);
  
    digitalWrite(5,HIGH);
    AudioZero.play(myFile);
    digitalWrite(5,LOW);
  }
}
