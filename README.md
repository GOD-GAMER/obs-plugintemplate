# 🎙️ OBS Audio Calibrator

**Get professional-sounding audio in OBS with zero audio engineering knowledge.**

This plugin walks you through an 8-step calibration wizard, listens to your voice and environment, and automatically configures a complete audio filter chain tailored to *your* microphone and *your* room.

No more guessing at compressor ratios. No more "why do I sound like a robot?" moments. Just click, speak, and you're done.

---

## ✨ What It Does

The wizard records a few short samples (~5 seconds each), measures your:
- **Noise floor** – the background hum/hiss when you're silent
- **Speaking levels** – how loud you are at different volumes
- **Vocal characteristics** – sibilance (harsh "S" sounds) and plosives ("P" and "B" pops)

Then it automatically applies a tuned filter chain:

| Filter | What It Does (Plain English) |
|--------|------------------------------|
| **Noise Suppression** | Removes constant background noise (fans, AC, street noise) so viewers only hear you |
| **Noise Gate** | Mutes the mic when you're not speaking – no more keyboard clicks or breathing |
| **Expander** | Like a gentler gate – quietly reduces background noise instead of hard-cutting it |
| **Gain** | Makes you louder or quieter to hit a consistent level |
| **Compressor** | Evens out your volume – makes quiet parts louder and loud parts quieter so you don't blow out ears when you yell |
| **Limiter** | A safety net that prevents audio from ever clipping (that harsh distortion when you're too loud) |
| **EQ** *(optional)* | Adjusts bass/treble frequencies – used for high-pass (removes rumble), low-pass (removes hiss), and de-essing (tames harsh "S" sounds) |

---

## 📋 Requirements

- **OBS Studio 30.0+** (Windows)
- **Windows 10/11 64-bit**
- A microphone source in OBS (typically "Audio Input Capture" or "Mic/Aux")

---

## 📦 Installation

1. Download the latest `.zip` from the [**Releases**](../../releases) page
2. **Close OBS Studio completely**
3. Extract and copy the contents to your OBS install folder:
   - **Default install:** `C:\Program Files\obs-studio\`
   - **Steam:** `C:\Program Files (x86)\Steam\steamapps\common\OBS Studio\`
   
   The zip contains `obs-plugins\64bit\obs-audio-calibrator.dll` – just merge the folders.

4. Start OBS and confirm it loaded:
   - **Tools → Audio Calibration Wizard** should appear in the menu

---

## 🎯 How To Use

1. **Tools → Audio Calibration Wizard**
2. Select your microphone from the dropdown
3. Click **Start**
4. For each of the 8 steps, click **Record** and follow the on-screen prompt:

| Step | What To Do |
|------|------------|
| 1️⃣ **Room Noise** | Stay completely silent – let it hear your room's background noise |
| 2️⃣ **Whisper** | Whisper quietly, close to the mic |
| 3️⃣ **Soft Voice** | Speak softly, like someone's sleeping nearby |
| 4️⃣ **Normal Voice** | Speak at your normal conversation volume |
| 5️⃣ **Steady Normal** | Keep a consistent normal volume for the full 5 seconds |
| 6️⃣ **Energetic** | Speak with energy like you're streaming – but don't shout |
| 7️⃣ **Sibilance** | Say words with "S" sounds: "She sells seashells" |
| 8️⃣ **Plosives** | Say words with "P" and "B" sounds: "Peter Piper picked peppers" |

5. When all steps show ✓, click **Apply to source**

**That's it!** Your microphone now has a professional filter chain.

---

## 🔧 What Gets Added

The wizard creates these filters on your source (all prefixed with "Audio Calibrator"):

```
🔇 Noise Suppression  →  Removes background noise
🚪 Noise Gate         →  Mutes when silent  
📉 Expander           →  Reduces quiet sounds
🔊 Gain               →  Adjusts overall volume
🎚️ Compressor         →  Evens out loud/quiet
🛑 Limiter            →  Prevents clipping
🎛️ EQ                 →  Frequency adjustments (if enabled)
```

**Re-running the wizard** removes the old filters and applies fresh ones based on new measurements.

---

## 🎛️ Advanced Options Explained

| Option | What It Means |
|--------|---------------|
| **High-pass filter** | Removes low rumble (trucks outside, footsteps, AC vibration). Cuts frequencies below ~80-120 Hz. |
| **Low-pass filter** | Removes high-frequency hiss. Cuts frequencies above ~12-16 kHz. |
| **De-esser** | Reduces harsh "S" and "T" sounds that can be piercing on some mics. |

These are implemented as a 3-band EQ approximation. For surgical precision, use a dedicated VST plugin.

---

## 🔊 Audio Terms Glossary

New to audio? Here's what the jargon means:

| Term | Definition |
|------|------------|
| **dB (decibels)** | Unit measuring loudness. 0 dB = maximum before distortion. -20 dB is quieter than -10 dB. |
| **Noise floor** | The constant quiet hum/hiss in your room when you're silent. Lower = better. |
| **RMS** | "Root Mean Square" – the average loudness over time (not the peak). |
| **Peak** | The loudest single moment in your audio. |
| **Clipping** | Harsh distortion when audio is too loud – sounds crackly/broken. |
| **Threshold** | The volume level where a filter starts working. |
| **Ratio** | How much compression is applied. 4:1 means 4 dB of input becomes 1 dB over threshold. |
| **Attack** | How fast a filter reacts when you get louder. |
| **Release** | How fast a filter stops working after you get quieter. |
| **Sibilance** | Harsh "S", "Sh", "Ch" sounds – common problem with condenser mics. |
| **Plosives** | "P", "B", "T" sounds that create a burst of air – causes mic "pops". |
| **High-pass** | Lets high frequencies through, blocks low ones (removes rumble). |
| **Low-pass** | Lets low frequencies through, blocks high ones (removes hiss). |

---

## ❓ Troubleshooting

### No sources in the dropdown
Add a microphone source in OBS first:
**Sources → + → Audio Input Capture → choose your mic**

### Level meters don't move
- Check the source isn't muted (click the speaker icon)
- Check Windows Sound Settings → Input → your mic is selected and volume is up
- Try speaking louder or moving closer to the mic

### Filters don't appear after Apply
1. Right-click your source → **Filters**
2. Look for "Audio Calibrator - …" entries
3. If missing, check: **Help → Log Files → View Current Log** for errors

### Audio sounds robotic or underwater
Noise suppression might be too aggressive. Re-run the wizard and:
- Choose a lighter suppression setting, or
- Disable noise suppression and rely on the gate/expander

### "Calibration incomplete" error
Make sure you completed all 8 steps. If you skipped one, click it and record again.

---

## 🗑️ Uninstall

1. Close OBS
2. Delete `obs-audio-calibrator.dll` from `obs-plugins\64bit\`
3. Restart OBS

**Note:** Filters already on sources remain until you manually remove them (right-click source → Filters → delete).

---

## 📄 License

GPL-2.0 – see [LICENSE](LICENSE)
