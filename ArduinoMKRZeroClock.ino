/////////////////////////////////////////////////////
//
// Alec Paul
// Program: Runs the Digital and Analog Cuckoo Clock
// Version: 6.1.0
// Date of last Revision: 11 January, 2020
// MIT License 2020
//
/////////////////////////////////////////////////////

#include <Wire.h>
#include <RTClib.h>
#include <U8g2lib.h>

#include <SPI.h>
#include <SD.h>
#include <AudioZero.h>

// for led driver
#define DATA1 3
#define DATA2 4
#define DATA3 5
#define CLOCK 13
#define CONTROL 14
#define ONBOARD 32
#define POWER 1 //TODO analog

/* Create an rtc object */
RTC_DS3231 rtc;

// initialize display objects for U8g2 library
U8G2_SSD1306_128X64_NONAME_F_HW_I2C leftDisplay(U8G2_R0);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C rightDisplay(U8G2_R0);

// I2C addresses
#define CLOCK_ADDRESS 0x68
#define LEFT_SCREEN 0x3C
#define RIGHT_SCREEN 0x3D

// timeout time for setting the time of clock
#define timeoutTime 200

File root;

// defines pins numbers
// clock motor pins
const byte stepPin = 8;
const byte dirPin = 9;
const byte sleepPin = 7;
const byte resetPin = 6;

// onboard led
const byte ledPin = 32;

// button pins
const byte setTimePin = 0;
const byte timeUpPin = 1;
const byte timeDownPin = 2;

// audio shutdown pin
#define shutdownPin 10

// OLED control pin
#define displayOffPin 1 // analog

// values for motor speed and new time comparisons
const int numSteps = 5;    // 1 second worth of motor rotation
const int timeSpeed = 600; // how slow the motor spins; time in ms
byte thisTime = 0;
byte oldTime = 0;
float temp = 0;

// debug value for number of times the clock time has been reset
// TODO: remove
int numChanges = 0;

// consts for strings and number of days in each month
const String months[] = {"January", "February", "March", "April", "May", "June",
                         "July", "August", "September", "October", "November", "December"};
const byte monthLens[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const String weeks[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// a value for the number of found sound files
int numFiles = 0;

// variables that need to be used over several functions.
boolean beingReset = false;
boolean timeout = false;
byte errorCheckDay = 0;

/* 
 * Run setup function
 */
void setup() {
  // set values of pins for clock motor
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(sleepPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  digitalWrite(dirPin, HIGH);
  digitalWrite(sleepPin, HIGH);
  digitalWrite(resetPin, HIGH);

  // set values of pins for buttons
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
    // following line sets the RTC to the date & time last recorded to the SD card
  
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

    // update rtc with the new time
    rtc.adjust(DateTime(thisYear, thisMonth, thisDay, thisHour, thisMinute, thisSecond));
    setDoW(thisWeek);
  }

  Serial.println();

  // initialize displays
  leftDisplay.setI2CAddress(LEFT_SCREEN*2);
  rightDisplay.setI2CAddress(RIGHT_SCREEN*2);
  leftDisplay.begin();
  rightDisplay.begin();
  leftDisplay.clear();
  rightDisplay.clear();

  // turn on onboard LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  // disable audio shutdown pin
  pinMode(shutdownPin, OUTPUT);
  digitalWrite(shutdownPin, LOW);

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

  // initialize the random object with a seed from analog pin 6
  randomSeed(analogRead(6));

  // initialize led driver pins
  pinMode(DATA1,OUTPUT);
  pinMode(DATA2,OUTPUT);
  pinMode(DATA3,OUTPUT);
  pinMode(CLOCK,OUTPUT);
  pinMode(CONTROL,OUTPUT);
  digitalWrite(DATA1,LOW);
  digitalWrite(DATA2,LOW);
  digitalWrite(DATA3,LOW);
  digitalWrite(CLOCK,LOW);
  digitalWrite(CONTROL,LOW);
}

/*
 * Print the directory of the SD card, with all folders, filenames, and 
 * size of all files. Set the number of .wav files to the numFiles value.
 * parameters:
 *    dir: the directory to print
 *    root: the directory higher than this object // TODO remove
 *    numTabs: the indentation for the current object in the directory
 */
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
      // if the current target object is a directory, recurse into it
      Serial.println("/");
      printDirectory(entry, (entryName + "/"), numTabs + 1);
    } else {
      // if the target object is a file, check for valid .wav file
      Serial.print("\t");
      if (entryName.endsWith(".WAV")) {
        Serial.print((String)numFiles + "\t");
        numFiles++;
      } else {
        Serial.print("\t");
      }
      // files have sizes, directories do not
      Serial.println(entry.size(), DEC);
    }
    
    entry.close();
  }
}

