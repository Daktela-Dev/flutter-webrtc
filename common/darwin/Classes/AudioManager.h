#import <Foundation/Foundation.h>
#import <WebRTC/WebRTC.h>
#import "AudioProcessingAdapter.h"

@interface AudioManager : NSObject

@property(nonatomic, strong) RTCDefaultAudioProcessingModule* _Nonnull audioProcessingModule;

@property(nonatomic, strong) AudioProcessingAdapter* _Nonnull capturePostProcessingAdapter;

@property(nonatomic, strong) AudioProcessingAdapter* _Nonnull renderPreProcessingAdapter;

+ (_Nonnull instancetype)sharedInstance;

- (void)addLocalAudioRenderer:(nonnull id<RTCAudioRenderer>)renderer;

- (void)removeLocalAudioRenderer:(nonnull id<RTCAudioRenderer>)renderer;

// iOS mute-RTP fix (#883433): mute the microphone by zeroing captured frames
// (track stays enabled, RTP keeps flowing as silence) instead of disabling the
// track (which stops RTP on iOS). Delegates to capturePostProcessingAdapter.
// Temporary: drop once an ADM that silences a disabled track natively is viable
// (CHANGELOG_DAKTELA.md "Carried patches" #4).
- (void)setCaptureMuted:(BOOL)muted;

@end
