#ifndef PlatformSpeechSynthesisProviderWPE_h
#define PlatformSpeechSynthesisProviderWPE_h
#if ENABLE(SPEECH_SYNTHESIS)

#include <wtf/MainThread.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>
#include <wtf/Deque.h>
#include <wtf/threads/BinarySemaphore.h>
#include <wtf/Threading.h>
/* Aplay Utils*/
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <alsa/asoundlib.h>

#ifndef LLONG_MAX
#define LLONG_MAX    9223372036854775807LL
#endif

#define WAV_FILE "/tmp/speechsynth.wav"
#define AUDIO_DEVICE "plughw:1" 

#ifndef le16toh
#include <asm/byteorder.h>
#define le16toh(x) __le16_to_cpu(x)
#define be16toh(x) __be16_to_cpu(x)
#define le32toh(x) __le32_to_cpu(x)
#define be32toh(x) __be32_to_cpu(x)
#endif

#define DEFAULT_FORMAT          SND_PCM_FORMAT_U8
#define DEFAULT_SPEED           8000
#define FORMAT_WAVE             2
#define NONBLOCK 0
#define OPENMODE 0

#define COMPOSE_ID(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))

#define WAV_RIFF        COMPOSE_ID('R','I','F','F')
#define WAV_FMT                 COMPOSE_ID('f','m','t',' ')
#define WAV_DATA                COMPOSE_ID('d','a','t','a')
#define BE_INT(v)               bswap_32(v)
#define LE_INT(v)               (v)

#define TO_CPU_INT(v, be) \
    ((be) ? BE_INT(v) : LE_INT(v))


typedef struct {
    u_int magic;            /* 'RIFF' */
    u_int length;           /* filelen */
    u_int type;             /* 'WAVE' */
} WaveHeader;
typedef struct {
    u_int type;             /* 'data' */
    u_int length;           /* samplecount */
} WaveChunkHeader;
typedef struct voc_header {
    u_char magic[20];       /* must be MAGIC_STRING */
    u_short headerlen;      /* Headerlength, should be 0x1A */
    u_short version;        /* VOC-file version */
    u_short coded_ver;      /* 0x1233-version */
} VocHeader;

typedef struct au_header {
    u_int magic;            /* '.snd' */
    u_int hdr_size;         /* size of header (min 24) */
    u_int data_size;        /* size of data */
    u_int encoding;         /* see to AU_FMT_XXXX */
    u_int sample_rate;      /* sample rate */
    u_int channels;         /* number of channels (voices) */
} AuHeader;



namespace WebCore {

    class PlatformSpeechSynthesizer;
    class PlatformSpeechSynthesisUtterance;
    class PlatformSpeechSynthesisVoice;
    class AplayWPE{

        public:
            AplayWPE(PlatformSpeechSynthesisProviderWPE*);
            ~AplayWPE();
            PlatformSpeechSynthesisProviderWPE* m_speechSynthesisProviderWPE;
            void playback(const char *name);
            void prgExit(int code);
        private:
            int m_stopDelay ;
            int m_wavFd;

            unsigned m_periodTime ;
            unsigned m_bufferTime ;
            size_t m_bitsPerSample;
            size_t m_bitsPerFrame;
            size_t m_chunkBytes;

            snd_pcm_uframes_t m_periodFrames ;
            snd_pcm_uframes_t m_bufferFrames ;

            off64_t m_pbrecCount;
            off64_t m_fdcount;

            void setHWParams(void);
            ssize_t pcmWrite(u_char *data, size_t count);
            ssize_t safeRead(int fd, void *buf, size_t count);
            ssize_t checkWavefile(int fd, u_char *_buffer, size_t size);
            void playbackStart(int fd, size_t loaded, off64_t count, const char *name);
            size_t checkWavefileRead(int fd, u_char *buffer, size_t *size, size_t reqsize);

    };
<<<<<<< HEAD
    int speechMain();
<<<<<<< HEAD
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
=======
>>>>>>> SpeechSynthesis aplay play/pause added
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
=======
    class PlatformSpeechSynthesisProviderWPE {

        public:

            explicit PlatformSpeechSynthesisProviderWPE(PlatformSpeechSynthesizer*);
            ~PlatformSpeechSynthesisProviderWPE();

            void initializeVoiceList(Vector<RefPtr<PlatformSpeechSynthesisVoice>>&);
            void pause();
            void resume();
            void speak(PassRefPtr<PlatformSpeechSynthesisUtterance>);
            void cancel();

            static void  speakFunctionThread (void*);
            static  float m_speechDuration ; /*private member to track speeh time*/

            int m_canPause ;
            int m_isPaused ;
            int m_cancelled ;
            snd_pcm_t *m_handle;
            u_char *m_audioBuf ;
            snd_pcm_uframes_t m_chunkSize; 
            snd_pcm_sframes_t (*writeiFunc)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);

        private:
            bool m_fliteInited;
            PlatformSpeechSynthesizer* m_platformSpeechSynthesizer;
            RefPtr<PlatformSpeechSynthesisUtterance> m_utterance;
            ThreadIdentifier m_speakThread ;
            snd_pcm_stream_t m_stream ;
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
            void doPause(void);
            void doResume(void);
            int playBackExit();



    };

>>>>>>> Speech synthesisApi Implementation
} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)

#endif // PlatformSpeechSynthesisProviderWPE_h
