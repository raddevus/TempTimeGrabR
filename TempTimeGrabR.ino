#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ds3231.h>
 
struct ts t;
const int pinTempSensor = A0;     // Grove - Temperature Sensor connect to A0
const int B = 4275;               // B value of the thermistor
const long R0 = 100000;            // R0 = 100k

LiquidCrystal_I2C lcd(0x3F, 20, 4);

void setup() {
  lcd.init();
  lcd.begin(20,4);
  lcd.setCursor(2,0);
  lcd.backlight();
  lcd.print("Hello, LCD!"); 
  Wire.begin();
  DS3231_init(DS3231_CONTROL_INTCN);
  /*----------------------------------------------------------------------------
  In order to synchronise your clock module, insert timetable values below !
  ----------------------------------------------------------------------------*/
  DS3231_get(&t);
  if (t.year == 1900){
    t.hour=12;
    t.min=9;
    t.sec=30;
    t.mday=21;
    t.mon=05;
    t.year=2021;
 
    DS3231_set(t);
  }
  
}

void loop() {
  displayDateTime();
  displayTemp();
  delay(1000);
}

void displayTemp(){
  lcd.setCursor(2,3);
  lcd.print(getTemp());
  
}

void displayDateTime(){
  DS3231_get(&t);
  lcd.setCursor(2,0);
  lcd.print("Date : ");
  if (t.mon < 10){
    lcd.print("0");
  }
  lcd.print(t.mon);
  lcd.print("/");
  if (t.mday < 10){
    lcd.print("0");
  }
  lcd.print(t.mday);
  lcd.print("/");
  lcd.print(t.year);
  lcd.setCursor(2,2);
  lcd.print("Hour : ");
    if (t.hour < 10){
    lcd.print("0");
  }
  lcd.print(t.hour);
  lcd.print(":");
  if (t.min < 10){
    lcd.print("0");
  }
  lcd.print(t.min);
  lcd.print(".");
  if (t.sec < 10){
    lcd.print("0");
  }
  lcd.print(t.sec);
}

float getTemp(){
  int a = analogRead(pinTempSensor);
  float R = 1023.0/a-1.0;
  R = R0*R;
  float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet
  temperature = (temperature * 1.8) + 32;
  return temperature;
}
