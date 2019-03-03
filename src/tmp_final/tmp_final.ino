#include <EEPROM.h>
#include <TM1637Display.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_WIRE_BUS 7

#define PIN_ALARM 8

#define PIN_BTN_DOWN 2
#define PIN_BTN_UP 3

#define CLK 6
#define DIO 5

const uint8_t SEG_CELSIUS[] = {
  SEG_A,
  SEG_A,
  SEG_A | SEG_B | SEG_F | SEG_G, // °
  SEG_A | SEG_D | SEG_E | SEG_F, // C
};

const uint8_t SEG_HUM[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, // U
};

const uint8_t SEG_LIM[] = {
  SEG_D | SEG_E | SEG_F, // L
  SEG_E | SEG_F,         // I
  SEG_E | SEG_G,         // m
  SEG_C | SEG_E | SEG_G  // m
};


TM1637Display display(CLK, DIO);

OneWire oneWire(PIN_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int humidity = 0, temperature = 0, oldTemperature = 0;
volatile unsigned long oldTime = 0, debouncingTime = 250, t2 = 0;
boolean forceDP = true, forceStop = false;

int limit = 24, oldLimit = 24;

void setup()
{
  Serial.begin(9600);
  Serial.println();
  delay(50);

  Serial.println("Started..");

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_UP, INPUT_PULLUP);

  pinMode(PIN_ALARM, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_BTN_UP), handleBTN_UP, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_DOWN), handleBTN_DOWN, FALLING);


  // Setup display
  uint8_t data[] = {0xff, 0xff, 0xff, 0xff};
  display.setBrightness(0x0f);

  // All segments on
  display.setSegments(data);

  sensors.begin();

  int l = EEPROM.read(0);
  if (l < 16 || l > 28) {
    limit = 24;
  } else {
    limit = l;
  }

  for (int i = 0; i < 3; i++) {
    display.showNumberDec(limit);
    delay(1000);
    display.setSegments(SEG_CELSIUS);
    delay(500);
  }

}

void loop()
{
  if ((millis() - t2) > 1000)
  {
    readDHT();
    t2 = millis();
  }

  if (temperature != oldTemperature || forceDP)
  {
    forceDP = false;
    display.setSegments(SEG_CELSIUS);
    display.showNumberDec(temperature, false, 2, 0);
  }

  if (limit != oldLimit) {
    display.showNumberDec(limit);
    delay(500);
    oldLimit = limit;
    forceDP = true;
  }

  if (temperature >= limit)
  {
    forceStop = false;
    alarm();
  }
}

void readDHT() {
  Serial.print(" Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperature readings
  Serial.println("DONE");
  /********************************************************************/
  Serial.print("Temperature is: ");
  Serial.print(sensors.getTempCByIndex(0)); // Why "byIndex"?
  oldTemperature = temperature;
  temperature = (int) sensors.getTempCByIndex(0);
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
  Serial.println("CALL ISR");
  if ((millis() - oldTime) > debouncingTime)
  {
    // innerhalb der entprellZeit nichts machen
    noInterrupts();
    limit += btnVal;
    EEPROM.write(0, limit);
    forceStop = true;
    display.showNumberDec(limit);
    oldTime = millis();
    interrupts();
  }
}

void alarm()
{
  for (int i = 0; i < 3; i++)
  {
    if (!forceStop)
    {
      digitalWrite(PIN_ALARM, HIGH);
      delay(250);
      digitalWrite(PIN_ALARM, LOW);
      delay(250);
    }
  }
}
