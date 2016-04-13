#ifndef PlatformSpeechSynthesisProviderWPE_h
#define PlatformSpeechSynthesisProviderWPE_h
#if ENABLE(SPEECH_SYNTHESIS)

#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class PlatformSpeechSynthesizer;
class PlatformSpeechSynthesisUtterance;
class PlatformSpeechSynthesisVoice;

class PlatformSpeechSynthesisProviderWPE {
public:

    explicit PlatformSpeechSynthesisProviderWPE(PlatformSpeechSynthesizer*);
    ~PlatformSpeechSynthesisProviderWPE();

    void initializeVoiceList(Vector<RefPtr<PlatformSpeechSynthesisVoice>>&);
    void pause();
    void resume();
    void speak(PassRefPtr<PlatformSpeechSynthesisUtterance>);
    void cancel();
private:
    bool m_fliteInited;
    float m_speechDuration ; /*private member to track speeh time*/
    PlatformSpeechSynthesizer* m_platformSpeechSynthesizer;
    RefPtr<PlatformSpeechSynthesisUtterance> m_utterance;
    enum SpeechEvent {
        SpeechError,
        SpeechCancel,
        SpeechPause,
        SpeechResume,
        SpeechStart,
        SpeechEnd
    };
    void fireSpeechEvent(SpeechEvent);
};

} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)

#endif // PlatformSpeechSynthesisProviderWPE_h
