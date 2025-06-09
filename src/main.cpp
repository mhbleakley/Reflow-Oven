#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Encoder.h>
#include <Timer.h>
#include <SPI.h>


// DECLARATIONS
void lcdStartScreen();
void handleButtonPressed();
void checkButtonPress();

// DEVICES

// lcd
LiquidCrystal_I2C lcd(0x27, 16, 2);

// encoder wheel/button
Encoder knob(2, 3);
long encoderMenuSelection = 0; // this is raw/4 as there are 4 pulses per detent

long rawEncoderValue = 0;
long lastRawEncoderValue = 0;

const int buttonPin = 4;
volatile bool buttonPressedFlag = false;

// relay
const int relayPin = 5;

// thermocouple
const int thermocoupleCSPin = 10;

// timer
Timer buttonTimer(300);

// REFLOW CHARACTERISTIC
int soakTimeS = 60;
int reflowTimeS = 40;

long soakTempC = 150;
long reflowTempC = 225;

// SYSTEM STATE
bool running = false;
bool editing = false;
int runPhase = 0;
int editingStep = 0;
int lastEditingStep = -1;

void setup() {

  // serial
  Serial.begin(9600);

  // LCD
  lcd.init();
  lcd.blink();
  lcd.backlight();

  lcdStartScreen();

  // timer
  buttonTimer.reset();

  // button
  pinMode(buttonPin, INPUT_PULLUP); // Use internal pull-up

  // Enable pin change interrupt on PCINT20 (digital pin 4)
  PCICR |= (1 << PCIE2);    // Enable PCINT2 interrupt group
  PCMSK2 |= (1 << PCINT20); // Enable interrupt on pin 4

  // SSR
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // thermocouple
  pinMode(thermocoupleCSPin, OUTPUT);
  digitalWrite(thermocoupleCSPin, HIGH);
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST); // confirmed
  // must drive CS high after reading for chip to update
  // bytes 31-18 are temp (31 is sign)
  // byte 17 is reserved, 16 indicates a fault (fault = 1)
  // bytes 15-4 are internal temp data (15 is sign)
  // byte 3 is reserved, bytes 2-0 indicate different faults
  // 2: short to VCC = 1
  // 1: short to GND = 1
  // 0: open circuit = 1

  byte data[4];
  while (true){
    digitalWrite(thermocoupleCSPin, LOW);

    for(int i = 0; i < 4; i++){
      data[i] = SPI.transfer(0xFF);
    }

    digitalWrite(thermocoupleCSPin, HIGH);

    Serial.print("Data:");
    for(int i = 0; i < 4; i++){
      Serial.print(data[i], HEX);
    }
    Serial.println();

    delay(1000);
  }
}

