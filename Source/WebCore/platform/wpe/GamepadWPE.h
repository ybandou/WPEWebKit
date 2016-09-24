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

#ifndef GAMEPAD_WPE_H
#define GAMEPAD_WPE_H

#if ENABLE(GAMEPAD)

#include <string>
#include <wtf/HashMap.h>
#include <wtf/RetainPtr.h>
#include <wtf/Threading.h>
#include <linux/joystick.h>

#include "PlatformGamepad.h"

using namespace std;

namespace WebCore {

class GamepadWPE : public PlatformGamepad {
public:
    GamepadWPE(String, unsigned);
    ~GamepadWPE();

    String id() { return m_id; }
     
    unsigned long long timestamp() { return m_lastTimestamp; }
    const Vector<double>& axisValues() const { return m_axisValues; }
    const Vector<double>& buttonValues() const { return m_buttonValues; }
    
private:
    enum EventStatus{
	deviceConnected,
	deviceDisconnected,
    };
 
    String m_hidDevice;
    int m_fileDescriptor;
    unsigned long long m_lastTimestamp;
   
    EventStatus m_EventStatus;
    ThreadIdentifier m_valueMonitoringThread;
    static void valueMonitoringThread(void*);
    
    bool m_connected;
    void updateForEvent(struct js_event);
  
    Vector<double> m_buttonValues;
    Vector<double> m_axisValues;
    float normalizeAxisValue(short value);
    float normalizeButtonValue(short value);
};

} // namespace WebCore

#endif // ENABLE(GAMEPAD)

#endif //GAMEPAD_WPE_H
