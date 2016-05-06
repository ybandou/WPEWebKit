
#include "config.h"
#include "PlatformSpeechSynthesizer.h"

#if ENABLE(SPEECH_SYNTHESIS)

#include <PlatformSpeechSynthesisVoice.h>
#include "PlatformSpeechSynthesisProviderWPE.h"
#include <PlatformSpeechSynthesisUtterance.h>

namespace WebCore {
    PlatformSpeechSynthesizer::PlatformSpeechSynthesizer(PlatformSpeechSynthesizerClient* client)
        : m_voiceListIsInitialized(false)
          , m_speechSynthesizerClient(client)
          , m_platformSpeechWrapper(std::make_unique<PlatformSpeechSynthesisProviderWPE>(this))
    {
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    }

    PlatformSpeechSynthesizer::~PlatformSpeechSynthesizer()
    {
    }

    void PlatformSpeechSynthesizer::initializeVoiceList()
    {
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        ASSERT(m_platformSpeechWrapper);
        m_platformSpeechWrapper->initializeVoiceList(m_voiceList);
    }

    void PlatformSpeechSynthesizer::pause()
    {
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        ASSERT(m_platformSpeechWrapper);
        m_platformSpeechWrapper->pause();
    }

    void PlatformSpeechSynthesizer::resume()
    {
        ASSERT(m_platformSpeechWrapper);
        m_platformSpeechWrapper->resume();
    }

    void PlatformSpeechSynthesizer::speak(PassRefPtr<PlatformSpeechSynthesisUtterance> utterance)
    {
        ASSERT(m_platformSpeechWrapper);
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        m_platformSpeechWrapper->speak(utterance);
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    }

    void PlatformSpeechSynthesizer::cancel()
    {
        ASSERT(m_platformSpeechWrapper);
        m_platformSpeechWrapper->cancel();
    }

} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)