void loop() {
  checkButtonPress();

  if (!running && !editing) {
    rawEncoderValue = knob.read();

    if (rawEncoderValue != lastRawEncoderValue){
      lastRawEncoderValue = rawEncoderValue;
      if (rawEncoderValue > 4) knob.write(4);
      if (rawEncoderValue < 0) knob.write(0);
    }

    if (rawEncoderValue == 0) { // "Run" should be selected
      if (encoderMenuSelection != 0) {
        encoderMenuSelection = 0;
        lcd.setCursor(4, 1); // Blink by "Run"
      }
    } else if (rawEncoderValue == 4) { // "Edit" should be selected
      if (encoderMenuSelection != 1) {
        encoderMenuSelection = 1;
        lcd.setCursor(12, 1); // Blink by "Edit"
      }
    }
  }

  if (editing){
    rawEncoderValue = knob.read();

    if (editingStep == 0){

      if (lastEditingStep < editingStep){
        lastEditingStep = editingStep;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Soak Temp:");
        lcd.setCursor(10, 0);
        lcd.print(soakTempC);
        knob.write(soakTempC * 4);
        lastRawEncoderValue = soakTempC * 4;
      }

      if (rawEncoderValue != lastRawEncoderValue){
        lastRawEncoderValue = rawEncoderValue;
        lcd.setCursor(10, 0);
        lcd.print("   ");
        lcd.setCursor(10, 0);
        lcd.print(rawEncoderValue / 4);
        soakTempC = rawEncoderValue / 4;
      }

    }else if (editingStep == 1){

      if (lastEditingStep < editingStep){
        lastEditingStep = editingStep;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Soak Time:");
        lcd.setCursor(10, 0);
        lcd.print(soakTimeS);
        knob.write(soakTimeS * 4);
        lastRawEncoderValue = soakTimeS * 4;
      }

      if (rawEncoderValue != lastRawEncoderValue){
        lastRawEncoderValue = rawEncoderValue;
        lcd.setCursor(10, 0);
        lcd.print("   ");
        lcd.setCursor(10, 0);
        lcd.print(rawEncoderValue / 4);
        soakTimeS = rawEncoderValue / 4;
      }

    }else if (editingStep == 2){

      if (lastEditingStep < editingStep){
        lastEditingStep = editingStep;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Reflow Temp:");
        lcd.setCursor(12, 0);
        lcd.print(reflowTempC);
        knob.write(reflowTempC * 4);
        lastRawEncoderValue = reflowTempC * 4;
      }

      if (rawEncoderValue != lastRawEncoderValue){
        lastRawEncoderValue = rawEncoderValue;
        lcd.setCursor(12, 0);
        lcd.print("   ");
        lcd.setCursor(12, 0);
        lcd.print(rawEncoderValue / 4);
        reflowTempC = rawEncoderValue / 4;
      }

    }else if (editingStep == 3){
    
      if (lastEditingStep < editingStep){
        lastEditingStep = editingStep;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Reflow Time:");
        lcd.setCursor(12, 0);
        lcd.print(reflowTimeS);
        knob.write(reflowTimeS * 4);
        lastRawEncoderValue = reflowTimeS * 4;
      }

      if (rawEncoderValue != lastRawEncoderValue){
        lastRawEncoderValue = rawEncoderValue;
        lcd.setCursor(12, 0);
        lcd.print("   ");
        lcd.setCursor(12, 0);
        lcd.print(rawEncoderValue / 4);
        reflowTimeS = rawEncoderValue / 4;
      }

    }else{
      editingStep = 0;
      editing = false;
    }
  }

  if (running){
    // lcd.setCursor(0, 0);
    // lcd.print("running");
    delay(100);
    running = false;
  }

}


void lcdStartScreen(){
  lcd.setCursor(0, 0);
  lcd.print("S");
  lcd.setCursor(1, 0);
  lcd.print(soakTempC); // 3 digits
  lcd.setCursor(4, 0);
  lcd.print("/");
  lcd.setCursor(5, 0);
  lcd.print(soakTimeS); // as many as 3?
  lcd.setCursor(8, 0);
  lcd.print("R");
  lcd.setCursor(9, 0);
  lcd.print(reflowTempC);
  lcd.setCursor(12, 0);
  lcd.print("/");
  lcd.setCursor(13, 0);
  lcd.print(reflowTimeS);

  lcd.setCursor(0, 1);
  lcd.print("Run:");
  lcd.setCursor(7, 1);
  lcd.print("Edit:");
  lcd.setCursor(4, 1);

  knob.write(0); // Start encoder value at 0
}

void handleButtonPressed(){
  if (!running && !editing){
    if (encoderMenuSelection == 0) running = true;
    if (encoderMenuSelection == 1) editing = true;
  }else if (editing){
    editingStep ++;
  }
}

void checkButtonPress() {
  if (buttonPressedFlag && buttonTimer.isExpired()) {
    buttonPressedFlag = false;
    handleButtonPressed();
  }
}

// ISR – triggered on pin change
ISR(PCINT2_vect) {
  if (digitalRead(buttonPin) == LOW && buttonTimer.isExpired()) {
    buttonPressedFlag = true;     // Set flag
    buttonTimer.reset();          // Start debounce timer
  }
}
