#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ds3231.h>
#include <SD.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

struct ts t;
const int pinTempSensor = A0;     // Grove - Temperature Sensor connect to A0
const int B = 4275;               // B value of the thermistor
const long R0 = 100000;            // R0 = 100k
const int DATA_BTN = 2;
const int DATA_LED = 3;
const int ROOM_BTN = 4;

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
// ROOM_COUNT must match number of rooms defined in allRooms array.
const int ROOM_COUNT = 7;
String allRooms[ROOM_COUNT]= {"basement","master","office","living","laundry","dining", "master bath"};
int currentRoomIdx = 0;
String currentRoom = allRooms[currentRoomIdx];
bool changeRoomBtnCurrent = false;
unsigned long lastTempReadMillis = 0;
float currentTemp = 0;
float prevTemp = 0;
byte writeFlag = 0;

SoftwareSerial SW_Serial(8, 9); // RX, TX

LiquidCrystal_I2C lcd(0x3F, 20, 4);

void setup() {
  lcd.init();
  lcd.begin(20,4);
  lcd.backlight();
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
  SW_Serial.begin(38400);
  
}

void loop() {
  displayDateTime();
  displayTemp();
  trySDCard();
  if (!isWritingData){
    // do not allow the room to change
    // (if data is being written),
    // even if user clicks button 
    checkChangeRoomButton();
    setRoom();
  }
  else{
    if (millis() - lastWriteTime > 5000){
      readTemp();
      // next line insures that the temp is only written
      // if it changed in the last 5 seconds
      if (currentTemp != prevTemp){
        writeTempData();
        Serial.print("writeFlag...");
        displayDataWrittenLcd();
      }
    }
  }
  checkWriteDataButton();
  // Handle BT Commands
  String command = "";
  while (SW_Serial.available()){
    command.concat((char)SW_Serial.read());
  }
  if (command != ""){
    Serial.println(command);
  }
  if (command == "getTemp"){
      SW_Serial.println(currentRoom + " : " + currentTemp);
  }
}

void displayDataWrittenLcd(){
  // a little indicator that data was written
  lcd.setCursor(18,2);
  if (writeFlag == 0){
    lcd.print("+");  
    writeFlag = 1;
  }
  else
  {
    lcd.print("-");
    writeFlag = 0;
  }
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
    dataFile.println(currentTemp);
    dataFile.close(); //Data isn't written until we run close()!
    Serial.print("writing data...");
   }
}

void checkChangeRoomButton(){
  roomBtnCurrent = debounce(changeRoomBtnCurrent, ROOM_BTN);
  if (roomBtnPrev == LOW && roomBtnCurrent == HIGH){
    changeRoomBtnCurrent = !changeRoomBtnCurrent;
  }
  roomBtnPrev = roomBtnCurrent;
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
  dataBtnPrev = dataBtnCurrent;
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
 
 lcd.print("SD ready");
 isSDCardInitialized = true;
}

void displayTemp(){
  if (millis() < 5000){
    // We don't want to read or display
    // the temp when the device boots up
    // because temp module isn't ready yet.
    return;
  }
  lcd.setCursor(0,3);
  lcd.print("F");
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
  if ((millis() - lastTempReadMillis) > 4500){
    int a = analogRead(pinTempSensor);
    float R = 1023.0/a-1.0;
    R = R0*R;
    float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet
    temperature = (temperature * 1.8) + 32;
    // store currentTemp in prevTemp for later use.
    prevTemp = currentTemp;
    currentTemp = temperature;
    lastTempReadMillis = millis();
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
