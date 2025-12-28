/*
 * Audio Analyzer Implementation
 * Copyright (C) 2025
 */

#include "audio-analyzer.hpp"
#include <plugin-support.h>

AudioAnalyzer::AudioAnalyzer()
{
}

AudioAnalyzer::~AudioAnalyzer()
{
    stopCapture();
}

float AudioAnalyzer::toDB(float amplitude)
{
    if (amplitude < 0.00001f)
        return -100.0f;
    return 20.0f * std::log10(amplitude);
}

float AudioAnalyzer::fromDB(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

float AudioAnalyzer::calculateRMS(const float *samples, size_t count)
{
    if (count == 0 || !samples)
        return 0.0f;
    
    double sum = 0.0;
    for (size_t i = 0; i < count; i++) {
        sum += static_cast<double>(samples[i] * samples[i]);
    }
    return static_cast<float>(std::sqrt(sum / static_cast<double>(count)));
}

void AudioAnalyzer::audioCallback(void *param, obs_source_t *source,
                                   const struct audio_data *audioData, bool muted)
{
    (void)source;
    AudioAnalyzer *analyzer = static_cast<AudioAnalyzer*>(param);
    if (analyzer) {
        analyzer->processAudio(audioData, muted);
    }
}

void AudioAnalyzer::processAudio(const struct audio_data *audioData, bool muted)
{
    if (!capturing.load() || muted || !audioData)
        return;
    
    if (audioData->frames == 0 || !audioData->data[0])
        return;
    
    // Process first channel (mono or left)
    const float *samples = reinterpret_cast<const float*>(audioData->data[0]);
    size_t frameCount = audioData->frames;
    
    // Calculate RMS
    float rms = calculateRMS(samples, frameCount);
    
    // Find peak in this buffer
    float peak = 0.0f;
    for (size_t i = 0; i < frameCount; i++) {
        float absVal = std::fabs(samples[i]);
        if (absVal > peak)
            peak = absVal;
    }
    
    // Apply smoothing to RMS
    smoothedRMS = smoothedRMS * (1.0f - SMOOTHING_FACTOR) + rms * SMOOTHING_FACTOR;
    
    // Convert to dB
    float rmsDB = toDB(smoothedRMS);
    float peakDB = toDB(peak);
    
    // Update atomic values
    currentRMS.store(rmsDB);
    currentPeak.store(peakDB);
    
    // Track max peak
    float currentMax = maxPeak.load();
    if (peakDB > currentMax) {
        maxPeak.store(peakDB);
    }
}

bool AudioAnalyzer::startCapture(obs_source_t *source)
{
    if (!source) {
        obs_log(LOG_WARNING, "[AudioAnalyzer] Cannot start capture: null source");
        return false;
    }
    
    // Stop any existing capture
    stopCapture();
    
    audioSource = obs_source_get_ref(source);
    if (!audioSource) {
        obs_log(LOG_WARNING, "[AudioAnalyzer] Failed to get source reference");
        return false;
    }
    
    // Reset levels
    currentRMS.store(0.0f);
    currentPeak.store(-100.0f);
    smoothedRMS = 0.0f;
    
    // Add audio capture callback
    obs_source_add_audio_capture_callback(audioSource, audioCallback, this);
    capturing.store(true);
    
    obs_log(LOG_INFO, "[AudioAnalyzer] Started capturing audio from: %s",
            obs_source_get_name(source));
    
    return true;
}

void AudioAnalyzer::stopCapture()
{
    if (capturing.load() && audioSource) {
        obs_source_remove_audio_capture_callback(audioSource, audioCallback, this);
        obs_source_release(audioSource);
        audioSource = nullptr;
    }
    capturing.store(false);
    
    obs_log(LOG_INFO, "[AudioAnalyzer] Stopped capturing audio");
}
