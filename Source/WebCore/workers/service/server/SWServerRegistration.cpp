/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SWServerRegistration.h"

#if ENABLE(SERVICE_WORKER)

#include "ExceptionData.h"
#include "SWServer.h"
#include "SWServerWorker.h"
#include "SecurityOrigin.h"
#include "ServiceWorkerRegistrationData.h"
#include "WorkerType.h"

namespace WebCore {

SWServerRegistration::SWServerRegistration(SWServer& server, const ServiceWorkerRegistrationKey& key)
    : m_jobTimer(*this, &SWServerRegistration::startNextJob)
    , m_server(server)
    , m_registrationKey(key)
{
}

SWServerRegistration::~SWServerRegistration()
{
    ASSERT(m_jobQueue.isEmpty());
}

void SWServerRegistration::enqueueJob(const ServiceWorkerJobData& jobData)
{
    // FIXME: Per the spec, check if this job is equivalent to the last job on the queue.
    // If it is, stack it along with that job.

    m_jobQueue.append(jobData);

    if (m_currentJob)
        return;

    if (!m_jobTimer.isActive())
        m_jobTimer.startOneShot(0_s);
}

void SWServerRegistration::startNextJob()
{
    ASSERT(isMainThread());
    ASSERT(!m_currentJob);
    ASSERT(!m_jobQueue.isEmpty());

    m_currentJob = std::make_unique<ServiceWorkerJobData>(m_jobQueue.takeFirst().isolatedCopy());

    switch (m_currentJob->type) {
    case ServiceWorkerJobType::Register:
        m_server.postTask(createCrossThreadTask(*this, &SWServerRegistration::runRegisterJob, *m_currentJob));
        return;
    }

    RELEASE_ASSERT_NOT_REACHED();
}

bool SWServerRegistration::isEmpty()
{
    ASSERT(!isMainThread());

    // Having or not-having an m_updateViaCache flag is currently
    // the signal as to whether or not this is an empty (i.e. "new") registration.
    // There will be a more explicit signal in the near future.
    return !m_updateViaCache;
}

SWServerWorker* SWServerRegistration::getNewestWorker()
{
    ASSERT(!isMainThread());
    if (m_installingWorker)
        return m_installingWorker.get();
    if (m_waitingWorker)
        return m_waitingWorker.get();

    return m_activeWorker.get();
}

void SWServerRegistration::runRegisterJob(const ServiceWorkerJobData& job)
{
    ASSERT(!isMainThread());
    ASSERT(job.type == ServiceWorkerJobType::Register);

    if (!shouldTreatAsPotentiallyTrustworthy(job.scriptURL))
        return rejectWithExceptionOnMainThread(ExceptionData { SecurityError, ASCIILiteral("Script URL is not potentially trustworthy") });

    // If the origin of job’s script url is not job’s referrer's origin, then:
    if (!protocolHostAndPortAreEqual(job.scriptURL, job.clientCreationURL))
        return rejectWithExceptionOnMainThread(ExceptionData { SecurityError, ASCIILiteral("Script origin does not match the registering client's origin") });

    // If the origin of job’s scope url is not job’s referrer's origin, then:
    if (!protocolHostAndPortAreEqual(job.scopeURL, job.clientCreationURL))
        return rejectWithExceptionOnMainThread(ExceptionData { SecurityError, ASCIILiteral("Scope origin does not match the registering client's origin") });

    // If registration is not null (in our parlance "empty"), then:
    if (!isEmpty()) {
        ASSERT(m_updateViaCache);

        m_uninstalling = false;
        auto* newestWorker = getNewestWorker();
        if (newestWorker && equalIgnoringFragmentIdentifier(job.scriptURL, newestWorker->scriptURL()) && job.registrationOptions.updateViaCache == *m_updateViaCache) {
            resolveWithRegistrationOnMainThread();
            return;
        }
    } else {
        m_scopeURL = job.scopeURL.isolatedCopy();
        m_scopeURL.removeFragmentIdentifier();
        m_updateViaCache = job.registrationOptions.updateViaCache;
    }

    runUpdateJob(job);
}

void SWServerRegistration::runUpdateJob(const ServiceWorkerJobData& job)
{
    // If registration is null (in our parlance "empty") or registration’s uninstalling flag is set, then:
    if (isEmpty())
        return rejectWithExceptionOnMainThread(ExceptionData { TypeError, ASCIILiteral("Cannot update a null/nonexistent service worker registration") });
    if (m_uninstalling)
        return rejectWithExceptionOnMainThread(ExceptionData { TypeError, ASCIILiteral("Cannot update a service worker registration that is uninstalling") });

    // If job’s job type is update, and newestWorker’s script url does not equal job’s script url with the exclude fragments flag set, then:
    auto* newestWorker = getNewestWorker();
    if (newestWorker && !equalIgnoringFragmentIdentifier(job.scriptURL, newestWorker->scriptURL()))
        return rejectWithExceptionOnMainThread(ExceptionData { TypeError, ASCIILiteral("Cannot update a service worker with a requested script URL whose newest worker has a different script URL") });

    // FIXME: At this point we are ready to actually fetch the script for the worker in the registering context.
    // For now we're still hard coding the same rejection we have so far.
    rejectWithExceptionOnMainThread(ExceptionData { UnknownError, ASCIILiteral("serviceWorker job scheduling is not yet implemented") });
}

void SWServerRegistration::rejectWithExceptionOnMainThread(const ExceptionData& exception)
{
    ASSERT(!isMainThread());
    m_server.postTaskReply(createCrossThreadTask(*this, &SWServerRegistration::rejectCurrentJob, exception));
}

void SWServerRegistration::resolveWithRegistrationOnMainThread()
{
    ASSERT(!isMainThread());
    m_server.postTaskReply(createCrossThreadTask(*this, &SWServerRegistration::resolveCurrentJob, data()));
}

void SWServerRegistration::rejectCurrentJob(const ExceptionData& exceptionData)
{
    ASSERT(isMainThread());
    ASSERT(m_currentJob);

    m_server.rejectJob(*m_currentJob, exceptionData);

    finishCurrentJob();
}

void SWServerRegistration::resolveCurrentJob(const ServiceWorkerRegistrationData& data)
{
    ASSERT(isMainThread());
    ASSERT(m_currentJob);

    m_server.resolveJob(*m_currentJob, data);

    finishCurrentJob();
}

void SWServerRegistration::finishCurrentJob()
{
    ASSERT(m_currentJob);
    ASSERT(!m_jobTimer.isActive());

    m_currentJob = nullptr;
    if (m_jobQueue.isEmpty())
        return;

    startNextJob();
}

ServiceWorkerRegistrationData SWServerRegistration::data() const
{
    return { m_registrationKey, identifier() };
}


} // namespace WebCore

#endif // ENABLE(SERVICE_WORKER)
