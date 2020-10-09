#include <Wire.h>
#include <RTClib.h>
#include <U8g2lib.h>

#include <SPI.h>
#include <SD.h>
#include <AudioZero.h>

/* Create an rtc object */
RTC_DS3231 rtc;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C leftDisplay(U8G2_R0);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C rightDisplay(U8G2_R0);

#define CLOCK_ADDRESS 0x68

#define LEFT_SCREEN 0x3C
#define RIGHT_SCREEN 0x3D

File root;

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
float temp = 0;

int numChanges = 0;

const String months[] = {"January", "February", "March", "April", "May", "June",
                         "July", "August", "September", "October", "November", "December"};
const byte monthLens[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const String weeks[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

int numFiles = 0;

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
    setDoW(thisWeek);
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  Serial.println();

  leftDisplay.setI2CAddress(LEFT_SCREEN*2);
  rightDisplay.setI2CAddress(RIGHT_SCREEN*2);
  leftDisplay.begin();
  rightDisplay.begin();
  leftDisplay.clear();
  rightDisplay.clear();

  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);

  // disable shutdown pin
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);

  // 48000 sample rate
  AudioZero.begin(48000);

  // Get number of .wav files
  root = SD.open("/");
  Serial.println("Looking for .wav files... (File Name, Index, Size)");
  printDirectory(root, "", 0);
  root.rewindDirectory();
  Serial.println("done!");
  Serial.print("Number of files: ");
  Serial.println(numFiles);

  randomSeed(analogRead(6));
}

void printDirectory(File dir, String root, int numTabs) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    
    String entryName = entry.name();
    Serial.print(entryName);
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, (entryName + "/"), numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t");
      if (entryName.endsWith(".WAV")) {
        Serial.print((String)numFiles + "\t");
        numFiles++;
      } else {
        Serial.print("\t");
      }
      Serial.println(entry.size(), DEC);
    }
    
    entry.close();
  }
}

