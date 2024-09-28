#include "wifi_manager.h"
#include "config.h"
#include "debug.h"

namespace wifi_manager
{

    /**
     * @brief Connect to Wi-Fi
     */
    void connect() {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            DEBUG(Serial.println("Connecting to WiFi..."));
        }
        DEBUG(Serial.println("Connected to WiFi"));
    }

    /**
     * @brief Send data over Wi-Fi
     */
    void sendData(int dB, float class1Percentage) {
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin(API_URL);
            http.addHeader("Content-Type", "application/json");
            http.addHeader("Authorization", AUTH_BEARER);

            // Format as {"prediction": 0.4, // Example
            //            "value": -10 // Example}
            String json = "{\"prediction\": " + String(class1Percentage) + ", \"value\": " + String(dB) + "}";
            Serial.println(json);

            int httpResponseCode = http.POST(json);

            DEBUG({
                if (httpResponseCode > 0) {
                    Serial.println(httpResponseCode);
                    Serial.println(http.getString());
                }
                else {
                    Serial.print("Error on sending POST: ");
                    Serial.println(httpResponseCode);
                }
            });
            http.end();
        }
        else {
            DEBUG(Serial.println("Error in WiFi connection"));
        }
    }
}
