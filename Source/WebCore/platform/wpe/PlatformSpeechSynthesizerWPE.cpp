
#include "config.h"
#include "PlatformSpeechSynthesizer.h"

#if ENABLE(SPEECH_SYNTHESIS)

#include <PlatformSpeechSynthesisVoice.h>
#include "PlatformSpeechSynthesisProviderWPE.h"
#include <PlatformSpeechSynthesisUtterance.h>

namespace WebCore {

/***********************************************************************
* @brief :  PlatformSpeechSynthesizer constructor
* @Function Name : PlatformSpeechSynthesizer(PlatformSpeechSynthesizerClient*);
* @param : PlatformSpeechSynthesizerClient* - To fire the events from platform.
*
***********************************************************************/

PlatformSpeechSynthesizer::PlatformSpeechSynthesizer(PlatformSpeechSynthesizerClient* client)
    : m_voiceListIsInitialized(false)
    , m_speechSynthesizerClient(client)
    , m_platformSpeechWrapper(std::make_unique<PlatformSpeechSynthesisProviderWPE>(this))
{
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
}
/***********************************************************************
* @brief : PlatformSpeechSynthesizer destuctor
* @Function Name : ~PlatformSpeechSynthesizer()
***********************************************************************/

PlatformSpeechSynthesizer::~PlatformSpeechSynthesizer()
{
}
/***********************************************************************
* @brief : Initialize voice list dynamically 
*              based on the platform support .
* @Function Name : void initializeVoiceList()
***********************************************************************/

void PlatformSpeechSynthesizer::initializeVoiceList()
{
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    ASSERT(m_platformSpeechWrapper);
    m_platformSpeechWrapper->initializeVoiceList(m_voiceList);
}
/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

void PlatformSpeechSynthesizer::pause()
{
    ASSERT(m_platformSpeechWrapper);
    m_platformSpeechWrapper->pause();
}
/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

void PlatformSpeechSynthesizer::resume()
{
    ASSERT(m_platformSpeechWrapper);
    m_platformSpeechWrapper->resume();
}
/***********************************************************************
* @brief : Speak interface to initiate spaeking of the text 
* @Function Name :void speak(PassRefPtr<PlatformSpeechSynthesisUtterance>)
*
* @param PlatformSpeechSynthesisUtterance - Controls the text queue
***********************************************************************/

void PlatformSpeechSynthesizer::speak(PassRefPtr<PlatformSpeechSynthesisUtterance> utterance)
{
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    ASSERT(m_platformSpeechWrapper);
    m_platformSpeechWrapper->speak(utterance);
}
/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

void PlatformSpeechSynthesizer::cancel()
{
    ASSERT(m_platformSpeechWrapper);
    m_platformSpeechWrapper->cancel();
}

} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)
