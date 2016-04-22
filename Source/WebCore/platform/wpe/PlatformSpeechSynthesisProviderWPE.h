#ifndef PlatformSpeechSynthesisProviderWPE_h
#define PlatformSpeechSynthesisProviderWPE_h
#if ENABLE(SPEECH_SYNTHESIS)

#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>
#include <wtf/MainThread.h>
#include <wtf/Deque.h>
#include <wtf/threads/BinarySemaphore.h>


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
   //ThreadIdentifier  m_speakThread;
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
    int speechMain();
    void fireSpeechEvent(SpeechEvent);
//  static void speakFunctionThread(void*);

   
 
//};
#if 0
class AplayWPE{
public:
    ~AplayWPE();
private:
#endif
   void prg_exit(int code);
   ssize_t safe_read(int fd, void *buf, size_t count);
   size_t test_wavefile_read(int fd, u_char *buffer, size_t *size, size_t reqsize);
   ssize_t test_wavefile(int fd, u_char *_buffer, size_t size);
   void set_params(void);
   u_char *remap_data(u_char *data, size_t count);
   ssize_t pcm_write(u_char *data, size_t count);
   off64_t calc_count(void);
   void playback_go(int fd, size_t loaded, off64_t count, char *name);
   void playback(char *name);
   void do_pause(void);


};
} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)

#endif // PlatformSpeechSynthesisProviderWPE_h
