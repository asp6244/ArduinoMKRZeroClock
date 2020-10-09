#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>

#include <SPI.h>
#include <SD.h>
#include <AudioZero.h>

// set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

/* Create an rtc object */
RTC_DS3231 rtc;

// defines pins numbers
const byte stepPin = 8;
const byte dirPin = 9;
const byte sleepPin = 7;
const byte resetPin = 6;
const byte ledPin = 32;

const byte setTimePin = 0;
const byte timeUpPin = 1;
const byte timeDownPin = 2;

const int numSteps = 5; // full rotation
const int timeSpeed = 600;
byte thisTime = 0;
byte oldTime = 0;
byte oldDay = 0;

byte weekDay = 0;

const String months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
const byte monthLens[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const String weeks[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

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
                        "zeroper"
                       };
const int numFiles = 61;

boolean beingReset = false;
boolean timeout = false;
byte errorCheckDay = 0;

void setup() {
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(sleepPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  digitalWrite(dirPin, HIGH);
  digitalWrite(sleepPin, HIGH);
  digitalWrite(resetPin, HIGH);

  pinMode(setTimePin, INPUT);
  pinMode(timeUpPin, INPUT);
  pinMode(timeDownPin, INPUT);

  Serial.begin(9600);
  //while (!Serial); // wait for serial port to connect.

  // setup SD-card
  Serial.print("Initializing SD card...");
  if (!SD.begin(28)) {
    Serial.println("Initialization failed!");
    abort();
  }
  Serial.println("Done!");

  File dateTime = SD.open("dateTime.txt");
  if (!dateTime) {
    // if the file didn't open, print an error:
    Serial.println("error opening dateTime.txt");
    abort();
  }

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
  
    Serial.println("dateTime.txt:");
  
    // read data from the file
    int thisYear = dateTime.read();
    byte thisMonth = dateTime.read();
    byte thisDay = dateTime.read();
    byte thisHour = dateTime.read();
    byte thisMinute = dateTime.read();
    byte thisSecond = dateTime.read();
    byte thisWeek = dateTime.read();
  
    Serial.print("Year: ");
    Serial.println(thisYear);
    Serial.print("Month: ");
    Serial.println(thisMonth);
    Serial.print("Day: ");
    Serial.println(thisDay);
    Serial.print("Hour: ");
    Serial.println(thisHour);
    Serial.print("Minute: ");
    Serial.println(thisMinute);
    Serial.print("Second: ");
    Serial.println(thisSecond);
    Serial.print("Week: ");
    Serial.println(thisWeek);
  
    // close the file:
    dateTime.close();
    
    rtc.adjust(DateTime(thisYear, thisMonth, thisDay, thisHour, thisMinute, thisSecond));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  Serial.println();

  lcd.init();
  lcd.backlight();

  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);

  // disable shutdown pin
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);

  // 24000 sample rate
  AudioZero.begin(24000);

  randomSeed(analogRead(6));
}

void loop() {
  DateTime now = rtc.now();
  thisTime = now.second();
  
  if (thisTime > oldTime || (oldTime == 59 && thisTime == 0)) {
    updateTime();

    rotateMotor();

    oldTime = thisTime;
    Serial.println(thisTime);

    if (digitalRead(setTimePin)) {
      if (beingReset) {
        resetTime();
        beingReset = false;
        now = rtc.now();
        oldTime = now.second();
      } else {
        beingReset = true;
      }
    } else {
      beingReset = false;
    }
  }

  delay(50);
}

void rotateMotor() {
  for (int x = 0; x < numSteps; x++) {
    digitalWrite(stepPin, HIGH);
    digitalWrite(ledPin, HIGH);
    delayMicroseconds(timeSpeed);
    digitalWrite(stepPin, LOW);
    digitalWrite(ledPin, LOW);
    delayMicroseconds(timeSpeed);
  }
}

void updateTime() {
  DateTime now = rtc.now();
  
  int year = now.year();
  byte month = now.month();
  byte day = now.day();
  if (day != oldDay) {
    oldDay = day;
    weekDay++;
    if (weekDay > 6) {
      weekDay = 0;
    }
  }

  updateUpperLCD(year, month, day, weekDay, true);

  byte hours = now.hour();
  byte rawHours = hours;
  String sunlight = "am";
  if (hours > 12) {
    hours -= 12;
    sunlight = "pm";
  } else if (hours == 0) {
    hours = 12;
  } else if (hours == 12) {
    sunlight = "pm";
  }

  byte minutes = now.minute();
  byte seconds = now.second();
  if (rawHours == 0 && minutes == 0 && seconds == 0) {
    updateLowerLCD(hours, minutes, seconds, sunlight, ' ');

    writeTime(year, month, day, rawHours, minutes, 1);

    int oldSecs = seconds;
    hourlyAlarm();
    now = rtc.now();
    seconds = now.second();

    while (oldSecs < seconds) {
      for (int i = 0; i < seconds - oldSecs; i++) {
        rotateMotor();
        updateLowerLCD(hours, minutes, seconds, sunlight, ' ');
        Serial.println(oldSecs + i);
      }
      oldSecs = seconds;
      now = rtc.now();
      seconds = now.second();
    }
    delay(500);
  }

  updateLowerLCD(hours, minutes, seconds, sunlight, ' ');
}

