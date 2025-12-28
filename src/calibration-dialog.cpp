/*
 * Calibration Dialog Implementation
 * Copyright (C) 2025
 * 
 * Enhanced with robust error handling and comprehensive audio filters:
 * - Noise Suppression (RNNoise)
 * - Noise Gate
 * - Expander
 * - Gain
 * - Compressor
 * - Limiter
 */

#include "calibration-dialog.hpp"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QFont>
#include <QStyle>
#include <QScrollArea>

// OBS-style color constants
namespace OBSColors {
    const QString background = "#1e1e1e";
    const QString backgroundAlt = "#252525";
    const QString backgroundLight = "#2d2d2d";
    const QString border = "#3c3c3c";
    const QString borderLight = "#4a4a4a";
    const QString text = "#e0e0e0";
    const QString textMuted = "#888888";
    const QString accent = "#2a7fff";
    const QString accentHover = "#3d8fff";
    const QString success = "#4caf50";
    const QString warning = "#ff9800";
    const QString error = "#f44336";
    const QString recording = "#e53935";
}

CalibrationDialog::CalibrationDialog(QWidget *parent)
    : QDialog(parent)
    , audioAnalyzer(std::make_unique<AudioAnalyzer>())
    , currentStep(0)
    , isRecording(false)
    , quietLevel(-60.0f)
    , normalLevel(-30.0f)
    , loudLevel(-10.0f)
    , recordingFrames(0)
    , recordingElapsedMs(0)
{
    setWindowTitle("Audio Calibration Wizard");
    setMinimumSize(500, 500);
    resize(560, 650);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    setupStyles();
    setupUI();
    
    // Timer for updating level meter (20 FPS)
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &CalibrationDialog::updateLevelMeter);
    
    // Timer for recording countdown
    recordingTimer = new QTimer(this);
    connect(recordingTimer, &QTimer::timeout, this, &CalibrationDialog::onRecordingTick);
    
    obs_log(LOG_INFO, "[AudioCalibrator] Calibration dialog initialized");
}

CalibrationDialog::~CalibrationDialog()
{
    if (updateTimer && updateTimer->isActive()) {
        updateTimer->stop();
    }
    if (recordingTimer && recordingTimer->isActive()) {
        recordingTimer->stop();
    }
    if (audioAnalyzer) {
        audioAnalyzer->stopCapture();
    }
    obs_log(LOG_INFO, "[AudioCalibrator] Calibration dialog destroyed");
}

void CalibrationDialog::setupStyles()
{
    // Apply OBS-style dark theme to the entire dialog
    QString styleSheet = QString(
        "QDialog {"
        "    background-color: %1;"
        "    color: %2;"
        "}"
        "QLabel {"
        "    color: %2;"
        "}"
        "QGroupBox {"
        "    background-color: %3;"
        "    border: 1px solid %4;"
        "    border-radius: 4px;"
        "    margin-top: 12px;"
        "    padding-top: 8px;"
        "    font-weight: bold;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 8px;"
        "    padding: 0 4px;"
        "    color: %2;"
        "}"
        "QComboBox {"
        "    background-color: %3;"
        "    border: 1px solid %4;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "    color: %2;"
        "    min-height: 24px;"
        "}"
        "QComboBox:hover {"
        "    border-color: %5;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    width: 24px;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: %3;"
        "    border: 1px solid %4;"
        "    selection-background-color: %6;"
        "    color: %2;"
        "}"
        "QPushButton {"
        "    background-color: %3;"
        "    border: 1px solid %4;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    color: %2;"
        "    font-weight: 500;"
        "    min-height: 28px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %7;"
        "    border-color: %5;"
        "}"
        "QPushButton:pressed {"
        "    background-color: %4;"
        "}"
        "QPushButton:disabled {"
        "    background-color: %1;"
        "    color: %8;"
        "    border-color: %4;"
        "}"
        "QPushButton#primaryButton {"
        "    background-color: %6;"
        "    border-color: %6;"
        "    color: white;"
        "}"
        "QPushButton#primaryButton:hover {"
        "    background-color: %9;"
        "}"
        "QPushButton#recordButton {"
        "    background-color: %3;"
        "    border: 2px solid %4;"
        "}"
        "QProgressBar {"
        "    border: 1px solid %4;"
        "    border-radius: 3px;"
        "    background-color: %1;"
        "    text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "    border-radius: 2px;"
        "}"
        "QCheckBox {"
        "    color: %2;"
        "    spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "    border: 1px solid %4;"
        "    border-radius: 3px;"
        "    background-color: %3;"
        "}"
        "QCheckBox::indicator:checked {"
        "    background-color: %6;"
        "    border-color: %6;"
        "}"
        "QCheckBox::indicator:hover {"
        "    border-color: %5;"
        "}"
    ).arg(OBSColors::background)
     .arg(OBSColors::text)
     .arg(OBSColors::backgroundAlt)
     .arg(OBSColors::border)
     .arg(OBSColors::borderLight)
     .arg(OBSColors::accent)
     .arg(OBSColors::backgroundLight)
     .arg(OBSColors::textMuted)
     .arg(OBSColors::accentHover);
    
    setStyleSheet(styleSheet);
}

