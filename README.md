# WaveForge

An original wavetable synthesizer (VST3 + Standalone) built with JUCE and C++.

## Features
- 16-frame wavetable (soft sine-like to bright saw), band-limited with mip levels, live waveform display
- Unison (1-7 voices), detune, stereo spread
- Lowpass filter with filter envelope amount
- ADSR amp envelope and an LFO that modulates wavetable position
- 16-voice polyphony

## Getting the installer from GitHub
1. Create a new repository on github.com (e.g. `WaveForge`).
2. Upload all the files in this folder, including the hidden `.github` folder (it contains the build workflow).
3. Open the repository's **Actions** tab. The "Build WaveForge" workflow runs automatically and takes about 10-20 minutes.
4. When the run finishes, download from the **Artifacts** section at the bottom of the run page:
   - `WaveForge-Windows-Setup` -> `WaveForge-Windows-Setup.exe`
   - `WaveForge-Mac-Installer` -> `WaveForge-Mac.pkg`
5. The Windows installer places the VST3 in `C:\Program Files\Common Files\VST3`.
   Rescan plugins in FL Studio, Ableton, Reaper, or your DAW of choice.

For a permanent download link, push a version tag (`git tag v0.1.0 && git push --tags`).
The installers will then be attached to a GitHub **Release**.

## Mac note
The installer is unsigned. The first time you open it: right-click -> Open.
