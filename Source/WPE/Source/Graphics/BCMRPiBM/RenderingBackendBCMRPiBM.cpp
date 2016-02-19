#include "Config.h"
#include "RenderingBackendBCMRPiBM.h"

#if WPE_BUFFER_MANAGEMENT(BCM_RPI)

#include <EGL/egl.h>
#include <cstring>

namespace WPE {

namespace Graphics {

RenderingBackendBCMRPiBM::RenderingBackendBCMRPiBM()
{
    m_display = wl_display_connect(nullptr);
    if (!m_display)
        return;

    static const struct wl_registry_listener registryListener = {
        // global
        [](void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t)
        {
            auto& backend = *static_cast<RenderingBackendBCMRPiBM*>(data);
            if (!std::strcmp(interface, "wl_compositor"))
                backend.m_compositor = static_cast<struct wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
        },
        // global_remove
        [](void*, struct wl_registry*, uint32_t) { },
    };

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &registryListener, this);
    wl_display_roundtrip(m_display);
}

RenderingBackendBCMRPiBM::~RenderingBackendBCMRPiBM()
{
    if (m_compositor)
        wl_compositor_destroy(m_compositor);
    if (m_registry)
        wl_registry_destroy(m_registry);
    if (m_display)
        wl_display_disconnect(m_display);
}

EGLNativeDisplayType RenderingBackendBCMRPiBM::nativeDisplay()
{
    return m_display;
}

std::unique_ptr<RenderingBackend::Surface> RenderingBackendBCMRPiBM::createSurface(uint32_t width, uint32_t height, uint32_t targetHandle, RenderingBackend::Surface::Client& client)
{
    return std::unique_ptr<RenderingBackendBCMRPiBM::Surface>(new RenderingBackendBCMRPiBM::Surface(*this, width, height, targetHandle, client));
}

std::unique_ptr<RenderingBackend::OffscreenSurface> RenderingBackendBCMRPiBM::createOffscreenSurface()
{
    return std::unique_ptr<RenderingBackendBCMRPiBM::OffscreenSurface>(new RenderingBackendBCMRPiBM::OffscreenSurface(*this));
}

RenderingBackendBCMRPiBM::Surface::Surface(const RenderingBackendBCMRPiBM& backend, uint32_t width, uint32_t height, uint32_t, Client&)
{
    m_surface = wl_compositor_create_surface(backend.m_compositor);
    if (!m_surface)
        return;

    m_window = wl_egl_window_create(m_surface, width, height);

    m_bufferData = { width, height, BufferDataBCMRPiBM::magicValue };
}

RenderingBackendBCMRPiBM::Surface::~Surface()
{
    if (m_window)
        wl_egl_window_destroy(m_window);
    if (m_surface)
        wl_surface_destroy(m_surface);
}

EGLNativeWindowType RenderingBackendBCMRPiBM::Surface::nativeWindow()
{
    return m_window;
}

void RenderingBackendBCMRPiBM::Surface::resize(uint32_t width, uint32_t height)
{
    if (m_window)
        wl_egl_window_resize(m_window, width, height, 0, 0);

    m_bufferData.width = width;
    m_bufferData.height = height;
}

RenderingBackend::BufferExport RenderingBackendBCMRPiBM::Surface::lockFrontBuffer()
{
    return std::make_tuple(-1, reinterpret_cast<uint8_t*>(&m_bufferData), sizeof(BufferDataBCMRPiBM));
}

void RenderingBackendBCMRPiBM::Surface::releaseBuffer(uint32_t)
{
}

RenderingBackendBCMRPiBM::OffscreenSurface::OffscreenSurface(const RenderingBackendBCMRPiBM&)
{
}

RenderingBackendBCMRPiBM::OffscreenSurface::~OffscreenSurface() = default;

EGLNativeWindowType RenderingBackendBCMRPiBM::OffscreenSurface::nativeWindow()
{
    return nullptr;
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BUFFER_MANAGEMENT(BCM_RPI)