void CalibrationDialog::setupUI()
{
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    dialogLayout->setSpacing(0);
    dialogLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create scroll area
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QString(
        "QScrollArea { background-color: %1; border: none; }"
        "QScrollBar:vertical { background-color: %1; width: 10px; margin: 0; }"
        "QScrollBar::handle:vertical { background-color: %2; border-radius: 5px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background-color: %3; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
    ).arg(OBSColors::background).arg(OBSColors::border).arg(OBSColors::borderLight));
    
    // Content widget inside scroll area
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    
    // Title
    titleLabel = new QLabel("Audio Calibration Wizard");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString(
        "font-size: 18px; font-weight: bold; color: %1; padding: 4px;"
    ).arg(OBSColors::text));
    mainLayout->addWidget(titleLabel);
    
    // Instructions
    instructionLabel = new QLabel(
        "Configure your microphone for consistent, professional audio.\n"
        "Record 3 voice samples, then apply optimized filters."
    );
    instructionLabel->setWordWrap(true);
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setStyleSheet(QString("color: %1; padding: 4px;").arg(OBSColors::textMuted));
    mainLayout->addWidget(instructionLabel);
    
    // Audio source selection
    QGroupBox *sourceGroup = new QGroupBox("Audio Source");
    QHBoxLayout *sourceLayout = new QHBoxLayout(sourceGroup);
    sourceLayout->setContentsMargins(12, 16, 12, 12);
    sourceCombo = new QComboBox();
    sourceCombo->setMinimumWidth(280);
    connect(sourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CalibrationDialog::onSourceChanged);
    populateAudioSources();
    QLabel *micLabel = new QLabel("Microphone:");
    micLabel->setStyleSheet(QString("color: %1;").arg(OBSColors::textMuted));
    sourceLayout->addWidget(micLabel);
    sourceLayout->addWidget(sourceCombo, 1);
    mainLayout->addWidget(sourceGroup);
    
    // Prompt frame - shows what to say
    promptFrame = new QFrame();
    promptFrame->setStyleSheet(QString(
        "QFrame {"
        "    background-color: %1;"
        "    border: 2px solid %2;"
        "    border-radius: 6px;"
        "}"
    ).arg(OBSColors::backgroundAlt).arg(OBSColors::border));
    QVBoxLayout *promptLayout = new QVBoxLayout(promptFrame);
    promptLayout->setSpacing(6);
    promptLayout->setContentsMargins(12, 12, 12, 12);
    
    QLabel *sayLabel = new QLabel("SAY THIS:");
    sayLabel->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: bold;").arg(OBSColors::textMuted));
    sayLabel->setAlignment(Qt::AlignCenter);
    promptLayout->addWidget(sayLabel);
    
    promptLabel = new QLabel("\"The quick brown fox jumps over the lazy dog.\"");
    promptLabel->setAlignment(Qt::AlignCenter);
    promptLabel->setWordWrap(true);
    promptLabel->setStyleSheet(QString(
        "font-size: 14px; font-weight: bold; color: %1; padding: 4px;"
    ).arg(OBSColors::accent));
    promptLayout->addWidget(promptLabel);
    
    countdownLabel = new QLabel("");
    countdownLabel->setAlignment(Qt::AlignCenter);
    countdownLabel->setStyleSheet(QString("color: %1; font-size: 22px; font-weight: bold;").arg(OBSColors::warning));
    promptLayout->addWidget(countdownLabel);
    
    // Recording progress bar
    recordingProgress = new QProgressBar();
    recordingProgress->setRange(0, RECORDING_DURATION_MS);
    recordingProgress->setValue(0);
    recordingProgress->setTextVisible(false);
    recordingProgress->setFixedHeight(6);
    recordingProgress->setStyleSheet(QString(
        "QProgressBar { background-color: %1; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background-color: %2; border-radius: 3px; }"
    ).arg(OBSColors::border).arg(OBSColors::accent));
    promptLayout->addWidget(recordingProgress);
    
    promptFrame->setVisible(false);
    mainLayout->addWidget(promptFrame);
    
    // Level meters
    meterGroup = new QGroupBox("Audio Levels");
    QVBoxLayout *meterLayout = new QVBoxLayout(meterGroup);
    meterLayout->setContentsMargins(12, 16, 12, 12);
    meterLayout->setSpacing(6);
    
    // RMS meter
    QHBoxLayout *rmsLayout = new QHBoxLayout();
    QLabel *levelLabel = new QLabel("Level:");
    levelLabel->setMinimumWidth(40);
    levelLabel->setStyleSheet(QString("color: %1;").arg(OBSColors::textMuted));
    rmsLayout->addWidget(levelLabel);
    levelMeter = new QProgressBar();
    levelMeter->setRange(0, 100);
    levelMeter->setValue(0);
    levelMeter->setTextVisible(false);
    levelMeter->setFixedHeight(18);
    levelMeter->setStyleSheet(QString(
        "QProgressBar { background-color: %1; border: 1px solid %2; border-radius: 3px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "stop:0 #2e7d32, stop:0.5 #4caf50, stop:0.7 #ffeb3b, stop:0.85 #ff9800, stop:1 #f44336); border-radius: 2px; }"
    ).arg(OBSColors::background).arg(OBSColors::border));
    rmsLayout->addWidget(levelMeter, 1);
    rmsLabel = new QLabel("-inf dB");
    rmsLabel->setMinimumWidth(55);
    rmsLabel->setAlignment(Qt::AlignRight);
    rmsLabel->setStyleSheet(QString("color: %1; font-family: monospace; font-size: 11px;").arg(OBSColors::text));
    rmsLayout->addWidget(rmsLabel);
    meterLayout->addLayout(rmsLayout);
    
    // Peak meter
    QHBoxLayout *peakLayout = new QHBoxLayout();
    QLabel *peakTextLabel = new QLabel("Peak:");
    peakTextLabel->setMinimumWidth(40);
    peakTextLabel->setStyleSheet(QString("color: %1;").arg(OBSColors::textMuted));
    peakLayout->addWidget(peakTextLabel);
    peakMeter = new QProgressBar();
    peakMeter->setRange(0, 100);
    peakMeter->setValue(0);
    peakMeter->setTextVisible(false);
    peakMeter->setFixedHeight(12);
    peakMeter->setStyleSheet(QString(
        "QProgressBar { background-color: %1; border: 1px solid %2; border-radius: 2px; }"
        "QProgressBar::chunk { background-color: %3; border-radius: 1px; }"
    ).arg(OBSColors::background).arg(OBSColors::border).arg(OBSColors::error));
    peakLayout->addWidget(peakMeter, 1);
    peakLabel = new QLabel("-inf dB");
    peakLabel->setMinimumWidth(55);
    peakLabel->setAlignment(Qt::AlignRight);
    peakLabel->setStyleSheet(QString("color: %1; font-family: monospace; font-size: 11px;").arg(OBSColors::text));
    peakLayout->addWidget(peakLabel);
    meterLayout->addLayout(peakLayout);
    
    mainLayout->addWidget(meterGroup);
    
    // Results display
    resultsGroup = new QGroupBox("Calibration Results");
    QVBoxLayout *resultsLayout = new QVBoxLayout(resultsGroup);
    resultsLayout->setContentsMargins(12, 16, 12, 12);
    resultsLayout->setSpacing(4);
    
    quietResult = new QLabel("1. Quiet Level:  --");
    quietResult->setStyleSheet(QString("color: %1; font-size: 11px;").arg(OBSColors::textMuted));
    normalResult = new QLabel("2. Normal Level: --");
    normalResult->setStyleSheet(QString("color: %1; font-size: 11px;").arg(OBSColors::textMuted));
    loudResult = new QLabel("3. Loud Level:   --");
    loudResult->setStyleSheet(QString("color: %1; font-size: 11px;").arg(OBSColors::textMuted));
    rangeResult = new QLabel("Dynamic Range:  --");
    rangeResult->setStyleSheet(QString("font-weight: bold; color: %1; margin-top: 2px; font-size: 11px;").arg(OBSColors::textMuted));
    
    resultsLayout->addWidget(quietResult);
    resultsLayout->addWidget(normalResult);
    resultsLayout->addWidget(loudResult);
    resultsLayout->addWidget(rangeResult);
    
    mainLayout->addWidget(resultsGroup);
    
    // Filter options
    optionsGroup = new QGroupBox("Audio Processing Filters");
    QGridLayout *optionsLayout = new QGridLayout(optionsGroup);
    optionsLayout->setContentsMargins(12, 16, 12, 12);
    optionsLayout->setSpacing(8);
    
    // Row 0: Noise Suppression
    enableNoiseSuppressionCheck = new QCheckBox("Noise Suppression (RNNoise)");
    enableNoiseSuppressionCheck->setChecked(true);
    enableNoiseSuppressionCheck->setToolTip("AI-powered noise removal - highly recommended");
    optionsLayout->addWidget(enableNoiseSuppressionCheck, 0, 0);
    
    noiseSuppressionLevel = new QComboBox();
    noiseSuppressionLevel->addItem("Low", -5);
    noiseSuppressionLevel->addItem("Medium", -10);
    noiseSuppressionLevel->addItem("High", -15);
    noiseSuppressionLevel->setCurrentIndex(1);  // Default to Medium
    noiseSuppressionLevel->setFixedWidth(100);
    optionsLayout->addWidget(noiseSuppressionLevel, 0, 1);
    
    // Row 1: Noise Gate
    enableNoiseGateCheck = new QCheckBox("Noise Gate");
    enableNoiseGateCheck->setChecked(true);
    enableNoiseGateCheck->setToolTip("Cuts audio below threshold - removes breathing sounds");
    optionsLayout->addWidget(enableNoiseGateCheck, 1, 0);
    
    // Row 2: Expander
    enableExpanderCheck = new QCheckBox("Expander");
    enableExpanderCheck->setChecked(false);
    enableExpanderCheck->setToolTip("Gentle noise reduction - alternative to noise gate");
    optionsLayout->addWidget(enableExpanderCheck, 2, 0);
    
    // Row 3: Gain
    enableGainCheck = new QCheckBox("Gain (Auto-calculated)");
    enableGainCheck->setChecked(true);
    enableGainCheck->setToolTip("Adjusts volume to broadcast standard");
    optionsLayout->addWidget(enableGainCheck, 3, 0);
    
    // Row 4: Compressor
    enableCompressorCheck = new QCheckBox("Compressor (Auto-calculated)");
    enableCompressorCheck->setChecked(true);
    enableCompressorCheck->setToolTip("Reduces dynamic range - evens out volume");
    optionsLayout->addWidget(enableCompressorCheck, 4, 0);
    
    // Row 5: Limiter
    enableLimiterCheck = new QCheckBox("Limiter (-6 dB ceiling)");
    enableLimiterCheck->setChecked(true);
    enableLimiterCheck->setToolTip("Prevents clipping on loud moments");
    optionsLayout->addWidget(enableLimiterCheck, 5, 0);
    
    mainLayout->addWidget(optionsGroup);
    
    // Status
    statusLabel = new QLabel("Select your microphone and click Start Calibration");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(QString("color: %1; padding: 6px; font-size: 11px;").arg(OBSColors::textMuted));
    mainLayout->addWidget(statusLabel);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);
    
    startButton = new QPushButton("Start Calibration");
    startButton->setObjectName("primaryButton");
    startButton->setMinimumHeight(34);
    connect(startButton, &QPushButton::clicked, this, &CalibrationDialog::onStartClicked);
    
    recordButton = new QPushButton("Record");
    recordButton->setObjectName("recordButton");
    recordButton->setMinimumHeight(34);
    recordButton->setEnabled(false);
    connect(recordButton, &QPushButton::clicked, this, &CalibrationDialog::onRecordClicked);
    
    applyButton = new QPushButton("Apply Filters");
    applyButton->setMinimumHeight(34);
    applyButton->setEnabled(false);
    connect(applyButton, &QPushButton::clicked, this, &CalibrationDialog::onApplyClicked);
    
    resetButton = new QPushButton("Reset");
    resetButton->setMinimumHeight(34);
    connect(resetButton, &QPushButton::clicked, this, &CalibrationDialog::onResetClicked);
    
    QPushButton *closeButton = new QPushButton("Close");
    closeButton->setMinimumHeight(34);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(recordButton);
    buttonLayout->addWidget(applyButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();
    
    // Set content widget to scroll area
    scrollArea->setWidget(contentWidget);
    dialogLayout->addWidget(scrollArea);
}

