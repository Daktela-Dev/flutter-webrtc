# Daktela fork — divergence from upstream

This file documents what this fork carries on top of [upstream flutter-webrtc/flutter-webrtc](https://github.com/flutter-webrtc/flutter-webrtc).

The fork's `main` branch always reflects the latest set of patches. Release tags (`daktela-<upstream-version>+<n>`) pin specific iterations the Daktela Flutter app can depend on via `pubspec.yaml`.

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

3. **Windows mute-RTP fix** (internal #858247)
   - Adds a `setMicrophoneMute` C++ handler that mutes by zeroing mic capture volume rather than disabling the track. Windows libwebrtc otherwise stops emitting RTP entirely when `track.enabled = false`, causing carriers with strict RTP-liveness timers (Euroweb ~30s, Telekom ~15–20s) to drop the call.
   - Tracked upstream as [flutter-webrtc#1861](https://github.com/flutter-webrtc/flutter-webrtc/issues/1861).
