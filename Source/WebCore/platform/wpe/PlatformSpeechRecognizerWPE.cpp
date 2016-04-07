#include "config.h"
#include "PlatformSpeechRecognizer.h"
#include "PlatformSpeechRecognitionProviderWPE.h"

using namespace WebCore;
using namespace std;

namespace WebCore {

PlatformSpeechRecognizer::PlatformSpeechRecognizer(PlatformSpeechRecognizerClient* client)
    : m_speechRecognizerClient(client)
    , m_platformSpeechWrapper(std::make_unique<PlatformSpeechRecognitionProviderWPE>(this))
{
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
}

PlatformSpeechRecognizer::~PlatformSpeechRecognizer()
{
}

void PlatformSpeechRecognizer::start()
{
    printf("Inside:%s:%s:%d\n", __FILE__, __func__, __LINE__);
    ASSERT(m_platformSpeechWrapper);
    m_platformSpeechWrapper->start();
}

void PlatformSpeechRecognizer::abort()
{
} 

void PlatformSpeechRecognizer::stop()
{
}

}