void CalibrationDialog::populateAudioSources()
{
    if (!sourceCombo) {
        obs_log(LOG_ERROR, "[AudioCalibrator] sourceCombo is null in populateAudioSources");
        return;
    }
    
    // Block signals to prevent onSourceChanged from firing during population
    sourceCombo->blockSignals(true);
    sourceCombo->clear();
    
    // Enumerate all sources to find audio inputs
    auto enumCallback = [](void *param, obs_source_t *source) -> bool {
        if (!param || !source) return true;
        
        QComboBox *combo = static_cast<QComboBox*>(param);
        
        uint32_t flags = obs_source_get_output_flags(source);
        const char *id = obs_source_get_id(source);
        
        // Check if it's an audio source
        if ((flags & OBS_SOURCE_AUDIO) && id) {
            // Look for input capture sources
            if (strstr(id, "input_capture") || 
                strstr(id, "wasapi_input") ||
                strstr(id, "coreaudio_input") ||
                strstr(id, "pulse_input") ||
                strstr(id, "alsa_input")) {
                
                const char *name = obs_source_get_name(source);
                if (name) {
                    obs_source_get_ref(source);  // Add reference since we're storing pointer
                    combo->addItem(QString::fromUtf8(name), 
                                  QVariant::fromValue(reinterpret_cast<quintptr>(source)));
                }
            }
        }
        return true;
    };
    
    obs_enum_sources(enumCallback, sourceCombo);
    
    if (sourceCombo->count() == 0) {
        sourceCombo->addItem("No audio input sources found", QVariant());
        if (startButton) startButton->setEnabled(false);
        obs_log(LOG_WARNING, "[AudioCalibrator] No audio input sources found");
    } else {
        obs_log(LOG_INFO, "[AudioCalibrator] Found %d audio input sources", sourceCombo->count());
    }
    
    // Re-enable signals after population is complete
    sourceCombo->blockSignals(false);
}

