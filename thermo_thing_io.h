/************************** Configuration ***********************************/

// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients. Adafruit IO, Adafruit MQTT, and ArduinoHttpClient
#include "config.h"

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <SPI.h>
#include <ESP8266WiFi.h>

// Pin 13 for temp sensor - one wire bus
// Pin 12, 16 for relays

// Relay 1 to pin 12
int SSR1 = 12;
// Relay 2 to pin 16
int SSR2 = 16;
// variable to store the value coming from the sensors
int sensorValue1 = 0;
int sensorValue2 = 0;
// variable to store the desired temp
int setTemp1 = 0;
int setTemp2 = 0;
// store the current temp
float currentTemp = 0;
float rhaenyraTemp = 0;
float viserionTemp = 0;

/************************ Program Starts Here *******************************/
// set up rhaenyra feed
AdafruitIO_Feed *rhae_setTemp = io.feed("rhae_setTemp");
AdafruitIO_Feed *rhae_currTemp = io.feed("rhae_currTemp");

// set up viserion feed
AdafruitIO_Feed *vis_setTemp = io.feed("vis_setTemp");
AdafruitIO_Feed *vis_currTemp = io.feed("vis_currTemp");

/************ Dallas One-Wire Setup ******************/ 
// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2
 
// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
 
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

DeviceAddress rhaenyra = {null}; // get the sensors address and put it here
DeviceAddress viserion = {null};

void setup() {

  // start the serial connection
  Serial.begin(115200);

  // wait for serial monitor to open
  while(! Serial);
  
  // Start up the sensors
  sensors.begin();
  
  sensors.setResolution(rhaenyra, 10);
  sensors.setResolution(viserion, 10);
  
  pinMode(SSR1, OUTPUT);
  pinMode(SSR2, OUTPUT);
  digitalWrite(SSR1, LOW); // set the relay normally off
  digitalWrite(SSR2, LOW);

  Serial.print("Connecting to Adafruit IO");

  // connect to io.adafruit.com
  io.connect();

  // attach message handler for the rhae_setTemp feed.
  rhae_setTemp->onMessage(handle_setTemp);

  // attach the same message handler for the rhae_currTemp feed.
  rhae_currTemp->onMessage(handle_currTemp);

  // attach a new message handler for the vis_setTemp feed.
  vis_setTemp->onMessage(handle_setTemp);
  
  // attach a new message handler for the vis_currTemp feed.
  vis_currTemp->onMessage(handle_currTemp);

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());

}

// convert temperature from C to F  
float currentTemperature(DeviceAddress deviceAddress){
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC == -127.00){
	  serial.print("Error");
  }else{
	  currentTemp = (DallasTemperature::toFahrenheit(tempC));
  }
  return currentTemp;
}

void loop(){
  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();
  
  sensors.getDeviceCount();
  sensors.requestTemperatures(); // Send the command to get temperatures
  
  rhaenyraTemp = currentTemperature(rhaenyra);
  viserionTemp = currentTemperature(viserion);

}

// you can also attach multiple feeds to the same
// message handler function. both rhae_setTemp and vis_setTemp
// are attached to this callback function, and messages
// for both will be received by this function.
void handle_setTemp(AdafruitIO_Data *data) {

  Serial.print("received <- ");

  // since we are using the same function to handle
  // messages for two feeds, we can use feedName() in
  // order to find out which feed the message came from.
  Serial.print(data->feedName());
  Serial.print(" ");

  // print out the received value
  Serial.println(data->value());

}

// you can also attach multiple feeds to the same
// message handler function. both rhae_currTemp and vis_currTemp
// are attached to this callback function, and messages
// for both will be received by this function.
void handle_currTemp(AdafruitIO_Data *data) {

  Serial.print("received <- ");

  // since we are using the same function to handle
  // messages for two feeds, we can use feedName() in
  // order to find out which feed the message came from.
  Serial.print(data->feedName());
  Serial.print(" ");

  // print out the received value
  Serial.println(data->value());
}