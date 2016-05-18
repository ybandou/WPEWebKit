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
