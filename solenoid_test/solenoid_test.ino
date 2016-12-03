void setup() {
  // put your setup code here, to run once:
  pinMode(9, INPUT);
  pinMode(10, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(digitalRead(9)){
    digitalWrite(10, HIGH);}
  else{
    digitalWrite(10, LOW);}
  delay(20);
}