/* 
 * Run main loop
 */
void loop() {
  // get new time
  DateTime now = rtc.now();
  thisTime = now.second();

  // check to see if this new time is different from the old one
  if (thisTime > oldTime || (oldTime == 59 && thisTime == 0)) {
    temp = rtc.getTemperature();

    // update the OLED displays
    updateTime();

    // rotate the motor
    rotateMotor();

    // update the LEDs
    updateLEDs(now);

    oldTime = thisTime;
    Serial.println(thisTime);

    // check for a request to reset the time
    if (digitalRead(setTimePin)) {
      // takes two seconds for the reset to be activated, thus the bool
      if (beingReset) {
        resetTime();
        beingReset = false;
        now = rtc.now();
        oldTime = now.second();
      } else {
        // this will execute when the button is held for >1 second but <2 seconds
        beingReset = true;
      }
    } else {
      beingReset = false;
    }
  }

  // delay to slow down polling
  delay(50);
}

/*
 * Rotate the motor equivalent to the angle of 1 second
 */
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

/*
 * Update the time on the OLED displays
 */
void updateTime() {
  // get new time
  DateTime now = rtc.now();
  
  int year = now.year();
  byte month = now.month();
  byte day = now.day();
  byte DoW = getDoW();

  // update the left OLED for year, month, day, DoW
  updateLeftOLED(year, month, day, DoW, true);

  // update time for am or pm
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

  // update hours, minutes, seconds
  byte minutes = now.minute();
  byte seconds = now.second();
  // check for cuckoo time
  if (minutes == 0 && seconds == 0) {
    updateRightOLED(hours, minutes, seconds, sunlight, ' ');

    // write to SD on every strike of midnight
    if (rawHours == 0) {
      writeTime(year, month, day, rawHours, minutes, 1, DoW);
    }

    int oldSecs = seconds; // save this time
    
    // run the cuckoo function
    hourlyAlarm();

    // update the time
    now = rtc.now();
    seconds = now.second();

    // rotate the motor to compensate for the time it 
    // took for the cuckoo to execute
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

  // update the right OLED for hours, minutes, and seconds
  updateRightOLED(hours, minutes, seconds, sunlight, ' ');
}

/*
 * Update the Left OLED with the new time
 * parameters:
 *    year: the new year
 *    month: the new month
 *    day: the new day of the month
 *    week: the new day of the week
 *    hideType: the character that will be displayed as whitespace
 *      when the time is being reset:
 *        'y': year is being updated
 *        'm': month is being updated
 *        'd': day is being updated
 *        ' ': no value is being updated
 */
void updateLeftOLED(int year, byte month, byte day, byte week, char hideType) {
  // format the values for the display as Strings
  String yearDisp = formatTime(year, false);
  String monthDisp = months[month - 1];
  String dayDisp = String(day);
  String weekDisp = weeks[(week % 7)];

  // add a suffix to the day
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

  // hide the correct value if one was specified
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

  // convert all Strings to char arrays
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

  // get correct position of suffix on display
  // based on value of display
  int suffixXPos = 25;
  if (day >= 10) {
    suffixXPos += 16;
  }

  // if the dislay switch is in the off position
  if(analogRead(displayOffPin)<1023*25/33) {
    // display contents to OLED using U8g2 library
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
  } else {
    leftDisplay.clear();
  }
}

/*
 * Update the right OLED with the new time
 * parameters:
 *    hours: the new hours
 *    minutes: the new minutes
 *    seconds: the new day of the seconds
 *    sunlight: "am" or "pm"
 *    hideType: the character that will be displayed as whitespace
 *      when the time is being reset:
 *        'h': hours is being updated
 *        'n': minutes is being updated
 *        's': seconds is being updated
 *        ' ': no value is being updated
 */
void updateRightOLED(byte hours, byte minutes, byte seconds, String sunlight, char hideType) {
  // format the values for the display as Strings
  String hourDisp = formatTime(hours, true);
  String minDisp = formatTime(minutes, false);
  String secDisp = formatTime(seconds, false);

  // hide the correct value if one was specified
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

  // convert all Strings to char arrays
  char sunlightChar[3];
  sunlight.toCharArray(sunlightChar, 3);
  char timeChar[9];
  (hourDisp + ":" + minDisp + ":" + secDisp).toCharArray(timeChar, 9);
  
  // format the temperature in C and F and produce a char array
  char tempFDisplay[7];
  processTemp(true).toCharArray(tempFDisplay, 7);
  char tempCDisplay[7];
  processTemp(false).toCharArray(tempCDisplay, 7);

  // debug value
  char numChar[3];
  ((String)numChanges).toCharArray(numChar, 3);

  // if the dislay switch is in the off position
  if(analogRead(displayOffPin)<1023*25/33) {
    // display contents to OLED using U8g2 library
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
  
      // debug value
      rightDisplay.drawStr(0, 10, numChar);
    } while ( rightDisplay.nextPage() );
  } else {
    rightDisplay.clear();
  }
}

