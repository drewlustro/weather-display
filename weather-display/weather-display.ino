/**
   weather-display.ino
   weather-display
   @description Runtime for weather display prototype
   @author Drew Lustro, Jesse Chorng
   @copyright 2016 Maxrelax Inc
*/

#include <Wire.h>
#include <SoftwareSerial.h>
#include <Process.h>
#include <Bridge.h>
#include <HttpClient.h>
#include "Arduino.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#define arr_len( x )  ( sizeof( x ) / sizeof( *x ) )

SoftwareSerial debugStatus = SoftwareSerial(0, 7);

Process date;
HttpClient client;

unsigned long time;
unsigned long lastFetchTime = 0;


const unsigned int FETCH_RATE_LIMIT = 60000; // once per minute
//const unsigned int FETCH_RATE_LIMIT = 1800000; // once per half hour

const int NUM_FORECAST_DISPLAYS = 3;

// API
const String WUNDER_API_KEY = "d289dcec22b2633c";
const String WUNDER_URL = "http://api.wunderground.com/api/db33644a6938c385/hourly/q/NY/New_York.json";


Adafruit_7segment   primaryDisplay = Adafruit_7segment();
Adafruit_AlphaNum4  forecastDisplay[NUM_FORECAST_DISPLAYS];

String meridiem;
char displayChar0, displayChar1, displayChar2, displayChar3;
int hours, minutes, seconds, month, day, year;
int lastFetch = -1;
int lastSecond = -1;

/*-------------------------------- Manual Input Data */
String summary[] = {"XXXX", "XXXX", "XXXX", "XXXX"};
int temperature[] = {0, 0, 0, 0};
int desiredOffsets[] = {0, 3, 7, 11}; // current, 4hrs, 8hrs, 12hrs
String zipcode = "11211";

/*------------------------------------------------------*/

struct HourlyData {
  double temperature;
  unsigned int hourOffset;
  unsigned long epochTime; 
  char summary[40];
};

void resetDebugStatusCursor() {
  // reset debugStatus cursor
  debugStatus.write(0xFE);
  debugStatus.write(0x48);
  delay(10);
}

String getStringPartByIndex(String data, char separator, int index) {

    int stringData = 0; 
    String dataPart = "";     
    
    for (int i = 0; i < data.length(); i++) {  
      
      if (data[i] == separator) {
        stringData++;
        
      } else if (stringData == index) {
        dataPart.concat(data[i]);
        
      } else if (stringData > index) {
        return dataPart;
      }

    }
    return dataPart;
}

bool isDesiredHourOffset(int offset) {
  for (int i = 0; i < arr_len(desiredOffsets); i++) {
    if (offset == desiredOffsets[i]) {
      return true;
    }
  }
  return false;
}

void fetchForecast() {

  
  String forecast;
  
  if (time - FETCH_RATE_LIMIT > lastFetchTime) {
    lastFetchTime = time;
    client.get("http://weather-proxy.maxrelax.co/weather.csv");
    Serial.println("Fetching weather...");
    Serial.flush();

    delay(500);
  }

  String hourDataRaw;
  int storeIndex = 0;
  
  while (client.available()) {
    
    hourDataRaw = client.readStringUntil('\n');  
    int offset = getStringPartByIndex(hourDataRaw, ',', 1).toInt();
    
    if (isDesiredHourOffset(offset)) {
    
      String temp = getStringPartByIndex(hourDataRaw, ',', 0);
      String epochTime = getStringPartByIndex(hourDataRaw, ',', 2);
      String condition = getStringPartByIndex(hourDataRaw, ',', 3);
  
  
      temperature[storeIndex] = temp.toInt();
      summary[storeIndex] = condition;
  
      Serial.println("HOUR " + String(offset) + ": " + temperature[storeIndex] + " /  " + summary[storeIndex]);
      Serial.flush();

      storeIndex++;
    }

    delay(50);
  }
  Serial.flush();
}


void initDebugStatus() {
  // start LCD
  debugStatus.begin(9600);

  // set the contrast, recommended 180-220
  debugStatus.write(0xFE);
  debugStatus.write(0x50);
  debugStatus.write(200);
  delay(10);

  // set the brightness 0-255
  debugStatus.write(0xFE);
  debugStatus.write(0x99);
  debugStatus.write(50);
  delay(10);

  // set RGB background color
  debugStatus.write(0xFE);
  debugStatus.write(0xD0);
  debugStatus.write((uint8_t)0xFF); // set to orange-red #FF4500
  debugStatus.write((uint8_t)0x45);
  debugStatus.write((uint8_t)0x00);
  delay(10);
}

