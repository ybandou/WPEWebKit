
#ifndef PlatformSpeechRecognizer_h
#define PlatformSpeechRecognizer_h

#if ENABLE(SPEECH_RECOGNITION)
#include <SpeechRecognitionResult.h>
#include <SpeechRecognitionError.h>

#if PLATFORM(WPE)
namespace WebCore {
class PlatformSpeechRecognitionProviderWPE;
}
#endif

#include "wtf/text/WTFString.h"


namespace WebCore {

class Page;
class SpeechGrammarList;
class SpeechRecognition;

class PlatformSpeechRecognizerClient {
    virtual void didStartAudio();
    virtual void didStartSound();
    virtual void didStartSpeech();
    virtual void didEndSpeech();
    virtual void didEndSound();
    virtual void didEndAudio();
    virtual void didReceiveResults(const Vector<RefPtr<SpeechRecognitionResult> >& newFinalResults, const Vector<RefPtr<SpeechRecognitionResult> >& currentInterimResults);
    virtual void didReceiveNoMatch(PassRefPtr<SpeechRecognitionResult>);
    virtual void didReceiveError(SpeechRecognitionError&);
    virtual void didStart();
    virtual void didEnd();
};

class PlatformSpeechRecognizer {
public:
    WEBCORE_EXPORT explicit PlatformSpeechRecognizer(PlatformSpeechRecognizerClient*);

    // FIXME: We have multiple virtual functions just so we can support a mock for testing.
    // Seems wasteful. Would be nice to find a better way.
    WEBCORE_EXPORT virtual ~PlatformSpeechRecognizer();

    virtual void start();
    virtual void stop();
    virtual void abort();

    PlatformSpeechRecognizerClient* client() const { return m_speechRecognizerClient; }

private:

    PlatformSpeechRecognizerClient* m_speechRecognizerClient;
#if PLATFORM(WPE)
    std::unique_ptr<PlatformSpeechRecognitionProviderWPE> m_platformSpeechWrapper;
#endif
};
//void provideSpeechRecognitionTo(Page*, SpeechRecognitionClient*);

} // namespace WebCore

#endif //ENABLE(SPEECH_RECOGNITION)
#endif // SpeechRecognitionClient_h
