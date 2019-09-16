#include <M5Stack.h>
#include "HCSR04.h"
#include <Wire.h>

#define TRIG 5
#define ECHO 2

HCSR04 hcsr04;

void setup() {
    M5.begin();
    Serial.begin(115200);
    while (!Serial) ;

    hcsr04.begin(TRIG, ECHO);

    M5.Lcd.setTextSize(3);
}

void loop() {
    float HCSR04_d = hcsr04.distance();
    Serial.printf("%.0f\n ", HCSR04_d * 10);

    delay(1000);
}
