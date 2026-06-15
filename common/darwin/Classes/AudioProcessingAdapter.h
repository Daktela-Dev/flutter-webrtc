#import <Foundation/Foundation.h>
#import <WebRTC/WebRTC.h>

@protocol ExternalAudioProcessingDelegate

- (void)audioProcessingInitializeWithSampleRate:(size_t)sampleRateHz channels:(size_t)channels;

- (void)audioProcessingProcess:(RTC_OBJC_TYPE(RTCAudioBuffer) * _Nonnull)audioBuffer;

- (void)audioProcessingRelease;

@end

@interface AudioProcessingAdapter : NSObject <RTCAudioCustomProcessingDelegate>

// When YES, captured audio frames are zeroed in-place before any processing /
// rendering, while the track stays enabled so RTP keeps flowing as silence.
// Used by the iOS mute-RTP fix (#883433) — see FlutterWebRTCPlugin setMicrophoneMute.
// Temporary: drop once an ADM that silences a disabled track natively is viable
// (CHANGELOG_DAKTELA.md "Carried patches" #4).
@property(atomic, assign) BOOL captureMuted;

- (nonnull instancetype)init;

- (void)addProcessing:(id<ExternalAudioProcessingDelegate> _Nonnull)processor;

- (void)removeProcessing:(id<ExternalAudioProcessingDelegate> _Nonnull)processor;

- (void)addAudioRenderer:(nonnull id<RTCAudioRenderer>)renderer;

- (void)removeAudioRenderer:(nonnull id<RTCAudioRenderer>)renderer;

@end
