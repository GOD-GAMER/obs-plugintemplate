/*
 * Calibration Dialog Header
 * Copyright (C) 2025
 * 
 * Enhanced Professional Audio Calibration with 8-step process
 * for fine-tuned, broadcast-quality audio settings
 */

#ifndef CALIBRATION_DIALOG_HPP
#define CALIBRATION_DIALOG_HPP

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QComboBox>
#include <QTimer>
#include <QGroupBox>
#include <QFrame>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>
#include <memory>
#include <vector>
#include "audio-analyzer.hpp"

class CalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrationDialog(QWidget *parent = nullptr);
    ~CalibrationDialog();

private slots:
    void onStartClicked();
    void onRecordClicked();
    void onApplyClicked();
    void onResetClicked();
    void updateLevelMeter();
    void onSourceChanged(int index);
    void onRecordingTick();

private:
    void setupUI();
    void setupStyles();
    void populateAudioSources();
    void startRecording();
    void stopRecording();
    void saveCurrentLevel();
    void advanceStep();
    void applyFilters(float gain, float threshold, float ratio);
    void updateResultsDisplay();
    void updatePromptForStep();
    obs_source_t* getSelectedSource();
    QString getPromptForStep(int step);
    QString getInstructionForStep(int step);
    
    // Robust filter application helpers
    bool removeExistingFilter(obs_source_t* source, const char* filterName);
    bool createFilter(obs_source_t* source, const char* filterId, 
                     const char* filterName, obs_data_t* settings);
    bool isFilterAvailable(const char* filterId);
    
    // Persistence
    void saveCalibrationData();
    void loadCalibrationData();
    QString getCalibrationFilePath();

    // UI Elements - Top Section (Recording) - PROMINENT
    QFrame *recordingFrame;
    QPushButton *recordButton;
    QLabel *promptLabel;
    QLabel *countdownLabel;
    QProgressBar *recordingProgress;
    QLabel *stepIndicatorLabel;
    
    // UI Elements - Other
    QLabel *titleLabel;
    QLabel *instructionLabel;
    QLabel *statusLabel;
    QLabel *peakLabel;
    QLabel *rmsLabel;
    
    // Results labels for 8 steps
    QLabel *step1Result;
    QLabel *step2Result;
    QLabel *step3Result;
    QLabel *step4Result;
    QLabel *step5Result;
    QLabel *step6Result;
    QLabel *step7Result;
    QLabel *step8Result;
    QLabel *rangeResult;
    QLabel *avgResult;
    
    QPushButton *startButton;
    QPushButton *applyButton;
    QPushButton *resetButton;
    
    QProgressBar *levelMeter;
    QProgressBar *peakMeter;
    QComboBox *sourceCombo;
    QTimer *updateTimer;
    QTimer *recordingTimer;
    
    QGroupBox *meterGroup;
    QGroupBox *resultsGroup;
    QGroupBox *basicFiltersGroup;
    QGroupBox *advancedFiltersGroup;
    
    // Filter options checkboxes - Basic
    QCheckBox *enableNoiseSuppressionCheck;
    QCheckBox *enableNoiseGateCheck;
    QCheckBox *enableExpanderCheck;
    QCheckBox *enableGainCheck;
    QCheckBox *enableCompressorCheck;
    QCheckBox *enableLimiterCheck;
    
    // Filter options checkboxes - Advanced
    QCheckBox *enableHighPassCheck;
    QCheckBox *enableLowPassCheck;
    QCheckBox *enableDeEsserCheck;
    QCheckBox *enableVSTCheck;
    
    // Settings
    QComboBox *noiseSuppressionLevel;
    QComboBox *highPassFreq;
    QComboBox *lowPassFreq;
    QComboBox *deEsserIntensity;

    // Audio analyzer
    std::unique_ptr<AudioAnalyzer> audioAnalyzer;

    // State - 8 calibration steps for fine-tuned accuracy
    int currentStep;          // 0=idle, 1-8=recording steps, 9=complete
    bool isRecording;
    
    // 8 voice level samples for comprehensive calibration
    float levels[8];     // Average RMS dB per step
    float peaks[8];      // Max peak dB per step

    // Recording window accumulation (for more stable measurements)
    double recordingRmsSumLinear = 0.0;
    int recordingRmsSamples = 0;
    float recordingPeakMaxDb = -100.0f;
    
    // Recording duration - extended for accuracy
    int recordingFrames;
    int recordingElapsedMs;
    
    static constexpr int RECORDING_DURATION_MS = 5000;  // 5 seconds per sample for accuracy
    static constexpr int RECORDING_TICK_MS = 100;       // Update every 100ms
    static constexpr int TOTAL_STEPS = 8;               // 8 calibration steps (~5 min total)
};

#endif // CALIBRATION_DIALOG_HPP
