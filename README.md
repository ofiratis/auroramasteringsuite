# Aurora Mastering Suite

Open-source stereo mastering plugin project for Ableton Live and other VST3/AU hosts.

## Included controls (26)
Input/output gain, global wet mix, high-pass filter, two shelving EQ bands, two bell EQ bands with adjustable Q, compressor with threshold/ratio/attack/release/knee/makeup, stereo width, soft saturation drive/mix, peak limiter input and ceiling, bypass. Input/output peak and output RMS displays. Parameter state saved with DAW projects.

**Important:** This ZIP contains complete **source code**, not a compiled or tested `.vst3`/`.component` binary. Building requires CMake, Git, a C++ toolchain, and internet access to fetch JUCE. This plugin has not been compiled or tested inside Ableton in this environment.

## Build on macOS (VST3 + AU + Standalone)
Install Xcode Command Line Tools, Git, CMake (3.22+), then run from the project directory:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 4
```

Find built plugin in `build/AuroraMastering_artefacts/Release/VST3/` (or `build/AuroraMastering_artefacts/VST3/`, depending on generator). Copy `Aurora Mastering.vst3` into `~/Library/Audio/Plug-Ins/VST3/`. In Ableton Live, enable VST3 system folders and rescan. AU goes in `~/Library/Audio/Plug-Ins/Components/`.

## Build on Windows
Install Visual Studio 2022 C++ tools, CMake, Git:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Copy the `.vst3` bundle from `build/AuroraMastering_artefacts/Release/VST3/` to `C:\Program Files\Common Files\VST3\`.

## Audio chain
Input → HPF → low shelf → low-mid bell → high-mid bell → high shelf → stereo dual-mono compressor → saturation → M/S width → sample-peak limiter → wet/dry → output.

## Limitations
- Limiter is **sample-peak**, not true-peak; it uses instantaneous attenuation and does not have lookahead or oversampling. It can distort transients.
- RMS is a block-based output meter, **not** integrated LUFS, LRA, or ITU-R BS.1770 loudness.
- No multiband dynamics, linear-phase EQ, oversampling, dithering, or reference matching.
- The limiter ceiling is **not** guaranteed after output gain or dry/wet mixing. For a safe final ceiling, use output 0 dB and wet 100%, and check with an independent true-peak meter.
- Parameters are read per block and EQ coefficients updated per block; automation can cause zipper noise. This is a prototype, not a production-validated mastering processor.
- The stereo compressor uses independent L/R envelopes, so strong asymmetrical transients can affect stereo balance.

## GitHub
Create a repository and upload all source files (not just the ZIP). GitHub Actions workflow builds the macOS and Windows binaries on pushes. Download artifacts from the Actions tab. Code is provided under the MIT license (see LICENSE).

## Suggested starting settings
For transparent mastering: input 0, EQ 0, compressor ratio 1.5–2, threshold adjusted for ~1–2 dB gain reduction (use your ears; this prototype does not meter gain reduction), width 100, saturation mix 0, limiter ceiling -1 dBFS, limiter input 0, wet 100, output 0.