void updateUpperLCD(int year, byte month, byte day, byte week, char hideType) {
  lcd.setCursor(0, 0);
  String yearDisp = formatTime(year, false);
  String monthDisp = months[month - 1];
  String dayDisp = String(day);
  String weekDisp = weeks[(week % 7)];
  switch(hideType) {
    case 'y':
      yearDisp = "    ";
      break;
    case 'm':
      monthDisp = "   ";
      break;
    case 'd':
      if(day < 10) {
        dayDisp = " ";
      } else {
        dayDisp = "  ";
      }
      break;
    case 'w':
      weekDisp = "   ";
      break;
  }
  
  lcd.print(weekDisp + ", " + monthDisp + " " + dayDisp + " " + yearDisp + " ");
}

void updateLowerLCD(byte hours, byte minutes, byte seconds, String sunlight, char hideType) {
  lcd.setCursor(1, 1);
  String hourDisp = formatTime(hours, true);
  String minDisp = formatTime(minutes, false);
  String secDisp = formatTime(seconds, false);
  switch(hideType) {
    case 'h':
      hourDisp = "  ";
      break;
    case 'n':
      minDisp = "  ";
      break;
    case 's':
      secDisp = "  ";
      break;
  }
  lcd.print(hourDisp + ":" + minDisp + ":" + secDisp + " " + sunlight + "    ");
}

