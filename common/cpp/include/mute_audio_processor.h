#ifndef MUTE_AUDIO_PROCESSOR_HXX
#define MUTE_AUDIO_PROCESSOR_HXX

#include <atomic>
#include <cstring>

#include "rtc_audio_processing.h"

namespace flutter_webrtc_plugin {

using namespace libwebrtc;

// Mute by zeroing the captured audio frames in the APM capture post-processing
// stage, leaving the track enabled so the encoder keeps emitting (silent) RTP.
//
// This is the Windows/desktop twin of the iOS frame-zeroing fix (#883433,
// AudioProcessingAdapter) and the cross-platform analog of Android's
// ExternalAudioProcessingFactory capture post-processing.
//
// It replaces the previous Windows mute (SetMicrophoneVolume(0), #858247):
// zeroing the capture device volume is defeated by libwebrtc's Automatic Gain
// Control, which drives the capture gain back toward its loudness target every
// cycle via the same volume API — so on many mics/drivers the agent's voice
// kept being captured and the remote heard everything. Frame-zeroing runs after
// the APM (AGC included), so it is AGC-proof and device-independent: zeros stay
// zero regardless of mic, driver or gain.
class MuteAudioProcessor : public RTCAudioProcessing::CustomProcessing {
 public:
  MuteAudioProcessor() = default;
  ~MuteAudioProcessor() override = default;

  void set_muted(bool muted) { muted_.store(muted); }
  bool muted() const { return muted_.load(); }

  void Initialize(int sample_rate_hz, int num_channels) override {}
  void Reset(int new_rate) override {}
  void Release() override {}

  // Called on the audio capture thread for every captured frame buffer, after
  // the APM has run. buffer_size is the buffer length in floats.
  void Process(int num_bands,
               int num_frames,
               int buffer_size,
               float* buffer) override {
    if (muted_.load() && buffer != nullptr && buffer_size > 0) {
      std::memset(buffer, 0, static_cast<size_t>(buffer_size) * sizeof(float));
    }
  }

 private:
  std::atomic<bool> muted_{false};
};

}  // namespace flutter_webrtc_plugin

#endif  // MUTE_AUDIO_PROCESSOR_HXX