obs_source_t* CalibrationDialog::getSelectedSource()
{
    if (!sourceCombo) return nullptr;
    
    int idx = sourceCombo->currentIndex();
    if (idx < 0) return nullptr;
    
    QVariant data = sourceCombo->itemData(idx);
    if (!data.isValid()) return nullptr;
    
    return reinterpret_cast<obs_source_t*>(data.value<quintptr>());
}

void CalibrationDialog::onSourceChanged(int index)
{
    // Safety check for invalid index or empty combo box
    if (index < 0 || !sourceCombo || sourceCombo->count() == 0) {
        return;
    }
    
    obs_source_t *source = getSelectedSource();
    if (source && updateTimer && updateTimer->isActive()) {
        if (audioAnalyzer) {
            audioAnalyzer->stopCapture();
            audioAnalyzer->startCapture(source);
        }
    }
}

void CalibrationDialog::updatePromptForStep()
{
    QString prompt;
    QString instruction;
    QString status;
    
    switch (currentStep) {
    case 1:
        prompt = "\"The quick brown fox jumps over the lazy dog.\"\n(Whisper quietly)";
        instruction = "Step 1 of 3: QUIET Voice\nSpeak in your quietest comfortable voice.";
        status = "Ready to record QUIET voice";
        promptLabel->setStyleSheet(QString("font-size: 14px; font-weight: bold; color: %1; padding: 4px;").arg("#64b5f6"));
        break;
    case 2:
        prompt = "\"She sells seashells by the seashore.\"\n(Normal voice)";
        instruction = "Step 2 of 3: NORMAL Voice\nSpeak in your regular talking voice.";
        status = "Ready to record NORMAL voice";
        promptLabel->setStyleSheet(QString("font-size: 14px; font-weight: bold; color: %1; padding: 4px;").arg("#81c784"));
        break;
    case 3:
        prompt = "\"PETER PIPER PICKED A PECK OF PICKLED PEPPERS!\"\n(Loud/Excited)";
        instruction = "Step 3 of 3: LOUD Voice\nSpeak in your loudest excited voice.";
        status = "Ready to record LOUD voice";
        promptLabel->setStyleSheet(QString("font-size: 14px; font-weight: bold; color: %1; padding: 4px;").arg("#ffb74d"));
        break;
    default:
        prompt = "";
        instruction = "";
        status = "";
        break;
    }
    
    if (promptLabel) promptLabel->setText(prompt);
    if (instructionLabel) instructionLabel->setText(instruction);
    if (statusLabel) {
        statusLabel->setText(status);
        statusLabel->setStyleSheet(QString("color: %1; padding: 6px; font-weight: bold; font-size: 11px;").arg(OBSColors::accent));
    }
}

