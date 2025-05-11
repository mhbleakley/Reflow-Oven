#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Encoder.h>

// DECLARATIONS
void startScreen();

// DEVICES
LiquidCrystal_I2C lcd(0x27, 16, 2);

Encoder knob(2, 3);
long logicalEncoderValue = 0; // this is raw/4 as there are 4 pulses per detent

long rawEncoderValue = 0;
long lastRawEncoderValue = 0;

const byte buttonPin = 4;
volatile bool buttonPressed = false;

const int buttonDelay = 500;

// REFLOW CHARACTERISTIC
int targetPreheatRampCpS = 3; // room temp to soak C/s
int targetSoakRampCpS = 0; // soak to reflow ramp up C/s if applicable 
int targetReflowRampCpS = 2; // ramp from soak end to reflow C/s
int targetCoolDownRampCpS = -4; // cool down ramp C/s

int soakTimeS = 60;
int reflowTimeS = 40;

long soakTempC = 150;
long soakEndTempC = 180;
long reflowTempC = 225;

// SYSTEM STATE
int running = 0;
int editing = 0;
int runPhase = 0;

void setup() {
  lcd.init();
  lcd.blink();
  lcd.backlight();

  startScreen();

  pinMode(buttonPin, INPUT_PULLUP); // Use internal pull-up

  // Enable pin change interrupt on PCINT20 (digital pin 4)
  PCICR |= (1 << PCIE2);    // Enable PCINT2 interrupt group
  PCMSK2 |= (1 << PCINT20); // Enable interrupt on pin 4
}

void loop() {

  if (!running && !editing) {
    rawEncoderValue = knob.read();

    if (rawEncoderValue != lastRawEncoderValue){
      lastRawEncoderValue = rawEncoderValue;
      if (rawEncoderValue > 4) knob.write(4);
      if (rawEncoderValue < 0) knob.write(0);
    }

    if (rawEncoderValue == 0) { // "Run" should be selected
      if (logicalEncoderValue != 0) {
        logicalEncoderValue = 0;
        lcd.setCursor(4, 1); // Blink by "Run"
      }
    } else if (rawEncoderValue == 4) { // "Edit" should be selected
      if (logicalEncoderValue != 1) {
        logicalEncoderValue = 1;
        lcd.setCursor(12, 1); // Blink by "Edit"
      }
    }

    if (buttonPressed){ // user selected an option
      buttonPressed = false; // reset flag
      delay(buttonDelay);
      if (logicalEncoderValue == 0){
        running = true;
      }else{
        editing = true;
      }
    }
  }

  if (editing){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Soak Temp:");
    lcd.setCursor(10, 0);
    lcd.print(soakTempC);
    knob.write(soakTempC * 4);
    lastRawEncoderValue = soakTempC * 4;

    while (!buttonPressed){ // edit soak temp
      rawEncoderValue = knob.read();
      if (rawEncoderValue != lastRawEncoderValue){
        lastRawEncoderValue = rawEncoderValue;
        lcd.setCursor(10, 0);
        lcd.print("   ");
        lcd.setCursor(10, 0);
        lcd.print(rawEncoderValue / 4);
      }
    }
    soakTempC = rawEncoderValue / 4;
    buttonPressed = false;
    delay(buttonDelay);

    editing = false;
    lcd.clear();
    startScreen();
  }

  if (running){
    // lcd.setCursor(0, 0);
    // lcd.print("running");
    delay(100);
    running = false;
  }

}


void startScreen(){
  lcd.setCursor(0, 0);
  lcd.print("S:");
  lcd.setCursor(2, 0);
  lcd.print(soakTempC);
  lcd.setCursor(6, 0);
  lcd.print("R:");
  lcd.setCursor(8, 0);
  lcd.print(reflowTempC);
  lcd.setCursor(12, 0);
  lcd.print("T:");
  lcd.setCursor(14, 0);
  lcd.print(reflowTimeS);

  lcd.setCursor(0, 1);
  lcd.print("Run:");
  lcd.setCursor(7, 1);
  lcd.print("Edit:");
  lcd.setCursor(4, 1);

  knob.write(0); // Start encoder value at 0
}

ISR(PCINT2_vect) {
  // Debounce or ignore if not falling edge
  if (digitalRead(buttonPin) == LOW) {
    buttonPressed = true;
  }
}
