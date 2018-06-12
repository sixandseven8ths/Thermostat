#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

// Pin 13 for temp sensor - one wire bus
// Pins 12, 16 for relays

// Relay 1 to pin 12
int SSR1 = 12;
// Relay 2 to pin 16
int SSR2 = 16;
int sensorValue1 = 0;  // variable to store the value coming from the sensor
int sensorValue2 = 0;
int setTemp1 = 0; // variable to store the desired temp
int setTemp2 = 0;
float currentTemp = 0; // store the current temp
float rhaenyraTemp = 0;
float viserionTemp = 0;

/************************* WiFi Setup *****************************/
#define WLAN_SSID 	"ilovetoast"
#define WLAN_PASS	"obiwankenobi"

/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "sixandseven8ths"
#define AIO_KEY         "28884aba271d410ca57353d20726fe9c"

/************ Global State (you don't need to change this!) ******************/
//Set up the WiFi client
WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/
// Setup a feed for subscribing
const char RHAE_SET_TEMP[] PROGMEM = AIO_USERNAME "/feeds/rhae_setTemp";
Adafruit_MQTT_Subscribe rhae_setTemp = Adafruit_MQTT_Subscribe(&mqtt, RHAE_SET_TEMP);

const char VIS_SET_TEMP[] PROGMEM = AIO_USERNAME "/feeds/vis_setTemp";
Adafruit_MQTT_Subscribe vis_setTemp = Adafruit_MQTT_Subscribe(&mqtt, VIS_SET_TEMP);

// Setup a feed for publishing
const char RHAE_CURR_TEMP[] PROGMEM = AIO_USERNAME "/feeds/rhae_currTemp";
Adafruit_MQTT_Publish rhae_currTemp = Adafruit_MQTT_Publish(&mqtt, RHAE_CURR_TEMP);

const char VIS_CURR_TEMP[] PROGMEM = AIO_USERNAME "/feeds/vis_currTemp";
Adafruit_MQTT_Publish vis_currTemp = Adafruit_MQTT_Publish(&mqtt, VIS_CURR_TEMP);

/************ Dallas One-Wire Setup ******************/ 
// Data wire is plugged into pin 3 on the Arduino
#define ONE_WIRE_BUS 3
 
// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
 
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

DeviceAddress rhaenyra = {null}; // get the sensors address and put it here
DeviceAddress viserion = {null};

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();
 
void setup()
{ 
	Serial.begin(115200);

  // Start up the library
  sensors.begin();
  
  sensors.setResolution(rhaenyra, 10);
  sensors.setResolution(viserion, 10);
  
  pinMode(SSR1, OUTPUT); // configure relay 1 as output pin
  pinMode(SSR2, OUTPUT); // configure relay 2 as output pin
  digitalWrite(SSR1, LOW); // set the relay normally off
  digitalWrite(SSR2, LOW); // set the relay normally off
  
	// Connect to WiFi access point
	Serial.println(); Serial.println();
	Serial.print("Connecting to ");
	Serial.println(WLAN_SSID);
	
	WiFi.begin(WLAN_SSID, WLAN_PASS);
	while (WiFI.status() != WL_CONNECTED){
		delay(500);
		Serial.println(".");
	}
	Serial.println();
	
	Serial.println("WiFi connected");
	Serial.println("IP address: "); Serial.println(WiFi.localIP());
  
  // setup MQTT subscription for the feeds
  mqtt.subscribe(&rhae_setTemp);
  mqtt.subscribe(&vis_setTemp);
}

uint32_t x=0;

// convert temperature from C to F  
  float currentTemperature(DeviceAddress deviceAddress){
	  float tempC = sensors.getTempC(deviceAddress);
	  if (tempC == -127.00){
		  lcd.print("Error");
	  }else{
		  currentTemp = (DallasTemperature::toFahrenheit(tempC));
	  }
	  return currentTemp;
  }
  
  void loop(){
  
	  // Ensure the connection to the MQTT server is alive (this will make the first
      // connection and automatically reconnect when disconnected).  See the MQTT_connect
      // function definition further below.
	  MQTT_connect();
	  
	  // this is our 'wait for incoming subscription packets' busy subloop
	  // set temps
	  Adafruit_MQTT_Subscribe *subscription;
	  
	  rhaenyraTemp = currentTemperature(rhaenyra); // current temp
	  viserionTemp = currentTemperature(viserion);
	  
	  while ((subscription = mqtt.readSubscription(5000)))
	  {
		  if (subscription == &rhae_setTemp) // Rhaenyra
		  {
			  Serial.print(F("Rhaenyra: "));
			  Serial.println((char *)rhae_setTemp.lastread);
			  
			  uint16_t setTemp1 = atoi((char *)rhae_setTemp.lastread); // convert to a number
			  
			  if (rhaenyraTemp > setTemp1){
				  digitalWrite(SSR1, LOW);
			  }elseif (rhaenyraTemp < setTemp1){
				  digitalWrite(SSR1, HIGH);
			  }
		  }
		  
		  if (subscription == &vis_setTemp) // Viserion
		  {
			  Serial.print(F("Viserion: "));
			  Serial.println((char *)vis_setTemp.lastread);
			  
			  uint16_t setTemp2 = atoi((char *)vis_setTemp.lastread);			  	  
			  
			  if (viserionTemp > setTemp2){
				  digitalWrite(SSR2, LOW);
			  }elseif (viserionTemp < setTemp2){
				  digitalWrite(SSR2, HIGH);
			  }
		  }
	  }
	  
	  // Now we can publish stuff - current temps
	  if (! rhae_currTemp.publish(rhae_temp))
		  Serial.println(F("Failed to publish temperature"));
	  else
		  Serial.println(F("Temperature published!"));
	  
	  if (! vis_currTemp.publish(vis_temp))
		  Serial.println(F("Failed to publish temperature"));
	  else
		  Serial.println(F("Temperature published!"));
	  
	  sensors.getDeviceCount();
	  sensors.requestTemperatures(); // Send the command to get temperatures
  
// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care of connecting.
void MQTT_connect() {
  int8_t ret;

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    uint8_t timeout = 10;
    while (timeout && (WiFi.status() != WL_CONNECTED)) {
      timeout--;
      delay(1000);
    }
  }
  
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");