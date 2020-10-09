/*     Simple Stepper Motor Control Exaple Code
 *      
 *  by Dejan Nedelkovski, www.HowToMechatronics.com
 *  
 */
// defines pins numbers
const int stepPin = 10; 
const int dirPin = 11;
const int sleepPin = 12;
const int resetPin = 9;
const int ledPin = 13;

const int numSteps = 200; // full rotation
const int timeSpeed = 600; 
double timeSec = 0;
unsigned long oldTime = 0;
unsigned long time0 = 0;
unsigned long thisTime = 0;
 
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
}

void loop() {
  time0 = micros();
  for(int x = 0; x < numSteps; x++) {
    digitalWrite(stepPin,HIGH);
    digitalWrite(ledPin,HIGH);
    delayMicroseconds(timeSpeed);
    digitalWrite(stepPin,LOW);
    digitalWrite(ledPin,LOW);
    delayMicroseconds(timeSpeed);
  }
  timeSec+=1000000;
  thisTime=micros();
  if(thisTime>=oldTime){
    oldTime=thisTime;
    delay((timeSec-thisTime)/1000);
  } else { 
    timeSec=thisTime;
    oldTime=thisTime;
    unsigned long difference = thisTime-time0;
    delay((1000000-difference)/1000);
    Serial.println("it did the thing");
  }
  Serial.print(timeSec);
  Serial.print(" ");
  Serial.println(thisTime);
}
