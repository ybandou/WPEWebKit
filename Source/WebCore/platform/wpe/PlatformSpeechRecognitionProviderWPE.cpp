
#include "config.h"

#if ENABLE(SPEECH_RECOGNITION)

#include <wtf/text/CString.h>
#include <unistd.h>
#include <wtf/MainThread.h>

#include <PlatformSpeechRecognizer.h>
#include "PlatformSpeechRecognitionProviderWPE.h"

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

PlatformSpeechRecognitionProviderWPE::PlatformSpeechRecognitionProviderWPE(PlatformSpeechRecognizer* client)
    : m_fireEventThread(0)
    , m_recognitionThread(0)
    , m_platformSpeechRecognizer(client)

{
    printf ("%s:%s:%d \n",__FILE__, __func__, __LINE__);
    initSpeechRecognition();

    m_fireEventStatus = FireEventStarted;

    printf("%s:%s:%d \n", __FILE__, __func__, __LINE__);
    if (!(m_fireEventThread = createThread(fireEventThread, this, "WebCore: SpeechEventFiringThread"))) {
        printf("Error in creating Speech Event Firing Thread\n");
    }     

}

PlatformSpeechRecognitionProviderWPE::~PlatformSpeechRecognitionProviderWPE()
{
    deinitSpeechRecognition();

    if (m_fireEventThread) {
        m_fireEventStatus = FireEventStopped;
        m_waitForEvents.signal();
        waitForThreadCompletion(m_fireEventThread);   
        m_fireEventThread = 0;
    }
}

int PlatformSpeechRecognitionProviderWPE::initSpeechRecognition()
{
    /* Initialize PocketSphinx Recognizer module */ 
    m_config = cmd_ln_init(NULL, contArgsDef, FALSE, NULL, TRUE);

    printf("%s:%s:%d \n", __FILE__, __func__, __LINE__);
   
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

void PlatformSpeechRecognitionProviderWPE::deinitSpeechRecognition()
{
    if (m_audioDevice) {
        ad_close(m_audioDevice);
    }
    if (m_recognizer) {
        free (m_recognizer);
    }
}

void PlatformSpeechRecognitionProviderWPE::recognizeFromDevice()
{
    if (!m_recognitionThread) {    
        m_recognitionStatus = RecognitionStarted;

        printf("%s:%s:%d \n", __FILE__, __func__, __LINE__);
        if (!(m_recognitionThread = createThread(recognitionThread, this, "WebCore: SpeechRecognitionThread"))) {
            printf("Error in creating Recognition Thread\n");
        }     
    }
}     

void PlatformSpeechRecognitionProviderWPE::recognitionThread (void* context)
{
    int16 adBuf[2048];
    uint8 uttStarted, inSpeech;
    int32 data;
    char const *text;
    PlatformSpeechRecognitionProviderWPE *providerContext = (PlatformSpeechRecognitionProviderWPE*) context;
   
    printf("%s:%s:%d \n", __FILE__, __func__, __LINE__);

    if (ad_start_rec( providerContext->m_audioDevice) < 0)
        printf("Failed to start recording\n");
    printf("%s:%s:%d \n", __FILE__, __func__, __LINE__);

    providerContext->m_speechEventQueue.append (std::make_pair(Start,""));
    providerContext->m_waitForEvents.signal();


    if (ps_start_utt(providerContext->m_recognizer) < 0)
        printf("Failed to start utterance\n");
    printf("%s:%s:%d \n", __FILE__, __func__, __LINE__);

    providerContext->m_speechEventQueue.append (std::make_pair(StartAudio,""));
    providerContext->m_waitForEvents.signal();

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

            printf("%s:%s:%d  in_speech = %d utt_started=%d \n",__FILE__, __func__, __LINE__, inSpeech, uttStarted);
            uttStarted = true;
            printf("Listening...\n");

            providerContext->m_speechEventQueue.append (std::make_pair(StartSound,""));
            providerContext->m_waitForEvents.signal();
        }

        if (!inSpeech && uttStarted) {
            
            /* speech -> silence transition, time to start new utterance  */
            ps_end_utt(providerContext->m_recognizer);

            providerContext->m_speechEventQueue.append (std::make_pair(StartSpeech,""));
            text = ps_get_hyp(providerContext->m_recognizer, NULL );
            if (text != NULL) {
                printf("%s\n", text);
                providerContext->m_speechEventQueue.append (std::make_pair(ReceiveResults, text));
                providerContext->m_waitForEvents.signal();
                fflush(stdout);
                /* Fire speech end events */
 
                printf("%s:%s:%d\n",__FILE__, __func__, __LINE__);
            }

            printf("%s:%s:%d\n",__FILE__, __func__, __LINE__);
            if (ps_start_utt(providerContext->m_recognizer) < 0)
                E_FATAL("Failed to start utterance\n");
            uttStarted = FALSE;
            providerContext->m_speechEventQueue.append (std::make_pair(EndSound,""));
            providerContext->m_waitForEvents.signal();

            E_INFO("Ready....\n");
        }
       //usleep(100 * 1000);
    }

    if (ad_stop_rec( providerContext->m_audioDevice) < 0)
        printf("Failed to start recording\n");

    providerContext->m_speechEventQueue.append (std::make_pair(EndAudio,""));
    providerContext->m_speechEventQueue.append (std::make_pair(End,""));
    providerContext->m_waitForEvents.signal();

    printf("%s:%s:%d\n\n",__FILE__, __func__, __LINE__ );

    return;
}