void CalibrationDialog::onStartClicked()
{
    obs_source_t *source = getSelectedSource();
    if (!source) {
        QMessageBox::warning(this, "Error", "Please select a valid audio source.");
        return;
    }
    
    currentStep = 1;
    if (startButton) startButton->setEnabled(false);
    if (recordButton) recordButton->setEnabled(true);
    if (sourceCombo) sourceCombo->setEnabled(false);
    if (promptFrame) promptFrame->setVisible(true);
    if (optionsGroup) optionsGroup->setEnabled(false);
    
    // Start audio capture
    if (audioAnalyzer) {
        audioAnalyzer->startCapture(source);
    }
    if (updateTimer) {
        updateTimer->start(50);  // 20 FPS
    }
    
    updatePromptForStep();
    obs_log(LOG_INFO, "[AudioCalibrator] Calibration started");
}

void CalibrationDialog::onRecordClicked()
{
    if (!isRecording) {
        startRecording();
    }
}

void CalibrationDialog::startRecording()
{
    isRecording = true;
    recordingElapsedMs = 0;
    recordingFrames = 0;
    
    if (audioAnalyzer) {
        audioAnalyzer->resetMaxPeak();
    }
    
    if (recordButton) {
        recordButton->setText("Recording...");
        recordButton->setEnabled(false);
        recordButton->setStyleSheet(QString(
            "QPushButton { background-color: %1; border-color: %1; color: white; }"
        ).arg(OBSColors::recording));
    }
    
    if (countdownLabel) countdownLabel->setText("3");
    if (recordingProgress) recordingProgress->setValue(0);
    
    if (statusLabel) {
        statusLabel->setText("RECORDING - Speak now!");
        statusLabel->setStyleSheet(QString("color: %1; padding: 6px; font-weight: bold; font-size: 12px;").arg(OBSColors::recording));
    }
    
    // Start recording timer
    if (recordingTimer) {
        recordingTimer->start(RECORDING_TICK_MS);
    }
    
    obs_log(LOG_INFO, "[AudioCalibrator] Recording started for step %d", currentStep);
}

void CalibrationDialog::onRecordingTick()
{
    recordingElapsedMs += RECORDING_TICK_MS;
    
    if (recordingProgress) {
        recordingProgress->setValue(recordingElapsedMs);
    }
    
    // Update countdown
    int secondsLeft = (RECORDING_DURATION_MS - recordingElapsedMs + 999) / 1000;
    if (countdownLabel) {
        if (secondsLeft > 0) {
            countdownLabel->setText(QString::number(secondsLeft));
        } else {
            countdownLabel->setText("Done!");
        }
    }
    
    // Check if recording is complete
    if (recordingElapsedMs >= RECORDING_DURATION_MS) {
        if (recordingTimer) recordingTimer->stop();
        stopRecording();
        saveCurrentLevel();
        advanceStep();
    }
}

void CalibrationDialog::stopRecording()
{
    isRecording = false;
    
    if (recordButton) {
        recordButton->setText("Record");
        recordButton->setStyleSheet("");
    }
    
    if (countdownLabel) countdownLabel->setText("");
    if (recordingProgress) recordingProgress->setValue(RECORDING_DURATION_MS);
    
    if (statusLabel) {
        statusLabel->setStyleSheet(QString("color: %1; padding: 6px; font-weight: bold; font-size: 11px;").arg(OBSColors::success));
    }
    
    obs_log(LOG_INFO, "[AudioCalibrator] Recording stopped for step %d", currentStep);
}

void CalibrationDialog::saveCurrentLevel()
{
    float peakLevel = audioAnalyzer ? audioAnalyzer->getMaxPeak() : -60.0f;
    
    switch (currentStep) {
    case 1:
        quietLevel = peakLevel;
        if (quietResult) {
            quietResult->setText(QString("1. Quiet Level:  %1 dB").arg(quietLevel, 0, 'f', 1));
            quietResult->setStyleSheet(QString("color: %1; font-size: 11px;").arg(OBSColors::success));
        }
        break;
    case 2:
        normalLevel = peakLevel;
        if (normalResult) {
            normalResult->setText(QString("2. Normal Level: %1 dB").arg(normalLevel, 0, 'f', 1));
            normalResult->setStyleSheet(QString("color: %1; font-size: 11px;").arg(OBSColors::success));
        }
        break;
    case 3:
        loudLevel = peakLevel;
        if (loudResult) {
            loudResult->setText(QString("3. Loud Level:   %1 dB").arg(loudLevel, 0, 'f', 1));
            loudResult->setStyleSheet(QString("color: %1; font-size: 11px;").arg(OBSColors::success));
        }
        break;
    }
    
    updateResultsDisplay();
    obs_log(LOG_INFO, "[AudioCalibrator] Saved level for step %d: %.1f dB", currentStep, peakLevel);
}

void CalibrationDialog::updateResultsDisplay()
{
    if (currentStep >= 3 && rangeResult) {
        float range = loudLevel - quietLevel;
        rangeResult->setText(QString("Dynamic Range:  %1 dB").arg(range, 0, 'f', 1));
        
        // Color code the range
        if (range > 30) {
            rangeResult->setStyleSheet(QString("font-weight: bold; color: %1; margin-top: 2px; font-size: 11px;").arg(OBSColors::warning));
        } else if (range > 20) {
            rangeResult->setStyleSheet(QString("font-weight: bold; color: %1; margin-top: 2px; font-size: 11px;").arg("#ffeb3b"));
        } else {
            rangeResult->setStyleSheet(QString("font-weight: bold; color: %1; margin-top: 2px; font-size: 11px;").arg(OBSColors::success));
        }
    }
}