void clearDisplays() {

  primaryDisplay.begin(0x70); // set address
  primaryDisplay.setBrightness(10); // set brightness, 1-15
  primaryDisplay.writeDigitRaw(0, 0x0); // turn off
  primaryDisplay.writeDigitRaw(1, 0x0);
  primaryDisplay.writeDigitRaw(3, 0x0); // location #2 is ":"
  primaryDisplay.writeDigitRaw(4, 0x0);
  primaryDisplay.writeDisplay();

  // FORECAST
  for (int i = 0; i < NUM_FORECAST_DISPLAYS; i++) {
    forecastDisplay[i].begin(0x71 + i); // assign I2C address to initialize display
    forecastDisplay[i].setBrightness(1); // set brightness, 1-15
    forecastDisplay[i].writeDigitRaw(i, 0x0); // turn off
    forecastDisplay[i].writeDisplay();
  }

}

void fetchDate() {
  if (!date.running())  {
    date.begin("date");
    date.addParameter("+%T-%D");
    date.run();
  }
}

void updatePrimaryDisplay() {
  primaryDisplay.writeDigitRaw(0, 0x0);
  primaryDisplay.writeDigitNum(1, temperature[0] / 10);
  primaryDisplay.writeDigitNum(3, temperature[0] % 10);
  primaryDisplay.writeDigitRaw(4, 0x0);
  primaryDisplay.writeDisplay();
}

void updateForecastDisplay() {
  for (int i = 0; i < NUM_FORECAST_DISPLAYS; i++) {

    String temperatureString = String(temperature[i + 1]);

    forecastDisplay[i].writeDigitRaw(0, 0x0);
    forecastDisplay[i].writeDigitAscii(1, temperatureString.charAt(0));
    if (temperature[i + 1] > 9 || temperature[i + 1] < 0) {
      forecastDisplay[i].writeDigitAscii(2, temperatureString.charAt(1));
    }
    forecastDisplay[i].writeDigitRaw(3, 0x0);
    forecastDisplay[i].writeDisplay();
  }

  delay(3000);
  
  for (int i = 0; i < NUM_FORECAST_DISPLAYS; i++) {

    String temperatureString = String(temperature[i + 1]);

    if (summary[i + 1] != "none" || temperature[i+1] == 0)  {
      forecastDisplay[i].writeDigitAscii(0, summary[i + 1].charAt(0));
      forecastDisplay[i].writeDigitAscii(1, summary[i + 1].charAt(1));
      forecastDisplay[i].writeDigitAscii(2, summary[i + 1].charAt(2));
      forecastDisplay[i].writeDigitAscii(3, summary[i + 1].charAt(3));
      forecastDisplay[i].writeDisplay();
      delay(100);
    }
  }

  delay(2000);
}

void updateDebugStatus() {
  
  resetDebugStatusCursor();
  
  // print current time
  if (lastSecond != seconds) {
    if (hours < 12) {
      if (hours <= 9) {
        debugStatus.print("0");
      }
      debugStatus.print(hours);
      meridiem = "A";
    } else  {
      if (hours - 12 <= 9)  {
        debugStatus.print("0");
      }
      debugStatus.print(hours - 12);
      meridiem = "P";
    }
    debugStatus.print(":");
    if (minutes <= 9) {
      debugStatus.print("0");
    }
    debugStatus.print(minutes);
    debugStatus.print(meridiem);

    // restart the date process:
    if (!date.running()) {
      date.begin("date");
      date.addParameter("+%T-%D");
      date.run();
    }
  }

  // print current date
  debugStatus.print(" ");
  debugStatus.print(month);
  debugStatus.print("/");
  debugStatus.print(day);
  debugStatus.print("/");
  debugStatus.println(year);
  debugStatus.print("ZIP Code: ");
  debugStatus.print(zipcode);
  delay(10);

  while (date.available() > 0)  {
    String dateString = date.readString();

    int firstColon = dateString.indexOf(":");
    int secondColon = dateString.lastIndexOf(":");
    int firstSlash = dateString.indexOf("/");
    int secondSlash = dateString.lastIndexOf("/");

    String hourString = dateString.substring(0, firstColon);
    String minString = dateString.substring(firstColon + 1, secondColon);
    String secString = dateString.substring(secondColon + 1);
    String monthString = dateString.substring(firstSlash - 2, firstSlash);
    String dayString = dateString.substring(firstSlash + 1, secondSlash);
    String yearString = dateString.substring(secondSlash + 1);

    hours = hourString.toInt();
    minutes = minString.toInt();
    lastSecond = seconds;
    seconds = secString.toInt();
    month = monthString.toInt();
    day = dayString.toInt();
    year = yearString.toInt();
  }
}


/*------------------------------------------------------*/

void setup() {
  Bridge.begin();
  //SerialUSB.begin(9600);
  Serial.begin(9600);
  
  while(!Serial);
  //while(!SerialUSB);
  
  
  time = millis() + FETCH_RATE_LIMIT; // initialize time
  lastFetchTime = 0;
  initDebugStatus();
  clearDisplays();
  fetchDate();
  
}

void loop() {
  time = millis() + FETCH_RATE_LIMIT;
  updatePrimaryDisplay();
  updateForecastDisplay();
  Serial.println("1 sec...");
  Serial.flush();
  fetchForecast();
}



