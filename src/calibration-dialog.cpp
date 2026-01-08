/*
 * Calibration Dialog Implementation
 * Copyright (C) 2025
 */

#include "calibration-dialog.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include "plugin-support.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QSpacerItem>

#include <algorithm>
#include <cmath>

static int dbToPercent(float db)
{
	// Map [-60..0] dB -> [0..100]
	if (db <= -60.0f)
		return 0;
	if (db >= 0.0f)
		return 100;
	return static_cast<int>(((db + 60.0f) / 60.0f) * 100.0f);
}

static float clampf(float value, float minValue, float maxValue)
{
	return std::max(minValue, std::min(value, maxValue));
}

CalibrationDialog::CalibrationDialog(QWidget *parent)
	: QDialog(parent), audioAnalyzer(std::make_unique<AudioAnalyzer>())
{
	setWindowTitle("Audio Calibration Wizard");
	setModal(false);
	setMinimumWidth(740);

	currentStep = 0;
	isRecording = false;
	recordingFrames = 0;
	recordingElapsedMs = 0;

	for (int i = 0; i < TOTAL_STEPS; i++)
		levels[i] = -100.0f;
	for (int i = 0; i < TOTAL_STEPS; i++)
		peaks[i] = -100.0f;

	recordingRmsSumLinear = 0.0;
	recordingRmsSamples = 0;
	recordingPeakMaxDb = -100.0f;

	setupUI();
	setupStyles();
	populateAudioSources();

	updateTimer = new QTimer(this);
	connect(updateTimer, &QTimer::timeout, this, &CalibrationDialog::updateLevelMeter);
	updateTimer->start(50);

	recordingTimer = new QTimer(this);
	connect(recordingTimer, &QTimer::timeout, this, &CalibrationDialog::onRecordingTick);
}

CalibrationDialog::~CalibrationDialog()
{
	if (audioAnalyzer)
		audioAnalyzer->stopCapture();
}

