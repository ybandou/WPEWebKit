 /*
 * Copyright (C) 2016 TATA ELXSI
 * Copyright (C) 2016 Metrological
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GAMEPAD_PROVIDER_WPE_H
#define GAMEPAD_PROVIDER_WPE_H

#if ENABLE(GAMEPAD)

#include <string>
#include <iostream>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/Threading.h>
#include <linux/joystick.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/StringHash.h>

#include "GamepadProvider.h"
#include "GamepadWPE.h"

using namespace std;

#define GD_NOT_CONNECTED  0
#define GD_CONNECTED  1
#define MAX_STR 50
#define MAX_DEVICE 10
#define GAMEPAD_PATH "/dev/input"

namespace WebCore {

class GamepadProviderClient;

class GamepadProviderWPE : public GamepadProvider {
    WTF_MAKE_NONCOPYABLE(GamepadProviderWPE);
    friend class NeverDestroyed<GamepadProviderWPE>;

    int m_MonitoringEnabled;
    int GDDeviceStatus[MAX_DEVICE];

public:
    WEBCORE_EXPORT static GamepadProviderWPE& singleton();

    virtual void startMonitoringGamepads(GamepadProviderClient*);
    virtual void stopMonitoringGamepads(GamepadProviderClient*);
    virtual const Vector<PlatformGamepad*>& platformGamepads(){ return m_gamepadVector; }

    GamepadProviderWPE(){};

private:
 
    std::unique_ptr<GamepadWPE> removeGamepadForDevice(String);

    void openAndScheduleManager();
    void closeAndUnscheduleManager();

    ThreadIdentifier  m_GDThread;
    static void processThread(void*);

    void deviceAdded(String);
    void deviceRemoved(String);

    unsigned indexForNewlyConnectedDevice();
    int getDeviceList(char* , vector<string>&);

    HashSet<GamepadProviderClient*> m_clients;
    bool m_shouldDispatchCallbacks;

    Vector<PlatformGamepad*> m_gamepadVector;
    HashMap<String, std::unique_ptr<GamepadWPE>> m_gamepadMap;
};
}
#endif

#endif//GAMEPAD_PROVIDER_WPE_H