String formatTime(int theTime, boolean hours) {
  if (theTime < 10) {
    if (hours) {
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
  File soundFile = SD.open("sounds/" + file + ".wav");
  if (!soundFile) {
    // if the file didn't open, print an error and stop
    Serial.println("error opening " + file);
  } else {
    Serial.println("Playing " + file);

    digitalWrite(5, HIGH);
    AudioZero.play(soundFile);
    digitalWrite(5, LOW);
  }
  soundFile.close();
}

void writeTime(int year, byte month, byte day, byte hours, byte minutes, byte seconds) {
  File dateTime = SD.open("dateTime.txt", O_WRITE | O_CREAT);
  if (dateTime) {
    Serial.print("Writing to dateTime.txt...");
    dateTime.write(year);
    dateTime.write(month);
    dateTime.write(day);
    dateTime.write(hours);
    dateTime.write(minutes);
    dateTime.write(seconds);
    dateTime.write(weekDay);
    // close the file:
    dateTime.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening dateTime.txt");
  }
}

void resetTime() {
  DateTime now = rtc.now();
  int year = now.year();
  byte month = now.month();
  byte day = now.day();
  byte hours = now.hour();
  byte minutes = now.minute();
  byte seconds = now.second();

  weekDay = processReset('w', year, month, day);
  updateUpperLCD(year, month, day, weekDay, ' ');
  month = processReset('m', year, month, day);
  day = errorCheckDay;
  updateUpperLCD(year, month, day, weekDay, ' ');
  day = processReset('d', year, month, day);
  updateUpperLCD(year, month, day, weekDay, ' ');
  year = processReset('y', year, month, day);
  day = errorCheckDay;
  updateUpperLCD(year, month, day, weekDay, ' ');
  hours = processReset('h', hours, minutes, seconds);
  processLCD(hours, minutes, seconds, ' ');
  minutes = processReset('n', hours, minutes, seconds);
  processLCD(hours, minutes, seconds, ' ');
  seconds = processReset('s', hours, minutes, seconds);
  processLCD(hours, minutes, seconds, ' ');

  rtc.adjust(DateTime(year, month, day, hours, minutes, seconds));

  writeTime(year, month, day, hours, minutes, seconds);

  timeout = false;
}

byte processReset(char type, byte data1, byte data2, byte data3) {
  while (digitalRead(setTimePin)) {
    delay(50);
  }
  
  boolean setting = true;
  int upTime = 0;
  int downTime = 0;
  int newTime = 0;
  int neutralTime = 0;
  errorCheckDay = data3;
  int settingTime = 0;

  if (type == 'y' || type == 'h') {
    newTime = data1;
  } else if (type == 'm' || type == 'n') {
    newTime = data2;
  } else if (type == 'd' || type == 's') {
    newTime = data3;
  } else if (type == 'w') {
    newTime = weekDay;
  }

  while (setting && !timeout) {
    if (digitalRead(timeUpPin)) {
      if (upTime % 5 == 0 && upTime <= 20) {
        newTime++;
      } else if (upTime % 3 == 0 && upTime > 20 && upTime <= 40) {
        newTime++;
      } else if (upTime % 2 == 0 && upTime > 40) {
        newTime++;
      }
      upTime++;
      neutralTime = 0;
    } else if (digitalRead(timeDownPin)) {
      if (downTime % 5 == 0 && downTime <= 20) {
        newTime--;
      } else if (downTime % 3 == 0 && downTime > 20 && downTime <= 40) {
        newTime--;
      } else if (downTime % 2 == 0 && downTime > 40) {
        newTime--;
      }
      downTime++;
      neutralTime = 0;
    } else if (digitalRead(setTimePin)) {
      setting = false;
    } else {
      upTime = 0;
      downTime = 0;
      neutralTime++;
      if(neutralTime >= 200) {
        setting = false;
        timeout = true;
      }
    }
    settingTime++;

    switch (type) {
      case 'w': {
        if (newTime < 0) {
          newTime = 6;
        }
        newTime = newTime % 7;

        if (neutralTime % 20 > 10 || settingTime < 5) {
          updateUpperLCD(data1, data2, data3, newTime, 'w');
        } else {
          updateUpperLCD(data1, data2, data3, newTime, ' ');
        }
        break;
      }
      case 'y': {
        //newTime = processTime(newTime, 99, 0, 100);

        if (newTime % 4 != 0 && data2 == 2 && data3 == 29) {
          data3 = 28;
          errorCheckDay = 28;
          Serial.print("Gotcha bitch: ");
          Serial.println(data3);
        }
        
        if (neutralTime % 20 > 10 || settingTime < 5) {
          updateUpperLCD(newTime, data2, data3, weekDay, 'y');
        } else {
          updateUpperLCD(newTime, data2, data3, weekDay, ' ');
        }
        break;
      }
      case 'm': {
        newTime = processTime(newTime, 12, 1, 12);

        byte monthLen = monthLens[newTime-1];
        if (data3 > monthLen) {
          if (newTime == 2 && data1%4 == 0) {
            data3 = 29;
            errorCheckDay = 29;
            Serial.println("broken");
          } else {
            data3 = monthLen;
            errorCheckDay = monthLen;
          }
        }
        
        if (neutralTime % 20 > 10 || settingTime < 5) {
          updateUpperLCD(data1, newTime, data3, weekDay, 'm');
        } else {
          updateUpperLCD(data1, newTime, data3, weekDay, ' ');
        }
        break;
      }
      case 'd': {
        if (data2 == 2) {
          if (data1 % 4 == 0) {
            newTime = processTime(newTime, 29, 1, 29);
          } else {
            newTime = processTime(newTime, 28, 1, 28);
          }
        } else {
          byte monthLen = monthLens[data2-1];
          newTime = processTime(newTime, monthLen, 1, monthLen);
        }
        
        if (neutralTime % 20 > 10 || settingTime < 5) {
          updateUpperLCD(data1, data2, newTime, weekDay, 'd');
        } else {
          updateUpperLCD(data1, data2, newTime, weekDay, ' ');
        }
        break;
      }
      case 'h': {
        newTime = processTime(newTime, 23, 0, 24);
        if (neutralTime % 20 > 10 || settingTime < 5) {
          processLCD(newTime, data2, data3, 'h');
        } else {
          processLCD(newTime, data2, data3, ' ');
        }
        break;
      }
      case 'n': {
        newTime = processTime(newTime, 59, 0, 60);
        if (neutralTime % 20 > 10 || settingTime < 5) {
          processLCD(data1, newTime, data3, 'n');
        } else {
          processLCD(data1, newTime, data3, ' ');
        }
        break;
      }
      case 's': {
        newTime = processTime(newTime, 59, 0, 60);
        if (neutralTime % 20 > 10 || settingTime < 5) {
          processLCD(data1, data2, newTime, 's');
        } else {
          processLCD(data1, data2, newTime, ' ');
        }
        break;
      }
    }
    delay(50);
  }
  return newTime;
}

byte processTime(int newTime, byte upLim, byte lowLim, byte change) {
  if (newTime > upLim) {
    newTime -= change;
  } else if (newTime < lowLim) {
    newTime += change;
  }
  return newTime;
}

void processLCD(byte hours, byte minutes, byte seconds, char hideType) {
  String sunlight = "am";
  if (hours > 12) {
    hours -= 12;
    sunlight = "pm";
  } else if (hours == 0) {
    hours = 12;
  } else if (hours == 12) {
    sunlight = "pm";
  }
  
  updateLowerLCD(hours, minutes, seconds, sunlight, hideType);
}
