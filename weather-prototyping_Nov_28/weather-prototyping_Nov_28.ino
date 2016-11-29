#include <Wire.h>
#include <SoftwareSerial.h>
#include <Process.h>
#include "Arduino.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

SoftwareSerial statusLCD = SoftwareSerial(0, 7);

Process date;

Adafruit_7segment   currentDisplay = Adafruit_7segment();
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

void setup() {
  Bridge.begin();
  // SerialUSB.begin(9600);

  // initialize current temperature LED display
  currentDisplay.begin(0x70); // set address
  currentDisplay.setBrightness(10); // set brightness, 1-15
  currentDisplay.writeDigitRaw(0, 0x0); // turn off
  currentDisplay.writeDigitRaw(1, 0x0);
  currentDisplay.writeDigitRaw(3, 0x0); // location #2 is ":"
  currentDisplay.writeDigitRaw(4, 0x0);
  currentDisplay.writeDisplay();

  // initialize forecast temperature LED displays
  for (int i = 0; i < 3; i++) {
    forecastDisplay[i].begin(0x71 + i); // assign I2C address to initialize display
    forecastDisplay[i].setBrightness(1); // set brightness, 1-15
    forecastDisplay[i].writeDigitRaw(i, 0x0); // turn off 
    forecastDisplay[i].writeDisplay();  
  }

  // start LCD
  statusLCD.begin(9600);

  // set the contrast, recommended 180-220
  statusLCD.write(0xFE);
  statusLCD.write(0x50);
  statusLCD.write(200);
  delay(10);

  // set the brightness 0-255
  statusLCD.write(0xFE);
  statusLCD.write(0x99);
  statusLCD.write(50);
  delay(10);

  // set RGB background color
  statusLCD.write(0xFE);
  statusLCD.write(0xD0);
  statusLCD.write((uint8_t)0xFF); // set to orange-red #FF4500
  statusLCD.write((uint8_t)0x45);
  statusLCD.write((uint8_t)0x00);
  delay(10);

  // run intial process to get time and date
  if (!date.running())  {
    date.begin("date");
    date.addParameter("+%T-%D");
    date.run();
  }
}

void loop() {

  currentDisplay.writeDigitRaw(0, 0x0);
  currentDisplay.writeDigitNum(1, temperature[0]/10);
  currentDisplay.writeDigitNum(3, temperature[0]%10); 
  currentDisplay.writeDigitRaw(4, 0x0);
  currentDisplay.writeDisplay();   

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

  // reset statusLCD cursor
  statusLCD.write(0xFE);
  statusLCD.write(0x48);
  delay(10);

  // print current time
  if (lastSecond != seconds) {
    if (hours < 12) {
      if (hours <= 9) {
        statusLCD.print("0");
      }
      statusLCD.print(hours);
      meridiem = "A";
    } else  {
      if (hours - 12 <= 9)  {
        statusLCD.print("0");
      }
      statusLCD.print(hours - 12);
      meridiem = "P";
    }
    statusLCD.print(":");
    if (minutes <= 9) {
      statusLCD.print("0");
    }
    statusLCD.print(minutes);
    statusLCD.print(meridiem);

    // restart the date process:
    if (!date.running()) {
      date.begin("date");
      date.addParameter("+%T-%D");
      date.run();
    }
  }

  // print current date
  statusLCD.print(" ");
  statusLCD.print(month);
  statusLCD.print("/");
  statusLCD.print(day);
  statusLCD.print("/");
  statusLCD.println(year);
  statusLCD.print("ZIP Code: ");
  statusLCD.print(zipcode);
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
