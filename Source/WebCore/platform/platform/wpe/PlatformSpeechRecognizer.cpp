/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "SpeechRecognizerWPE.h"

#include "public/testing/WebTestDelegate.h"
#include "public/web/WebSpeechRecognitionResult.h"
#include "public/web/WebSpeechRecognizerClient.h"

using namespace WebCore;
using namespace std;

namespace WebTestRunner {

namespace {

// Task class for calling a client function that does not take any parameters.
typedef void (WebSpeechRecognizerClient::*ClientFunctionPointer)(const WebSpeechRecognitionHandle&);
class ClientCallTask : public SpeechRecognizerWPE::Task {
public:
    ClientCallTask(SpeechRecognizerWPE* wpe, ClientFunctionPointer function)
        : SpeechRecognizerWPE::Task(wpe)
        , m_function(function)
    {
    }

    virtual void run() OVERRIDE { (m_recognizer->client()->*m_function)(m_recognizer->handle()); }

private:
    ClientFunctionPointer m_function;
};

// Task for delivering a result event.
class ResultTask : public SpeechRecognizerWPE::Task {
public:
    ResultTask(SpeechRecognizerWPE* wpe, const WebString transcript, float confidence)
        : SpeechRecognizerWPE::Task(wpe)
        , m_transcript(transcript)
        , m_confidence(confidence)
    {
    }

    virtual void run() OVERRIDE
    {
        WebVector<WebString> transcripts(static_cast<size_t>(1));
        WebVector<float> confidences(static_cast<size_t>(1));
        transcripts[0] = m_transcript;
        confidences[0] = m_confidence;
        WebVector<WebSpeechRecognitionResult> finalResults(static_cast<size_t>(1));
        WebVector<WebSpeechRecognitionResult> interimResults;
        finalResults[0].assign(transcripts, confidences, true);

        m_recognizer->client()->didReceiveResults(m_recognizer->handle(), finalResults, interimResults);
    }

private:
    WebString m_transcript;
    float m_confidence;
};

// Task for delivering a nomatch event.
class NoMatchTask : public SpeechRecognizerWPE::Task {
public:
    NoMatchTask(SpeechRecognizerWPE* wpe) : MockWebSpeechRecognizer::Task(wpe) { }
    virtual void run() OVERRIDE { m_recognizer->client()->didReceiveNoMatch(m_recognizer->handle(), WebSpeechRecognitionResult()); }
};

// Task for delivering an error event.
class ErrorTask : public SpeechRecognizerWPE::Task {
public:
    ErrorTask(SpeechRecognizerWPE* wpe, WebSpeechRecognizerClient::ErrorCode code, const WebString& message)
        : SpeechRecognizerWPE::Task(wpe)
        , m_code(code)
        , m_message(message)
    {
    }

    virtual void run() OVERRIDE { m_recognizer->client()->didReceiveError(m_recognizer->handle(), m_message, m_code); }

private:
    WebSpeechRecognizerClient::ErrorCode m_code;
    WebString m_message;
};

} // namespace

SpeechRecognizerWPE::MockWebSpeechRecognizer()
    : m_wasAborted(false)
    , m_taskQueueRunning(false)
    , m_delegate(0)
{
}

SpeechRecognizerWPE::~MockWebSpeechRecognizer()
{
    clearTaskQueue();
}

void SpeechRecognizerWPE::setDelegate(WebTestDelegate* delegate)
{
    m_delegate = delegate;
}

void SpeechRecognizerWPE::start(const WebSpeechRecognitionHandle& handle, const WebSpeechRecognitionParams& params, WebSpeechRecognizerClient* client)
{
    m_wasAborted = false;
    m_handle = handle;
    m_client = client;

    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didStart));
    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didStartAudio));
    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didStartSound));

    if (!m_wpeTranscripts.empty()) {
        BLINK_ASSERT(m_wpeTranscripts.size() == m_wpeConfidences.size());

        for (size_t i = 0; i < m_wpeTranscripts.size(); ++i)
            m_taskQueue.push_back(new ResultTask(this, m_wpeTranscripts[i], m_wpeConfidences[i]));

        m_wpeTranscripts.clear();
        m_wpeConfidences.clear();
    } else
        m_taskQueue.push_back(new NoMatchTask(this));

    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didEndSound));
    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didEndAudio));
    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didEnd));

    startTaskQueue();
}

void SpeechRecognizerWPE::stop(const WebSpeechRecognitionHandle& handle, WebSpeechRecognizerClient* client)
{
    m_handle = handle;
    m_client = client;

    // FIXME: Implement.
    BLINK_ASSERT_NOT_REACHED();
}

void SpeechRecognizerWPE::abort(const WebSpeechRecognitionHandle& handle, WebSpeechRecognizerClient* client)
{
    m_handle = handle;
    m_client = client;

    clearTaskQueue();
    m_wasAborted = true;
    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didEnd));
    startTaskQueue();
}

void SpeechRecognizerWPE::addMockResult(const WebString& transcript, float confidence)
{
    m_wpeTranscripts.push_back(transcript);
    m_wpeConfidences.push_back(confidence);
}

void SpeechRecognizerWPE::setError(const WebString& error, const WebString& message)
{
    WebSpeechRecognizerClient::ErrorCode code;
    if (error == "OtherError")
        code = WebSpeechRecognizerClient::OtherError;
    else if (error == "NoSpeechError")
        code = WebSpeechRecognizerClient::NoSpeechError;
    else if (error == "AbortedError")
        code = WebSpeechRecognizerClient::AbortedError;
    else if (error == "AudioCaptureError")
        code = WebSpeechRecognizerClient::AudioCaptureError;
    else if (error == "NetworkError")
        code = WebSpeechRecognizerClient::NetworkError;
    else if (error == "NotAllowedError")
        code = WebSpeechRecognizerClient::NotAllowedError;
    else if (error == "ServiceNotAllowedError")
        code = WebSpeechRecognizerClient::ServiceNotAllowedError;
    else if (error == "BadGrammarError")
        code = WebSpeechRecognizerClient::BadGrammarError;
    else if (error == "LanguageNotSupportedError")
        code = WebSpeechRecognizerClient::LanguageNotSupportedError;
    else
        return;

    clearTaskQueue();
    m_taskQueue.push_back(new ErrorTask(this, code, message));
    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didEnd));
    startTaskQueue();
}

void SpeechRecognizerWPE::startTaskQueue()
{
    if (m_taskQueueRunning)
        return;
    m_delegate->postTask(new StepTask(this));
    m_taskQueueRunning = true;
}

void SpeechRecognizerWPE::clearTaskQueue()
{
    while (!m_taskQueue.empty()) {
        delete m_taskQueue.front();
        m_taskQueue.pop_front();
    }
    m_taskQueueRunning = false;
}

void SpeechRecognizerWPE::StepTask::runIfValid()
{
    if (m_object->m_taskQueue.empty()) {
        m_object->m_taskQueueRunning = false;
        return;
    }

    Task* task = m_object->m_taskQueue.front();
    m_object->m_taskQueue.pop_front();
    task->run();
    delete task;

    if (m_object->m_taskQueue.empty()) {
        m_object->m_taskQueueRunning = false;
        return;
    }

    m_object->m_delegate->postTask(new StepTask(m_object));
}

}
