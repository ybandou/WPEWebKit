#ifndef PlatformSpeechRecognitionProviderWPE_h
#define PlatformSpeechRecognitionProviderWPE_h

#if ENABLE(SPEECH_RECOGNITION)

#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>
#include <wtf/Deque.h>

extern "C" {
#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>
#include <sphinxbase/cmd_ln.h>
#include <pocketsphinx.h>
}

namespace WebCore {

class PlatformSpeechRecognizer;
class PlatformSpeechRecognitionUtterance;
class PlatformSpeechRecognitionVoice;

class PlatformSpeechRecognitionProviderWPE {
public:

    explicit PlatformSpeechRecognitionProviderWPE(PlatformSpeechRecognizer*);
    ~PlatformSpeechRecognitionProviderWPE();

    void start();
    void stop();
    void abort();

private:
    enum SpeechEvent {
        Start,
        StartAudio,
        StartSound,
        StartSpeech,
        End,
        EndAudio,
        EndSound,
        EndSpeech,
        ReceiveResults,
        ReceiveNoMatch,
        ReceiveError
    };
    enum RecognitionStatus {
        RecognitionStarted,
        RecognitionStopped,
        RecognitionAborted
    };
    enum FireEventStatus {
        FireEventStarted,
        FireEventStopped,
    };


    void (*m_callBack)(char const *);
    int  initSpeech();
    void deinitSpeech();
    void recognizeFromDevice( void (*)(char const *) );
    
    /* pocket sphinx specific */
    cmd_ln_t     *m_config;
    ad_rec_t     *m_audioDevice;
    ps_decoder_t *m_recognizer;

    pthread_t       m_fireEventThread;
    FireEventStatus m_fireEventStatus;
    static void*    fireEventThread(void*);


    pthread_t         m_recognitionThread;
    RecognitionStatus m_recognitionStatus;
    static void*      recognitionThread (void*);
    
    Vector <std::pair<SpeechEvent, const char*>> m_speechEventQueue;

    PlatformSpeechRecognizer* m_platformSpeechRecognizer;
};

} // namespace WebCore

#endif // ENABLE(SPEECH_RECOGNITION)

#endif // PlatformSpeechRecognitionProviderWPE_h
