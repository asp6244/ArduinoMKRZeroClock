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
  time0 = millis();
  for(int x = 0; x < numSteps; x++) {
    digitalWrite(stepPin,HIGH);
    digitalWrite(ledPin,HIGH);
    delayMicroseconds(timeSpeed); 
    digitalWrite(stepPin,LOW);
    digitalWrite(ledPin,LOW);
    delayMicroseconds(timeSpeed);
  }
  timeSec+=1000;
  if(millis()>oldTime){
    oldTime=millis();
    delay(timeSec-millis());
    Serial.println(micros());
  } else { 
    timeSec=millis();
    unsigned long difference = millis()-time0;
    delay(difference);
  }
}
