#include <Arduino.h>
#include <Ethernet2.h> 

#include <SPI.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timer.h>
#include <Thingplus.h>

#include <SimpleDHT.h>
SimpleDHT11 dht11;

#define CONNECT_LED_GPIO  LED_BUILTIN // Thing+ connection indicator LED

#if defined(ARDUINO_ARCH_SAMD) // Atmel ARM Cortex core based MCU series
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif

/////////////////////////////////////////////////////////////////////////////
byte mac[6] = {0x00, 0x08, 0xdc, 0x52, 0xda, 0x32}; 			//FIXME MAC ADDRESS
const char *apikey = "C265GEoOibV5-tXst7Xbn8zJ_gI=";  																//FIXME APIKEY
const char *ledId = "led-0008dc52da32-0";									//FIXME LED ID
const char *temperatureId = "temperature-0008dc52da32-0";	//FIXME TEMPERATURE ID
/////////////////////////////////////////////////////////////////////////////

Timer t;

int ledOffTimer = 0;
int ledBlinkTimer = 0;

int LED_GPIO = 9;
int DHT11_GPIO = 4;
int reportIntervalSec = 60;

static EthernetClient ethernetClient;

byte temperature = 0, humidity = 0;

static void _serialInit(void)
{
	Serial.begin(115200);
	while (!Serial);// wait for serial port to connect.
	Serial.println();
}

static void _ethernetInit(void) {
	Ethernet.begin(mac);		
	Serial.print("[INFO] IP:");
	Serial.println(Ethernet.localIP());
}

static void _gpioInit(void) {  
	pinMode(LED_GPIO, OUTPUT);

  // Thingplus connection indicator LED
  pinMode(CONNECT_LED_GPIO, OUTPUT);
  digitalWrite(CONNECT_LED_GPIO, LOW);
}

static void _ledOff(void) {
	t.stop(ledBlinkTimer);
	digitalWrite(LED_GPIO, LOW);
}

char* actuatingCallback(const char *id, const char *cmd, JsonObject& options) {
	if (strcmp(id, ledId) == 0) {
		t.stop(ledBlinkTimer);
		t.stop(ledOffTimer);

		if (strcmp(cmd, "on") == 0) {
			int duration = options.containsKey("duration") ? options["duration"] : 0;

			digitalWrite(LED_GPIO, HIGH);

			if (duration)
				ledOffTimer = t.after(duration, _ledOff);

			return "success";
		}
		else  if (strcmp(cmd, "off") == 0) {
			_ledOff();
			return "success";
		}
		else  if(strcmp(cmd, "blink") == 0) {
			if (!options.containsKey("interval")) {
				Serial.println(F("[ERR] No blink interval"));
				return NULL;
			}

			ledBlinkTimer = t.oscillate(LED_GPIO, options["interval"], HIGH);

			if (options.containsKey("duration"))
				ledOffTimer = t.after(options["duration"], _ledOff);

			return "success";
		}
		else {
			return NULL;
		}
	}
 
	return NULL;
}

void setup() {
	_serialInit();  
	_ethernetInit();
	_gpioInit();
  
	Thingplus.begin(ethernetClient, mac, apikey);
	Thingplus.actuatorCallbackSet(actuatingCallback);

	Thingplus.connect();
  
  digitalWrite(CONNECT_LED_GPIO, HIGH); // LED on: Thing+ connection indicator
}

time_t current;
time_t nextReportInterval = now();

// Temperature & Humidity
void DHT11Get() {
  dht11.read(DHT11_GPIO, &temperature, &humidity, NULL);
}

void loop() {
	Thingplus.loop();
	t.update();

	current = now();

	if (current > nextReportInterval) {

    DHT11Get();
  
		Thingplus.gatewayStatusPublish(true, reportIntervalSec * 3);

		Thingplus.sensorStatusPublish(ledId, true, reportIntervalSec * 3);
    
		Thingplus.sensorStatusPublish(temperatureId, true, reportIntervalSec * 3);
		Thingplus.valuePublish(temperatureId, temperature);

		nextReportInterval = current + reportIntervalSec;
	}
}
