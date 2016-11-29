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
#include "Arduino.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

SoftwareSerial debugStatus = SoftwareSerial(0, 7);

Process date;

Adafruit_7segment   primaryDisplay = Adafruit_7segment();
Adafruit_AlphaNum4  forecastDisplay[3];

String meridiem;
char displayChar0, displayChar1, displayChar2, displayChar3;
int hours, minutes, seconds, month, day, year;
int lastSecond = -1;

/*-------------------------------- Manual Input Data */
String weatherCondition[] = {"none", "none", "RAIN", "SNOW"};
String zipcode = "11211";
int temperature[] = {56, 57, 55, 56};

/*------------------------------------------------------*/




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
  for (int i = 0; i < 3; i++) {

    String temperatureString = String(temperature[i + 1]);

    forecastDisplay[i].writeDigitRaw(0, 0x0);
    forecastDisplay[i].writeDigitAscii(1, temperatureString.charAt(0));
    forecastDisplay[i].writeDigitAscii(2, temperatureString.charAt(1));
    forecastDisplay[i].writeDigitRaw(3, 0x0);
    forecastDisplay[i].writeDisplay();
    delay(500);
    if (weatherCondition[i + 1] != "none")  {
      forecastDisplay[i].writeDigitAscii(0, weatherCondition[i + 1].charAt(0));
      forecastDisplay[i].writeDigitAscii(1, weatherCondition[i + 1].charAt(1));
      forecastDisplay[i].writeDigitAscii(2, weatherCondition[i + 1].charAt(2));
      forecastDisplay[i].writeDigitAscii(3, weatherCondition[i + 1].charAt(3));
      forecastDisplay[i].writeDisplay();
      delay(500);
    }
  }
}

void updateDebugStatus() {
  // reset debugStatus cursor
  debugStatus.write(0xFE);
  debugStatus.write(0x48);
  delay(10);

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
  initDebugStatus();
  clearDisplays();
  fetchDate();
}

void loop() {
  updatePrimaryDisplay();
  updateForecastDisplay();
  updateDebugStatus();
}