void CalibrationDialog::advanceStep()
{
    currentStep++;
    if (audioAnalyzer) {
        audioAnalyzer->resetMaxPeak();
    }
    
    switch (currentStep) {
    case 2:
    case 3:
        updatePromptForStep();
        if (recordButton) recordButton->setEnabled(true);
        if (recordingProgress) recordingProgress->setValue(0);
        break;
        
    case 4:
    {
        // Calibration complete
        if (recordButton) recordButton->setEnabled(false);
        if (applyButton) {
            applyButton->setEnabled(true);
            applyButton->setObjectName("primaryButton");
            applyButton->style()->unpolish(applyButton);
            applyButton->style()->polish(applyButton);
        }
        if (promptFrame) promptFrame->setVisible(false);
        if (optionsGroup) optionsGroup->setEnabled(true);
        
        float range = loudLevel - quietLevel;
        if (instructionLabel) {
            instructionLabel->setText(
                QString("Calibration Complete!\n"
                "Voice Range: %1 dB  (Quiet: %2 / Normal: %3 / Loud: %4)")
                .arg(range, 0, 'f', 1)
                .arg(quietLevel, 0, 'f', 1)
                .arg(normalLevel, 0, 'f', 1)
                .arg(loudLevel, 0, 'f', 1)
            );
            instructionLabel->setStyleSheet(QString("color: %1; padding: 4px;").arg(OBSColors::text));
        }
        if (statusLabel) {
            statusLabel->setText("Select filters and click Apply!");
            statusLabel->setStyleSheet(QString("color: %1; padding: 6px; font-weight: bold; font-size: 12px;").arg(OBSColors::success));
        }
        
        obs_log(LOG_INFO, "[AudioCalibrator] Calibration complete. Range: %.1f dB", range);
        break;
    }
    }
}

void CalibrationDialog::updateLevelMeter()
{
    if (!audioAnalyzer || !audioAnalyzer->isCapturing())
        return;
    
    float rms = audioAnalyzer->getCurrentRMS();
    float peak = audioAnalyzer->getCurrentPeak();
    
    // Convert dB to percentage (range: -60 to 0 dB)
    auto dbToPercent = [](float db) -> int {
        int percent = static_cast<int>((db + 60.0f) / 60.0f * 100.0f);
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        return percent;
    };
    
    if (levelMeter) levelMeter->setValue(dbToPercent(rms));
    if (peakMeter) peakMeter->setValue(dbToPercent(peak));
    
    // Update labels
    if (rmsLabel) {
        if (rms > -100) {
            rmsLabel->setText(QString("%1 dB").arg(rms, 0, 'f', 1));
        } else {
            rmsLabel->setText("-inf dB");
        }
    }
    
    if (peakLabel) {
        if (peak > -100) {
            peakLabel->setText(QString("%1 dB").arg(peak, 0, 'f', 1));
        } else {
            peakLabel->setText("-inf dB");
        }
    }
}

void CalibrationDialog::onResetClicked()
{
    // Stop everything
    if (updateTimer) updateTimer->stop();
    if (recordingTimer) recordingTimer->stop();
    if (audioAnalyzer) audioAnalyzer->stopCapture();
    isRecording = false;
    currentStep = 0;
    recordingElapsedMs = 0;
    
    // Reset UI
    if (startButton) startButton->setEnabled(true);
    if (recordButton) {
        recordButton->setEnabled(false);
        recordButton->setText("Record");
        recordButton->setStyleSheet("");
    }
    if (applyButton) {
        applyButton->setEnabled(false);
        applyButton->setObjectName("");
    }
    if (sourceCombo) sourceCombo->setEnabled(true);
    if (promptFrame) promptFrame->setVisible(false);
    if (optionsGroup) optionsGroup->setEnabled(true);
    
    // Reset levels
    quietLevel = -60.0f;
    normalLevel = -30.0f;
    loudLevel = -10.0f;
    
    // Reset display
    if (quietResult) {
        quietResult->setText("1. Quiet Level:  --");
        quietResult->setStyleSheet(QString("color: %1; font-size: 11px;").arg(OBSColors::textMuted));
    }
    if (normalResult) {
        normalResult->setText("2. Normal Level: --");
        normalResult->setStyleSheet(QString("color: %1; font-size: 11px;").arg(OBSColors::textMuted));
    }
    if (loudResult) {
        loudResult->setText("3. Loud Level:   --");
        loudResult->setStyleSheet(QString("color: %1; font-size: 11px;").arg(OBSColors::textMuted));
    }
    if (rangeResult) {
        rangeResult->setText("Dynamic Range:  --");
        rangeResult->setStyleSheet(QString("font-weight: bold; color: %1; margin-top: 2px; font-size: 11px;").arg(OBSColors::textMuted));
    }
    
    if (levelMeter) levelMeter->setValue(0);
    if (peakMeter) peakMeter->setValue(0);
    if (recordingProgress) recordingProgress->setValue(0);
    if (rmsLabel) rmsLabel->setText("-inf dB");
    if (peakLabel) peakLabel->setText("-inf dB");
    if (countdownLabel) countdownLabel->setText("");
    
    if (instructionLabel) {
        instructionLabel->setText(
            "Configure your microphone for consistent, professional audio.\n"
            "Record 3 voice samples, then apply optimized filters."
        );
        instructionLabel->setStyleSheet(QString("color: %1; padding: 4px;").arg(OBSColors::textMuted));
    }
    
    if (statusLabel) {
        statusLabel->setText("Reset - Ready to start again");
        statusLabel->setStyleSheet(QString("color: %1; padding: 6px; font-size: 11px;").arg(OBSColors::textMuted));
    }
    
    obs_log(LOG_INFO, "[AudioCalibrator] Reset complete");
}

