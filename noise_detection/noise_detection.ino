#include "wifi_manager.h"
#include "microphone.h"
#include "lights.h"
#include "config.h"
#include "debug.h"

void setup() {
    DEBUG({
        Serial.begin(115200);
        while (!Serial);
        Serial.println("Noise prediction and dBA Noise level over Wlan");
    });

    // Initialize LEDs
    lights::initialize();

    // Connect to WiFi
    wifi_manager::connect();

    // Initialize the microphone (including starting inference)
    if (!microphone::initialize()) {
        DEBUG(Serial.println("Failed to initialize the microphone"));
        return;
    }

    DEBUG(microphone::printInferenceSettings());
}

void loop() {
    static int maxSamples = 1;                   // One sample takes approx. 5 sec
    static int sampleCount = 0;                  // Number of samples collected
    static float totalDbA = 0;                   // Total dBA for the sample window
    static float totalPrediction = 0;            // Total prediction percentage (class 1, noise) for the sample window

    // Run inference
    microphone::InferenceResult inferenceResult = microphone::runFullInference();

    if (!inferenceResult.success) {
        DEBUG(Serial.println("ERR: Failed to run inference"));
        return;
    }
    
    lights::controlLEDs(inferenceResult.dBA);

    // Accumulate the results    
    totalDbA += inferenceResult.dBA;
    totalPrediction += inferenceResult.class1Percentage;
    sampleCount++;

    // Check if the specified number of samples has been collected
    if (sampleCount >= maxSamples) {
        // Calculate the final average values after collecting all samples
        //float finalAverageDbA = totalDbA / sampleCount;
        int finalAverageDbA = (int)(totalDbA / sampleCount + 0.5f);
        float finalAveragePrediction = totalPrediction / sampleCount;

        // Send the final average data over Wi-Fi
        wifi_manager::sendData(finalAverageDbA, finalAveragePrediction);

        // Print the final results for debugging
        DEBUG({
            Serial.println("___________________________________________________________");
            Serial.print("Average dBA of the samples: ");
            Serial.println(finalAverageDbA);
            Serial.print("Average class1 (Noise) prediction of the samples: ");
            Serial.println(finalAveragePrediction);
        });

        // Control the LEDs based on the dBA or prediction results
        //lights::controlLEDs(finalAverageDbA);

        // Reset the counters for the next sample collection window
        totalDbA = 0;
        totalPrediction = 0;
        sampleCount = 0;  // Restart the sample count
    }
}
