#ifndef WPE_Graphics_Wayland_BufferFactoryBCMRPi_h
#define WPE_Graphics_Wayland_BufferFactoryBCMRPi_h

#if WPE_BUFFER_MANAGEMENT(BCM_RPI)

#include "BufferFactory.h"

#define BUILD_WAYLAND
#include <EGL/egl.h>
#include <glib.h>
#include <unordered_map>
#include <wayland-server.h>

namespace WPE {

namespace Graphics {

class BufferFactoryBCMRPi : public BufferFactory {
public:
    BufferFactoryBCMRPi();
    virtual ~BufferFactoryBCMRPi();

    std::pair<bool, std::pair<uint32_t, uint32_t>> preferredSize() override { return { false, { 0, 0 } }; };
    std::pair<const uint8_t*, size_t> authenticate() override { return { nullptr, 0 }; };
    uint32_t constructRenderingTarget(uint32_t, uint32_t) override;
    std::pair<bool, std::pair<uint32_t, struct wl_buffer*>> createBuffer(int, const uint8_t*, size_t) override;
    void frameComplete() override;
    void destroyBuffer(uint32_t, struct wl_buffer*) override;

    struct ClientSurface {
        struct wl_resource* resource;
        struct wl_resource* attachedBufferResource;
    };

private:
    struct {
        struct wl_display* display;
        struct wl_global* compositor;
        GSource* source;
        EGLDisplay eglDisplay;
        ClientSurface surface;
    } m_client;

    std::unordered_map<uint32_t, struct wl_buffer*> m_buffers;
};

} // namespace Graphics

} // namespace WPE

#endif // WPE_BUFFER_MANAGEMENT(BCMRPi)

#endif // WPE_Graphics_Wayland_BufferFactoryWLBCMRPi_h
