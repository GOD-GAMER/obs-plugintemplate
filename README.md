# 🎙️ OBS Audio Calibrator

**Automatically configure your microphone for professional, broadcast-quality audio in OBS Studio.**

No more guessing compressor settings or noise gate thresholds. This plugin measures your actual voice and creates the perfect filter chain — in under 30 seconds.

---

## ✨ What It Does

The Audio Calibrator is a one-time setup wizard that:

1. **Listens** to your quiet, normal, and loud speaking voice
2. **Analyzes** your vocal range and background noise
3. **Applies** perfectly tuned audio filters automatically

**The result?** Whether you whisper or shout, your stream hears consistent, clean audio.

---

## 🎯 Features

| Feature | Description |
|---------|-------------|
| **3-Step Voice Calibration** | Measures your quiet, normal, and loud voice levels |
| **Smart Filter Chain** | Applies 6 professional audio filters in optimal order |
| **AI Noise Suppression** | RNNoise removes keyboard clicks, fans, and background noise |
| **Noise Gate** | Cuts audio when you're not speaking |
| **Auto Gain** | Brings your voice to broadcast standard (-16 dB) |
| **Compressor** | Evens out volume differences between quiet and loud |
| **Limiter** | Prevents clipping on sudden loud sounds |
| **Visual Feedback** | Real-time level meters show your audio |
| **OBS Dark Theme** | Matches OBS Studio's native look |

---

## 📋 Requirements

- **OBS Studio** version 30.0 or newer
- **Windows** 10 or 11 (64-bit)
- A microphone configured as an Audio Input Capture source in OBS

---

## 📥 Installation

### Step 1: Download the Plugin

1. Go to the [**Releases**](../../releases) page
2. Download the latest `obs-audio-calibrator-windows.zip`
3. Save it somewhere easy to find (like your Downloads folder)

### Step 2: Extract the Files

1. Right-click the downloaded `.zip` file
2. Select **Extract All...**
3. Click **Extract**

You'll see a folder containing:
```
obs-audio-calibrator.dll
```

### Step 3: Install to OBS

1. **Close OBS Studio** if it's running

2. Open File Explorer and navigate to:
   ```
   C:\Program Files\obs-studio\obs-plugins\64bit
   ```

3. **Copy** the `obs-audio-calibrator.dll` file into this folder

4. If Windows asks for administrator permission, click **Continue**

### Step 4: Verify Installation

1. Open **OBS Studio**
2. Click the **Tools** menu at the top
3. You should see **Audio Calibration Wizard** in the list

✅ **Installation complete!**

---

## 🚀 How to Use

### Before You Start

Make sure you have a microphone added to OBS:

1. In OBS, click the **+** button under **Sources**
2. Select **Audio Input Capture**
3. Name it (e.g., "Microphone")
4. Select your microphone from the dropdown
5. Click **OK**

### Running the Calibration

#### Step 1: Open the Wizard

1. Click **Tools** in the OBS menu bar
2. Select **Audio Calibration Wizard**

#### Step 2: Select Your Microphone

1. In the wizard window, find the **Audio Source** dropdown
2. Select your microphone source
3. Verify the level meter responds when you speak

#### Step 3: Start Calibration

1. Click the **Start Calibration** button
2. The wizard will guide you through 3 recording steps

#### Step 4: Record Your Quiet Voice

1. You'll see a phrase to read: *"The quick brown fox jumps over the lazy dog."*
2. Click **Record**
3. **Whisper** the phrase in your quietest comfortable voice
4. Recording automatically stops after 3 seconds

#### Step 5: Record Your Normal Voice

1. A new phrase appears: *"She sells seashells by the seashore."*
2. Click **Record**
3. **Speak normally** — your everyday talking voice
4. Recording stops after 3 seconds

#### Step 6: Record Your Loud Voice

1. Final phrase: *"PETER PIPER PICKED A PECK OF PICKLED PEPPERS!"*
2. Click **Record**
3. **Speak loudly** — your excited streaming voice
4. Recording stops after 3 seconds

#### Step 7: Choose Your Filters

After recording, you'll see checkboxes for each filter:

| Filter | Recommended | What It Does |
|--------|-------------|--------------|
| ✅ Noise Suppression | Yes | Removes background noise |
| ✅ Noise Gate | Yes | Silences audio when not speaking |
| ⬜ Expander | Optional | Gentler alternative to noise gate |
| ✅ Gain | Yes | Adjusts volume to standard level |
| ✅ Compressor | Yes | Evens out loud and quiet parts |
| ✅ Limiter | Yes | Prevents audio clipping |

#### Step 8: Apply Filters

1. Review the selected filters
2. Click **Apply Filters**
3. Confirm by clicking **Yes**