void CalibrationDialog::setupUI()
{
	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->setSpacing(12);

	// Title
	titleLabel = new QLabel("Professional Audio Calibration");
	titleLabel->setObjectName("titleLabel");
	mainLayout->addWidget(titleLabel);

	instructionLabel = new QLabel("Select your microphone/audio source, then start calibration.");
	instructionLabel->setWordWrap(true);
	mainLayout->addWidget(instructionLabel);

	// Recording frame (top + prominent)
	recordingFrame = new QFrame(this);
	recordingFrame->setObjectName("recordingFrame");
	auto *recordLayout = new QVBoxLayout(recordingFrame);
	recordLayout->setSpacing(8);

	auto *recordTopRow = new QHBoxLayout();
	stepIndicatorLabel = new QLabel("Step: — / 8");
	stepIndicatorLabel->setObjectName("stepIndicator");
	recordTopRow->addWidget(stepIndicatorLabel);
	recordTopRow->addStretch();

	recordButton = new QPushButton("Record");
	recordButton->setEnabled(false);
	recordButton->setMinimumHeight(36);
	connect(recordButton, &QPushButton::clicked, this, &CalibrationDialog::onRecordClicked);
	recordTopRow->addWidget(recordButton);

	recordLayout->addLayout(recordTopRow);

	promptLabel = new QLabel("Press Start to begin.");
	promptLabel->setWordWrap(true);
	promptLabel->setObjectName("promptLabel");
	recordLayout->addWidget(promptLabel);

	countdownLabel = new QLabel(" ");
	recordLayout->addWidget(countdownLabel);

	recordingProgress = new QProgressBar(this);
	recordingProgress->setRange(0, RECORDING_DURATION_MS);
	recordingProgress->setValue(0);
	recordLayout->addWidget(recordingProgress);

	mainLayout->addWidget(recordingFrame);

	// Source selection
	auto *sourceRow = new QHBoxLayout();
	sourceRow->addWidget(new QLabel("Audio source:"));
	sourceCombo = new QComboBox(this);
	connect(sourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CalibrationDialog::onSourceChanged);
	sourceRow->addWidget(sourceCombo, 1);
	mainLayout->addLayout(sourceRow);

	// Meters
	meterGroup = new QGroupBox("Live Levels", this);
	auto *meterLayout = new QGridLayout(meterGroup);

	levelMeter = new QProgressBar(this);
	levelMeter->setRange(0, 100);
	levelMeter->setTextVisible(false);
	peakMeter = new QProgressBar(this);
	peakMeter->setRange(0, 100);
	peakMeter->setTextVisible(false);

	rmsLabel = new QLabel("RMS: -∞ dB");
	peakLabel = new QLabel("Peak: -∞ dB");

	meterLayout->addWidget(new QLabel("RMS"), 0, 0);
	meterLayout->addWidget(levelMeter, 0, 1);
	meterLayout->addWidget(rmsLabel, 0, 2);
	meterLayout->addWidget(new QLabel("Peak"), 1, 0);
	meterLayout->addWidget(peakMeter, 1, 1);
	meterLayout->addWidget(peakLabel, 1, 2);

	mainLayout->addWidget(meterGroup);

	// Filters
	basicFiltersGroup = new QGroupBox("Filters (Basic)", this);
	auto *basicLayout = new QGridLayout(basicFiltersGroup);

	enableNoiseSuppressionCheck = new QCheckBox("Noise suppression", this);
	enableNoiseGateCheck = new QCheckBox("Noise gate", this);
	enableExpanderCheck = new QCheckBox("Expander", this);
	enableGainCheck = new QCheckBox("Gain", this);
	enableCompressorCheck = new QCheckBox("Compressor", this);
	enableLimiterCheck = new QCheckBox("Limiter", this);

	enableNoiseSuppressionCheck->setChecked(true);
	enableNoiseGateCheck->setChecked(true);
	enableExpanderCheck->setChecked(true);
	enableGainCheck->setChecked(true);
	enableCompressorCheck->setChecked(true);
	enableLimiterCheck->setChecked(true);

	noiseSuppressionLevel = new QComboBox(this);
	noiseSuppressionLevel->addItems({"Low (-15 dB)", "Medium (-25 dB)", "High (-35 dB)"});
	noiseSuppressionLevel->setCurrentIndex(1);

	basicLayout->addWidget(enableNoiseSuppressionCheck, 0, 0);
	basicLayout->addWidget(noiseSuppressionLevel, 0, 1);
	basicLayout->addWidget(enableNoiseGateCheck, 1, 0);
	basicLayout->addWidget(enableExpanderCheck, 2, 0);
	basicLayout->addWidget(enableGainCheck, 3, 0);
	basicLayout->addWidget(enableCompressorCheck, 4, 0);
	basicLayout->addWidget(enableLimiterCheck, 5, 0);

	mainLayout->addWidget(basicFiltersGroup);

	advancedFiltersGroup = new QGroupBox("Filters (Advanced)", this);
	auto *advLayout = new QGridLayout(advancedFiltersGroup);

	enableHighPassCheck = new QCheckBox("High-pass", this);
	enableLowPassCheck = new QCheckBox("Low-pass", this);
	enableDeEsserCheck = new QCheckBox("De-esser", this);
	enableVSTCheck = new QCheckBox("VST plugin", this);

	highPassFreq = new QComboBox(this);
	highPassFreq->addItems({"80 Hz", "100 Hz", "120 Hz"});
	highPassFreq->setCurrentIndex(0);

	lowPassFreq = new QComboBox(this);
	lowPassFreq->addItems({"12 kHz", "10 kHz", "8 kHz"});
	lowPassFreq->setCurrentIndex(0);

	deEsserIntensity = new QComboBox(this);
	deEsserIntensity->addItems({"Light", "Medium", "Strong"});
	deEsserIntensity->setCurrentIndex(1);

	advLayout->addWidget(enableHighPassCheck, 0, 0);
	advLayout->addWidget(highPassFreq, 0, 1);
	advLayout->addWidget(enableLowPassCheck, 1, 0);
	advLayout->addWidget(lowPassFreq, 1, 1);
	advLayout->addWidget(enableDeEsserCheck, 2, 0);
	advLayout->addWidget(deEsserIntensity, 2, 1);
	advLayout->addWidget(enableVSTCheck, 3, 0);

	mainLayout->addWidget(advancedFiltersGroup);

	// Results
	resultsGroup = new QGroupBox("Results", this);
	auto *resultsLayout = new QGridLayout(resultsGroup);

	auto makeResultLabel = [this](const QString &text) {
		auto *label = new QLabel(text, this);
		label->setMinimumWidth(240);
		return label;
	};

	step1Result = makeResultLabel("1) —");
	step2Result = makeResultLabel("2) —");
	step3Result = makeResultLabel("3) —");
	step4Result = makeResultLabel("4) —");
	step5Result = makeResultLabel("5) —");
	step6Result = makeResultLabel("6) —");
	step7Result = makeResultLabel("7) —");
	step8Result = makeResultLabel("8) —");
	rangeResult = makeResultLabel("Range: —");
	avgResult = makeResultLabel("Average: —");

	resultsLayout->addWidget(step1Result, 0, 0);
	resultsLayout->addWidget(step2Result, 1, 0);
	resultsLayout->addWidget(step3Result, 2, 0);
	resultsLayout->addWidget(step4Result, 3, 0);
	resultsLayout->addWidget(step5Result, 0, 1);
	resultsLayout->addWidget(step6Result, 1, 1);
	resultsLayout->addWidget(step7Result, 2, 1);
	resultsLayout->addWidget(step8Result, 3, 1);
	resultsLayout->addWidget(rangeResult, 4, 0);
	resultsLayout->addWidget(avgResult, 4, 1);

	mainLayout->addWidget(resultsGroup);

	// Status + buttons
	statusLabel = new QLabel("Ready.");
	statusLabel->setWordWrap(true);
	mainLayout->addWidget(statusLabel);

	auto *buttonsRow = new QHBoxLayout();
	startButton = new QPushButton("Start");
	connect(startButton, &QPushButton::clicked, this, &CalibrationDialog::onStartClicked);

	applyButton = new QPushButton("Apply to source");
	applyButton->setEnabled(false);
	connect(applyButton, &QPushButton::clicked, this, &CalibrationDialog::onApplyClicked);

	resetButton = new QPushButton("Reset");
	connect(resetButton, &QPushButton::clicked, this, &CalibrationDialog::onResetClicked);

	buttonsRow->addWidget(startButton);
	buttonsRow->addWidget(applyButton);
	buttonsRow->addStretch();
	buttonsRow->addWidget(resetButton);

	mainLayout->addLayout(buttonsRow);
}

