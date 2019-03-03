#include <Arduino.h>

#define PIN_BUTTON_UP 13


void setup()
{
    Serial.begin(9600); 
}

void loop()
{
    Serial.println("Hello world");
    delay(1000);
}