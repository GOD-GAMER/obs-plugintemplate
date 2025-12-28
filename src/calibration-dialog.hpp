/*
 * Calibration Dialog Header
 * Copyright (C) 2025
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
    
    // Robust filter application helpers
    bool removeExistingFilter(obs_source_t* source, const char* filterName);
    bool createFilter(obs_source_t* source, const char* filterId, 
                     const char* filterName, obs_data_t* settings);
    bool isFilterAvailable(const char* filterId);

    // UI Elements
    QLabel *titleLabel;
    QLabel *instructionLabel;
    QLabel *statusLabel;
    QLabel *peakLabel;
    QLabel *rmsLabel;
    QLabel *quietResult;
    QLabel *normalResult;
    QLabel *loudResult;
    QLabel *rangeResult;
    QLabel *promptLabel;
    QLabel *countdownLabel;
    
    QPushButton *startButton;
    QPushButton *recordButton;
    QPushButton *applyButton;
    QPushButton *resetButton;
    
    QProgressBar *levelMeter;
    QProgressBar *peakMeter;
    QProgressBar *recordingProgress;
    QComboBox *sourceCombo;
    QTimer *updateTimer;
    QTimer *recordingTimer;
    
    QGroupBox *meterGroup;
    QGroupBox *resultsGroup;
    QGroupBox *optionsGroup;
    QFrame *promptFrame;
    
    // Filter options checkboxes
    QCheckBox *enableNoiseSuppressionCheck;
    QCheckBox *enableNoiseGateCheck;
    QCheckBox *enableExpanderCheck;
    QCheckBox *enableGainCheck;
    QCheckBox *enableCompressorCheck;
    QCheckBox *enableLimiterCheck;
    
    // Noise suppression level
    QComboBox *noiseSuppressionLevel;

    // Audio analyzer
    std::unique_ptr<AudioAnalyzer> audioAnalyzer;

    // State
    int currentStep;          // 0=idle, 1=quiet, 2=normal, 3=loud, 4=complete
    bool isRecording;
    float quietLevel;
    float normalLevel;
    float loudLevel;
    
    // Recording duration
    int recordingFrames;
    int recordingElapsedMs;
    static constexpr int RECORDING_DURATION_MS = 3000;  // 3 seconds per sample
    static constexpr int RECORDING_TICK_MS = 100;       // Update every 100ms
};

#endif // CALIBRATION_DIALOG_HPP
