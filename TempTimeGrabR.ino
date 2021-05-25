#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ds3231.h>
#include <SD.h>
#include <EEPROM.h>
 
struct ts t;
const int pinTempSensor = A0;     // Grove - Temperature Sensor connect to A0
const int B = 4275;               // B value of the thermistor
const long R0 = 100000;            // R0 = 100k
const int DATA_BTN = 2;
const int DATA_LED = 3;
const int ROOM_BTN = 4;
const int ROOM_COUNT = 7;

// EEPROM Memory location (index) where last room idx is stored
const int ROOMIDX_FIRST_BYTE = 0;

int dataBtnPrev = LOW;
int dataBtnCurrent = LOW;

int roomBtnPrev = LOW;
int roomBtnCurrent = LOW;

unsigned long lastWriteTime = 0;

const int CS_PIN = 10;
bool isSDCardInitialized = false;
bool isWritingData = false;
String allRooms[ROOM_COUNT]= {"basement","master","office","living","laundry","dining", "master bath"};
int currentRoomIdx = 0;
String currentRoom = allRooms[currentRoomIdx];
bool changeRoomBtnCurrent = false;
unsigned long lastTempRead = 0;
float currentTemp = 0;

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

  Serial.begin(9600);
  Serial.println("Initializing Card");
  
  //CS pin must be configured as an output
  pinMode(CS_PIN, OUTPUT);
  pinMode(pinTempSensor, INPUT);
  pinMode(DATA_BTN, INPUT);
  pinMode(ROOM_BTN, INPUT);
  pinMode(DATA_LED, OUTPUT);

  loadLastRoomUsed();
  
}

void loop() {
  displayDateTime();
  displayTemp();
  trySDCard();
  checkChangeRoomButton();
  if (!isWritingData){
    setRoom();
  }
  else{
    if (millis() - lastWriteTime > 5000){
      writeTempData();
    }
  }
  checkWriteDataButton();
  
  delay(500);
}

void loadLastRoomUsed(){
  currentRoomIdx = EEPROM.read(ROOMIDX_FIRST_BYTE);
  // insuring the currentRoomIdx is always a valid value.
  if (currentRoomIdx >= (ROOM_COUNT) || currentRoomIdx < 0){
    currentRoomIdx = 0;
  }
  Serial.println(currentRoomIdx);
}

void writeTempData(){
  lastWriteTime = millis();
  //Write the room to EEPROM so it'll be loaded the next
  // time the device is restarted.
  writeDataToEEProm((byte)currentRoomIdx);
   File dataFile = SD.open("2021T.csv", FILE_WRITE);
   if (dataFile)
   {
    dataFile.print(currentRoom);
    dataFile.print(",");
    dataFile.print(getTime());
    dataFile.print(",");
    readTemp();
    dataFile.println(currentTemp);
    dataFile.close(); //Data isn't written until we run close()!
   }
}

void checkChangeRoomButton(){
  roomBtnCurrent = debounce(changeRoomBtnCurrent, ROOM_BTN);
  if (roomBtnPrev == LOW && roomBtnCurrent == HIGH){
    changeRoomBtnCurrent = !changeRoomBtnCurrent;
  }
  if (changeRoomBtnCurrent){
    changeRoomBtnCurrent = false;
      if (currentRoomIdx == ROOM_COUNT-1)
      {
        currentRoomIdx = 0;
        return;
      }
      currentRoomIdx++;
  }
}

void checkWriteDataButton(){
  dataBtnCurrent = debounce(isWritingData, DATA_BTN);
  if (dataBtnPrev == LOW && dataBtnCurrent == HIGH){
    isWritingData = !isWritingData;
  }
  if (isWritingData){
    // turn on data writing and LED
    digitalWrite(DATA_LED, HIGH);
    
  }
  else{
    //turn off data writing and LED
    //isWritingData = false;
    digitalWrite(DATA_LED, LOW);
    
  }
}

boolean debounce(boolean last, int button)
{
 boolean current = digitalRead(button);    // Read the button state
 if (last != current)                      // If it's different…
 {
  delay(5);                                // Wait 5ms
  current = digitalRead(button);           // Read it again
 }
 return current;                           // Return the current value
}

void setRoom(){
  currentRoom = allRooms[currentRoomIdx];
  lcd.setCursor(1,1);
  lcd.print(currentRoom);
  unsigned int roomNameLength = currentRoom.length();
  byte displaySpaces = (byte)(20 - roomNameLength);
  char spaces[displaySpaces];
  memset(spaces, ' ', displaySpaces-1);
  spaces[displaySpaces] = '\0';
  lcd.print(spaces);
}

void trySDCard(){
 if (isSDCardInitialized){
  return;
 }
 lcd.setCursor(1,2);
 if (!SD.begin(CS_PIN))
 {
  lcd.print("Card Failure");
  Serial.println("Card Failure");
  return;
 }
 Serial.println("Card Ready"); 
 
 lcd.print("SD initialized");
 isSDCardInitialized = true;
}

void displayTemp(){
  lcd.setCursor(2,3);
  if (!isWritingData){
    // I'm controlling how often the temp module
    // is read from in an effort to determine if it
    // becomes more accurate.
    readTemp();
  }
  lcd.print(currentTemp);
  
}

void displayDateTime(){
  DS3231_get(&t);
  //Print Date
  lcd.setCursor(0,0);
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
  lcd.print(" ");
  //Print Time
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

String getTime(){
  DS3231_get(&t);
  String dateTime = "";

  if (t.mon < 10){
    dateTime += "0";
  }
  dateTime += String(t.mon) + "/";
  
  if (t.mday < 10){
    dateTime += "0";
  }
  dateTime += String(t.mday) + "/" + String(t.year);
  dateTime += " ";
  if (t.hour < 10){
    dateTime += "0";
  }
  dateTime += String(t.hour) + ":";
  if (t.min < 10){
    dateTime += "0";
  }
  dateTime += String(t.min) + ".";
  if (t.sec < 10){
    dateTime += "0";
  }
  dateTime+= String(t.sec);
  return dateTime;
}

void readTemp(){
  // only allowing the temp module to be read from every 5 seconds
  if ((millis() - lastTempRead) > 4500){
    int a = analogRead(pinTempSensor);
    float R = 1023.0/a-1.0;
    R = R0*R;
    float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet
    temperature = (temperature * 1.8) + 32;
    currentTemp = temperature;
    lastTempRead = millis();
  }
}

void writeDataToEEProm(byte targetValue){
  if (EEPROM.read(ROOMIDX_FIRST_BYTE) == targetValue){
    // Nothing to do since the value is already set.
    // We only want to write to EEPROM when it has actually
    // changed since we want to save EEPROM writes.
    return;
  }
  EEPROM.write(ROOMIDX_FIRST_BYTE, targetValue);
}