void CalibrationDialog::setupStyles()
{
	// Keep styling minimal; rely on OBS/Qt theme where possible.
	setStyleSheet(
		"QLabel#titleLabel { font-size: 18px; font-weight: 600; }"
		"QFrame#recordingFrame { border: 1px solid rgba(255,255,255,0.15); border-radius: 6px; padding: 10px; }"
		"QLabel#promptLabel { font-size: 14px; font-weight: 500; }"
		"QLabel#stepIndicator { font-weight: 600; }"
	);
}

void CalibrationDialog::populateAudioSources()
{
	sourceCombo->clear();
	sourceCombo->addItem("Select a source...");

	struct EnumCtx {
		QComboBox *combo;
	};

	EnumCtx ctx{sourceCombo};
	obs_enum_sources(
		[](void *param, obs_source_t *source) {
			auto *c = static_cast<EnumCtx *>(param);
			if (!source)
				return true;

			const uint32_t flags = obs_source_get_output_flags(source);
			if ((flags & OBS_SOURCE_AUDIO) == 0)
				return true;

			const char *name = obs_source_get_name(source);
			if (!name || !*name)
				return true;

			c->combo->addItem(QString::fromUtf8(name));
			return true;
		},
		&ctx);
}

obs_source_t *CalibrationDialog::getSelectedSource()
{
	if (sourceCombo->currentIndex() <= 0)
		return nullptr;

	const QString name = sourceCombo->currentText();
	if (name.isEmpty())
		return nullptr;

	return obs_get_source_by_name(name.toUtf8().constData());
}

