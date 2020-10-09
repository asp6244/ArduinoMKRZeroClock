#include <RTCZero.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <SPI.h>
#include <SD.h>
#include <AudioZero.h>

// set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

/* Create an rtc object */
RTCZero rtc;

// defines pins numbers
const byte stepPin = 8;
const byte dirPin = 9;
const byte sleepPin = 7;
const byte resetPin = 6;
const byte ledPin = 32;

const byte setTimePin = 0;
const byte timeUpPin = 1;
const byte timeDownPin = 2;

const int numSteps = 200; // full rotation
const int timeSpeed = 600;
byte thisTime = 0;
byte oldTime = 0;

/* Change these values to set the current initial time and time*/
const byte year = 20;
const byte month = 3;
const byte day = 11;
const byte hours = 19;
const byte minutes = 9;
const byte seconds = 50;

const String months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
                        };

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
    while (true);
  }
  Serial.println("Done!");

  File dateTime = SD.open("dateTime.txt");
  if (!dateTime) {
    // if the file didn't open, print an error:
    Serial.println("error opening dateTime.txt");
    while (true);
  }

  Serial.println("dateTime.txt:");

  // read data from the file
  byte thisYear = dateTime.read();
  byte thisMonth = dateTime.read();
  byte thisDay = dateTime.read();
  byte thisHour = dateTime.read();
  byte thisMinute = dateTime.read();
  byte thisSecond = dateTime.read();

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

  // close the file:
  dateTime.close();

  long oldTime = year * 33177600 + month * 2764800 + day * 86400 + hours * 3600 + minutes * 60 + seconds;
  long thisTime = thisYear * 33177600 + thisMonth * 2764800 + thisDay * 86400 + thisHour * 3600 +
                  thisMinute * 60 + thisSecond;

  rtc.begin();
  if (thisTime > oldTime) {
    rtc.setTime(thisHour, thisMinute, thisSecond);
    rtc.setDate(thisDay, thisMonth, thisYear);
    Serial.println("Using SD time");
  } else {
    rtc.setTime(hours, minutes, seconds);
    rtc.setDate(day, month, year);
    Serial.println("Using code time");
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
  thisTime = rtc.getSeconds();
  if (thisTime > oldTime || (oldTime == 59 && thisTime == 0)) {
    updateTime();

    rotateMotor();

    oldTime = thisTime;
    Serial.println(thisTime);

    if (digitalRead(setTimePin)) {
      if (beingReset) {
        resetTime();
        beingReset = false;
      } else {
        beingReset = true;
      }
    } else {
      beingReset = false;
    }
  }
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
  byte year = rtc.getYear();
  byte month = rtc.getMonth();
  byte day = rtc.getDay();

  updateUpperLCD(year, month, day);

  byte hours = rtc.getHours();
  byte rawHours = hours;
  String sunlight = "am";
  if (hours > 12) {
    hours -= 12;
    sunlight = "pm";
  } else if (hours == 0) {
    hours = 12;
  }

  byte minutes = rtc.getMinutes();
  byte seconds = rtc.getSeconds();
  if (minutes == 0 && seconds == 0) {
    updateLowerLCD(hours, minutes, seconds, sunlight);

    writeTime(year, month, day, rawHours, minutes, 1);

    int oldSecs = seconds;
    hourlyAlarm();
    seconds = rtc.getSeconds();

    while (oldSecs < seconds) {
      for (int i = 0; i < seconds - oldSecs; i++) {
        rotateMotor();
        updateLowerLCD(hours, minutes, seconds, sunlight);
        Serial.println(oldSecs + i);
      }
      oldSecs = seconds;
      seconds = rtc.getSeconds();
    }
    delay(500);
  }

  updateLowerLCD(hours, minutes, seconds, sunlight);
}

void updateUpperLCD(byte year, byte month, byte day) {
  lcd.setCursor(1, 0);
  lcd.print(months[month - 1] + " " + String(day) + ", 20" + String(formatTime(year, false)) + "    ");
}

