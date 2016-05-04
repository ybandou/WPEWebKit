#ifndef PlatformSpeechRecognitionProviderWPE_h
#define PlatformSpeechRecognitionProviderWPE_h

#if ENABLE(SPEECH_RECOGNITION)

#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>
#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>
#include <pocketsphinx.h>

namespace WebCore {

class PlatformSpeechRecognizer;
class PlatformSpeechRecognitionUtterance;
class PlatformSpeechRecognitionVoice;

class PlatformSpeechRecognitionProviderWPE {
public:
    enum RecognitionStatus {
        RecognitionStarted,
        RecognitionStopped,
        RecognitionAborted
    };
    

    explicit PlatformSpeechRecognitionProviderWPE(PlatformSpeechRecognizer*);
    ~PlatformSpeechRecognitionProviderWPE();

    void start();
    void stop();
    void abort();

private:
    void  recognizeFromMIC( void (*)(char const *) );
    static void* recognitionThread (void*);

    PlatformSpeechRecognizer* m_platformSpeechRecognizer;

    void (*m_callBack)(char const *);
    RecognitionStatus m_recognitionStatus;
    pthread_t m_recognitionThread;

};

} // namespace WebCore

#endif // ENABLE(SPEECH_RECOGNITION)

#endif // PlatformSpeechRecognitionProviderWPE_h
