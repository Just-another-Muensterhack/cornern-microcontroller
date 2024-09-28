#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <HTTPClient.h>

namespace wifi_manager {
    void connect();
    void sendData(int dB, float class1Percentage);
}

#endif // WIFI_MANAGER_H
