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

#include "config.h"

#include "SpeechRecognition.h"

#if ENABLE(SPEECH_RECOGNITION)

#include "SpeechRecognition.h"

#include "Document.h"
#include "ExceptionCode.h"
#include "Page.h"
#include "SpeechRecognitionController.h"
#include "SpeechRecognitionError.h"
#include "SpeechRecognitionEvent.h"

namespace WebCore {


PassRefPtr<SpeechRecognition> SpeechRecognition::create()
{
    printf("@@@@@@@@@@@@@@@@@@%s:%s:%d\n\n",__FILE__, __func__, __LINE__ );

    RefPtr<SpeechRecognition> speechRecognition(adoptRef(new SpeechRecognition()));
    return speechRecognition.release();
}

void SpeechRecognition::start(ExceptionCode& exceptionCode)
{
    printf("@@@@@@@@@@@@@@@@@@%s:%s:%d\n\n",__FILE__, __func__, __LINE__ );

    if (m_started) {
        exceptionCode = INVALID_STATE_ERR;
        return;
    }
    printf("@@@@@@@@@@@@@@@@@@%s:%s:%d\n\n",__FILE__, __func__, __LINE__ );

    m_finalResults.clear();
    printf("@@@@@@@@@@@@@@@@@@%s:%s:%d\n\n",__FILE__, __func__, __LINE__ );
    
    if (!m_platformSpeechRecognizer)
        m_platformSpeechRecognizer = std::make_unique<PlatformSpeechRecognizer>(this);

    m_platformSpeechRecognizer->start();

}

void SpeechRecognition::stopFunction()
{
    if (m_started && !m_stopping) {
        m_stopping = true;
        /* Add code to stop */
    }
}

void SpeechRecognition::abort()
{
    if (m_started && !m_stopping) {
        m_stopping = true;
        /* Add code to abort */
    }
}

void SpeechRecognition::didStartAudio()
{
    dispatchEvent(Event::create(eventNames().audiostartEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didStartSound()
{
    dispatchEvent(Event::create(eventNames().soundstartEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didStartSpeech()
{
    dispatchEvent(Event::create(eventNames().speechstartEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didEndSpeech()
{
    dispatchEvent(Event::create(eventNames().speechendEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didEndSound()
{
    dispatchEvent(Event::create(eventNames().soundendEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didEndAudio()
{
    dispatchEvent(Event::create(eventNames().audioendEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didReceiveResults(const Vector<RefPtr<SpeechRecognitionResult> >& newFinalResults, const Vector<RefPtr<SpeechRecognitionResult> >& currentInterimResults)
{
    unsigned long resultIndex = m_finalResults.size();

    for (size_t i = 0; i < newFinalResults.size(); ++i)
        m_finalResults.append(newFinalResults[i]);

    Vector<RefPtr<SpeechRecognitionResult> > results = m_finalResults;
    for (size_t i = 0; i < currentInterimResults.size(); ++i)
        results.append(currentInterimResults[i]);

    dispatchEvent(SpeechRecognitionEvent::createResult(resultIndex, results));
}

void SpeechRecognition::didReceiveNoMatch(PassRefPtr<SpeechRecognitionResult> result)
{
    dispatchEvent(SpeechRecognitionEvent::createNoMatch(result));
}

void SpeechRecognition::didReceiveError(SpeechRecognitionError& error)
{
    dispatchEvent(error);
    m_started = false;
}

void SpeechRecognition::didStart()
{
    dispatchEvent(Event::create(eventNames().startEvent, /*canBubble=*/false, /*cancelable=*/false));
}

void SpeechRecognition::didEnd()
{
    m_started = false;
    m_stopping = false;
    if (!m_stoppedByActiveDOMObject)
        dispatchEvent(Event::create(eventNames().endEvent, /*canBubble=*/false, /*cancelable=*/false));
}

EventTargetInterface SpeechRecognition::eventTargetInterface() const
{
    return (EventTargetInterface)0;// EventTargetNames::SpeechRecognition;
}

ScriptExecutionContext* SpeechRecognition::scriptExecutionContext() const
{
    return 0;
}

void SpeechRecognition::stop()
{
    m_stoppedByActiveDOMObject = true;
    if (hasPendingActivity())
        abort();
}


bool SpeechRecognition::hasPendingActivity() const
{
    return m_started;
}

SpeechRecognition::SpeechRecognition()
    : m_grammars(SpeechGrammarList::create()) // FIXME: The spec is not clear on the default value for the grammars attribute.
    , m_continuous(false)
    , m_interimResults(false)
    , m_maxAlternatives(1)
    , m_stoppedByActiveDOMObject(false)
    , m_started(false)
    , m_stopping(false)
{
    //Document* document = downcast<Document>(scriptExecutionContext());

    //Page* page = document->page();
    //ASSERT(page);
    printf("@@@@@@@@@@@@@@@@@@%s:%s:%d\n\n",__FILE__, __func__, __LINE__ );


    // FIXME: Need to hook up with Page to get notified when the visibility changes.
}

SpeechRecognition::~SpeechRecognition()
{
}

} // namespace WebCore
#endif //ENABLE(SPEECH_RECOGNITION)

