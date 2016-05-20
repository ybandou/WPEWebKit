#include "config.h"

#if ENABLE(GAMEPAD)

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <wtf/MainThread.h>
#include <wtf/CurrentTime.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#include "GamepadWPE.h"
#include "Logging.h"

namespace WebCore {

GamepadWPE::GamepadWPE(String hidDevice, unsigned index)
    : PlatformGamepad(index)
    , m_hidDevice(hidDevice)
    , m_valueMonitoringThread(0)
{
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    m_fileDescriptor=open(m_hidDevice.utf8().data(), O_RDONLY | O_NONBLOCK);
    if (m_fileDescriptor < 0)
	return;
   
    /* Read device Id */
    char deviceId[1024];
    if (ioctl(m_fileDescriptor, JSIOCGNAME(sizeof(deviceId)), deviceId) < 0)
	    return;

    m_id = String(deviceId).simplifyWhiteSpace();
   
    /* Read Button/Axis numbers */ 
    uint8_t numberOfAxis;
    uint8_t numberOfButtons;
    if ( (ioctl(m_fileDescriptor, JSIOCGAXES, &numberOfAxis) < 0 ) || 
         (ioctl(m_fileDescriptor, JSIOCGBUTTONS, &numberOfButtons) < 0 ) ) {
        LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
        return;
    }
    m_axisValues.fill(0.0, numberOfAxis);
    m_buttonValues.fill(0.0, numberOfButtons);

    /* Start gamepad value monitioring thread */
    m_EventStatus = deviceConnected;
    if(!(m_valueMonitoringThread = createThread(valueMonitoringThread, this, 
                                               "WebCore: Button and Axis values monitoring thread"))){
        LOG_ERROR( "Error in creating Value Monitoring Thread\n");
    }
}

GamepadWPE::~GamepadWPE()
{
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    if (m_fileDescriptor != -1)
        close(m_fileDescriptor);

    if(m_valueMonitoringThread)
    {
        m_EventStatus = deviceDisconnected;
        waitForThreadCompletion(m_valueMonitoringThread);
        m_valueMonitoringThread = 0;
    }
}

void GamepadWPE::updateForEvent(struct js_event event)
{
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    if (!(event.type & JS_EVENT_AXIS || event.type & JS_EVENT_BUTTON))
        return;

    // Mark the device as connected only if it is not yet connected, 
    //the event is not an initialization and the value is not 0 
    //(indicating a genuine interaction with the device).
    if (!m_connected && !(event.type & JS_EVENT_INIT) && event.value)
        m_connected = true;

    if (event.type & JS_EVENT_AXIS) {
       	m_axisValues[event.number] = normalizeAxisValue(event.value);
    }
    else if (event.type & JS_EVENT_BUTTON) {
       	m_buttonValues[event.number] = normalizeButtonValue(event.value);
    }   
    m_lastTimestamp = event.time;
}

void GamepadWPE::valueMonitoringThread(void* context)
{ 
   LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
   GamepadWPE* GamepadWPEcontext = (GamepadWPE*) context;
   struct js_event event;
   while(GamepadWPEcontext->m_EventStatus == deviceConnected){
       read(GamepadWPEcontext->m_fileDescriptor, &event, sizeof(struct js_event));
       GamepadWPEcontext->updateForEvent(event);
   }
}

float GamepadWPE::normalizeAxisValue(short value)
{
    // Normalize from range [-32767, 32767] into range [-1.0, 1.0]
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    return value / 32767.0f;
}

float GamepadWPE::normalizeButtonValue(short value)
{
    // Normalize from range [0, 1] into range [0.0, 1.0]
    LOG(Gamepad, "%s(%s:%d)\n",__func__,__FILE__, __LINE__);
    return value / 1.0f;
}
} // namespace WebCore

#endif// ENABLE(GAMEPAD)