/*
 * A helper function for formatting integer times to
 * Strings.
 * parameters:
 *    theTime: the time to format
 *    hours:   a boolean of whether this value is the hours value
 * returns: the value as a String
 */
String formatTime(int theTime, boolean hours) {
  if (theTime < 10) {
    // the hours value does not have a leading 0
    if (hours) {
      return " " + String(theTime);
    } else {
      return "0" + String(theTime);
    }
  } else {
    return String(theTime);
  }
}

/*
 * A function to format the temperature value into a String, 
 * converting them if necessary.
 * parameters:
 *    fahrenheit: whether this value is for fahrenheit or not
 * returns: the temp as a String
 */
String processTemp(bool fahrenheit) {
  float tempDisplay = temp;
  char unit = 'C';
  // convert if necessary
  if (fahrenheit) {
    tempDisplay = temp*9.0/5.0 + 32.0;
    unit = 'F';
  }
  // format value to String
  tempDisplay = round(tempDisplay*10.0)/10.0; // produces one decimal place
  return (String(tempDisplay).substring(0,4) + "\xB0" + unit);
}

/*
 * Update the LED drivers to display the time in seconds
 */
void updateLEDs(DateTime now) {
  // create empty array for all leds
  bool ledsSec[60], ledsMin[60], ledsHour[60];
  for(int i=0; i<60; i++) {
    ledsSec[i] = 0;
    ledsMin[i] = 0;
    ledsHour[i] = 0;
  }

  // fill arrays with time data
  ledsSec[now.second()] = 1;
  ledsMin[now.minute()] = 1;
  ledsHour[(now.hour()%12)*5 + now.minute()/12] = 1;

  // update LEDs with data
  displayAllLEDs(ledsSec, ledsMin, ledsHour);
}

/*
 * Update the LEDs with the array containing the 
 *  configuration of LEDs
 */
