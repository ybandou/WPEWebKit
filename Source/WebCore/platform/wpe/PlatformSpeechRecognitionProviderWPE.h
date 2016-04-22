#ifndef PlatformSpeechRecognitionProviderWPE_h
#define PlatformSpeechRecognitionProviderWPE_h

#if ENABLE(SPEECH_RECOGNITION)

#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>
#include <wtf/Deque.h>
#include <wtf/threads/BinarySemaphore.h>

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


    int  initSpeechRecognition();
    void deinitSpeechRecognition();
    void recognizeFromDevice();
    
    /* pocket sphinx specific */
    cmd_ln_t       *m_config;
    ad_rec_t       *m_audioDevice;
    ps_decoder_t   *m_recognizer;

    BinarySemaphore   m_waitForEvents;
    FireEventStatus   m_fireEventStatus;
    ThreadIdentifier  m_fireEventThread;
    static void       fireEventThread(void*);
    void              fireSpeechEvent(const auto&);//TODO: input paramter type has to be modified to avoid warning

    ThreadIdentifier  m_recognitionThread;
    RecognitionStatus m_recognitionStatus;
    static void       recognitionThread (void*);
    
    Vector <std::pair<SpeechEvent, const char*>> m_speechEventQueue;

    PlatformSpeechRecognizer* m_platformSpeechRecognizer;
};

} // namespace WebCore

#endif // ENABLE(SPEECH_RECOGNITION)

#endif // PlatformSpeechRecognitionProviderWPE_h
