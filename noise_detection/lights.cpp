#include "lights.h"
#include "config.h"
#include "debug.h"

namespace lights {

    // Initialize LED pins
    void initialize() {
        pinMode(GREEN_LED, OUTPUT);
        pinMode(YELLOW_LED, OUTPUT);
        pinMode(RED_LED, OUTPUT);

        // turn off
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(YELLOW_LED, LOW);
        digitalWrite(RED_LED, LOW);

        DEBUG(Serial.println("LEDS: Initialized"));
    }

    // Control the LEDs based on dBA values
    void controlLEDs(float dB) {
        if (dB < LIMIT_LOW) {
            // If dBA is less than 50, turn on Green LED and turn off the others
            digitalWrite(GREEN_LED, HIGH);
            digitalWrite(YELLOW_LED, LOW);
            digitalWrite(RED_LED, LOW);
        } else if (dB < LIMIT_HIGH) {
            // If dBA is between 50 and 79, turn on Yellow LED and turn off the others
            digitalWrite(GREEN_LED, LOW);
            digitalWrite(YELLOW_LED, HIGH);
            digitalWrite(RED_LED, LOW);
        } else {
            // If dBA is 80 or higher, turn on Red LED and turn off the others
            digitalWrite(GREEN_LED, LOW);
            digitalWrite(YELLOW_LED, LOW);
            digitalWrite(RED_LED, HIGH);
        }
    }
}
