
#include "config.h"
#include "PlatformSpeechRecognitionProviderWPE.h"

#if ENABLE(SPEECH_RECOGNITION)

#include <NotImplemented.h>
#include <PlatformSpeechRecognizer.h>
#include <wtf/text/CString.h>
#include <unistd.h>


namespace WebCore {

static void callBack(char const *buffer)
{
    printf("\n%s:%s:%d String = %s\n", __FILE__, __func__, __LINE__, buffer);
}

void PlatformSpeechRecognitionProviderWPE::recognizeFromMIC(void (*callback)(char const *buffer))
{
  
    m_recognitionStatus = RecognitionStarted;
    m_callBack = callback;

    if (pthread_create(&m_recognitionThread, NULL, &recognitionThread, this))
    {
        printf("Error in creating Recognition Thread\n");
    }     
}     

void* PlatformSpeechRecognitionProviderWPE::recognitionThread (void* context)
{
    PlatformSpeechRecognitionProviderWPE *providerContext = (PlatformSpeechRecognitionProviderWPE*) context;
    int16 adBuf[2048];
    uint8 uttStarted, inSpeech;
    int32 data;
    char const *hyp;
    static ps_decoder_t *ps;
    ad_rec_t *ad;

    if ((ad = ad_open_dev("plughw:1", 16000)) == NULL) {
        printf("Failed to open audio device\n");
    }

    if (ad_start_rec(ad) < 0)
        printf("Failed to start recording\n");

    if (ps_start_utt(ps) < 0)
        printf("Failed to start utterance\n");

    uttStarted = FALSE;
    E_INFO("Ready....\n");

    while ( providerContext->m_recognitionStatus == RecognitionStarted ) {
        if ((data = ad_read(ad, adBuf, 2048)) < 0)
        {
            E_FATAL("Failed to read audio\n");
            printf("Failed to read audio");
        }
        printf("\n%s:%s:%d\n",__FILE__, __func__, __LINE__);

        ps_process_raw(ps, adBuf, data, false, false);
        inSpeech = ps_get_in_speech(ps);

        printf("\n%s:%s:%d  in_speech = %d utt_started=%d \n",__FILE__, __func__, __LINE__, inSpeech, uttStarted);

        if (inSpeech && !uttStarted) {
            uttStarted = true;
            printf("Listening...\n");
        }
        printf("\n%s:%s:%d\n",__FILE__, __func__, __LINE__);

        if (!inSpeech && uttStarted) {
            /* speech -> silence transition, time to start new utterance  */
            ps_end_utt(ps);
            hyp = ps_get_hyp(ps, NULL );
            if (hyp != NULL) {
                printf("%s\n", hyp);
                providerContext->m_callBack(hyp);
                fflush(stdout);
            }
            printf("\n%s:%s:%d\n",__FILE__, __func__, __LINE__);

            if (ps_start_utt(ps) < 0)
                E_FATAL("Failed to start utterance\n");
            uttStarted = FALSE;
            E_INFO("Ready....\n");
        }
       //usleep(100 * 1000);
    }

    ad_close(ad);
    return NULL;
}

PlatformSpeechRecognitionProviderWPE::PlatformSpeechRecognitionProviderWPE(PlatformSpeechRecognizer* client)
    : m_platformSpeechRecognizer(client)
{
    printf ("This is line %d of file %s (function %s) FLITE INITED *********\n",__LINE__, __FILE__, __func__);
}

PlatformSpeechRecognitionProviderWPE::~PlatformSpeechRecognitionProviderWPE()
{

}

void PlatformSpeechRecognitionProviderWPE::start()
{
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    recognizeFromMIC(callBack);
}

void PlatformSpeechRecognitionProviderWPE::abort()
{
}

void PlatformSpeechRecognitionProviderWPE::stop()
{
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    m_recognitionStatus = RecognitionStopped;
    pthread_join(m_recognitionThread, NULL);   
}

} // namespace WebCore

#endif
