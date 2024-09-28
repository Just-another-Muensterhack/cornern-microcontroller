#ifndef MICROPHONE_H
#define MICROPHONE_H

#include <driver/i2s.h>

namespace microphone {

    void printInferenceSettings();

    struct InferenceResult {
        float dBA;
        float class1Percentage;
        bool success;
    };

    InferenceResult runFullInference();
    bool initialize();
    bool recordInference();
    void stopInference();
    int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr);
    int i2s_init(uint32_t sampling_rate);
    void capture_samples(void *arg);
    float calculateDbfs();
    float calculateDbA();

}

#endif // MICROPHONE_H