void displayAllLEDs(bool ledsSec[60], bool ledsMin[60], bool ledsHour[60]) {
  //
  // update left leds
  //
  digitalWrite(CONTROL,HIGH);
  Serial.print("Left  ");
  
  // set all previous bits to 0
  for(int i=0; i<35 ; i++) {
    displayLED(0, 0, 0);
    Serial.print(0);
  }
  
  // send the start signal
  displayLED(1, 1, 1); 
  Serial.print(" 1 ");

  // display all elements of the array one by one
  for(int i=0; i<35; i++) {
    displayLED(ledsSec[i], ledsMin[i], ledsHour[i]);
    Serial.print(ledsSec[i]);
  }
  Serial.println();

  //
  // update right leds
  //
  digitalWrite(CONTROL,LOW);
  Serial.print("Right ");
  
  // set all previous bits to 0
  for(int i=35; i<70 ; i++) {
    displayLED(0, 0, 0);
    Serial.print(0);
  }
  
  // send the start signal
  displayLED(1, 1, 1); 
  Serial.print(" 1 ");

  // display all elements of the array one by one 
  for(int i=35; i<60 ; i++) {
    displayLED(ledsSec[i], ledsMin[i], ledsHour[i]);
    Serial.print(ledsSec[i]);
  }
  // set the unused LEDs to 0
  for(int i=60; i<70 ; i++) {
    displayLED(0, 0, 0);
    Serial.print(0);
  }
  Serial.println();
}

/**
 * Update the next led in the shift registers with 
 *  a specified value.
 */
void displayLED(bool ledSec, bool ledMin, bool ledHour) {
  digitalWrite(DATA1,ledSec);
  digitalWrite(DATA2,ledMin);
  digitalWrite(DATA3,ledHour);
  digitalWrite(CLOCK,HIGH);
  digitalWrite(CLOCK,LOW);
}

/*
 * The cuckoo function for playing a random sound file from the speaker.
 */
void hourlyAlarm() {
  Serial.println("Playing hourly alarm");
  // get a random index within the range of the number of files
  int index = random(numFiles);
  Serial.println("Random index: " + (String)index);

  // get the file at the specified index
  String fileName = randomFile(root, index);
  root.rewindDirectory();

  // check for valid file
  if (fileName.equals("")) {
    Serial.println("Error: no valid file found at index " + (String)index);
  } else {
    File soundFile = SD.open(fileName);
    if (!soundFile) {
      // if the file didn't open, print an error and exit
      Serial.println("error opening " + fileName);
    } else {
      Serial.println("Playing " + fileName);

      // play the file
      digitalWrite(shutdownPin, HIGH);
      AudioZero.play(soundFile);
      digitalWrite(shutdownPin, LOW);
      Serial.println("End of file.");
    }
    soundFile.close();
  }
}

/*
 * Helper function for determining the directory stack
 * where the file is located.
 * This function calls itself recursively for each level of
 * depth in the directory.
 * parameters: 
 *    dir:   the current directory
 *    index: the index of the target file
 * returns: the String representing the file location
 */
String randomFile(File dir, int index) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    
    String entryName = entry.name();
    if (entry.isDirectory()) {
      // if the current target object is a directory, recurse into it
      String fileName = randomFile(entry, index);
      if (!fileName.equals("")) {
        return entryName + "/" + fileName;
      }
    } else {
      // if the target object is a file, check for target .wav file
      if (entryName.endsWith(".WAV")) {
        if (index == 0) {
          Serial.println("Entry found: " + entryName);
          return entryName;
        }
        // if not, decriment index
        index--;
      }
    }
    entry.close();
  }
  // return "" if no file was found
  return "";
}

/*
 * Write the time to dateTime.txt on the SD card for access 
 * if the clock loses power.
 * parameters:
 *    year:    the new year value
 *    month:   the new month value
 *    day:     the new day of munth value
 *    hours:   the new hours value
 *    minutes: the new minutes value
 *    seconds: the new seconds value
 *    DoW:     the new day of week value
 */
