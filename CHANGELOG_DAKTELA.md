# Daktela fork — divergence from upstream

This file documents what this fork carries on top of [upstream flutter-webrtc/flutter-webrtc](https://github.com/flutter-webrtc/flutter-webrtc).

The fork's `main` branch always reflects the latest set of patches. Release tags (`daktela-<upstream-version>+<n>`) pin specific iterations the Daktela Flutter app can depend on via `pubspec.yaml`.

## daktela-1.3.1+4 — 2026-10-30

Built on `daktela-1.3.1+3`. Adds **patch 5 (Windows/desktop audio-source deadlock)** and **extends patch 4 (iOS mute-RTP fix) to macOS** (internal #958966): `setMicrophoneMute` on macOS now zeroes the APM capture frames like iOS instead of disabling the track, and a new capture resets the flag on macOS too. The macOS `isEnabled` mute path is gone. The Dart `Helper.setMicrophoneMute` guard now covers `Platform.isMacOS`. Patches 1–3 unchanged; patch 2 (`restartAudioDeviceModule`) stays for device changes only.

## daktela-1.3.1+3 — 2026-06-15

Built on `daktela-1.3.1+2`. **Reworks patch 3 (Windows/desktop mute)** from capture-volume-zeroing to APM frame-zeroing — fixes the regression where the remote heard the agent the whole time despite pressing mute (internal #86ca91cmq). Patches 1, 2, 4 unchanged.

## daktela-1.3.1+2 — 2026-06-12

Built on `daktela-1.3.1+1`. Adds **patch 4 (iOS mute-RTP fix)** — see Carried patches below. Patches 1–3 unchanged.

## daktela-1.3.1+1 — 2026-04-29

Built on upstream **release 1.3.1** ([5253217](https://github.com/flutter-webrtc/flutter-webrtc/commit/5253217)).

### Why we're stuck on 1.3.1

Upstream versions after 1.3.1 ship libwebrtc m144, which has an audio regression tracked at [flutter-webrtc#2018](https://github.com/flutter-webrtc/flutter-webrtc/issues/2018). We can't upgrade past 1.3.1 until that's resolved upstream.

### Carried patches

1. **Speaker toggle off/on fix**
   - Same fix has already been merged upstream as [76540f6c362a6d029d9dbd0e65c99a4f8e8a5220](https://github.com/flutter-webrtc/flutter-webrtc/commit/76540f6c362a6d029d9dbd0e65c99a4f8e8a5220), but only in versions **after** 1.3.1 — so we still need to carry it locally on this fork.
   - Drop this patch once we can upgrade past 1.3.1.

2. **macOS ADM restart**
   - Adds `restartAudioDeviceModule` channel method (macOS only). Fixes sample-rate corruption (chipmunk / deep-voice) after Bluetooth device transitions with PlatformDefault ADM.
   - Daktela-specific. No upstream equivalent.

3. **Windows/desktop mute-RTP fix** (internal #858247; reworked in `+3` for #86ca91cmq)
   - Constraint: mute must keep RTP flowing — disabling the track (`track.enabled = false`) stops Windows libwebrtc RTP entirely, so carriers with strict RTP-liveness timers (Euroweb ~30s, Telekom ~15–20s) drop the call.
   - **`+1`/`+2` approach (volume-zero) — removed in `+3`:** the `setMicrophoneMute` C++ handler zeroed the capture device volume (`SetMicrophoneVolume(0)`). libwebrtc's Automatic Gain Control defeated it — AGC drives the capture gain straight back toward its loudness target via the same volume API — so on many mics/drivers (external/USB especially; built-in mics often masked it) the agent's voice kept being captured and **the remote heard everything despite "mute"** (#86ca91cmq).
   - **`+3` approach (frame-zeroing):** mirrors the iOS fix (patch 4). The handler now toggles a flag on a capture post-processing hook — new `MuteAudioProcessor` (`common/cpp/include/mute_audio_processor.h`) registered via `RTCAudioProcessing::SetCapturePostProcessing` — that `memset`s the captured frames to zero while muted, leaving the track enabled so the encoder keeps emitting silent RTP. AGC-proof (runs after the APM) and device-independent. A new capture resets the flag (`FlutterMediaStream::GetUserAudio`). Affects Windows + Linux (`common/cpp`); macOS uses the darwin frame-zeroing of patch 4 since `+4`.
   - Tracked upstream as [flutter-webrtc#1861](https://github.com/flutter-webrtc/flutter-webrtc/issues/1861).

4. **iOS/macOS mute-RTP fix** (internal #883433, #958966) — *added in `+2` for iOS, extended to macOS in `+4`*
   - macOS (`+4`): the app used to mute by disabling the track **and** stopping the ADM recording (to turn off the orange mic indicator). With no capture frames the encoder sent nothing, and a trunk with a ~20 s RTP timer dropped every call muted that long (#958966). macOS now takes the same `captureMuted` path as iOS; the ADM keeps recording, so the macOS mic indicator stays lit while muted (accepted trade-off, same as iOS/Windows).
   - Constraint: `captureMuted` is process-global, not per call, and every new audio capture clears it. Safe today because the app rejects a second call before opening a capture; a future call-waiting / second-line feature must not open the mic while a muted call is active, or that call goes live without the UI noticing.
   - On iOS, disabling the track (`audioTrack.isEnabled = NO`) stops outgoing RTP entirely — same failure mode as Windows (patch 3 / #858247). Carriers with strict RTP-liveness timers (Telekom HU ~13–20s at vimpex) then tear the call down. Hold is unaffected (SIP-signalled `sendonly`); mute is not.
   - Fix: `setMicrophoneMute` on iOS mutes by **zeroing the captured audio frames** in `AudioProcessingAdapter` (the APM capture post-processing delegate), leaving `track.enabled = YES` so the encoder keeps emitting silent RTP. New surface: `AudioProcessingAdapter.captureMuted`, `AudioManager setCaptureMuted:`; a new capture resets the flag (`FlutterRTCMediaStream` getUserAudio) so each call starts unmuted; Dart `Helper.setMicrophoneMute` guard extended to `Platform.isWindows || Platform.isIOS`.
   - **Why not `RTCAudioDeviceModule setMicrophoneMuted:` / `setMuteMode:`** (present in the WebRTC-SDK 137 headers): the factory uses the **PlatformDefault/CoreAudio ADM** (`initWithAudioDeviceModuleType:0`). On that ADM, `SetMicrophoneMute` takes the `kAudioOutputUnitProperty_EnableIO` path (stops capture → reproduces the bug), the getter is `RTC_DCHECK_NOTREACHED`, `setMuteMode:` only works on the AudioEngine ADM, and mute state wouldn't survive audio-unit restarts. **Do not replace this with the ADM mute API.**
   - **Why not switch to the AudioEngine ADM** (which emits silence on a disabled track natively and wouldn't need this patch at all): it was tried and reverted — it regresses iOS Bluetooth routing / first-call-no-audio / hold (internal 86c9h18fb) and is unstable on macOS. On the desktop app **4.4.8** (AudioEngine ADM) operators hit duplicate audio / echo / choppy-stuttering quality on macOS (internal 872825 "seznam", 863331, 874360); all of it disappeared after **4.4.9** downgraded to this PlatformDefault/CoreAudio ADM. The macOS screen-share-audio + mic crash (#1986) is the same trade.
   - **Drop this patch once** we can run an ADM that emits silence on a muted (disabled) track *without* stopping capture — i.e. the **AudioEngine ADM** once its iOS BT/first-call/hold regressions (86c9h18fb) and macOS audio instability (872825 / 863331 / #1986) are fixed, **or** a post-1.3.1 / libwebrtc m144+ upstream once [flutter-webrtc#2018](https://github.com/flutter-webrtc/flutter-webrtc/issues/2018) is resolved. Until one of those lands, frame-zeroing is the only ADM-independent fix.
   - Tracked upstream as [flutter-webrtc#1861](https://github.com/flutter-webrtc/flutter-webrtc/issues/1861) / [dart-sip-ua#563](https://github.com/cloudwebrtc/dart-sip-ua/issues/563). Windows/Linux (frame-zeroing as of `+3`, see patch 3) and Android (OS-level mute) paths are unchanged by this patch.

5. **Windows/desktop audio-source deadlock** (internal TASK-22.7) — *added in `+4`*
   - libwebrtc's `~LocalAudioSource` holds `sink_lock_` and then takes `CustomAudioTransportImpl::capture_lock_`; the ADM capture thread takes them in the opposite order. A source destroyed while the mic is capturing can deadlock the signaling and capture threads for good, and the UI thread waiting on the track dispose hangs with them (window "Not Responding", reproduced at 36–226 calls). Reported upstream as [webrtc-sdk/libwebrtc#160](https://github.com/webrtc-sdk/libwebrtc/issues/160).
   - Fix: `getUserMedia` audio reuses one `RTCAudioSource` (`FlutterWebRTCBase::audio_input_source_`) instead of creating one per call, so no `LocalAudioSource` is destroyed while the plugin lives. Tracks are still created and disposed per call. Holding the source does not keep the mic open: recording follows the sending streams.
   - Drop this patch once the vendored `libwebrtc.dll` contains the upstream fix for #160.
