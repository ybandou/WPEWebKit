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

#pragma once

#if ENABLE(SERVICE_WORKER)

#include "SWServerRegistration.h"
#include "ServiceWorkerJob.h"
#include "ServiceWorkerRegistrationKey.h"
#include <wtf/CrossThreadQueue.h>
#include <wtf/CrossThreadTask.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/Identified.h>
#include <wtf/RunLoop.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Threading.h>

namespace WebCore {

class SWServerRegistration;
struct ExceptionData;
struct ServiceWorkerRegistrationData;

class SWServer {
public:
    class Connection : public Identified<Connection> {
    friend class SWServer;
    public:
        WEBCORE_EXPORT virtual ~Connection();

    protected:
        WEBCORE_EXPORT Connection(SWServer&, uint64_t identifier);
        SWServer& server() { return m_server; }

        WEBCORE_EXPORT void scheduleJobInServer(const ServiceWorkerJobData&);

    private:
        virtual void rejectJobInClient(uint64_t jobIdentifier, const ExceptionData&) = 0;
        virtual void resolveJobInClient(uint64_t jobIdentifier, const ServiceWorkerRegistrationData&) = 0;

        SWServer& m_server;
    };

    WEBCORE_EXPORT SWServer();
    WEBCORE_EXPORT ~SWServer();

    void scheduleJob(const ServiceWorkerJobData&);
    void rejectJob(const ServiceWorkerJobData&, const ExceptionData&);
    void resolveJob(const ServiceWorkerJobData&, const ServiceWorkerRegistrationData&);
    void postTask(CrossThreadTask&&);
    void postTaskReply(CrossThreadTask&&);

private:
    void registerConnection(Connection&);
    void unregisterConnection(Connection&);

    void taskThreadEntryPoint();
    void handleTaskRepliesOnMainThread();

    HashMap<uint64_t, Connection*> m_connections;
    HashMap<ServiceWorkerRegistrationKey, std::unique_ptr<SWServerRegistration>> m_registrations;

    RefPtr<Thread> m_taskThread;
    Lock m_taskThreadLock;

    CrossThreadQueue<CrossThreadTask> m_taskQueue;
    CrossThreadQueue<CrossThreadTask> m_taskReplyQueue;

    Lock m_mainThreadReplyLock;
    bool m_mainThreadReplyScheduled { false };
};

} // namespace WebCore

#endif // ENABLE(SERVICE_WORKER)
