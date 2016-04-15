
#include "config.h"
#include "PlatformSpeechRecognitionProviderWPE.h"

#if ENABLE(SPEECH_RECOGNITION)

#include <NotImplemented.h>
#include <PlatformSpeechRecognizer.h>
#include <wtf/text/CString.h>
#include <unistd.h>


static const arg_t contArgsDef[] = {
    POCKETSPHINX_OPTIONS,
    /* Argument file. */
    {"-argfile",
     ARG_STRING,
     NULL,
     "Argument file giving extra arguments."},
    {"-adcdev",
     ARG_STRING,
     NULL,
     "Name of audio device to use for input."},
    {"-time",
     ARG_BOOLEAN,
     "no",
     "Print word times in file transcription."},
    CMDLN_EMPTY_OPTION
};

namespace WebCore {

static void callBack(char const *buffer)
{
    printf("\n%s:%s:%d String = %s\n", __FILE__, __func__, __LINE__, buffer);
}

void PlatformSpeechRecognitionProviderWPE::recognizeFromDevice(void (*callback)(char const *buffer))
{
  
    m_recognitionStatus = RecognitionStarted;
    m_callBack = callback;

    printf("Inside %s:%s:%d \n", __FILE__, __func__, __LINE__);
    if (pthread_create(&m_recognitionThread, NULL, &recognitionThread, this)) {
        printf("Error in creating Recognition Thread\n");
    }     

}     

void* PlatformSpeechRecognitionProviderWPE::recognitionThread (void* context)
{
    int16 adBuf[2048];
    uint8 uttStarted, inSpeech;
    int32 data;
    char const *text;
    PlatformSpeechRecognitionProviderWPE *providerContext = (PlatformSpeechRecognitionProviderWPE*) context;
   
    printf("Inside %s:%s:%d \n", __FILE__, __func__, __LINE__);

    if (ad_start_rec( providerContext->m_audioDevice) < 0)
        printf("Failed to start recording\n");
    printf("Inside %s:%s:%d \n", __FILE__, __func__, __LINE__);

    if (ps_start_utt(providerContext->m_recognizer) < 0)
        printf("Failed to start utterance\n");
    printf("Inside %s:%s:%d \n", __FILE__, __func__, __LINE__);

    uttStarted = FALSE;
    printf("Ready....\n");

    while ( providerContext->m_recognitionStatus == RecognitionStarted ) {
        if ((data = ad_read(providerContext->m_audioDevice, adBuf, 2048)) < 0) {
        
            E_FATAL("Failed to read audio\n");
            printf("Failed to read audio");
        }

        ps_process_raw(providerContext->m_recognizer, adBuf, data, false, false);
        inSpeech = ps_get_in_speech(providerContext->m_recognizer);

        if (inSpeech && !uttStarted) {

            printf("\n%s:%s:%d  in_speech = %d utt_started=%d \n",__FILE__, __func__, __LINE__, inSpeech, uttStarted);
            uttStarted = true;
            printf("Listening...\n");
            /* Fire speech started events */
            providerContext->m_speechEventQueue.append (std::make_pair(Start,""));
            providerContext->m_speechEventQueue.append (std::make_pair(StartAudio,""));
            providerContext->m_speechEventQueue.append (std::make_pair(StartSound,""));

        }

        if (!inSpeech && uttStarted) {
            /* speech -> silence transition, time to start new utterance  */
            ps_end_utt(providerContext->m_recognizer);
            text = ps_get_hyp(providerContext->m_recognizer, NULL );
            if (text != NULL) {
                printf("%s\n", text);
                providerContext->m_callBack(text);
                providerContext->m_speechEventQueue.append (std::make_pair(ReceiveResults, text));
                fflush(stdout);
                /* Fire speech end events */
                providerContext->m_speechEventQueue.append (std::make_pair(EndSound,""));
                providerContext->m_speechEventQueue.append (std::make_pair(EndAudio,""));
  
                printf("\n%s:%s:%d\n",__FILE__, __func__, __LINE__);
            }

            printf("\n%s:%s:%d\n",__FILE__, __func__, __LINE__);
            if (ps_start_utt(providerContext->m_recognizer) < 0)
                E_FATAL("Failed to start utterance\n");
            uttStarted = FALSE;
            E_INFO("Ready....\n");
        }
       //usleep(100 * 1000);
    }

    if (ad_stop_rec( providerContext->m_audioDevice) < 0)
        printf("Failed to start recording\n");

    providerContext->m_speechEventQueue.append (std::make_pair(End,""));
    printf("End of Thread %s:%s:%d\n\n",__FILE__, __func__, __LINE__ );

    return NULL;
}

PlatformSpeechRecognitionProviderWPE::PlatformSpeechRecognitionProviderWPE(PlatformSpeechRecognizer* client)
    : m_platformSpeechRecognizer(client)
{
    printf ("This is line %d of file %s (function %s) *********\n",__LINE__, __FILE__, __func__);
    initSpeech();

    m_fireEventStatus = FireEventStarted;

    printf("Inside %s:%s:%d \n", __FILE__, __func__, __LINE__);
    if (pthread_create(&m_fireEventThread, NULL, &fireEventThread, this)) {
        printf("Error in creating Recognition Thread\n");
    }     

}

PlatformSpeechRecognitionProviderWPE::~PlatformSpeechRecognitionProviderWPE()
{
    deinitSpeech();

    m_fireEventStatus = FireEventStopped;
    pthread_join(m_fireEventThread, NULL);   
   
}

int PlatformSpeechRecognitionProviderWPE::initSpeech()
{
    /* Initialize PocketSphinx Recognizer module */ 
    m_config = cmd_ln_init(NULL, contArgsDef, FALSE, NULL, TRUE);

    printf("Inside %s:%s:%d \n", __FILE__, __func__, __LINE__);
   
    ps_default_search_args(m_config);
    m_recognizer = ps_init(m_config);
    if (m_recognizer == NULL) {
        printf("Failed to create recognizer, hence exiting the thread\n");
        return -1;
    }

    /* Open Microphone device */
    if ((m_audioDevice = ad_open_dev("plughw:1", 16000)) == NULL) {
        printf("Failed to open audio device\n");
        return -1;
    }

    return 0;
}

void PlatformSpeechRecognitionProviderWPE::deinitSpeech()
{
    if (m_audioDevice) {
        ad_close(m_audioDevice);
    }
    if (m_recognizer) {
        free (m_recognizer);
    }
}

void PlatformSpeechRecognitionProviderWPE::start()
{
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    recognizeFromDevice(callBack);
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

void* PlatformSpeechRecognitionProviderWPE::fireEventThread(void* context)
{
    PlatformSpeechRecognitionProviderWPE *providerContext = (PlatformSpeechRecognitionProviderWPE*) context;

    printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
    while (providerContext->m_fireEventStatus == FireEventStarted) {
        for (const auto& eventQueue: providerContext->m_speechEventQueue) {
    
            SpeechEvent speechEvent = eventQueue.first;

            switch (speechEvent) {
            case Start:
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
                providerContext->m_platformSpeechRecognizer->client()->didStart();
                break;
            case StartAudio:
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
                providerContext->m_platformSpeechRecognizer->client()->didStartAudio();
                break;
            case StartSound:
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
                providerContext->m_platformSpeechRecognizer->client()->didStartSound();
                break;
            case StartSpeech:
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
                providerContext->m_platformSpeechRecognizer->client()->didStartSpeech();
                break;
            case End:
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
                providerContext->m_platformSpeechRecognizer->client()->didEnd();
                break;
            case EndAudio:
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
                providerContext->m_platformSpeechRecognizer->client()->didEndAudio();
                break;
            case EndSound:
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
                providerContext->m_platformSpeechRecognizer->client()->didEndSound();
                break;
            case EndSpeech:
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
                providerContext->m_platformSpeechRecognizer->client()->didEndSpeech();
                break;
            case ReceiveResults: {

                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
                String transcripts (eventQueue.second);
                double confidence;
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);

                Vector<RefPtr<SpeechRecognitionAlternative> > alternatives;
                alternatives.append(SpeechRecognitionAlternative::create(transcripts, confidence));
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);

                Vector<RefPtr<SpeechRecognitionResult>> finalResults;
                Vector<RefPtr<SpeechRecognitionResult>> interimResults;
                 
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
                finalResults.append(SpeechRecognitionResult::create(alternatives, true)); 
                providerContext->m_platformSpeechRecognizer->client()->didReceiveResults(finalResults, interimResults);

                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
                break;
                }
            case ReceiveNoMatch:
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
//                providerContext->m_platformSpeechRecognizer->client()->didReceiveNoMatch();
                break;
            case ReceiveError:
                printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
//                providerContext->m_platformSpeechRecognizer->client()->didReceiveError();
                break;
            default:
                printf("%s:%s:%d invalid event %d\n",__FILE__, __func__, __LINE__, speechEvent);
                break;
            }

            providerContext->m_speechEventQueue.removeFirst(eventQueue);
        }
    }
    return NULL;
}

} // namespace WebCore

#endif