// ============================================================================
// ROBUST FILTER HELPERS
// ============================================================================

bool CalibrationDialog::isFilterAvailable(const char* filterId)
{
    if (!filterId) return false;
    
    // Check if the filter type exists in OBS
    const char *name = obs_source_get_display_name(filterId);
    return name != nullptr && strlen(name) > 0;
}

bool CalibrationDialog::removeExistingFilter(obs_source_t* source, const char* filterName)
{
    if (!source || !filterName) return false;
    
    obs_source_t *existingFilter = obs_source_get_filter_by_name(source, filterName);
    if (existingFilter) {
        obs_source_filter_remove(source, existingFilter);
        obs_source_release(existingFilter);
        obs_log(LOG_INFO, "[AudioCalibrator] Removed existing filter: %s", filterName);
        return true;
    }
    return false;
}

bool CalibrationDialog::createFilter(obs_source_t* source, const char* filterId, 
                                      const char* filterName, obs_data_t* settings)
{
    if (!source || !filterId || !filterName) {
        obs_log(LOG_ERROR, "[AudioCalibrator] Invalid parameters for createFilter");
        return false;
    }
    
    // Remove existing filter first
    removeExistingFilter(source, filterName);
    
    // Create new filter
    obs_source_t *newFilter = obs_source_create(filterId, filterName, settings, nullptr);
    if (!newFilter) {
        obs_log(LOG_ERROR, "[AudioCalibrator] Failed to create filter: %s (type: %s)", filterName, filterId);
        return false;
    }
    
    obs_source_filter_add(source, newFilter);
    obs_source_release(newFilter);
    obs_log(LOG_INFO, "[AudioCalibrator] Created filter: %s", filterName);
    return true;
}

void CalibrationDialog::onApplyClicked()
{
    obs_source_t *source = getSelectedSource();
    if (!source) {
        QMessageBox::warning(this, "Error", "No audio source selected.");
        return;
    }
    
    // Calculate optimal settings
    float range = loudLevel - quietLevel;
    float targetOutput = -16.0f;  // Broadcast standard
    float gainNeeded = targetOutput - normalLevel;
    
    // Clamp gain to reasonable values
    gainNeeded = std::max(-20.0f, std::min(30.0f, gainNeeded));
    
    // Compressor settings based on range
    float threshold = normalLevel + (range * 0.3f);
    float ratio = 2.0f + (range / 15.0f);
    ratio = std::max(2.0f, std::min(8.0f, ratio));
    
    // Noise gate threshold based on quiet level
    float gateThreshold = quietLevel - 10.0f;  // Below quietest speech
    gateThreshold = std::max(-60.0f, std::min(-20.0f, gateThreshold));
    
    // Build summary
    QStringList filterList;
    if (enableNoiseSuppressionCheck && enableNoiseSuppressionCheck->isChecked()) {
        filterList << QString("  Noise Suppression (RNNoise): %1")
            .arg(noiseSuppressionLevel->currentText());
    }
    if (enableNoiseGateCheck && enableNoiseGateCheck->isChecked()) {
        filterList << QString("  Noise Gate: %1 dB threshold").arg(gateThreshold, 0, 'f', 1);
    }
    if (enableExpanderCheck && enableExpanderCheck->isChecked()) {
        filterList << QString("  Expander: %1 dB threshold").arg(gateThreshold + 5, 0, 'f', 1);
    }
    if (enableGainCheck && enableGainCheck->isChecked()) {
        filterList << QString("  Gain: %1 dB").arg(gainNeeded, 0, 'f', 1);
    }
    if (enableCompressorCheck && enableCompressorCheck->isChecked()) {
        filterList << QString("  Compressor: %1 dB threshold, %2:1 ratio")
            .arg(threshold, 0, 'f', 1).arg(ratio, 0, 'f', 1);
    }
    if (enableLimiterCheck && enableLimiterCheck->isChecked()) {
        filterList << "  Limiter: -6 dB ceiling";
    }
    
    if (filterList.isEmpty()) {
        QMessageBox::warning(this, "No Filters", "Please select at least one filter to apply.");
        return;
    }
    
    QString summary = QString(
        "The following filters will be applied:\n\n%1\n\n"
        "Filter order is optimized for best audio quality.\n"
        "Existing AutoCal filters will be replaced."
    ).arg(filterList.join("\n"));
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Apply Audio Filters");
    msgBox.setText(summary);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    msgBox.setStyleSheet(styleSheet());
    
    if (msgBox.exec() == QMessageBox::Yes) {
        applyFilters(gainNeeded, threshold, ratio);
    }
}

