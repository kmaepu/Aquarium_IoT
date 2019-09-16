#include <M5Stack.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2 // DS18B20 on arduino pin2 corresponds to D4 on physical board

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

void setup() {
  M5.begin();
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setTextSize(2);
}

void loop() {
  float celsius;
  float fahrenheit;

  DS18B20.begin();
  int count = DS18B20.getDS18Count();

  //M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("Devices found: ");
  M5.Lcd.println(count);

  if (count > 0) {

    DS18B20.requestTemperatures();

    for (int i = 0; i < count; i++) {
      celsius = DS18B20.getTempCByIndex(i);
      fahrenheit = celsius * 1.8 + 32.0;

      celsius = round(celsius);
      fahrenheit = round(fahrenheit);

      M5.Lcd.print("Device ");
      M5.Lcd.print(i);
      M5.Lcd.print(": ");
      M5.Lcd.print(celsius, 0);
      M5.Lcd.print( "C / ");
      M5.Lcd.print(fahrenheit, 0);
      M5.Lcd.println("F");
    }
  }
  delay(2000);
}
