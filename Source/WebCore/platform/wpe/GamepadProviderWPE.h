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