void loop() {
  DateTime now = rtc.now();
  thisTime = now.second();
  
  if (thisTime > oldTime || (oldTime == 59 && thisTime == 0)) {
    temp = rtc.getTemperature();
    
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
  byte DoW = getDoW();

  updateLeftOLED(year, month, day, DoW, true);

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
  if (minutes == 0 && seconds == 0) {
    updateRightOLED(hours, minutes, seconds, sunlight, ' ');

    if (rawHours == 0) {
      writeTime(year, month, day, rawHours, minutes, 1, DoW);
    }

    int oldSecs = seconds;
    hourlyAlarm();
    now = rtc.now();
    seconds = now.second();

    while (oldSecs < seconds) {
      for (int i = 0; i < seconds - oldSecs; i++) {
        rotateMotor();
        updateRightOLED(hours, minutes, seconds, sunlight, ' ');
        Serial.println(oldSecs + i);
      }
      oldSecs = seconds;
      now = rtc.now();
      seconds = now.second();
    }
    delay(500);
  }

  updateRightOLED(hours, minutes, seconds, sunlight, ' ');
}

void updateLeftOLED(int year, byte month, byte day, byte week, char hideType) {
  String yearDisp = formatTime(year, false);
  String monthDisp = months[month - 1];
  String dayDisp = String(day);
  String weekDisp = weeks[(week % 7)];
  
  String daySuffix = "th";
  if (day < 11 || day > 13) {
    switch (day % 10) {
      case 1:
        daySuffix = "st";
        break;
      case 2:
        daySuffix = "nd";
        break;
      case 3:
        daySuffix = "rd";
        break;
    }
  }
  
  switch (hideType) {
    case 'y':
      yearDisp = " ";
      break;
    case 'm':
      monthDisp = " ";
      break;
    case 'd':
      dayDisp = " ";
      daySuffix = " ";
      break;
    case 'w':
      weekDisp = " ";
      break;
  }

  char yearChar[5];
  yearDisp.toCharArray(yearChar, 5);
  char monthChar[10];
  monthDisp.toCharArray(monthChar, 10);
  char dayChar[3];
  dayDisp.toCharArray(dayChar, 3);
  char weekChar[10];
  weekDisp.toCharArray(weekChar, 10);
  char suffixChar[3];
  daySuffix.toCharArray(suffixChar, 3);

  int suffixXPos = 25;
  if (day >= 10) {
    suffixXPos += 16;
  }

  leftDisplay.firstPage();
  do {
    leftDisplay.setFont(u8g2_font_profont22_mf);
    leftDisplay.drawStr(80, 63, yearChar);
    leftDisplay.setFont(u8g2_font_profont29_tf);
    leftDisplay.drawStr(10, 63, dayChar);
    leftDisplay.setFont(u8g2_font_profont22_mf);
    leftDisplay.drawStr(suffixXPos, 63, suffixChar);
    leftDisplay.drawStr(0, 35, monthChar);
    leftDisplay.drawStr(0, 15, weekChar);
  } while (leftDisplay.nextPage());
}

void updateRightOLED(byte hours, byte minutes, byte seconds, String sunlight, char hideType) {
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

  char sunlightChar[3];
  sunlight.toCharArray(sunlightChar, 3);
  char timeChar[9];
  (hourDisp + ":" + minDisp + ":" + secDisp).toCharArray(timeChar, 9);
  char tempFDisplay[7];
  processTemp(true).toCharArray(tempFDisplay, 7);
  char tempCDisplay[7];
  processTemp(false).toCharArray(tempCDisplay, 7);

  char numChar[3];
  ((String)numChanges).toCharArray(numChar, 3);

  rightDisplay.firstPage();
  do {
    rightDisplay.setFont(u8g2_font_profont22_mf);
    rightDisplay.drawStr(100, 45, sunlightChar);
    rightDisplay.setFont(u8g2_font_profont29_tf);
    rightDisplay.drawStr(0, 30, timeChar);
    rightDisplay.setFont(u8g2_font_profont22_mf);
    rightDisplay.drawStr(0, 63, tempFDisplay);
    rightDisplay.setFont(u8g2_font_profont15_mf);
    rightDisplay.drawStr(83, 63, tempCDisplay);

    rightDisplay.drawStr(0, 10, numChar                             );
  } while ( rightDisplay.nextPage() );
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

String processTemp(bool fahrenheit) {
  float tempDisplay = temp;
  char unit = 'C';
  if (fahrenheit) {
    tempDisplay = temp*9.0/5.0 + 32.0; // comment this out for celcius
    unit = 'F';
  }
  tempDisplay = round(tempDisplay*10.0)/10.0;
  return (String(tempDisplay).substring(0,4) + "\xB0" + unit);
}

void hourlyAlarm() {
  Serial.println("Playing hourly alarm");
  int index = random(numFiles);
  Serial.println("Random index: " + (String)index);
  
  String fileName = randomFile(root, index);
  root.rewindDirectory();
  
  if (fileName.equals("")) {
    Serial.println("Error: no valid file found at index " + (String)index);
  } else {
    File soundFile = SD.open(fileName);
    if (!soundFile) {
      // if the file didn't open, print an error and stop
      Serial.println("error opening " + fileName);
    } else {
      Serial.println("Playing " + fileName);
      
      digitalWrite(5, HIGH);
      AudioZero.play(soundFile);
      digitalWrite(5, LOW);
      Serial.println("End of file.");
    }
    soundFile.close();
  }
}

String randomFile(File dir, int index) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    
    String entryName = entry.name();
    if (entry.isDirectory()) {
      String fileName = randomFile(entry, index);
      if (!fileName.equals("")) {
        return entryName + "/" + fileName;
      }
    } else {
      // files have sizes, directories do not
      if (entryName.endsWith(".WAV")) {
        if (index == 0) {
          Serial.println("Entry found: " + entryName);
          return entryName;
        }
        index--;
      }
    }
    entry.close();
  }
  return "";
}