void CalibrationDialog::onSourceChanged(int)
{
	if (!audioAnalyzer)
		return;

	obs_source_t *source = getSelectedSource();
	if (!source) {
		audioAnalyzer->stopCapture();
		statusLabel->setText("Select a valid audio source.");
		return;
	}

	audioAnalyzer->startCapture(source);
	obs_source_release(source);
	statusLabel->setText("Capturing audio from selected source.");
}

void CalibrationDialog::onStartClicked()
{
	if (sourceCombo->currentIndex() <= 0) {
		statusLabel->setText("Please select an audio source first.");
		return;
	}

	// Ensure capture is active
	onSourceChanged(sourceCombo->currentIndex());

	currentStep = 1;
	startButton->setEnabled(false);
	recordButton->setEnabled(true);
	applyButton->setEnabled(false);

	for (int i = 0; i < TOTAL_STEPS; i++)
		levels[i] = -100.0f;
	for (int i = 0; i < TOTAL_STEPS; i++)
		peaks[i] = -100.0f;

	updatePromptForStep();
	updateResultsDisplay();
	statusLabel->setText("Step 1 ready. Press Record when you are ready.");
}

void CalibrationDialog::onRecordClicked()
{
	if (currentStep <= 0 || currentStep > TOTAL_STEPS) {
		statusLabel->setText("Press Start to begin calibration.");
		return;
	}

	if (isRecording) {
		stopRecording();
		return;
	}

	startRecording();
}

void CalibrationDialog::startRecording()
{
	if (!audioAnalyzer || !audioAnalyzer->isCapturing()) {
		statusLabel->setText("Audio capture is not active. Select a source.");
		return;
	}

	isRecording = true;
	recordButton->setText("Stop");

	recordingElapsedMs = 0;
	recordingFrames = 0; // reused as sample counter
	recordingProgress->setRange(0, RECORDING_DURATION_MS);
	recordingProgress->setValue(0);

	// Initialize recording-window accumulators
	recordingRmsSumLinear = 0.0;
	recordingRmsSamples = 0;
	recordingPeakMaxDb = -100.0f;
	levels[currentStep - 1] = -100.0f;
	peaks[currentStep - 1] = -100.0f;
	audioAnalyzer->resetMaxPeak();

	onRecordingTick();
	recordingTimer->start(RECORDING_TICK_MS);

	statusLabel->setText("Recording... speak the prompt naturally.");
}

void CalibrationDialog::stopRecording()
{
	if (!isRecording)
		return;

	isRecording = false;
	recordButton->setText("Record");
	recordingTimer->stop();

	saveCurrentLevel();
	updateResultsDisplay();

	advanceStep();
}

void CalibrationDialog::saveCurrentLevel()
{
	if (currentStep < 1 || currentStep > TOTAL_STEPS)
		return;

	const int index = currentStep - 1;
	const float avgRms = levels[index];
	float maxPeak = peaks[index];
	if (maxPeak <= -99.0f)
		maxPeak = audioAnalyzer ? audioAnalyzer->getMaxPeak() : -100.0f;

	statusLabel->setText(QString("Saved step %1: avg RMS %2 dB, max peak %3 dB")
					.arg(currentStep)
					.arg(avgRms, 0, 'f', 1)
					.arg(maxPeak, 0, 'f', 1));
}

void CalibrationDialog::advanceStep()
{
	if (currentStep < 1)
		return;

	if (currentStep >= TOTAL_STEPS) {
		currentStep = TOTAL_STEPS + 1;
		recordButton->setEnabled(false);
		applyButton->setEnabled(true);
		statusLabel->setText("Calibration complete. Review results and click Apply.");
		stepIndicatorLabel->setText("Complete");
		promptLabel->setText("Calibration complete.");
		countdownLabel->setText(" ");
		return;
	}

	currentStep++;
	updatePromptForStep();
	statusLabel->setText(QString("Step %1 ready. Press Record when ready.").arg(currentStep));
}

