/*     Simple Stepper Motor Control Exaple Code
 *      
 *  by Dejan Nedelkovski, www.HowToMechatronics.com
 *  
 */
// defines pins numbers
const int stepPin = 8; 
const int dirPin = 9;
const int sleepPin = 7;
const int resetPin = 6;
const int ledPin = 32;

const int numSteps = 200; // full rotation
const int timeSpeed = 600; 
 
void setup() {
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(ledPin,OUTPUT);
  pinMode(sleepPin,OUTPUT);
  pinMode(resetPin,OUTPUT);
  digitalWrite(sleepPin, HIGH);
  digitalWrite(resetPin, HIGH);
  Serial.begin(9600);
}

void loop() {
  digitalWrite(dirPin,HIGH); // Enables the motor to move in a particular direction
  for(int x = 0; x < numSteps; x++) {
    digitalWrite(stepPin,HIGH);
    digitalWrite(ledPin,HIGH);
    delayMicroseconds(timeSpeed); 
    digitalWrite(stepPin,LOW);
    digitalWrite(ledPin,LOW);
    delayMicroseconds(timeSpeed);
  }
//  delay(1000.0-(millis()-time0));
}
