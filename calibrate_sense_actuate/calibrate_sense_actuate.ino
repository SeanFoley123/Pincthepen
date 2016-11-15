#include <EEPROM.h>
#include <Servo.h>

int S0 = 7;   //Settings pins
int S1 = 6;
int S2 = 5;
int S3 = 4;

int taosOutPin = 3;   //Output pin; outputs square wave w/frequency proportional to light intensity

int green_LED = 11;   //User interface LEDS

int LED = 2;

int sense_but = 8;    //User input buttons and debounce variables
int sense_state;
int last_sense_reading = LOW;
long last_sense_time = 0;

int mix_but = 9;
int mix_state;
int last_mix_reading = LOW;
long last_mix_time = 0;

long debounce_delay = 50;

int black[3];   //Black calibration data; returns as an unsigned long, but the value is never greater than 255 if things are working
int white[3];   //White calibration data

int color[3];

int sol_pin1 = 10;

void setup()
{
  Serial.begin(9600);

  //initialize feedback LEDs
  pinMode(green_LED, OUTPUT);
  pinMode(LED, OUTPUT);

  //user control buttons
  pinMode(mix_but, INPUT);
  pinMode(sense_but, INPUT);

  //color mode selection
  pinMode(S2, OUTPUT); //S2 pinE
  pinMode(S3, OUTPUT); //s3 pinF

  //color response pin
  pinMode(taosOutPin, INPUT); //taosOutPin pinC

  //communication freq (sensitivity) selection
  pinMode(S0, OUTPUT); //S0 pinB
  pinMode(S1, OUTPUT); //S1 pinA

  pinMode(sol_pin1, OUTPUT);

  readEE(black, 0);     //Read previous calibtration data, if there is any
  readEE(white, 1);
}

void writeEE(int *thing, int pos){     //Write RGB value thing to the EEPROM, where pos is which position to write to (e.g. 0th, 1st, 2nd, etc.)
  for(int i=0; i<3; i++){
    byte one = (thing[i] >> 8)&0xFF;    //Find most significant byte
    byte two = (thing[i])&0xFF;
    EEPROM.write(pos*6+i*2, one);         //Store big endian
    EEPROM.write(pos*6+i*2+1, two);   
  }
}

void readEE(int *thing, int pos){       //Read from pos and put that value in thing.
  for(int i=0; i<3; i++){
    int one = EEPROM.read (pos*6+i*2);    //Find first byte
    int two = EEPROM.read(pos*6+i*2+1);   //Find second byte
    thing[i] = (one << 8) + two;        //Make it into a two-byte int type! Then store it
  }
}

void loop()
{
  int sense_reading = digitalRead(sense_but);   //Check for user input
  int mix_reading = digitalRead(mix_but);
  
  if(sense_reading != last_sense_reading){      //Debounce
    last_sense_time = millis();
  }
  
  if(mix_reading != last_mix_reading){
    last_mix_time = millis();
  }
  
  if(millis()-last_sense_time > debounce_delay && sense_reading != sense_state){      //If the sense button is pressed, check the value on the sensor
    sense_state = sense_reading;
    if(sense_state == HIGH){
      senseColor();
    }
  }

  else if(millis()-last_mix_time > debounce_delay && mix_reading != mix_state){       //If the calibrate button is pressed, run calibration sequence
    mix_state = mix_reading;
    if(mix_state == HIGH){
      calibrate();
    }
  }

//  else if(millis()-last_mix_time > debounce_delay && mix_reading != mix_state){       //Mixing button
//    mix_state = mix_reading;
//    if(mix_state == HIGH){
//      digitalWrite(sol_pin1, HIGH);
//    }
//    else{
//      digitalWrite(sol_pin1, LOW);
//    }
//  }
  
  last_sense_reading = sense_reading;
  last_mix_reading = mix_reading;
}

void calibrate()    //Get and print values for black and white swatches
{
 colorRead(black);
 colorRead(white);
 writeEE(black, 0);     //Store black in the 0th position in the EEPROM
 writeEE(white, 1);
}

void senseColor()         //Detect and print a color normalized for the current calibration values
{
  colorRead(color);     //Read the current color
  float norm_color[3];
  for(int i = 0; i<3; i++){
    norm_color[i] = (float)(black[i]-color[i])/(float)(black[i]-white[i]);
    Serial.print(norm_color[i]);
    Serial.print(' ');
  }
  Serial.print('\n');
  if(norm_color[0]>1.5*norm_color[1] && norm_color[0]>1.5*norm_color[2]){
    digitalWrite(sol_pin1, HIGH);
    delay(4000);
    digitalWrite(sol_pin1, LOW);
  }
}

void colorRead(int *thing)    //Sets the values of thing to each color reading in ms. Thing is an unsigned long array {R, G, B}
{
  for(int i = 0; i < 10; i++){         //Blink to tell the person they need to get it over the color
    digitalWrite(green_LED, HIGH);
    delay(200);
    digitalWrite(green_LED, LOW);
    delay(200);
  }
  
  digitalWrite(green_LED, HIGH);     //Taking measurement
  digitalWrite(LED, HIGH);
  delay(500);                        //Hold that light on for 1 sec total + computation time

  taosMode(2);    //turn on sensor

  //setting for a delay to let the sensor sit for a moment before taking a reading.
  int sensorDelay = 10;
  byte color_selects[3] = {B00000000, B00000011, B00000010};       //The different bitmasks R G B on pins S2 and S3
  for(int k=0; k < 3; k++){
    digitalWrite(S2, color_selects[k]&B00000001);         //Select color filter
    digitalWrite(S3, color_selects[k]&B00000010);
    
    int reading[10];        //Sum 10 readings
    for(int j=0; j < 10; j++){
      int readPulse = pulseIn(taosOutPin, LOW, 8000);      //Returns how long taosOutPin remains low or 80000 in ms; essentially period/2
      if(readPulse < .1){     //Fixes timeout behavior; instead of returning a 0, it'll return an 80000
        readPulse = 8000;
      }
//      Serial.println(readPulse);
      reading[j] = readPulse;
      delay(sensorDelay);
    }
//    Serial.println(' ');
    int most_common = -1, max_frequency = 0;
    for(int i=0; i<10; i++){
      if(reading[i] != -1){
        int frequency = 1;
        for(int j=i+1; j<10; j++){
          if(reading[i] == reading[j]){
            frequency+=1;
            reading[j] = -1;
          }
        }
        if(frequency > max_frequency){
          most_common = reading[i];
          max_frequency = frequency;
        }
      }
    }
    thing[k] = most_common;
  }
  taosMode(0);        //turn off sensor to save energy
  delay(500);         //Keep light on
  digitalWrite(green_LED, LOW);
  digitalWrite(LED, LOW);
}

// Operation modes, controlled by hi/lo settings on S0 and S1 pins.
//setting mode to zero will put taos into low power mode. taosMode(0);
//To-do; allow controlling the sensor LED w/mode

void taosMode(byte mode) {
  //Mode 0 -> Powerdown, 1 -> 2% output freq scaling
  //2 -> 20% output freq scaling, 3 -> 100% scaling (highest sensitivity
  digitalWrite(S1, B00000001 & mode);
  digitalWrite(S0, B00000010 & mode);
}
