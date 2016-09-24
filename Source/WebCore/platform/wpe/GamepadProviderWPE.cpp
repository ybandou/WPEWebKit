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

#include "config.h"

#if ENABLE(GAMEPAD)
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <vector>
#include <errno.h>
#include <memory.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/MainThread.h>

#include "Logging.h"
#include "GamepadProviderWPE.h"
#include "GamepadProviderClient.h"
#include "PlatformGamepad.h"

using namespace std;
namespace WebCore {

GamepadProviderWPE& GamepadProviderWPE::singleton()
{
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    static NeverDestroyed<GamepadProviderWPE> sharedListener;
    return sharedListener;
}

unsigned GamepadProviderWPE::indexForNewlyConnectedDevice()
{
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    unsigned index = 0;
    while (index < m_gamepadVector.size() && m_gamepadVector[index])
        ++index;

    return index;
}

void GamepadProviderWPE::startMonitoringGamepads(GamepadProviderClient* client)
{
    //start a thread to monitor presence of device entry
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    m_MonitoringEnabled = 1;

    if (!m_GDThread) {
        if(!(m_GDThread=createThread(&processThread, this, "WebCore : processThread"))){
            LOG_ERROR("Error in creating Value Monitoring Thread\n");
        }
    }
    ASSERT(!m_clients.contains(client));
    m_clients.add(client);
}

void GamepadProviderWPE::stopMonitoringGamepads(GamepadProviderClient* client)
{
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    ASSERT(m_clients.contains(client));
    m_clients.remove(client);
    if (m_clients.isEmpty()) {
        m_MonitoringEnabled = 0;
        waitForThreadCompletion(m_GDThread);
    }
}

void GamepadProviderWPE::deviceAdded(String GDDeviceName)
{
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    ASSERT(!m_gamepadMap.get(GDDeviceName));

    unsigned index = indexForNewlyConnectedDevice();
    
    std::unique_ptr<GamepadWPE> gamepad = std::make_unique<GamepadWPE>(GDDeviceName, index);

    if (m_gamepadVector.size() <= index)
        m_gamepadVector.resize(index + 1);
    
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    m_gamepadVector[index] = gamepad.get();
    m_gamepadMap.set(GDDeviceName, WTFMove(gamepad));

    for (auto& client : m_clients){
        client->platformGamepadConnected(*m_gamepadVector[index]);
    }
}

void GamepadProviderWPE::deviceRemoved(String GDDeviceName)
{
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    std::unique_ptr<GamepadWPE> removedGamepad = removeGamepadForDevice(GDDeviceName);
    ASSERT(removedGamepad);

    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    for (auto& client : m_clients){
        client->platformGamepadDisconnected(*removedGamepad);
    }
    LOG(Gamepad, "\nDevice removed");
}

std::unique_ptr<GamepadWPE> GamepadProviderWPE::removeGamepadForDevice(String GDDeviceName)
{
   LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
   std::unique_ptr<GamepadWPE> result = m_gamepadMap.take(GDDeviceName);
   ASSERT(result);
    
    auto i = m_gamepadVector.find(result.get());
    if(i != notFound)
        m_gamepadVector[i] = nullptr;
    LOG(Gamepad, "\nRemoved Gamepad For Device %s", GDDeviceName.utf8().data());

    return result;
}

void GamepadProviderWPE::processThread(void* context)
{
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    vector<string> deviceName;
    int GDDeviceStatus[MAX_DEVICE] = {0};
    memset(GDDeviceStatus,0,MAX_DEVICE);
    GamepadProviderWPE *WPECtx = (GamepadProviderWPE*) context;
    GamepadProviderWPE* listener = static_cast<GamepadProviderWPE*>(context);
    while(WPECtx->m_MonitoringEnabled)
    {
        LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
        deviceName.clear();
        WPECtx->getDeviceList(((char*)GAMEPAD_PATH), deviceName);
        for (unsigned int Itr=0; Itr < deviceName.size(); Itr++)
        {
            String GDDeviceName;
            StringBuilder result;
            String GDDeviceName1;
            result.append(GAMEPAD_PATH);
            result.append("/");
            result.append(deviceName[Itr].c_str());
            GDDeviceName = result.toString();
	    if (access(GDDeviceName.utf8().data(), 0) != -1)
            {
                LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
                if (GDDeviceStatus[Itr] == GD_NOT_CONNECTED)
                {
                    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
                    GDDeviceStatus[Itr] = GD_CONNECTED;
                    callOnMainThread([listener, GDDeviceName]{
                        LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
                        listener->deviceAdded(GDDeviceName);
                    });
                }
            }
            else			
            {
                LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
                if(GDDeviceStatus[Itr] == GD_CONNECTED)
                {
                    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
                    GDDeviceStatus[Itr] = GD_NOT_CONNECTED;
                    callOnMainThread([listener, GDDeviceName]{
                        LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
                        listener->deviceRemoved(GDDeviceName);
                    });
                }
            }
        }
    }
}

int GamepadProviderWPE::getDeviceList(char* gamepadPath, vector<string> &deviceName)
{
    DIR *dp;
    struct dirent *dirp;

    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    if((dp  = opendir(gamepadPath)) == NULL) {
        LOG_ERROR("Error in opening gamepadpath\n");
        return errno;
    }
    while ((dirp = readdir(dp)) != NULL) {
        if( (string(dirp->d_name).at(0)) != '.' && !(string(dirp->d_name).find("js")) )
        {
            LOG(Gamepad, "\ndirp->d_name : %s", dirp->d_name);
            deviceName.push_back(string(dirp->d_name));
        }
    }
    closedir(dp);
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    return 0;
}

}
#endif // ENABLE(GAMEPAD)