void CalibrationDialog::updatePromptForStep()
{
	if (currentStep < 1 || currentStep > TOTAL_STEPS) {
		stepIndicatorLabel->setText("Step: — / 8");
		promptLabel->setText("Press Start to begin.");
		instructionLabel->setText("Select your microphone/audio source, then start calibration.");
		return;
	}

	stepIndicatorLabel->setText(QString("Step: %1 / %2").arg(currentStep).arg(TOTAL_STEPS));
	promptLabel->setText(getPromptForStep(currentStep));
	instructionLabel->setText(getInstructionForStep(currentStep));
}

QString CalibrationDialog::getPromptForStep(int step)
{
	switch (step) {
	case 1:
		return "Room noise: stay silent";
	case 2:
		return "Whisper: \"this is a quiet test\"";
	case 3:
		return "Soft voice: \"today is a good day\"";
	case 4:
		return "Normal voice: \"I will speak clearly into the mic\"";
	case 5:
		return "Normal (steady): \"my voice should sound consistent\"";
	case 6:
		return "Energetic: \"welcome everyone, thanks for joining\"";
	case 7:
		return "S sounds: \"simple sounds stay smooth\"";
	case 8:
		return "Plosives: \"please put the popcorn back\"";
	default:
		return "";
	}
}

QString CalibrationDialog::getInstructionForStep(int step)
{
	switch (step) {
	case 1:
		return "Be quiet for 5 seconds so we can measure room noise.";
	case 2:
		return "Speak very softly, close to the mic.";
	case 3:
		return "Speak softly as if someone is sleeping nearby.";
	case 4:
		return "Speak normally like a call/meeting.";
	case 5:
		return "Keep a steady normal volume for the full 5 seconds.";
	case 6:
		return "Speak with energy (like streaming), but don’t shout.";
	case 7:
		return "Say it normally; this helps sibilance tuning.";
	case 8:
		return "Say it normally; this helps plosive handling.";
	default:
		return "";
	}
}

void CalibrationDialog::onRecordingTick()
{
	if (!isRecording)
		return;

	recordingElapsedMs += RECORDING_TICK_MS;
	if (recordingElapsedMs > RECORDING_DURATION_MS)
		recordingElapsedMs = RECORDING_DURATION_MS;

	recordingProgress->setValue(recordingElapsedMs);

	const int remainingMs = RECORDING_DURATION_MS - recordingElapsedMs;
	countdownLabel->setText(QString("Time remaining: %1.%2s")
					.arg(remainingMs / 1000)
					.arg((remainingMs % 1000) / 100));

	// Accumulate RMS in linear space for a more stable average.
	if (audioAnalyzer && audioAnalyzer->isCapturing()) {
		const float rmsDb = audioAnalyzer->getCurrentRMS();
		const float peakDb = audioAnalyzer->getCurrentPeak();

		recordingRmsSumLinear += static_cast<double>(AudioAnalyzer::fromDB(rmsDb));
		recordingRmsSamples++;
		recordingPeakMaxDb = std::max(recordingPeakMaxDb, peakDb);

		const double avgLinear = recordingRmsSamples > 0
					? (recordingRmsSumLinear / static_cast<double>(recordingRmsSamples))
					: 0.0;
		levels[currentStep - 1] = AudioAnalyzer::toDB(static_cast<float>(avgLinear));
		peaks[currentStep - 1] = recordingPeakMaxDb;
	}

	if (recordingElapsedMs >= RECORDING_DURATION_MS) {
		stopRecording();
	}
}

void CalibrationDialog::updateLevelMeter()
{
	if (!audioAnalyzer || !audioAnalyzer->isCapturing()) {
		levelMeter->setValue(0);
		peakMeter->setValue(0);
		rmsLabel->setText("RMS: -∞ dB");
		peakLabel->setText("Peak: -∞ dB");
		return;
	}

	const float rms = audioAnalyzer->getCurrentRMS();
	const float peak = audioAnalyzer->getCurrentPeak();

	levelMeter->setValue(dbToPercent(rms));
	peakMeter->setValue(dbToPercent(peak));
	rmsLabel->setText(QString("RMS: %1 dB").arg(rms, 0, 'f', 1));
	peakLabel->setText(QString("Peak: %1 dB").arg(peak, 0, 'f', 1));
}

