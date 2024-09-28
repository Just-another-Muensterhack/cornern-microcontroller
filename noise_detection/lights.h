#ifndef LIGHTS_H
#define LIGHTS_H

#include <Arduino.h>

namespace lights {

    // Function to initialize the LED pins
    void initialize();

    // Function to control the LEDs based on dBA value
    void controlLEDs(float dB);

}

#endif
