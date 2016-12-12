#include <EEPROM.h>

int S0 = 7;   //Frequency output settings pins
int S1 = 6;
int S2 = 5;   //Color filter selection pins
int S3 = 4;
int taosOutPin = 3;   //Output pin; outputs square wave w/frequency proportional to light intensity
int green_LED = 13;   //User interface LEDS
int LED = 2;          //Lighting-up LED on color sensor
int sense_but = 8;    //User input buttons
int cal_but = 9;
int sol_pin1 = 12;    //Solenoid valve pins; NEVER ACTIVATE MORE THAN ONE AT A TIME
int sol_pin2 = A3;    //Water solenoid
int sol_pin3 = A4;
int sol_pin4 = A5;
int solenoids[4] = {sol_pin1, sol_pin2, sol_pin3, sol_pin4};     //Water is the last one
int syringe = 11;     //Pin for syringe motor; needs to be a PWM pin

int sense_state;        //Button debounce variables
int last_sense_reading = LOW;
long last_sense_time = 0;
int cal_state;
int last_cal_reading = LOW;
long last_cal_time = 0;
long debounce_delay = 50;

int black[3];   //Black calibration data; returns as an unsigned long, but the value is never greater than 255 if things are working
int white[3];   //White calibration data
int color[3];   //Color data

