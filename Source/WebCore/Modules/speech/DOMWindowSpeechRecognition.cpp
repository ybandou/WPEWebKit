/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DOMWindowSpeechRecognition.h"

#if ENABLE(SPEECH_RECOGNITION)

#include "DOMWindow.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {
    
DOMWindowSpeechRecognition::DOMWindowSpeechRecognition(DOMWindow* window)
    : DOMWindowProperty(window->frame())
{
}

DOMWindowSpeechRecognition::~DOMWindowSpeechRecognition()
{
}

const char* DOMWindowSpeechRecognition::supplementName()
{
    return "DOMWindowSpeechRecognition";
}

// static
DOMWindowSpeechRecognition* DOMWindowSpeechRecognition::from(DOMWindow* window)
{
    DOMWindowSpeechRecognition* supplement = static_cast<DOMWindowSpeechRecognition*>(Supplement<DOMWindow>::from(window, supplementName()));
    if (!supplement) {
        auto newSupplement = std::make_unique<DOMWindowSpeechRecognition>(window);
        supplement = newSupplement.get();
        provideTo(window, supplementName(), WTFMove(newSupplement));
    }
    return supplement;
}

// static
SpeechRecognition* DOMWindowSpeechRecognition::speechRecognition(DOMWindow& window)
{
    printf("Inside %s:%s:%d\n", __FILE__, __func__, __LINE__);
    return DOMWindowSpeechRecognition::from(&window)->speechRecognition(window.scriptExecutionContext());
}

SpeechRecognition* DOMWindowSpeechRecognition::speechRecognition(ScriptExecutionContext* context)
{
    printf("Inside %s:%s:%d\n", __FILE__, __func__, __LINE__);

    if (!m_speechRecognition && frame())
        m_speechRecognition = SpeechRecognition::create(context);
    return m_speechRecognition.get();
}
    
} // namespace WebCore

#endif // ENABLE(SPEECH_RECOGNITION)
