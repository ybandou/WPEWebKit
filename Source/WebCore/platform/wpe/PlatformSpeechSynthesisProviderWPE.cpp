
#include "config.h"
#include "PlatformSpeechSynthesisProviderWPE.h"
#if ENABLE(SPEECH_SYNTHESIS)
#ifndef FLITE_C_INCLUDED
#define FLITE_C_INCLUDED
extern "C" {
#include <flite_voice_list.c>
}
#endif
#include <NotImplemented.h>
#include <PlatformSpeechSynthesisUtterance.h>
#include <PlatformSpeechSynthesisVoice.h>
#include <PlatformSpeechSynthesizer.h>
#include <wtf/text/CString.h>
#include <unistd.h>


namespace WebCore {

/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

PlatformSpeechSynthesisProviderWPE::PlatformSpeechSynthesisProviderWPE(PlatformSpeechSynthesizer* client)
    : m_fliteInited(false)
    , m_speechDuration(0.0)
    , m_platformSpeechSynthesizer(client)
{
    flite_init(); 
    m_fliteInited = true;
    printf ("This is line %d of file %s (function %s) FLITE INITED \n",__LINE__, __FILE__, __func__);
}

/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

PlatformSpeechSynthesisProviderWPE::~PlatformSpeechSynthesisProviderWPE()
{
    delete_val(flite_voice_list); flite_voice_list=0;

}
/***********************************************************************
* @brief : Init the  Voice list
* @Function Name : void initializeVoiceList(Vector<RefPtr<PlatformSpeechSynthesisVoice>>&);
*
* @param  : Vector<RefPtr<PlatformSpeechSynthesisVoice>>& ,
*           Reference to voice list.  
* @return : Void 
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::initializeVoiceList(Vector<RefPtr<PlatformSpeechSynthesisVoice>>& voiceList)
{
    cst_voice *voice;
    const cst_val *v;
    WTF::String voiceId = "default";
    WTF::String langSupport = "en-US";
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    
    flite_voice_list = flite_set_voice_list();
    for (v=flite_voice_list; v; v=val_cdr(v))
    {
        voice = val_voice(val_car(v));
        /*Convert the  voice to human readable form */
           
        String voiceName = voice->name;
        if(voiceName.contains("kal",true))
        {
            printf("Kal sound is heard \n");
            langSupport = "en-US" ;
            voiceName = "US English Male";
        } 
        else if(voiceName.contains("rms",true))
        {
            printf("rms sound is heard \n");
            langSupport = "en-US" ;
            voiceName = "US English Male";
        } 

        else if(voiceName.contains("awb",true))
        {
            printf("awb sound is heard \n");
            langSupport = "en-Scott" ;
            voiceName = "Scottish English Male";
        } 
        else if(voiceName.contains("slt",true))
        {
            printf("slt sound is heard \n");
            langSupport = "en-US" ;
            voiceName = "US English Female";
        } 
        else
        {
            printf("Support for this language has to be add \n");
        }
        voiceList.append(PlatformSpeechSynthesisVoice::create(String(voiceId), voiceName,langSupport, true, true));
    }

}

void PlatformSpeechSynthesisProviderWPE::pause()
{
    notImplemented();
}

/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::resume()
{
    notImplemented();
}

/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::speak(PassRefPtr<PlatformSpeechSynthesisUtterance> utterance)
{
    m_utterance = utterance;
    cst_voice *v;
    
    if (! m_fliteInited || !utterance) {
        fireSpeechEvent(SpeechError);
        return;
    }
    
    v = register_cmu_us_kal(NULL); //TODO: 
                                   // this has to be set based on user settings
    m_speechDuration =  flite_text_to_speech((m_utterance->text()).utf8().data(),v,"play");
    printf ("This is line %d of file %s (function %s) speak for  duration= %f\n",__LINE__, __FILE__, __func__,m_speechDuration);
    fireSpeechEvent(SpeechStart);
    sleep(int(m_speechDuration));
    fireSpeechEvent(SpeechEnd);

}

/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::cancel()
{
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    if (!m_fliteInited || !m_utterance)
        return;
   //TODO 
       
    fireSpeechEvent(SpeechCancel);
    m_utterance = nullptr;

}

/***********************************************************************
* @brief :  To fire  platform events to the generic module 
* @Function Name : void fireSpeechEvent(SpeechEvent speechEvent)
* @param : SpeechEvent - Event marker
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::fireSpeechEvent(SpeechEvent speechEvent)
{
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    switch (speechEvent) {
    case SpeechStart:
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        m_platformSpeechSynthesizer->client()->didStartSpeaking(m_utterance);
        break;
    case SpeechPause:
        m_platformSpeechSynthesizer->client()->didPauseSpeaking(m_utterance);
        break;
    case SpeechResume:
        m_platformSpeechSynthesizer->client()->didResumeSpeaking(m_utterance);
        break;
    case SpeechError:
        m_fliteInited = false;
    case SpeechCancel:
        m_platformSpeechSynthesizer->client()->speakingErrorOccurred(m_utterance);
        break;
    case SpeechEnd:
        printf ("This is line %d of file %s (function %s) \n",__LINE__, __FILE__, __func__);
        m_platformSpeechSynthesizer->client()->didFinishSpeaking(m_utterance);
        break;
    default:
        ASSERT_NOT_REACHED();
    };
}


} // namespace WebCore

#endif //speech synthesis
