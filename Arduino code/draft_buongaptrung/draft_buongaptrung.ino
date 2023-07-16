#include "DHT.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR 0x27
#define LCD_COLUMNS 16
#define LCD_LINES 2
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

#define DHTPIN 11 // Chân kết nối cảm biến DHT
#define DHTTYPE DHT11 // Loại cảm biến DHT

const int redLed = 13; // Chân đèn LED kết nối với chân 13 trên Arduino
const int buzzerPin = 12; // Chân buzzer kết nối với chân 12 trên Arduino

DHT dht(DHTPIN, DHTTYPE);

// Thời gian ấp trứng
static uint8_t dayTimeToHatch;
static uint8_t hourTimeToHatch;
static uint8_t minTimeToHatch;

// Nhiệt độ ấp trứng
static uint8_t setTemp = 20;
static float currentTemp; // currentTemp = dht.readTemperature()

// Trạng thái quá trình [0]: chờ, [1]: ấp, [2]: tiếp tục, [3]: set time hatch.
static uint8_t prState = 0;

uint8_t state = 0;

#define millisWaiting 1000 // 1s
#define millisHatching 1000 // 1s
#define millisBuzzer 10005 // 5s
#define millisAlarm 100025 // 25s

void updateDisplayTimeToHatch(uint8_t day, uint8_t hour, uint8_t min) {
lcd.setCursor(0, 0);
lcd.print("TimeHatch:");
if (day <= 9) {
lcd.print("0");
}
lcd.print(day);
lcd.print("d ");
if (hour <= 9) {
lcd.print("0");
}
lcd.print(hour);
lcd.print("h ");
if (min <= 9) {
lcd.print("0");
}
lcd.print(min);
lcd.print("m");
}

void updateDisplayTemp() {
lcd.setCursor(0, 1);
lcd.print("Temp: ");
lcd.print((uint8_t)dht.readTemperature());
lcd.print(" C");
}

void tempAdjusting() {
currentTemp = dht.readTemperature();

if (currentTemp >= setTemp + 1) {
digitalWrite(redLed, LOW);
} else if (currentTemp <= setTemp - 1) {
digitalWrite(redLed, HIGH);
}
}

void sendData_th() {
float currentTemp = dht.readTemperature();
float currentHumidity = dht.readHumidity();

if (isnan(currentTemp) || isnan(currentHumidity)) {
Serial.println("Failed to read from DHT");
} else {
Serial.print(currentTemp);
Serial.print(",");
Serial.print(currentHumidity);
Serial.print(",");
}
}

uint8_t processInputState(String input) {
input.trim(); // Loại bỏ khoảng trắng đầu và cuối chuỗi

if (input.startsWith("setTemp")) {
int setTempIndex = input.indexOf("setTemp") + 7;
setTemp = input.substring(setTempIndex).toInt();
return state; // Trả về giá trị trạng thái hiện tại
}

if (input.startsWith("prState")) {
int prStateIndex = input.indexOf("prState") + 7;
state = input.substring(prStateIndex).toInt();
return state;
}
}

void setup() {
Serial.begin(9600);
lcd.begin(LCD_COLUMNS, LCD_LINES);
lcd.backlight();
dht.begin();

pinMode(redLed, OUTPUT);
pinMode(buzzerPin, OUTPUT);
}

void loop() {
sendData_th();
Serial.println();
uint8_t day;
uint8_t hour;
uint8_t min;
uint8_t resetStatus = 0;

if (Serial.available()) {
String input = Serial.readString();
input.trim(); // Loại bỏ khoảng trắng đầu và cuối chuỗi

scss
Copy code
if (input.startsWith("dayTimeToHatch")) {
  int dayTimeIndex = input.indexOf("dayTimeToHatch") + 14;
  int hourTimeIndex = input.indexOf("hourTimeToHatch") + 15;
  int minuteTimeIndex = input.indexOf("minuteTimeToHatch") + 17;
  int prStateIndex = input.indexOf("prState") + 7;

  day = input.substring(dayTimeIndex, hourTimeIndex).toInt();
  hour = input.substring(hourTimeIndex, minuteTimeIndex).toInt();
  min = input.substring(minuteTimeIndex, prStateIndex).toInt();
  state = input.substring(prStateIndex).toInt();
}

if (input.startsWith("prState")) {
  int prStateIndex = input.indexOf("prState") + 7;
  state = input.substring(prStateIndex).toInt();
}

if (input.startsWith("Reset")) {
  day = 0;
  hour = 0;
  min = 0;
  updateDisplayTimeToHatch(day, hour, min);
  updateDisplayTemp();
  tempAdjusting();
}

if (input.startsWith("setTemp")) {
  state = processInputState(input);
  tempAdjusting();
}

unsigned long previousMillis = millis();
unsigned long interval = 1000; // Đơn vị milliseconds (1 giây)

while (state == 1) {
  if (Serial.available()) {
    String inputState = Serial.readString();
    state = processInputState(inputState);
    if (inputState.startsWith("Reset")) resetStatus = 1;
  }
  if (resetStatus) {
    day = 0;
    hour = 0;
    min = 0;
    updateDisplayTimeToHatch(0, 0, 0);
    updateDisplayTemp();
    tempAdjusting();
    break;
  }
  if (state == 0) {
    break;
  }

  unsigned long currentMillis = millis();

  // Gửi tín hiệu nhiệt độ và độ ẩm về Qt liên tục sau mỗi 2 giây
  sendData_th();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (min == 0) {
      if (hour == 0) {
        if (day == 0) {
          state = 2;
          break;
        } else {
          day--;
          hour = 23;
          min = 59;
        }
      } else {
        hour--;
        min = 59;
      }
    } else {
      min--;
    }

    // Cập nhật hiển thị trên LCD
    updateDisplayTimeToHatch(day, hour, min);
    updateDisplayTemp();
    tempAdjusting();

    Serial.print(day);
    Serial.print(",");
    Serial.print(hour);
    Serial.print(",");
    Serial.println(min);
    delay(2000);
  }
}
}

delay(3000);
}
