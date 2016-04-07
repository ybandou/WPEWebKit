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

#ifndef SpeechRecognition_h
#define SpeechRecognition_h

#if ENABLE(SPEECH_RECOGNITION)

#include <PlatformSpeechRecognizer.h>
#include "ActiveDOMObject.h"
#include "EventTarget.h"
#include "SpeechGrammarList.h"
#include "wtf/Compiler.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class ScriptExecutionContext;
class SpeechRecognitionController;
class SpeechRecognitionError;
class SpeechRecognitionResult;
class SpeechRecognitionResultList;
class PlatformSpeechRecognizerClient;

class SpeechRecognition : public PlatformSpeechRecognizerClient, public RefCounted<SpeechRecognition>, public EventTarget {
public:
    static PassRefPtr<SpeechRecognition> create();
    ~SpeechRecognition();

    // Attributes.
    PassRefPtr<SpeechGrammarList> grammars() { return m_grammars; }
    void setGrammars(PassRefPtr<SpeechGrammarList> grammars) { m_grammars = grammars; }
    String lang() { return m_lang; }
    void setLang(const String& lang) { m_lang = lang; }
    bool continuous() { return m_continuous; }
    void setContinuous(bool continuous) { m_continuous = continuous; }
    bool interimResults() { return m_interimResults; }
    void setInterimResults(bool interimResults) { m_interimResults = interimResults; }
    unsigned long maxAlternatives() { return m_maxAlternatives; }
    void setMaxAlternatives(unsigned long maxAlternatives) { m_maxAlternatives = maxAlternatives; }

    // Callable by the user.
    void start(ExceptionCode&);
    void stopFunction();
    void abort();

    // Called by the SpeechRecognizer.
    void didStartAudio() override;
    void didStartSound() override;
    void didStartSpeech() override;
    void didEndSpeech() override;
    void didEndSound() override;
    void didEndAudio() override;
    void didReceiveResults(const Vector<RefPtr<SpeechRecognitionResult> >& newFinalResults, const Vector<RefPtr<SpeechRecognitionResult> >& currentInterimResults) override;
    void didReceiveNoMatch(PassRefPtr<SpeechRecognitionResult>) override;
    void didReceiveError(SpeechRecognitionError&) override;
    void didStart() override;
    void didEnd() override;


    // EventTarget.
    EventTargetInterface eventTargetInterface() const override;
    ScriptExecutionContext* scriptExecutionContext() const override;   
 
    // ActiveDOMObject.
    bool hasPendingActivity() const;
    void stop(); 

    using RefCounted<SpeechRecognition>::ref;
    using RefCounted<SpeechRecognition>::deref;

private:
    friend class RefCounted<SpeechRecognition>;

    SpeechRecognition();


    // EventTarget
    void refEventTarget() override { ref(); }
    void derefEventTarget() override { deref(); }
    EventTargetData* eventTargetData() override { return &m_eventTargetData; }
    EventTargetData& ensureEventTargetData() override { return m_eventTargetData; }

    RefPtr<SpeechGrammarList> m_grammars;
    String m_lang;
    bool m_continuous;
    bool m_interimResults;
    unsigned long m_maxAlternatives;

    EventTargetData m_eventTargetData;
    
    std::unique_ptr<PlatformSpeechRecognizer> m_platformSpeechRecognizer;
    bool m_stoppedByActiveDOMObject;
    bool m_started;
    bool m_stopping;
    Vector<RefPtr<SpeechRecognitionResult> > m_finalResults;

};

} // namespace WebCore

#endif //ENABLE(SPEECH_RECOGNITION)

#endif // SpeechRecognition_h