void setup(){
  Serial.begin(9600);
  
  pinMode(green_LED, OUTPUT);   //Initialize pins
  pinMode(LED, OUTPUT);
  pinMode(cal_but, INPUT);
  pinMode(sense_but, INPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(taosOutPin, INPUT);
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(sol_pin1, OUTPUT);
  pinMode(sol_pin2, OUTPUT);
  pinMode(sol_pin3, OUTPUT);
  pinMode(sol_pin4, OUTPUT);
  pinMode(syringe, OUTPUT);

  readEE(black, 0);     //Read previous calibtration data
  readEE(white, 1);}     //Convention is that black is stored in the first position in the EEPROM and white is stored in the second

void loop(){
  int sense_reading = digitalRead(sense_but);   //Check for user input
  int cal_reading = digitalRead(cal_but);
  
  if(sense_reading != last_sense_reading){      //Debounce
    last_sense_time = millis();}
  
  if(cal_reading != last_cal_reading){
    last_cal_time = millis();}
  
  if(millis()-last_sense_time > debounce_delay && sense_reading != sense_state){      //If the sense button is pressed, check the value on the sensor
    sense_state = sense_reading;
    if(sense_state == HIGH){
//      senseColor()
      pump_ink(sol_pin2);}}

  else if(millis()-last_cal_time > debounce_delay && cal_reading != cal_state){       //If the calibrate button is pressed, run calibration sequence
    cal_state = cal_reading;
    if(cal_state == HIGH){
//      calibrate()
        pump_ink(sol_pin4);}}
  
  last_sense_reading = sense_reading;         //Store current readings
  last_cal_reading = cal_reading;}

void calibrate(){    //Get and print values for black and white swatches
 colorRead(black);
 colorRead(white);
 writeEE(black, 0);     //Store calibration values in EEPROM
 writeEE(white, 1);}

void senseColor(){         //Detect and serial print a color normalized for the current calibration values
  colorRead(color);     //Read the current color
  float norm_color[3];
  for(int i = 0; i<3; i++){
    norm_color[i] = (float)(black[i]-color[i])/(float)(black[i]-white[i]);      //Normalize color reading
    Serial.print(norm_color[i]);      //Print to serial terminal
    Serial.print(' ');}
  Serial.print('\n');
  pump_ink(sol_pin2);}

void colorRead(int *thing){    //Sets the values of thing to each color reading in ms. Thing is an int array {R, G, B}
  for(int i = 0; i < 10; i++){         //Blink to tell the person they need to get the color sensor over the color
    digitalWrite(green_LED, HIGH);
    delay(200);
    digitalWrite(green_LED, LOW);
    delay(200);}
  
  digitalWrite(green_LED, HIGH);     //Signal that you're taking measurement
  digitalWrite(LED, HIGH);           //Turn on measurement LED
  delay(500);                        //Hold that light on for 1 sec total + computation time

  taosMode(2);    //turn on sensor

  int sensorDelay = 10;     //Setting for a delay to let the sensor sit for a moment before taking a reading.
  int num_readings = 10;    //Number of samples to take for each color
  byte color_selects[3] = {B00000000, B00000011, B00000010};       //The different bitmasks R G B on pins S2 and S3
  for(int k=0; k < 3; k++){
    digitalWrite(S2, color_selects[k]&B00000001);         //Select color filter
    digitalWrite(S3, color_selects[k]&B00000010);
    
    int reading[num_readings];        //Record num_readings readings
    for(int j=0; j < num_readings; j++){
      int readPulse = pulseIn(taosOutPin, LOW, 8000);      //Returns how long taosOutPin remains low (period/2) or 80000 in ms
      if(readPulse < .1){     //Fixes timeout behavior; instead of returning a 0, it'll return an 80000
        readPulse = 8000;}
      reading[j] = readPulse;
      delay(sensorDelay);}
      
    int most_common = -1, max_frequency = 0;      //Code to find mode of reading; maybe inefficient, but there are only 10 values and they're not often scrolled through
    for(int i=0; i<num_readings; i++){
      if(reading[i] != -1){       //-1 is a placeholder for "don't count this"
        int frequency = 1;        //Register that there was one instance of this number
        for(int j=i+1; j<num_readings; j++){    //Scroll through all remaining numbers
          if(reading[i] == reading[j]){       //Register that there was another instance of that number, then turn that instance into a -1
            frequency+=1;
            reading[j] = -1;}}
        if(frequency > max_frequency){      //If it's more common than the previous most_common number, set this one to the most common
          most_common = reading[i];
          max_frequency = frequency;}}}
    thing[k] = most_common;}                //Return the mode
    
  taosMode(0);        //turn off sensor to save energy
  delay(500);         //Keep feedback LED on
  digitalWrite(green_LED, LOW);     //Turn off LEDs
  digitalWrite(LED, LOW);}

void writeEE(int *thing, int pos){     //Write RGB array thing to the EEPROM, where pos is which position to write to (e.g. 0th, 1st, 2nd, etc.)
  for(int i=0; i<3; i++){
    byte one = (thing[i] >> 8)&0xFF;    //Find most significant byte
    byte two = (thing[i])&0xFF;
    EEPROM.write(pos*6+i*2, one);         //Store big endian
    EEPROM.write(pos*6+i*2+1, two);}}

void readEE(int *thing, int pos){       //Read from pos and put that value in thing.
  for(int i=0; i<3; i++){
    int one = EEPROM.read (pos*6+i*2);    //Find first byte
    int two = EEPROM.read(pos*6+i*2+1);   //Find second byte
    thing[i] = (one << 8) + two;}}        //Make it into a two-byte int type! Then put it in the array thing

void taosMode(byte mode){       //Set operation mode
  //Mode 0 -> Powerdown, 1 -> 2% output freq scaling
  //2 -> 20% output freq scaling, 3 -> 100% scaling (highest sensitivity
  digitalWrite(S1, B00000001 & mode);
  digitalWrite(S0, B00000010 & mode);}

void pen_write(){        //Wrapper code for whatever is needed to get the right color ink into the pen.
  int which_pin = -1;     //Stores the pin of the most prominent color
  int value = -1.0;         //Stores the value of that color
  for(int i = 0; i<3; i++){
    if((float)(black[i]-color[i])/(float)(black[i]-white[i])>value){
      which_pin = i;
      value = (float)(black[i]-color[i])/(float)(black[i]-white[i]);}}      //Normalize color reading
  int correct_sol = solenoids[which_pin];       //Choose solenoid corresponding to most prominent color
  pump_ink(solenoids[3]);      //Flush with water
  pump_ink(correct_sol);}

void pump_ink(int sol){
  int valve_time = 4000;        //How long the valve is open
  digitalWrite(sol, HIGH);
  Serial.print(sol);
  Serial.println(" High");
  analogWrite(syringe, 100);    //Set motor speed
  delay(valve_time);
  digitalWrite(sol, LOW);     //Pump servo
  Serial.print(sol);
  Serial.println(" Low");}
