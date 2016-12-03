//This code should spin the syringe servo from start_point to end_point, defined in the move_syringe function, when you press a button. You'll need to define the buttonPin
#include <Servo.h>
int syringe_servo_pin = 13;     //Pin for servo
const int buttonPin = 2;    // the number of the pushbutton pin; you'll have to set this
Servo syringe_servo;      //Initialize servo to control syringe

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  syringe_servo.attach(syringe_servo_pin);     //Attach servo to digital
  pinMode(buttonPin, INPUT);
}

void loop() {
  int reading = digitalRead(buttonPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;
      if(buttonState == HIGH){
        move_syringe();
      }
      }
    }
  lastButtonState = reading;
}
  
void move_syringe() {
  int start_point = 0;
  int end_point = 60;
  int step_size = 1;
  for(int i=start_point; i<end_point; i+=step_size){
    syringe_servo.write(i);  
    delay(10);
  }
}