void PlatformSpeechRecognitionProviderWPE::start()
{
    printf("%s:%s:%d \n", __FILE__, __func__, __LINE__);
    recognizeFromDevice();
}

void PlatformSpeechRecognitionProviderWPE::abort()
{
}

void PlatformSpeechRecognitionProviderWPE::stop()
{
    printf("%s:%s:%d \n", __FILE__, __func__, __LINE__);
    if (m_recognitionThread) {
        m_recognitionStatus = RecognitionStopped;
        waitForThreadCompletion (m_recognitionThread);   
        m_recognitionThread = 0;
    }
}

void PlatformSpeechRecognitionProviderWPE::fireEventThread(void* context)
{
    PlatformSpeechRecognitionProviderWPE *providerContext = (PlatformSpeechRecognitionProviderWPE*) context;

    printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
    while (providerContext->m_fireEventStatus == FireEventStarted) {

        while (providerContext->m_waitForEvents.wait(0)) {

            for (const auto& eventQueue: providerContext->m_speechEventQueue) {

                callOnMainThread([providerContext, eventQueue] { 
                    providerContext->fireSpeechEvent(eventQueue);  
                });
                providerContext->m_speechEventQueue.removeFirst(eventQueue);
            }
        }
    }
    return;
}

void PlatformSpeechRecognitionProviderWPE::fireSpeechEvent(const auto& eventQueue) //TODO: input paramter type has to be modified to avoid warning
{
    SpeechEvent speechEvent = eventQueue.first;
     
    switch (speechEvent) {
    case Start:
        printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
        m_platformSpeechRecognizer->client()->didStart();
        break;
    case StartAudio:
        printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
        m_platformSpeechRecognizer->client()->didStartAudio();
        break;
    case StartSound:
        printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
        m_platformSpeechRecognizer->client()->didStartSound();
        break;
    case StartSpeech:
        printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
        m_platformSpeechRecognizer->client()->didStartSpeech();
        break;
    case End:
        printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
        m_platformSpeechRecognizer->client()->didEnd();
        break;
    case EndAudio:
        printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
        m_platformSpeechRecognizer->client()->didEndAudio();
        break;
    case EndSound:
        printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
        m_platformSpeechRecognizer->client()->didEndSound();
        break;
    case EndSpeech:
        printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
        m_platformSpeechRecognizer->client()->didEndSpeech();
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
        m_platformSpeechRecognizer->client()->didReceiveResults(finalResults, interimResults);
        printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
        break;
                }
    case ReceiveNoMatch:
        printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
//        m_platformSpeechRecognizer->client()->didReceiveNoMatch();
        break;
    case ReceiveError:
        printf("%s:%s:%d \n",__FILE__, __func__, __LINE__);
//        m_platformSpeechRecognizer->client()->didReceiveError();
        break;
    default:
        printf("%s:%s:%d invalid event %d\n",__FILE__, __func__, __LINE__, speechEvent);
        break;
    }
}

} // namespace WebCore

#endif
