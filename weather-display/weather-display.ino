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
#include <ArduinoJson.h>
#include "Arduino.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

SoftwareSerial debugStatus = SoftwareSerial(0, 7);

Process date;
//HttpClient client;

unsigned long time;
unsigned long lastFetchTime = 0;


const unsigned int FETCH_RATE_LIMIT = 60000; // once per minute
//const unsigned int FETCH_RATE_LIMIT = 1800000; // once per half hour

const int NUM_FORECAST_DISPLAYS = 3;

// API
//const String YAHOO_API_CLIENT = "dj0yJmk9UVpZOWFEdUxRaHFxJmQ9WVdrOVQwUk5NMlJYTkdVbWNHbzlNQS0tJnM9Y29uc3VtZXJzZWNyZXQmeD03YQ--";
//const String YAHOO_API_SECRET = "7a9fb4c7a538a785daae5c90dc686f311103ec58";
const String WUNDER_API_KEY = "d289dcec22b2633c";
const String WUNDER_URL = "http://api.wunderground.com/api/db33644a6938c385/hourly/q/NY/New_York.json";

// JSON
const size_t MAX_HTTP_CONTENT_SIZE =  JSON_ARRAY_SIZE(36) + JSON_OBJECT_SIZE(1) + 361*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 36*JSON_OBJECT_SIZE(20) + 36*JSON_OBJECT_SIZE(27); // 64Kb
const size_t JSON_BUFFER_SIZE =  JSON_ARRAY_SIZE(36) + JSON_OBJECT_SIZE(1) + 361*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 36*JSON_OBJECT_SIZE(20) + 36*JSON_OBJECT_SIZE(27);
StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;

Adafruit_7segment   primaryDisplay = Adafruit_7segment();
Adafruit_AlphaNum4  forecastDisplay[NUM_FORECAST_DISPLAYS];

String meridiem;
char displayChar0, displayChar1, displayChar2, displayChar3;
int hours, minutes, seconds, month, day, year;
int lastSecond = -1;
int lastFetch = -1;


/*-------------------------------- Manual Input Data */
String weatherCondition[] = {"none", "none", "RAIN", "SNOW"};
String zipcode = "11211";
int temperature[] = {56, 57, 55, 56};

/*------------------------------------------------------*/

struct HourlyData {
  char epoch[11];
  char hour[2];
  char temperature[3];
  char temperatureMetric[3];
  char condition[30];
};

void resetDebugStatusCursor() {
  // reset debugStatus cursor
  debugStatus.write(0xFE);
  debugStatus.write(0x48);
  delay(10);
}

void fetchForecast() {

  HttpClient client;
  String forecast;
  
  if (time - FETCH_RATE_LIMIT > lastFetchTime) {
    lastFetchTime = time;
    //client.get("https://query.yahooapis.com/v1/public/yql -d q=\"select%20item.forecast%20from%20weather.forecast%20where%20woeid%20in%20(select%20woeid%20from%20geo.places(1)%20where%20text=%22brooklyn,%20ny%22)\" -d format=json");  
    client.get("http://api.wunderground.com/api/db33644a6938c385/hourly/q/NY/New_York.json");
    //resetDebugStatusCursor();
    Serial.println("Fetching weather...");
    delay(2000);
  }
  
  if (client.available() > 0) {
    char response[MAX_HTTP_CONTENT_SIZE];
    size_t length = client.readBytes(response, MAX_HTTP_CONTENT_SIZE);
    response[length] = 0; // end string
    Serial.println(response);

    HourlyData hourlyData;
    if (parseHourlyData(response, &hourlyData, 0)) {
      Serial.println("done until next request.");
    }
    
  }

  Serial.flush();
}

bool parseHourlyData(char* content, struct HourlyData* hourlyData, int hourOffset) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  // This is only required if you use StaticJsonBuffer.
  
  // Allocate a temporary memory pool on the stack
  

  JsonObject& root = jsonBuffer.parseObject(content);

  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }

  
  // Here were copy the strings we're interested in
  strcpy(hourlyData->epoch, root["hourly_forecast"][hourOffset]["FCTTIME"]["epoch"]);
  strcpy(hourlyData->hour, root["hourly_forecast"][hourOffset]["FCTTIME"]["hour"]);
  strcpy(hourlyData->temperature, root["hourly_forecast"][hourOffset]["temp"]["english"]);
  strcpy(hourlyData->temperatureMetric, root["hourly_forecast"][hourOffset]["temp"]["metric"]);
  strcpy(hourlyData->condition, root["hourly_forecast"][hourOffset]["condition"]);

  Serial.println("Parse Hourly Data success!");
  
  String demo = "";
  demo.concat("Hour: ");
  demo.concat(hourlyData->hour);
  demo.concat(" Temp: ");
  demo.concat(hourlyData->temperature);
  demo.concat("º F ");
  demo.concat("'");
  demo.concat(hourlyData->condition);
  demo.concat("'");
  
  Serial.println(demo);
  // It's not mandatory to make a copy, you could just use the pointers
  // Since, they are pointing inside the "content" buffer, so you need to make
  // sure it's still in memory when you read the string

  return true;
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
  for (int i = 0; i < 3; i++) {
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
  primaryDisplay.writeDigitRaw(0, 0x1);
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
    forecastDisplay[i].writeDigitAscii(2, temperatureString.charAt(1));
    forecastDisplay[i].writeDigitRaw(3, 0x0);
    forecastDisplay[i].writeDisplay();
    delay(100);
    if (weatherCondition[i + 1] != "none")  {
      forecastDisplay[i].writeDigitAscii(0, weatherCondition[i + 1].charAt(0));
      forecastDisplay[i].writeDigitAscii(1, weatherCondition[i + 1].charAt(1));
      forecastDisplay[i].writeDigitAscii(2, weatherCondition[i + 1].charAt(2));
      forecastDisplay[i].writeDigitAscii(3, weatherCondition[i + 1].charAt(3));
      forecastDisplay[i].writeDisplay();
      delay(100);
    }
  }
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
  SerialUSB.begin(9600);
  Serial.begin(9600);
  
  while(!Serial);
  
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
  
  fetchForecast();
  //updateDebugStatus();

  delay(1000);
}