void writeTime(int year, byte month, byte day, byte hours, byte minutes, byte seconds, byte DoW) {
  File dateTime = SD.open("dateTime.txt", O_WRITE | O_CREAT);
  if (dateTime) {
    Serial.print("Writing to dateTime.txt...");
    dateTime.write(year);
    dateTime.write(month);
    dateTime.write(day);
    dateTime.write(hours);
    dateTime.write(minutes);
    dateTime.write(seconds);
    dateTime.write(DoW);
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
  byte DoW = getDoW();

  Serial.println("Updating week");
  DoW = processReset('w', year, month, day, DoW);
  updateLeftOLED(year, month, day, DoW, ' ');
  Serial.println("Updating month");
  month = processReset('m', year, month, day, DoW);
  day = errorCheckDay;
  updateLeftOLED(year, month, day, DoW, ' ');
  Serial.println("Updating day");
  day = processReset('d', year, month, day, DoW);
  updateLeftOLED(year, month, day, DoW, ' ');
  Serial.println("Updating year");
  year = processReset('y', year, month, day, DoW);
  day = errorCheckDay;
  updateLeftOLED(year, month, day, DoW, ' ');
  Serial.println("Updating hours");
  hours = processReset('h', hours, minutes, seconds, DoW);
  processLCD(hours, minutes, seconds, ' ');
  Serial.println("Updating minutes");
  minutes = processReset('n', hours, minutes, seconds, DoW);
  processLCD(hours, minutes, seconds, ' ');
  Serial.println("Updating seconds");
  seconds = processReset('s', hours, minutes, seconds, DoW);
  processLCD(hours, minutes, seconds, ' ');

  rtc.adjust(DateTime(year, month, day, hours, minutes, seconds));
  setDoW(DoW);

  writeTime(year, month, day, hours, minutes, seconds, DoW);

  numChanges++;

  timeout = false;
}

int processReset(char type, int data1, byte data2, byte data3, byte DoW) {
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
    newTime = DoW;
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
          updateLeftOLED(data1, data2, data3, newTime, 'w');
        } else {
          updateLeftOLED(data1, data2, data3, newTime, ' ');
        }
        break;
      }
      case 'y': {
        if (newTime % 4 != 0 && data2 == 2 && data3 == 29) {
          data3 = 28;
          errorCheckDay = 28;
          Serial.print("Gotcha bitch: ");
          Serial.println(data3);
        }
        
        if (neutralTime % 20 > 10 || settingTime < 5) {
          updateLeftOLED(newTime, data2, data3, DoW, 'y');
        } else {
          updateLeftOLED(newTime, data2, data3, DoW, ' ');
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
          updateLeftOLED(data1, newTime, data3, DoW, 'm');
        } else {
          updateLeftOLED(data1, newTime, data3, DoW, ' ');
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
          updateLeftOLED(data1, data2, newTime, DoW, 'd');
        } else {
          updateLeftOLED(data1, data2, newTime, DoW, ' ');
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
  
  updateRightOLED(hours, minutes, seconds, sunlight, hideType);
}


/////////////// These functions were taken from the DS3231 Github ///////////////
/////////////// https://github.com/NorthernWidget/DS3231/blob/master/DS3231.cpp ///////////////

byte getDoW() {
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(0x03);
  Wire.endTransmission();

  Wire.requestFrom(CLOCK_ADDRESS, 1);
  return bcdToDec(Wire.read());
}

void setDoW(byte DoW) {
  // Sets the Day of Week
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(0x03);
  Wire.write(decToBcd(DoW));  
  Wire.endTransmission();
}

byte bcdToDec(byte val) {
// Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}

byte decToBcd(byte val) {
// Convert normal decimal numbers to binary coded decimal
  return ( (val/10*16) + (val%10) );
}
