/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PlatformSpeechRecognizer_h
#define PlatformSpeechRecognizer_h

#if ENABLE(SPEECH_RECOGNITION)
#include <SpeechRecognitionResult.h>
#include <SpeechRecognitionError.h>

#if PLATFORM(WPE)
namespace WebCore {
class PlatformSpeechRecognitionProviderWPE;
}
#endif

#include "wtf/text/WTFString.h"


namespace WebCore {

class Page;
class SpeechGrammarList;
class SpeechRecognition;

class PlatformSpeechRecognizerClient {
    virtual void didStartAudio();
    virtual void didStartSound();
    virtual void didStartSpeech();
    virtual void didEndSpeech();
    virtual void didEndSound();
    virtual void didEndAudio();
    virtual void didReceiveResults(const Vector<RefPtr<SpeechRecognitionResult> >& newFinalResults, const Vector<RefPtr<SpeechRecognitionResult> >& currentInterimResults);
    virtual void didReceiveNoMatch(PassRefPtr<SpeechRecognitionResult>);
    virtual void didReceiveError(SpeechRecognitionError&);
    virtual void didStart();
    virtual void didEnd();
};

class PlatformSpeechRecognizer {
public:
    WEBCORE_EXPORT explicit PlatformSpeechRecognizer(PlatformSpeechRecognizerClient*);

    // FIXME: We have multiple virtual functions just so we can support a mock for testing.
    // Seems wasteful. Would be nice to find a better way.
    WEBCORE_EXPORT virtual ~PlatformSpeechRecognizer();

    virtual void start();
    virtual void stop();
    virtual void abort();

    PlatformSpeechRecognizerClient* client() const { return m_speechRecognizerClient; }

private:

    PlatformSpeechRecognizerClient* m_speechRecognizerClient;
#if PLATFORM(WPE)
    std::unique_ptr<PlatformSpeechRecognitionProviderWPE> m_platformSpeechWrapper;
#endif
};
//void provideSpeechRecognitionTo(Page*, SpeechRecognitionClient*);

} // namespace WebCore

#endif //ENABLE(SPEECH_RECOGNITION)
#endif // SpeechRecognitionClient_h