void CalibrationDialog::updateResultsDisplay()
{
	auto fmt = [](int idx, float v) {
		if (v <= -99.0f)
			return QString("%1) —").arg(idx);
		return QString("%1) %2 dB").arg(idx).arg(v, 0, 'f', 1);
	};

	step1Result->setText(fmt(1, levels[0]));
	step2Result->setText(fmt(2, levels[1]));
	step3Result->setText(fmt(3, levels[2]));
	step4Result->setText(fmt(4, levels[3]));
	step5Result->setText(fmt(5, levels[4]));
	step6Result->setText(fmt(6, levels[5]));
	step7Result->setText(fmt(7, levels[6]));
	step8Result->setText(fmt(8, levels[7]));

	float minV = 999.0f;
	float maxV = -999.0f;
	double sum = 0.0;
	int count = 0;

	for (int i = 0; i < TOTAL_STEPS; i++) {
		if (levels[i] <= -99.0f)
			continue;
		minV = std::min(minV, levels[i]);
		maxV = std::max(maxV, levels[i]);
		sum += levels[i];
		count++;
	}

	if (count == 0) {
		rangeResult->setText("Range: —");
		avgResult->setText("Average: —");
		return;
	}

	const float avg = static_cast<float>(sum / static_cast<double>(count));
	rangeResult->setText(QString("Range: %1 .. %2 dB").arg(minV, 0, 'f', 1).arg(maxV, 0, 'f', 1));
	avgResult->setText(QString("Average: %1 dB").arg(avg, 0, 'f', 1));
}

void CalibrationDialog::onApplyClicked()
{
	if (currentStep <= TOTAL_STEPS) {
		statusLabel->setText("Finish all steps before applying.");
		return;
	}

	obs_source_t *source = getSelectedSource();
	if (!source) {
		statusLabel->setText("Select a valid audio source.");
		return;
	}

	for (int i = 0; i < TOTAL_STEPS; i++) {
		if (levels[i] <= -99.0f) {
			statusLabel->setText("Calibration data incomplete. Please rerun and complete all steps.");
			obs_source_release(source);
			return;
		}
	}

	// After the Step-1 shift, "program" voice is steps 4-6 (normal/steady/energetic)
	const float noiseFloor = levels[0];
	const float normal = levels[3];
	const float steady = levels[4];
	const float energetic = levels[5];
	const float avgProgram = (normal + steady + energetic) / 3.0f;
	const float loudPeak = std::max({peaks[3], peaks[4], peaks[5]});
	const float dynamic = energetic - normal;

	// Target a stable RMS around -18 dB for OBS meters
	const float targetRms = -18.0f;
	float gainDb = clampf(targetRms - avgProgram, -18.0f, 18.0f);

	// Prevent obvious clipping by keeping predicted peak under -3 dBFS
	const float predictedPeakAfterGain = loudPeak + gainDb;
	if (predictedPeakAfterGain > -3.0f)
		gainDb -= (predictedPeakAfterGain + 3.0f);
	gainDb = clampf(gainDb, -18.0f, 18.0f);

	float ratio = 4.0f;
	if (dynamic > 14.0f)
		ratio = 6.0f;
	else if (dynamic < 8.0f)
		ratio = 3.0f;

	// Compressor threshold: slightly under program RMS
	float thresholdDb = clampf(avgProgram - 5.0f, -45.0f, -10.0f);
	(void)noiseFloor;

	applyFilters(gainDb, thresholdDb, ratio);

	obs_source_release(source);
	statusLabel->setText("Filters applied to selected source.");
}

void CalibrationDialog::onResetClicked()
{
	if (isRecording)
		stopRecording();

	currentStep = 0;
	startButton->setEnabled(true);
	recordButton->setEnabled(false);
	applyButton->setEnabled(false);

	for (int i = 0; i < TOTAL_STEPS; i++)
		levels[i] = -100.0f;
	for (int i = 0; i < TOTAL_STEPS; i++)
		peaks[i] = -100.0f;

	recordingProgress->setValue(0);
	stepIndicatorLabel->setText("Step: — / 8");
	promptLabel->setText("Press Start to begin.");
	countdownLabel->setText(" ");
	instructionLabel->setText("Select your microphone/audio source, then start calibration.");
	statusLabel->setText("Ready.");
	updateResultsDisplay();
}

