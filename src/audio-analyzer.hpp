/*
 * Audio Analyzer - Captures and analyzes audio levels
 * Copyright (C) 2025
 */

#ifndef AUDIO_ANALYZER_HPP
#define AUDIO_ANALYZER_HPP

#include <obs.h>
#include <cmath>
#include <atomic>
#include <mutex>
#include <vector>

class AudioAnalyzer {
public:
    AudioAnalyzer();
    ~AudioAnalyzer();

    // Start/stop capturing from a source
    bool startCapture(obs_source_t *source);
    void stopCapture();

    // Get current levels
    float getCurrentRMS() const { return currentRMS.load(); }
    float getCurrentPeak() const { return currentPeak.load(); }
    float getMaxPeak() const { return maxPeak.load(); }
    
    // Convert to dB
    static float toDB(float amplitude);
    static float fromDB(float db);
    
    // Reset peak tracking
    void resetMaxPeak() { maxPeak.store(-100.0f); }

    // Check if capturing
    bool isCapturing() const { return capturing.load(); }

private:
    static void audioCallback(void *param, obs_source_t *source,
                              const struct audio_data *audioData, bool muted);
    
    void processAudio(const struct audio_data *audioData, bool muted);
    float calculateRMS(const float *samples, size_t count);

    obs_source_t *audioSource = nullptr;
    std::atomic<bool> capturing{false};
    std::atomic<float> currentRMS{0.0f};
    std::atomic<float> currentPeak{-100.0f};
    std::atomic<float> maxPeak{-100.0f};
    
    // Smoothing
    float smoothedRMS = 0.0f;
    static constexpr float SMOOTHING_FACTOR = 0.1f;
};

#endif // AUDIO_ANALYZER_HPP