void updateLowerLCD(byte hours, byte minutes, byte seconds, String sunlight) {
  lcd.setCursor(1, 1);
  lcd.print(formatTime(hours, true) + ":" + formatTime(minutes, false) + ":" +
            formatTime(seconds, false) + " " + sunlight + "    ");
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

void writeTime(byte year, byte month, byte day, byte hours, byte minutes, byte seconds) {
  File dateTime = SD.open("dateTime.txt", O_WRITE | O_CREAT);
  if (dateTime) {
    Serial.print("Writing to dateTime.txt...");
    dateTime.write(year);
    dateTime.write(month);
    dateTime.write(day);
    dateTime.write(hours);
    dateTime.write(minutes);
    dateTime.write(seconds);
    // close the file:
    dateTime.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening dateTime.txt");
  }
}

void resetTime() {
  byte year = rtc.getYear();
  byte month = rtc.getMonth();
  byte day = rtc.getDay();
  byte hours = rtc.getHours();
  byte minutes = rtc.getMinutes();
  byte seconds = rtc.getSeconds();

  month = processReset('m', year, month, day);
  day = processReset('d', year, month, day);
  year = processReset('y', year, month, day);
  hours = processReset('h', hours, minutes, seconds);
  minutes = processReset('n', hours, minutes, seconds);
  seconds = processReset('s', hours, minutes, seconds);

  rtc.setYear(year);
  rtc.setMonth(month);
  rtc.setDay(day);
  rtc.setHours(hours);
  rtc.setMinutes(minutes);
  rtc.setSeconds(seconds);

  writeTime(year, month, day, hours, minutes, seconds);
}

byte processReset(char type, byte data1, byte data2, byte data3) {
  while (digitalRead(setTimePin)) {
    delay(50);
  }
  
  boolean setting = true;
  int upTime = 0;
  int downTime = 0;
  int newTime = 0;

  if (type == 'y' || type == 'h') {
    newTime = data1;
  } else if (type == 'm' || type == 'n') {
    newTime = data2;
  } else if (type == 'd' || type == 's') {
    newTime = data3;
  }

  while (setting) {
    if (digitalRead(timeUpPin)) {
      if (upTime % 5 == 0 && upTime <= 20) {
        newTime++;
      } else if (upTime % 3 == 0 && upTime > 20 && upTime <= 40) {
        newTime++;
      } else if (upTime % 2 == 0 && upTime > 40) {
        newTime++;
      }
      upTime++;
    } else if (digitalRead(timeDownPin)) {
      if (downTime % 5 == 0 && downTime <= 20) {
        newTime--;
      } else if (downTime % 3 == 0 && downTime > 20 && downTime <= 40) {
        newTime--;
      } else if (downTime % 2 == 0 && downTime > 40) {
        newTime--;
      }
      downTime++;
    } else if (digitalRead(setTimePin)) {
      setting = false;
    } else {
      upTime = 0;
      downTime = 0;
    }

    switch (type) {
      case 'y':
        newTime = processTime(newTime, 99, 0, 100);
        updateUpperLCD(newTime, data2, data3);
        break;
      case 'm':
        newTime = processTime(newTime, 12, 1, 12);
        updateUpperLCD(data1, newTime, data3);
        break;
      case 'd':
        if (data2 == 4 || data2 == 6 || data2 == 9 || data2 == 11) {
          newTime = processTime(newTime, 30, 1, 30);
        } else if (data2 == 2) {
          if (data1 % 4 == 0) {
            newTime = processTime(newTime, 29, 1, 29);
          } else {
            newTime = processTime(newTime, 28, 1, 28);
          }
        } else {
          newTime = processTime(newTime, 31, 1, 31);
        }
        updateUpperLCD(data1, data2, newTime);
        break;
      case 'h':
        newTime = processTime(newTime, 23, 0, 24);
        processLCD(newTime, data2, data3);
        break;
      case 'n':
        newTime = processTime(newTime, 59, 0, 60);
        processLCD(data1, newTime, data3);
        break;
      case 's':
        newTime = processTime(newTime, 59, 0, 60);
        processLCD(data1, data2, newTime);
        break;
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

void processLCD(byte hours, byte data2, byte data3) {
  String sunlight = "am";
  if (hours > 12) {
    hours -= 12;
    sunlight = "pm";
  } else if (hours == 0) {
    hours = 12;
  }
  updateLowerLCD(hours, data2, data3, sunlight);
}