void writeTime(int year, byte month, byte day, byte hours, byte minutes, byte seconds, byte DoW) {
  // open the file to overwrite it 
  // or create it if it does not already exist
  File dateTime = SD.open("dateTime.txt", O_WRITE | O_CREAT);
  if (dateTime) { // if file opened properly
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

/*
 * Processes the changes the user makes when they want to 
 * reset the time.
 */
void resetTime() {
  // get the updated time
  DateTime now = rtc.now();
  int year = now.year();
  byte month = now.month();
  byte day = now.day();
  byte hours = now.hour();
  byte minutes = now.minute();
  byte seconds = now.second();
  byte DoW = getDoW();

  // get the new value for week
  Serial.println("Updating week");
  DoW = processReset('w', year, month, day, DoW); 
  updateLeftOLED(year, month, day, DoW, ' ');

  // get the new value for month
  Serial.println("Updating month");
  month = processReset('m', year, month, day, DoW);
  day = errorCheckDay;
  updateLeftOLED(year, month, day, DoW, ' ');
  
  // get the new value for day of month
  Serial.println("Updating day of month");
  day = processReset('d', year, month, day, DoW);
  updateLeftOLED(year, month, day, DoW, ' ');
  
  // get the new value for year
  Serial.println("Updating year");
  year = processReset('y', year, month, day, DoW);
  day = errorCheckDay;
  updateLeftOLED(year, month, day, DoW, ' ');

  // get the new value for hours
  Serial.println("Updating hours");
  hours = processReset('h', hours, minutes, seconds, DoW);
  processOLED(hours, minutes, seconds, ' ');

  // get the new value for minutes
  Serial.println("Updating minutes");
  minutes = processReset('n', hours, minutes, seconds, DoW);
  processOLED(hours, minutes, seconds, ' ');

  // get the new value for seconds
  Serial.println("Updating seconds");
  seconds = processReset('s', hours, minutes, seconds, DoW);
  processOLED(hours, minutes, seconds, ' ');

  // update the RTC with the new time
  rtc.adjust(DateTime(year, month, day, hours, minutes, seconds));
  setDoW(DoW);

  // save the new time to the SD card
  writeTime(year, month, day, hours, minutes, seconds, DoW);

  // increment the debug value
  numChanges++;

  // reset timeout value
  timeout = false;
}

/* 
 * This fuunction handles a user resetting the time.
 * The program works with two sets of data:     
 *    Year, Month, Day, 
 * or
 *    Hour, Minute, Second,
 * since the two sets of data are updated simultaneously
 * with the same function. 
 * parameters:
 *    type: a character representing the value being changed     
 *      'y': year; 'm': month; 'd': day;          
 *      'h': hour; 'n': minute; 's': second; 
 *      'w': day of week;
 *    data1: the value of the first data, either year or hour
 *    data2: the value of the second data, either month or minute
 *    data3: the value of the third data, either day or second
 *    DoW:   the value of the day of the week
 * returns: the new time value
*/
int processReset(char type, int data1, byte data2, byte data3, byte DoW) {
  // debounce the input
  while (digitalRead(setTimePin)) {
    delay(50);
  }

  // set initial values
  boolean setting = true;  // controls whether the times is still being set
  int upTime = 0;       // the length of time the increment button has been held
  int downTime = 0;     // the length of time the decriment button has been held
  int newTime = 0;      // the new time value
  int neutralTime = 0;  // the length of time since a button has been released
  errorCheckDay = data3; // check the day of month for error
  int settingTime = 0;   // how long the value has been in the updating state

  // get the value of the type being changed.
  if (type == 'y' || type == 'h') {
    newTime = data1;
  } else if (type == 'm' || type == 'n') {
    newTime = data2;
  } else if (type == 'd' || type == 's') {
    newTime = data3;
  } else if (type == 'w') {
    newTime = DoW;
  }

  // while the clock is still checking for a new value
  while (setting && !timeout) {
    // if the value is being increased
    if (digitalRead(timeUpPin)) {
      // increment at a particular rate
      if (upTime % 5 == 0 && upTime <= 20) {
        // slow, every 5th cycle
        newTime++;
      } else if (upTime % 3 == 0 && upTime > 20 && upTime <= 40) {
        // moderate, every 3th cycle
        newTime++;
      } else if (upTime % 2 == 0 && upTime > 40) {
        // fast, every 2th cycle
        newTime++;
      }
      upTime++;
      neutralTime = 0;
    }
    // if the value is being decrimented
    else if (digitalRead(timeDownPin)) {
      // decriment at a particular rate
      if (downTime % 5 == 0 && downTime <= 20) {
        // slow, every 5th cycle
        newTime--;
      } else if (downTime % 3 == 0 && downTime > 20 && downTime <= 40) {
        // moderate, every 3th cycle
        newTime--;
      } else if (downTime % 2 == 0 && downTime > 40) {
        // fast , every 2th cycle
        newTime--;
      }
      downTime++;
      neutralTime = 0;
    }
    // checkt to see if the value is done being set
    else if (digitalRead(setTimePin)) {
      setting = false;
    }
    // otherwise, increment neutral time and check for timeout
    else {
      upTime = 0;
      downTime = 0;
      neutralTime++;
      if(neutralTime >= timeoutTime) {
        setting = false;
        timeout = true;
      }
    }
    // increment how long the value has been in the updating state
    settingTime++;


    // Update the OLED displays.
    // Different checks and updates are made depending on 
    // what value is being updated.
    switch (type) {
      // if day of week is being updated
      case 'w': {
        // update the time if it is out of its bounds
        if (newTime < 0) {
          newTime = 6;
        }
        newTime = newTime % 7;

        // update the OLED, flashing the DoW value
        if (neutralTime % 20 > 10 || settingTime < 5) {
          updateLeftOLED(data1, data2, data3, newTime, 'w');
        } else {
          updateLeftOLED(data1, data2, data3, newTime, ' ');
        }
        break;
      }
      
      // if year is being updated
      case 'y': {
        // check to see if a day of month error has occured when the year changes
        if (newTime % 4 != 0 && data2 == 2 && data3 == 29) {
          // if so, correct the day
          data3 = 28;
          errorCheckDay = 28;
          // print debug message
          Serial.print("Gotcha bitch: ");
          Serial.println(data3);
        }
        
        // update the OLED, flashing the year value
        if (neutralTime % 20 > 10 || settingTime < 5) {
          updateLeftOLED(newTime, data2, data3, DoW, 'y');
        } else {
          updateLeftOLED(newTime, data2, data3, DoW, ' ');
        }
        break;
      }
      
      // if month is being updated
      case 'm': {
        // check to see if the month is out of its bounds
        newTime = processTime(newTime, 12, 1, 12);

        // check to see if an error in the day of month when the 
        // month changes (changing from January 30th to February 30th)
        byte monthLen = monthLens[newTime-1];
        if (data3 > monthLen) { // if the day > days in month
          if (newTime == 2 && data1%4 == 0) { // February on a leap year
            data3 = 29;
            errorCheckDay = 29;
            // print debug message
            Serial.println("broken");
          } else { // otherwise some other day/month error
            data3 = monthLen;
            errorCheckDay = monthLen;
          }
        }
        
        // update the OLED, flashing the month value
        if (neutralTime % 20 > 10 || settingTime < 5) {
          updateLeftOLED(data1, newTime, data3, DoW, 'm');
        } else {
          updateLeftOLED(data1, newTime, data3, DoW, ' ');
        }
        break;
      }
      
      // if day of month is being updated
      case 'd': {
        // check to see if the day of month is out of its bounds
        if (data2 == 2) {
          if (data1 % 4 == 0) { // February on leap year
            newTime = processTime(newTime, 29, 1, 29);
          } else { // February on any other year
            newTime = processTime(newTime, 28, 1, 28);
          }
        } else { // any other month
          byte monthLen = monthLens[data2-1];
          newTime = processTime(newTime, monthLen, 1, monthLen);
        }
        
        // update the OLED, flashing the day of month value
        if (neutralTime % 20 > 10 || settingTime < 5) {
          updateLeftOLED(data1, data2, newTime, DoW, 'd');
        } else {
          updateLeftOLED(data1, data2, newTime, DoW, ' ');
        }
        break;
      }
      
      // if hours is being updated
      case 'h': {
        // check to see if the hours is out of its bounds
        newTime = processTime(newTime, 23, 0, 24);
        
        // update the OLED, flashing the hours value
        if (neutralTime % 20 > 10 || settingTime < 5) {
          processOLED(newTime, data2, data3, 'h');
        } else {
          processOLED(newTime, data2, data3, ' ');
        }
        break;
      }
      
      // if minutes is being updated
      case 'n': {
        // check to see if the minutes is out of its bounds
        newTime = processTime(newTime, 59, 0, 60);
        
        // update the OLED, flashing the minutes value
        if (neutralTime % 20 > 10 || settingTime < 5) {
          processOLED(data1, newTime, data3, 'n');
        } else {
          processOLED(data1, newTime, data3, ' ');
        }
        break;
      }
      
      // if seconds is being udpated
      case 's': {
        // check to see if the seconds is out of its bounds
        newTime = processTime(newTime, 59, 0, 60);
        
        // update the OLED, flashing the seconds value
        if (neutralTime % 20 > 10 || settingTime < 5) {
          processOLED(data1, data2, newTime, 's');
        } else {
          processOLED(data1, data2, newTime, ' ');
        }
        break;
      }
    }
    // delay to slow down polling
    delay(50);
  }
  // return the new time
  return newTime;
}

/*
 * Check to see if the newTime is out of its bounds, and correct it if 
 * it is.
 * parameters:
 *    newTime: the value being checked
 *    upLim:   the upper limit of the value (inclusive)
 *    lowLim:  the lower limit of the value (inclusive)
 *    change:  the difference between the upper and lower limit +1 // TODO fix this
 * returs: the new, bounded value
 */
byte processTime(int newTime, byte upLim, byte lowLim, byte change) {
  if (newTime > upLim) {
    newTime -= change;
  } else if (newTime < lowLim) {
    newTime += change;
  }
  return newTime;
}

/*
 * Helper function to prepocess the values for the right OLED before it 
 * is updated. It checks and updates the "am" or "pm" values.
 * parameters:
 *    hours:    the new hours value
 *    minutes:  the new minutes value
 *    seconds:  the new seconds value
 *    hydeType: the character that will be displayed as whitespace
 *      when the time is being reset:
 *        'h': hours is being updated
 *        'n': minutes is being updated
 *        's': seconds is being updated
 *        ' ': no value is being updated
 *  TODO have return hours for polymorphism
 */
void processOLED(byte hours, byte minutes, byte seconds, char hideType) {
  String sunlight = "am";
  if (hours > 12) {
    hours -= 12;
    sunlight = "pm";
  } else if (hours == 0) {
    hours = 12;
  } else if (hours == 12) {
    sunlight = "pm";
  }

  // update the right OLED with the correct values
  updateRightOLED(hours, minutes, seconds, sunlight, hideType);
}

/*
 * These functions are used to get and set the day of week 
 * from the RTC, as the native library does not have
 * a way of deing that easily.
 */

//////// These functions were taken from the DS3231 Github ///////////////
//////// https://github.com/NorthernWidget/DS3231/blob/master/DS3231.cpp ///////////////

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