bool CalibrationDialog::isFilterAvailable(const char *filterId)
{
	if (!filterId || !*filterId)
		return false;
	return obs_get_source_output_flags(filterId) != 0;
}

bool CalibrationDialog::removeExistingFilter(obs_source_t *source, const char *filterName)
{
	if (!source || !filterName)
		return false;

	obs_source_t *filter = obs_source_get_filter_by_name(source, filterName);
	if (!filter)
		return false;

	obs_source_filter_remove(source, filter);
	obs_source_release(filter);
	return true;
}

bool CalibrationDialog::createFilter(obs_source_t *source, const char *filterId, const char *filterName,
					obs_data_t *settings)
{
	if (!source || !filterId || !filterName)
		return false;

	if (!isFilterAvailable(filterId)) {
		obs_log(LOG_WARNING, "[AudioCalibrator] Filter id not available: %s", filterId);
		return false;
	}

	obs_source_t *filter = obs_source_create(filterId, filterName, settings, nullptr);
	if (!filter) {
		obs_log(LOG_WARNING, "[AudioCalibrator] Failed creating filter %s (%s)", filterName, filterId);
		return false;
	}

	obs_source_filter_add(source, filter);
	obs_source_release(filter);
	return true;
}

void CalibrationDialog::applyFilters(float gain, float threshold, float ratio)
{
	obs_source_t *source = getSelectedSource();
	if (!source)
		return;

	const float noiseFloor = levels[0];
	const float avgProgram = (levels[3] + levels[4] + levels[5]) / 3.0f;
	const float gateOpenDb = clampf(std::max(noiseFloor + 15.0f, avgProgram - 25.0f), -60.0f, -10.0f);
	const float gateCloseDb = clampf(gateOpenDb - 6.0f, -60.0f, -12.0f);

	// Remove our previously-applied filters first (idempotent)
	removeExistingFilter(source, "Audio Calibrator - Noise Suppression");
	removeExistingFilter(source, "Audio Calibrator - Noise Gate");
	removeExistingFilter(source, "Audio Calibrator - Expander");
	removeExistingFilter(source, "Audio Calibrator - Gain");
	removeExistingFilter(source, "Audio Calibrator - Compressor");
	removeExistingFilter(source, "Audio Calibrator - Limiter");
	removeExistingFilter(source, "Audio Calibrator - EQ");
	removeExistingFilter(source, "Audio Calibrator - VST");

	// Noise suppression
	if (enableNoiseSuppressionCheck->isChecked()) {
		obs_data_t *settings = obs_data_create();
		int suppressLevel = -25;
		switch (noiseSuppressionLevel->currentIndex()) {
		case 0: suppressLevel = -15; break;
		case 1: suppressLevel = -25; break;
		case 2: suppressLevel = -35; break;
		default: break;
		}
		obs_data_set_int(settings, "suppress_level", suppressLevel);
		obs_data_set_string(settings, "method", "rnnoise");
		createFilter(source, "noise_suppress_filter", "Audio Calibrator - Noise Suppression", settings);
		obs_data_release(settings);
	}

	// Noise gate
	if (enableNoiseGateCheck->isChecked()) {
		obs_data_t *settings = obs_data_create();
		obs_data_set_double(settings, "open_threshold", static_cast<double>(gateOpenDb));
		obs_data_set_double(settings, "close_threshold", static_cast<double>(gateCloseDb));
		obs_data_set_int(settings, "attack_time", 25);
		obs_data_set_int(settings, "hold_time", 200);
		obs_data_set_int(settings, "release_time", 150);
		createFilter(source, "noise_gate_filter", "Audio Calibrator - Noise Gate", settings);
		obs_data_release(settings);
	}

	// Expander (gentle)
	if (enableExpanderCheck->isChecked()) {
		obs_data_t *settings = obs_data_create();
		obs_data_set_string(settings, "presets", "expander");
		obs_data_set_double(settings, "ratio", 2.0);
		obs_data_set_double(settings, "threshold", -40.0);
		obs_data_set_int(settings, "attack_time", 10);
		obs_data_set_int(settings, "release_time", 50);
		obs_data_set_double(settings, "output_gain", 0.0);
		obs_data_set_string(settings, "detector", "RMS");
		createFilter(source, "expander_filter", "Audio Calibrator - Expander", settings);
		obs_data_release(settings);
	}

	// Gain
	if (enableGainCheck->isChecked()) {
		obs_data_t *settings = obs_data_create();
		obs_data_set_double(settings, "db", static_cast<double>(gain));
		createFilter(source, "gain_filter", "Audio Calibrator - Gain", settings);
		obs_data_release(settings);
	}

	// Compressor
	if (enableCompressorCheck->isChecked()) {
		obs_data_t *settings = obs_data_create();
		obs_data_set_double(settings, "threshold", static_cast<double>(threshold));
		obs_data_set_double(settings, "ratio", static_cast<double>(ratio));
		obs_data_set_int(settings, "attack_time", 6);
		obs_data_set_int(settings, "release_time", 60);
		obs_data_set_double(settings, "output_gain", 0.0);
		obs_data_set_string(settings, "sidechain_source", "none");
		createFilter(source, "compressor_filter", "Audio Calibrator - Compressor", settings);
		obs_data_release(settings);
	}

	// Limiter
	if (enableLimiterCheck->isChecked()) {
		obs_data_t *settings = obs_data_create();
		obs_data_set_double(settings, "threshold", -3.0);
		obs_data_set_int(settings, "release_time", 60);
		createFilter(source, "limiter_filter", "Audio Calibrator - Limiter", settings);
		obs_data_release(settings);
	}

	// Advanced: EQ-based approximations for high-pass/low-pass/de-esser
	float lowDb = 0.0f;
	float midDb = 0.0f;
	float highDb = 0.0f;

	if (enableHighPassCheck->isChecked()) {
		switch (highPassFreq->currentIndex()) {
		case 0: lowDb -= 4.0f; break;  // 80 Hz (light)
		case 1: lowDb -= 6.0f; break;  // 100 Hz
		case 2: lowDb -= 8.0f; break;  // 120 Hz
		default: break;
		}
	}

	if (enableLowPassCheck->isChecked()) {
		switch (lowPassFreq->currentIndex()) {
		case 0: highDb -= 3.0f; break; // 12 kHz (light)
		case 1: highDb -= 6.0f; break; // 10 kHz
		case 2: highDb -= 9.0f; break; // 8 kHz
		default: break;
		}
	}

	if (enableDeEsserCheck->isChecked()) {
		switch (deEsserIntensity->currentIndex()) {
		case 0: highDb -= 2.0f; break;
		case 1: highDb -= 4.0f; break;
		case 2: highDb -= 6.0f; break;
		default: break;
		}
	}

	if (std::fabs(lowDb) > 0.01f || std::fabs(midDb) > 0.01f || std::fabs(highDb) > 0.01f) {
		obs_data_t *settings = obs_data_create();
		obs_data_set_double(settings, "low", static_cast<double>(lowDb));
		obs_data_set_double(settings, "mid", static_cast<double>(midDb));
		obs_data_set_double(settings, "high", static_cast<double>(highDb));
		createFilter(source, "basic_eq_filter", "Audio Calibrator - EQ", settings);
		obs_data_release(settings);
	}

	// Advanced: VST (only if available)
	if (enableVSTCheck->isChecked()) {
		obs_data_t *settings = obs_data_create();
		createFilter(source, "vst_filter", "Audio Calibrator - VST", settings);
		obs_data_release(settings);
	}

	if (enableVSTCheck->isChecked() && !isFilterAvailable("vst_filter")) {
		statusLabel->setText("VST filter is not available in this OBS build.");
	}

	obs_source_release(source);
}
