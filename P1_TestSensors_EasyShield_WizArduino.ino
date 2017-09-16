#include <Arduino.h>
#include <Time.h>
#include <TimeLib.h>
#include <SimpleDHT.h>

#if defined(ARDUINO_ARCH_SAMD) // Atmel ARM Cortex core based MCU series
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif

#define LIGHT_THRESHOLD 700

SimpleDHT11 dht11;

int DHT11_GPIO = 4;
int LED_GPIO = 9;
int BUZZER_GPIO = 5;
int LIGHT_GPIO = A1;

int reportIntervalMSec = 1000; // 1000ms = 1sec

// Buzzer
#define NUMTONES 2
int buzzerTones[NUMTONES] = {700, 0};
int buzzerIntervalMSec = 200;
int currentTone = 0;
bool flag_buzzer_on = false;
time_t currentMillis;
time_t nextBuzzerInterval = millis();

void setup()
{
  Serial.begin(115200);
  while(!Serial);
  
  pinMode(LED_GPIO, OUTPUT);
}

time_t current;
time_t nextReportInterval = millis();

void loop()
{
  int err;
  byte temperature, humidity;
  int lightVal;
  
  current = millis();
  if (current > nextReportInterval) {
        
    // CDS: Light
    lightVal = analogRead(LIGHT_GPIO);
    Serial.print("Light = ");
    Serial.print(lightVal);
   
    Serial.print(", LED_R State: ");
    if(lightVal > LIGHT_THRESHOLD)
    {
      digitalWrite(LED_GPIO, LOW);
      Serial.println("OFF");
      
      flag_buzzer_on = false;
      analogWrite(BUZZER_GPIO, 0);
    }
    else
    {
      digitalWrite(LED_GPIO, HIGH);
      Serial.println("ON");
      flag_buzzer_on = true;
    }
  
    // DHT11: Temperature & Humidity
    if((err=dht11.read(DHT11_GPIO, &temperature, &humidity, NULL))==0)
    {
      Serial.print("temperature:");
      Serial.print(temperature);
      Serial.print("'C");
      Serial.print(" / humidity:");
      Serial.print(humidity);    
      Serial.println("%");
      Serial.println();
    }
    else
    {
      Serial.println();
      Serial.print("Error No :");
      Serial.print(err);
      Serial.println();    
    }    
    nextReportInterval = current + reportIntervalMSec;
  }

  // Buzzer
  if(flag_buzzer_on == true) {
     currentMillis = current;
     if(currentMillis > nextBuzzerInterval) {
       analogWrite(BUZZER_GPIO, buzzerTones[currentTone++]);
       
       if(currentTone >= NUMTONES) currentTone = 0;
       nextBuzzerInterval = currentMillis + buzzerIntervalMSec;
     }
  } else {
    currentTone = 0;
  }
}

