VOG POLY SYNTH PRO - JUCE SOURCE PROJECT
========================================
This is C++ source for a native Windows VST3 instrument and standalone app.
It is NOT a precompiled Windows installer or finished VST3 binary.

BUILD ON WINDOWS
1. Install Visual Studio 2022 (Desktop development with C++), Git, CMake 3.22+.
2. Connect to internet (CMake downloads JUCE 8.0.10 from GitHub).
3. Unzip the project. Double-click Build-Windows.bat.
4. On success, locate output under build/VOGPolySynthPro_artefacts/Release.
5. Copy VOG Poly Synth Pro.vst3 into C:\Program Files\Common Files\VST3\.
6. Open Reason 14, rescan plugins, create an instrument track, load VOG Poly Synth Pro.
7. Connect USB MIDI keyboard (e.g., MiniNova), enable it in Reason MIDI preferences, select the instrument track, and play.
8. Standalone build can be launched separately; direct MIDI device selection in standalone is managed by JUCE's audio/MIDI device settings where available.

IMPLEMENTED IN THIS SOURCE
- Six oscillators with independent on/off, waveform, level, tune, detune, pan parameters.
- Rotary control UI for oscillator controls, ADSR, six-band EQ, master and tempo.
- 16-voice polyphonic MIDI note playback, pitch bend, sustain pedal, modulation input.
- 6-band parametric EQ (60/150/400/1000/3000/8000 Hz).
- 3-minute stereo 24-bit WAV recording and Save WAV chooser.
- Named patch save/load through .vogpatch files, DAW state persistence.
- Hold toggle, selectable rhythmic divisions, tempo parameter.
- Large dark-blue interface with onscreen keyboard representation.

KNOWN LIMITATIONS / NEXT DEVELOPMENT
- This source has not been compiled or tested on Windows or in Reason 14.
- Rhythm divisions and tempo are UI/automation parameters; arpeggiator/loop playback
  engine is not yet implemented. Hold keeps released notes sounding.
- Pan parameters are stored but are not yet applied per oscillator in audio rendering.
- Onscreen keyboard currently displays notes but is not wired to synth note events.
- Mod wheel is accepted but not yet mapped to a modulation destination.
- DSP waveforms are basic and not bandlimited (aliasing at high notes).
- No full effects rack, advanced modulation matrix or standalone MIDI selection panel yet.
- WAV recording allocates a three-minute stereo buffer at the active sample rate.

IMPORTANT: This is a source-code development build, not a verified production plugin.