✅ **Done!** Your microphone is now professionally calibrated.

---

## 🔧 Viewing Your Filters

To see or adjust the filters that were applied:

1. In OBS, find your microphone in **Sources**
2. Right-click it and select **Filters**
3. You'll see filters prefixed with `AutoCal-`:
   - AutoCal-NoiseSuppression
   - AutoCal-NoiseGate
   - AutoCal-Gain
   - AutoCal-Compressor
   - AutoCal-Limiter

You can manually tweak any setting, or run the calibration wizard again.

---

## 🔄 Re-Calibrating

Changed your mic? Moved to a quieter room? Just run the wizard again:

1. **Tools** → **Audio Calibration Wizard**
2. Complete the 3-step calibration
3. Apply new filters

The wizard automatically removes old `AutoCal-` filters before applying new ones.

---

## ❓ Troubleshooting

### "No audio input sources found"

**Solution:** Add a microphone source first:
1. Click **+** under Sources in OBS
2. Add **Audio Input Capture**
3. Select your microphone
4. Try the wizard again

### Level meter not moving

**Possible causes:**
- Wrong microphone selected in the wizard
- Microphone is muted in Windows
- Microphone volume is too low

**Solution:**
1. Check Windows Sound Settings
2. Ensure your mic is set as the default input
3. Increase the microphone volume

### Filters not appearing after applying

**Solution:**
1. Right-click your source → **Filters**
2. Look for filters starting with `AutoCal-`
3. If missing, check the OBS log: **Help** → **Log Files** → **View Current Log**

### Audio sounds robotic or choppy

**Solution:** The noise suppression might be too aggressive:
1. Open your source's Filters
2. Find `AutoCal-NoiseSuppression`
3. Lower the suppression level, or disable it

### Plugin not showing in Tools menu

**Solution:**
1. Make sure OBS is fully closed
2. Verify the DLL is in `C:\Program Files\obs-studio\obs-plugins\64bit`
3. Restart OBS
4. If still missing, check **Help** → **Log Files** for errors

---

## 📊 Understanding Your Results

After calibration, the wizard shows your voice measurements:

| Measurement | Good Range | Meaning |
|-------------|------------|---------|
| Quiet Level | -50 to -35 dB | Your whisper volume |
| Normal Level | -30 to -18 dB | Everyday speaking |
| Loud Level | -15 to -6 dB | Excited/shouting |
| Dynamic Range | 15-25 dB | Difference between quiet and loud |

**Dynamic Range Tips:**
- **Under 15 dB:** You speak at a consistent volume (good!)
- **15-25 dB:** Normal range, compressor will help
- **Over 25 dB:** Large variation, heavier compression applied

---

## 🎚️ Filter Settings Explained

### Noise Suppression (RNNoise)
- **What:** AI-powered noise removal
- **Removes:** Keyboard clicks, fans, AC, traffic
- **Levels:** Low, Medium (default), High

### Noise Gate
- **What:** Silences audio below a threshold
- **Threshold:** Set 10 dB below your quietest speech
- **Great for:** Eliminating breathing sounds between words

### Expander
- **What:** Gradually reduces quiet sounds (softer than gate)
- **Use when:** Noise gate sounds too harsh

### Gain
- **What:** Raises or lowers overall volume
- **Target:** -16 dB (broadcast/streaming standard)

### Compressor
- **What:** Reduces the gap between quiet and loud
- **Threshold:** Based on your normal speaking level
- **Ratio:** Based on your dynamic range (2:1 to 8:1)

### Limiter
- **What:** Hard ceiling that prevents clipping
- **Ceiling:** -6 dB (safe headroom for streaming)

---

## 🗑️ Uninstalling

1. Close OBS Studio
2. Navigate to `C:\Program Files\obs-studio\obs-plugins\64bit`
3. Delete `obs-audio-calibrator.dll`
4. Restart OBS

**Note:** Filters applied to your sources will remain. To remove them:
1. Right-click the source → **Filters**
2. Select each `AutoCal-` filter
3. Click the **−** button to remove

---

## 💬 Support

Having issues? Here's how to get help:

1. **Check Troubleshooting** section above
2. **View OBS Log:** Help → Log Files → View Current Log
3. **Open an Issue:** Use the [Issues](../../issues) tab on GitHub

When reporting issues, please include:
- Your OBS Studio version
- Your Windows version
- The relevant section of your OBS log

---

## 📜 License

This project is open source and available under the [GPL-2.0 License](LICENSE).

---

## 🙏 Acknowledgments

- OBS Studio team for the plugin API
- RNNoise for AI noise suppression technology
- The streaming community for feedback and testing

---

**Made with ❤️ for streamers who want great audio without the hassle.**
