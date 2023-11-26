#include <Arduino.h>
#include "LittleFS.h"

#define LED 2

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  File file = LittleFS.open("/index.html", "r");
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  
  Serial.println("File Content:");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED, HIGH);
  Serial.println("LED is on");
  delay(1000);
  digitalWrite(LED, LOW);
  Serial.println("LED is off");
  delay(1000);
}