
#include "config.h"
#include "DOMWindowSpeechSynthesis.h"

#if ENABLE(SPEECH_SYNTHESIS)

#include "DOMWindow.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {
    
DOMWindowSpeechSynthesis::DOMWindowSpeechSynthesis(DOMWindow* window)
    : DOMWindowProperty(window->frame())
{
}

DOMWindowSpeechSynthesis::~DOMWindowSpeechSynthesis()
{
}

const char* DOMWindowSpeechSynthesis::supplementName()
{
    return "DOMWindowSpeechSynthesis";
}

// static
DOMWindowSpeechSynthesis* DOMWindowSpeechSynthesis::from(DOMWindow* window)
{
    DOMWindowSpeechSynthesis* supplement = static_cast<DOMWindowSpeechSynthesis*>(Supplement<DOMWindow>::from(window, supplementName()));
    if (!supplement) {
        auto newSupplement = std::make_unique<DOMWindowSpeechSynthesis>(window);
        supplement = newSupplement.get();
        provideTo(window, supplementName(), WTFMove(newSupplement));
    }
    return supplement;
}

// static
SpeechSynthesis* DOMWindowSpeechSynthesis::speechSynthesis(DOMWindow& window)
{
    return DOMWindowSpeechSynthesis::from(&window)->speechSynthesis();
}

SpeechSynthesis* DOMWindowSpeechSynthesis::speechSynthesis()
{
    if (!m_speechSynthesis && frame())
        m_speechSynthesis = SpeechSynthesis::create();
    return m_speechSynthesis.get();
}
    
} // namespace WebCore

#endif // ENABLE(SPEECH_SYNTHESIS)