void CalibrationDialog::applyFilters(float gain, float threshold, float ratio)
{
    obs_source_t *source = getSelectedSource();
    if (!source) {
        obs_log(LOG_ERROR, "[AudioCalibrator] No source selected for filter application");
        return;
    }
    
    const char* sourceName = obs_source_get_name(source);
    obs_log(LOG_INFO, "[AudioCalibrator] Applying filters to: %s", sourceName ? sourceName : "unknown");
    
    int filtersApplied = 0;
    int filtersFailed = 0;
    
    // Calculate noise gate threshold from quiet level
    float gateThreshold = quietLevel - 10.0f;
    gateThreshold = std::max(-60.0f, std::min(-20.0f, gateThreshold));
    
    // ========================================================================
    // 1. NOISE SUPPRESSION (RNNoise) - First in chain
    // ========================================================================
    if (enableNoiseSuppressionCheck && enableNoiseSuppressionCheck->isChecked()) {
        obs_data_t *settings = obs_data_create();
        
        // RNNoise settings
        float suppressionDb = noiseSuppressionLevel->currentData().toFloat();
        obs_data_set_double(settings, "suppress_level", static_cast<double>(suppressionDb));
        
        // Try nvidia first, then rnnoise
        bool created = false;
        if (isFilterAvailable("noise_suppress_filter_v2")) {
            obs_data_set_string(settings, "method", "rnnoise");
            created = createFilter(source, "noise_suppress_filter_v2", "AutoCal-NoiseSuppression", settings);
        } else if (isFilterAvailable("noise_suppress_filter")) {
            created = createFilter(source, "noise_suppress_filter", "AutoCal-NoiseSuppression", settings);
        }
        
        if (created) filtersApplied++; else filtersFailed++;
        obs_data_release(settings);
    }
    
    // ========================================================================
    // 2. NOISE GATE - Cuts below threshold
    // ========================================================================
    if (enableNoiseGateCheck && enableNoiseGateCheck->isChecked()) {
        obs_data_t *settings = obs_data_create();
        obs_data_set_double(settings, "open_threshold", static_cast<double>(gateThreshold + 5));
        obs_data_set_double(settings, "close_threshold", static_cast<double>(gateThreshold));
        obs_data_set_double(settings, "attack_time", 25.0);
        obs_data_set_double(settings, "hold_time", 200.0);
        obs_data_set_double(settings, "release_time", 150.0);
        
        if (createFilter(source, "noise_gate_filter", "AutoCal-NoiseGate", settings)) {
            filtersApplied++;
        } else {
            filtersFailed++;
        }
        obs_data_release(settings);
    }
    
    // ========================================================================
    // 3. EXPANDER - Gentle noise reduction alternative
    // ========================================================================
    if (enableExpanderCheck && enableExpanderCheck->isChecked()) {
        obs_data_t *settings = obs_data_create();
        obs_data_set_double(settings, "ratio", 4.0);
        obs_data_set_double(settings, "threshold", static_cast<double>(gateThreshold + 10));
        obs_data_set_double(settings, "attack_time", 10.0);
        obs_data_set_double(settings, "release_time", 100.0);
        obs_data_set_double(settings, "output_gain", 0.0);
        obs_data_set_string(settings, "detector", "RMS");
        obs_data_set_string(settings, "presets", "expander");
        
        if (createFilter(source, "expander_filter", "AutoCal-Expander", settings)) {
            filtersApplied++;
        } else {
            filtersFailed++;
        }
        obs_data_release(settings);
    }
    
    // ========================================================================
    // 4. GAIN - Volume adjustment
    // ========================================================================
    if (enableGainCheck && enableGainCheck->isChecked()) {
        obs_data_t *settings = obs_data_create();
        obs_data_set_double(settings, "db", static_cast<double>(gain));
        
        if (createFilter(source, "gain_filter", "AutoCal-Gain", settings)) {
            filtersApplied++;
        } else {
            filtersFailed++;
        }
        obs_data_release(settings);
    }
    
    // ========================================================================
    // 5. COMPRESSOR - Dynamic range reduction
    // ========================================================================
    if (enableCompressorCheck && enableCompressorCheck->isChecked()) {
        obs_data_t *settings = obs_data_create();
        obs_data_set_double(settings, "ratio", static_cast<double>(ratio));
        obs_data_set_double(settings, "threshold", static_cast<double>(threshold));
        obs_data_set_double(settings, "attack_time", 6.0);
        obs_data_set_double(settings, "release_time", 60.0);
        obs_data_set_double(settings, "output_gain", 0.0);
        obs_data_set_string(settings, "sidechain_source", "");
        
        if (createFilter(source, "compressor_filter", "AutoCal-Compressor", settings)) {
            filtersApplied++;
        } else {
            filtersFailed++;
        }
        obs_data_release(settings);
    }
    
    // ========================================================================
    // 6. LIMITER - Final protection
    // ========================================================================
    if (enableLimiterCheck && enableLimiterCheck->isChecked()) {
        obs_data_t *settings = obs_data_create();
        obs_data_set_double(settings, "threshold", -6.0);
        obs_data_set_double(settings, "release_time", 60.0);
        
        if (createFilter(source, "limiter_filter", "AutoCal-Limiter", settings)) {
            filtersApplied++;
        } else {
            filtersFailed++;
        }
        obs_data_release(settings);
    }
    
    // ========================================================================
    // RESULT
    // ========================================================================
    obs_log(LOG_INFO, "[AudioCalibrator] Applied %d filters, %d failed", filtersApplied, filtersFailed);
    
    QString resultMsg;
    if (filtersFailed == 0) {
        resultMsg = QString("Successfully applied %1 filter(s)!\n\n"
            "Check your source's Filters panel.\n"
            "All filters are prefixed with 'AutoCal-'\n\n"
            "You can fine-tune these settings anytime in OBS.")
            .arg(filtersApplied);
    } else {
        resultMsg = QString("Applied %1 filter(s), but %2 failed.\n\n"
            "Some filters may not be available in your OBS version.\n"
            "Check the OBS log for details.")
            .arg(filtersApplied).arg(filtersFailed);
    }
    
    QMessageBox successBox(this);
    successBox.setWindowTitle(filtersFailed == 0 ? "Success" : "Partial Success");
    successBox.setText(resultMsg);
    successBox.setStyleSheet(styleSheet());
    successBox.exec();
    
    if (filtersFailed == 0) {
        accept();
    }
}
