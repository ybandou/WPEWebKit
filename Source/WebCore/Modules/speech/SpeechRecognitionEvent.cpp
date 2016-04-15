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

#if ENABLE(SPEECH_RECOGNITION)
#include "SpeechRecognitionEvent.h"

namespace WebCore {

SpeechRecognitionEventInit::SpeechRecognitionEventInit()
    : resultIndex(0)
{
}

PassRefPtr<SpeechRecognitionEvent> SpeechRecognitionEvent::create()
{
    return adoptRef(new SpeechRecognitionEvent());
}

PassRefPtr<SpeechRecognitionEvent> SpeechRecognitionEvent::create(const AtomicString& eventName, const SpeechRecognitionEventInit& initializer)
{
    printf("%s:%s:%d \n\n", __FILE__, __func__, __LINE__);

    return adoptRef(new SpeechRecognitionEvent(eventName, initializer));
}

SpeechRecognitionEvent& SpeechRecognitionEvent::createResult(unsigned long resultIndex, const Vector<RefPtr<SpeechRecognitionResult> >& results)
{
    printf("%s:%s:%d \n\n", __FILE__, __func__, __LINE__);
    return (*new SpeechRecognitionEvent(eventNames().resultEvent, resultIndex, SpeechRecognitionResultList::create(results)));
}

SpeechRecognitionEvent& SpeechRecognitionEvent::createNoMatch(PassRefPtr<SpeechRecognitionResult> result)
{
    printf("%s:%s:%d \n\n", __FILE__, __func__, __LINE__);

    if (result) {
        Vector<RefPtr<SpeechRecognitionResult> > results;
        results.append(result);
        return (*new SpeechRecognitionEvent(eventNames().nomatchEvent, 0, SpeechRecognitionResultList::create(results)));
    }

    return (*new SpeechRecognitionEvent(eventNames().nomatchEvent, 0, 0));
}

EventInterface SpeechRecognitionEvent::eventInterface() const
{
    return EventInterface(0);// SpeechRecognitionEventInterfaceType;
}

SpeechRecognitionEvent::SpeechRecognitionEvent()
    : m_resultIndex(0)
{
    printf("%s:%s:%d \n\n", __FILE__, __func__, __LINE__);

}

SpeechRecognitionEvent::SpeechRecognitionEvent(const AtomicString& eventName, const SpeechRecognitionEventInit& initializer)
    : Event(eventName, initializer)
    , m_resultIndex(initializer.resultIndex)
    , m_results(initializer.results)
{
    printf("%s:%s:%d \n\n", __FILE__, __func__, __LINE__);

}

SpeechRecognitionEvent::SpeechRecognitionEvent(const AtomicString& eventName, unsigned long resultIndex, PassRefPtr<SpeechRecognitionResultList> results)
    : Event(eventName, /*canBubble=*/false, /*cancelable=*/false)
    , m_resultIndex(resultIndex)
    , m_results(results)
{
    printf("%s:%s:%d \n\n", __FILE__, __func__, __LINE__);

}

SpeechRecognitionEvent::~SpeechRecognitionEvent()
{
}
} // namespace WebCore
#endif //ENABLE(SPEECH_RECOGNITION)
