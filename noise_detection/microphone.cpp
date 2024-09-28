/* Copyright (c) 2022 EdgeImpulse Inc.
 * Copyright (c) 2024 TinyAIoT Project Team, University Münster
 * Copyright (c) 2024 MünsterHack Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <DuoNoise_inferencing.h>
#include <math.h>
#include "config.h"
#include "microphone.h"
#include "debug.h"

// Check if the classifier sensor is configured correctly for the microphone
#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif

namespace microphone {

    typedef struct {
        int16_t *buffer;
        uint8_t buf_ready;
        uint32_t buf_count;
        uint32_t n_samples;
    } inference_t;

    static inference_t inference;
    static const uint32_t sample_buffer_size = 2048;      // Size of the buffer for audio samples
    static signed short sampleBuffer[sample_buffer_size]; // Buffer for raw audio data
    static bool debug_nn = false;                         // Set this to true to see e.g. features generated from the raw signal
    static bool record_status = true;                     // Flag to control recording status

    /**
     * @brief Print sumary inference settings (from model_metadata.h)
     */
    void printInferenceSettings() {
        ei_printf("Inferencing settings:\n");
        ei_printf("\tInterval: ");
        ei_printf_float((float)EI_CLASSIFIER_INTERVAL_MS);
        ei_printf(" ms.\n");
        ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
        ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
        ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));
    }

    /**
     * @brief Initialize and start microphone inference
     */
    bool initialize() {      
        analogReadResolution(12);
        
        // Allocate inference buffer
        inference.buffer = (int16_t *)ps_malloc(EI_CLASSIFIER_RAW_SAMPLE_COUNT * sizeof(int16_t));
        if (inference.buffer == NULL) {
            DEBUG(ei_printf("ERR: Could not allocate inference buffer (size %d)\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT));
            return false;
        }

        inference.buf_count = 0;
        inference.n_samples = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
        inference.buf_ready = 0;

        // Initialize the I2S interface
        if (i2s_init(EI_CLASSIFIER_FREQUENCY)) {
            DEBUG(ei_printf("ERR: Failed to start I2S interface!\r\n"));
            return false;
        }

        // Allow a brief delay before starting inference
        ei_sleep(100);
        record_status = true;

        // Start the task for capturing samples
        xTaskCreate(capture_samples, "CaptureSamples", 1024 * 32, (void *)sample_buffer_size, 10, NULL);

        // Indicate recording has started
        DEBUG(ei_printf("Recording...\n"));

        return true;
    }

    /**
     * @brief Run the inference and return the result.
     */
    InferenceResult runFullInference() {
        InferenceResult result;
        result.success = false;

        if (!recordInference()) {
            return result;
        }

        signal_t signal;
        signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
        signal.get_data = &microphone_audio_signal_get_data;

        ei_impulse_result_t ei_result = {0};
        EI_IMPULSE_ERROR r = run_classifier(&signal, &ei_result, debug_nn);
        if (r != EI_IMPULSE_OK) {
            DEBUG(ei_printf("ERR: Failed to run classifier (%d)\n", r));
            return result;
        }

        // Log predictions and calculate the most probable class
        int pred_index = 0;
        float pred_value = 0;

        DEBUG({
            ei_printf("Predictions ");
            ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
                      ei_result.timing.dsp, ei_result.timing.classification, ei_result.timing.anomaly);
            ei_printf(": \n");
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                ei_printf("    %s: ", ei_result.classification[ix].label);
                ei_printf_float(ei_result.classification[ix].value);
                ei_printf("\n");

                if (ei_result.classification[ix].value > pred_value) {
                    pred_index = ix;
                    pred_value = ei_result.classification[ix].value;
                }
            }

#if EI_CLASSIFIER_HAS_ANOMALY == 1
            ei_printf("    anomaly score: ");
            ei_printf_float(ei_result.anomaly);
            ei_printf("\n");
#endif

            // Print the A-Weighted sound level (dBA)
            //Serial.printf("A-Weighted Sound Level (dBA): %f\n", calculateDbA());
        });

        result.dBA = calculateDbA();
        result.class1Percentage = ei_result.classification[1].value * 100;
        result.success = true;

        return result;
    }

    /**
     * @brief Calculate the unweighted sound level (dBFS) with the MEMS microphone
     */
    float calculateDbfs() {
        float rms = 0;
        for (uint32_t i = 0; i < inference.n_samples; i++) {
            rms += (inference.buffer[i] * inference.buffer[i]);
        }
        rms = sqrt(rms / inference.n_samples);
        return 20 * log10(rms / 32768.0);
    }

    /**
     * @brief Calculate the dBA with the sound level meter
     */
    float calculateDbA() {
        float voltageValue,dbValue;
        voltageValue = analogRead(SoundSensorPin) / 4095.0 * VREF;
        
        if (voltageValue <= 0.6) dbValue = 0.0;
        if (voltageValue >= 2.6) dbValue = 130.0;
        else dbValue = voltageValue * 50.0;  //convert voltage to decibel value
        
        return dbValue;
    }

    /**
     * @brief Audio inference callback function
     */
    void audio_inference_callback(uint32_t n_bytes) {
        for (int i = 0; i < n_bytes >> 1; i++) {
            inference.buffer[inference.buf_count++] = sampleBuffer[i];

            if (inference.buf_count >= inference.n_samples) {
                inference.buf_count = 0;
                inference.buf_ready = 1;
            }
        }
    }

    /**
     * @brief Capture samples from the I2S interface
     */
    void capture_samples(void *arg) {
        int32_t i2s_bytes_to_read = (int32_t)arg;
        size_t bytes_read = i2s_bytes_to_read;

        while (record_status) {
            i2s_read((i2s_port_t)1, (void *)sampleBuffer, i2s_bytes_to_read, &bytes_read, 100);

            if (bytes_read <= 0) {
                DEBUG(ei_printf("Error in I2S read : %d", bytes_read));
            }
            else {
                if (bytes_read < i2s_bytes_to_read) {
                    DEBUG(ei_printf("Partial I2S read"));
                }

                for (int x = 0; x < i2s_bytes_to_read / 2; x++) {
                    sampleBuffer[x] = (int16_t)(sampleBuffer[x]) * 8;
                }

                if (record_status) {
                    audio_inference_callback(i2s_bytes_to_read);
                }
                else {
                    break;
                }
            }
        }
        vTaskDelete(NULL);
    }

    /**
     * @brief Record inference data
     */
    bool recordInference() {
        while (inference.buf_ready == 0) {
            delay(10);
        }
        inference.buf_ready = 0;
        return true;
    }

    /**
     * @brief Get raw audio signal data
     */
    int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
        numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);
        return 0;
    }

    /**
     * @brief Deinitialize the I2S interface
     */
    void stopInference() {
        record_status = false;
        vTaskDelay(100);
        i2s_driver_uninstall((i2s_port_t)1);
        free(inference.buffer);
    }

    /**
     * @brief Initialize the I2S interface
     */
    int i2s_init(uint32_t sampling_rate) {
        i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
            .sample_rate = sampling_rate,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
            .communication_format = I2S_COMM_FORMAT_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 8,
            .dma_buf_len = 512,
            .use_apll = false,
            .tx_desc_auto_clear = false,
            .fixed_mclk = -1};

        i2s_pin_config_t pin_config = {
            .bck_io_num = I2S_SCK,
            .ws_io_num = I2S_WS,
            .data_out_num = -1,
            .data_in_num = I2S_SD};

        esp_err_t ret = 0;

        ret = i2s_driver_install((i2s_port_t)1, &i2s_config, 0, NULL);
        if (ret != ESP_OK) {
            DEBUG(ei_printf("Error in i2s_driver_install"));
        }

        ret = i2s_set_pin((i2s_port_t)1, &pin_config);
        if (ret != ESP_OK) {
            DEBUG(ei_printf("Error in i2s_set_pin"));
        }

        ret = i2s_zero_dma_buffer((i2s_port_t)1);
        if (ret != ESP_OK) {
            DEBUG(ei_printf("Error in initializing DMA buffer with 0"));
        }

        return int(ret);
    }

    /**
     * @brief Deinitialize I2S interface
     */
    int i2s_deinit(void) {
        i2s_driver_uninstall((i2s_port_t)1);
        return 0;
    }
}
