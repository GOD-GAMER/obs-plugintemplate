# OBS Audio Calibrator

Automatically calibrates a microphone source in OBS Studio and applies a tuned filter chain.

The wizard records a few short samples (about 5 seconds each), measures your noise floor and speaking levels, and then applies filters (noise suppression, gate, expander, gain, compressor, limiter). Optional “advanced” controls add a simple EQ to approximate high‑pass/low‑pass and de‑essing.

## Requirements

- OBS Studio 30.0+ (Windows)
- Windows 10/11 64-bit
- A microphone source present in OBS (typically “Audio Input Capture”)

## Installation

1. Download the latest Windows zip from the [Releases](../../releases) page.
2. Extract it.
3. Close OBS Studio.
4. Copy `obs-audio-calibrator.dll` into your OBS plugins folder:

   - Default installer: `C:\Program Files\obs-studio\obs-plugins\64bit`
   - Steam install is usually under: `...\Steam\steamapps\common\OBS Studio\obs-plugins\64bit`

5. Start OBS Studio and confirm the menu item exists:

   - Tools → Audio Calibration Wizard

## Usage

1. Open Tools → Audio Calibration Wizard.
2. Select your microphone/audio source from the “Audio source” dropdown.
3. Click Start.
4. For each step, click Record and follow the prompt. Each step records for ~5 seconds.

Calibration steps:

1. Room noise (stay silent)
2. Whisper
3. Soft voice
4. Normal voice
5. Normal (steady)
6. Energetic voice
7. “S” sounds (sibilance)
8. Plosives (p/b sounds)

When all steps are complete, click Apply to source.

## What Gets Added To Your Source

The wizard creates filters on the selected source with names like:

- Audio Calibrator - Noise Suppression
- Audio Calibrator - Noise Gate
- Audio Calibrator - Expander
- Audio Calibrator - Gain
- Audio Calibrator - Compressor
- Audio Calibrator - Limiter
- Audio Calibrator - EQ (only if advanced EQ options are enabled)

Re-running the wizard removes the previous “Audio Calibrator - …” filters and re-applies them.

## Notes On Advanced Filters

- High-pass / Low-pass / De-esser: implemented as a simple 3‑band EQ adjustment (approximation). For precise filtering/de‑essing, use a VST plugin.
- VST plugin: the plugin will try to add a VST filter if OBS exposes `vst_filter`. If it isn’t available, you’ll see a message in the wizard status.

## Troubleshooting

### No sources in the dropdown

Add a microphone source in OBS first:

Sources → + → Audio Input Capture → choose your microphone.

### Level meters don’t move

- Confirm the correct source is selected in the wizard.
- Confirm the source is not muted.
- Check Windows input device settings and levels.

### Filters don’t appear after Apply

Right-click the source → Filters and look for “Audio Calibrator - …”. If missing, check:

Help → Log Files → View Current Log

### Noise suppression sounds too aggressive

Re-run the wizard and select a lighter suppression level, or disable Noise suppression.

## Uninstall

1. Close OBS.
2. Delete `obs-audio-calibrator.dll` from your OBS plugins folder.
3. Restart OBS.

Filters already added to sources remain until you remove them manually.

## License

GPL-2.0. See [LICENSE](LICENSE).
