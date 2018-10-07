#include <Arduino.h>
#include <TM1637Display.h>
#include <DHT.h>

#define DHT_PIN D4
#define DHT_TYPE DHT22

#define CLK D6
#define DIO D5

#define BTN_UP D1
#define BTN_DOWN D2

#define ALARM D3

#define TEST_DELAY 2000

const uint8_t SEG_CELSIUS[] = {
    SEG_A,
    SEG_A,
    SEG_A | SEG_B | SEG_F | SEG_G, // °
    SEG_A | SEG_D | SEG_E | SEG_F, // C
};

const uint8_t SEG_HUM[] = {
    SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, // H
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, // U
};

const uint8_t SEG_LIM[] = {
    SEG_D | SEG_E | SEG_F, // L
    SEG_E | SEG_F,         // I
    SEG_E | SEG_G,         // m
    SEG_C | SEG_E | SEG_G  // m
};

TM1637Display display(CLK, DIO);
DHT dht(DHT_PIN, DHT_TYPE);

int humidity = 0, temperature = 0, oldTemperature = 0;
volatile unsigned long alteZeit = 0, entprellZeit = 250;
boolean forceDP = true, forceStop = false;

int limit = 24;

void setup()
{
    Serial.begin(115200);
    Serial.println();
    delay(50);

    pinMode(BTN_UP, INPUT);
    pinMode(BTN_DOWN, INPUT);
    pinMode(ALARM, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(BTN_UP), handleBTN_UP, RISING);
    attachInterrupt(digitalPinToInterrupt(BTN_DOWN), handleBTN_DOWN, RISING);

    Serial.println("Start");
    readDHT();
    delay(1000);

    uint8_t data[] = {0xff, 0xff, 0xff, 0xff};
    display.setBrightness(0x0f);

    // All segments on
    display.setSegments(data);
}

void loop()
{
    readDHT();

    delay(1000);

    if (temperature != oldTemperature || forceDP)
    {
        forceDP = false;
        display.setSegments(SEG_CELSIUS);
        display.showNumberDec(temperature, false, 2, 0);
    }

    if (temperature > limit)
    {
        forceStop = false;
        alarm();
    }
}

void readDHT()
{
    oldTemperature = temperature;
    temperature = 0;
    humidity = 0;
    for (int i = 0; i < 10; i++)
    {
        humidity += dht.readHumidity();
        temperature += dht.readTemperature();
    }

    humidity = humidity / 10;
    temperature = (temperature / 10) - 3;

    Serial.print("humideity: ");
    Serial.println(humidity);
    Serial.print("temperature: ");
    Serial.println(temperature);
    Serial.println(" ");

    if (humidity <= 0.001)
    {
        humidity = 0;
    }
    if (temperature <= 0.001)
    {
        temperature = 0;
    }
}

void handleBTN_DOWN()
{
    handleIsr(-1);
}

void handleBTN_UP()
{
    handleIsr(1);
}

void handleIsr(int btnVal)
{
    if ((millis() - alteZeit) > entprellZeit)
    {
        // innerhalb der entprellZeit nichts machen
        noInterrupts();
        Serial.println("UP");
        alteZeit = millis(); // letzte Schaltzeit merken
        limit += btnVal;
        display.showNumberDec(limit);
        forceDP = true;
        forceStop = true;
        interrupts();
    }
}

void alarm()
{
    for (int i = 0; i < 3; i++)
    {
        if (!forceStop)
        {
            digitalWrite(ALARM, HIGH);
            delay(250);
            digitalWrite(ALARM, LOW);
            delay(250);
        }
    }
}
