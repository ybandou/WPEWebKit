#include "Config.h"
#include "BufferFactoryBCMRPi.h"

#if WPE_BUFFER_MANAGEMENT(BCM_RPI)

#include "BufferDataBCMRPiBM.h"
#include "WaylandDisplay.h"
#include "wayland-dispmanx-client-protocol.h"
#include <EGL/eglext.h>
#include <bcm_host.h>

namespace WPE {

namespace Graphics {

class EventSource {
public:
    static GSourceFuncs sourceFuncs;

    GSource source;
    GPollFD pfd;
    struct wl_display* display;
};

GSourceFuncs EventSource::sourceFuncs = {
    // prepare
    [](GSource*, gint* timeout) -> gboolean
    {
        *timeout = -1;
        return FALSE;
    },
    // check
    [](GSource* base) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);
        return !!source->pfd.revents;
    },
    // dispatch
    [](GSource* base, GSourceFunc, gpointer) -> gboolean
    {
        auto* source = reinterpret_cast<EventSource*>(base);
        struct wl_display* display = source->display;

        if (source->pfd.revents & G_IO_IN) {
            wl_event_loop_dispatch(wl_display_get_event_loop(display), -1);
            wl_display_flush_clients(display);
        }

        if (source->pfd.revents & (G_IO_ERR | G_IO_HUP))
            return FALSE;

        source->pfd.revents = 0;
        return TRUE;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

static const struct wl_surface_interface g_surfaceInterface = {
    // destroy
    [](struct wl_client*, struct wl_resource*) { },
    // attach
    [](struct wl_client*, struct wl_resource* surfaceResource, struct wl_resource* bufferResource, int32_t, int32_t)
    {
        auto& clientSurface = *static_cast<BufferFactoryBCMRPi::ClientSurface*>(wl_resource_get_user_data(surfaceResource));
        clientSurface.attachedBufferResource = bufferResource;
    },
    // damage
    [](struct wl_client*, struct wl_resource*, int32_t, int32_t, int32_t, int32_t) { },
    // frame
    [](struct wl_client*, struct wl_resource*, uint32_t) { },
    // set_opaque_region
    [](struct wl_client*, struct wl_resource*, struct wl_resource*) { },
    // set_input_region
    [](struct wl_client*, struct wl_resource*, struct wl_resource*) { },
    // commit
    [](struct wl_client*, struct wl_resource*) { },
};

static const struct wl_compositor_interface g_compositorInterface = {
    // create_surface
    [](struct wl_client* client, struct wl_resource* compositorResource, uint32_t id)
    {
        auto& clientSurface = *static_cast<BufferFactoryBCMRPi::ClientSurface*>(wl_resource_get_user_data(compositorResource));
        clientSurface.resource = wl_resource_create(client, &wl_surface_interface, 1, id);
        wl_resource_set_implementation(clientSurface.resource, &g_surfaceInterface, &clientSurface, nullptr);
    },
    // create_region
    [](struct wl_client*, struct wl_resource*, uint32_t) { },
};

BufferFactoryBCMRPi::BufferFactoryBCMRPi()
{
    bcm_host_init();

    m_client.display = wl_display_create();
    if (!m_client.display)
        return;

    if (!wl_display_add_socket(m_client.display, "wpe-bcmrpi"))
        setenv("WAYLAND_DISPLAY", "wpe-bcmrpi", 1);

    m_client.compositor = wl_global_create(m_client.display, &wl_compositor_interface, 1, &m_client.surface,
        [](struct wl_client* client, void* data, uint32_t, uint32_t id)
        {
            struct wl_resource* resource = wl_resource_create(client, &wl_compositor_interface, 1, id);
            wl_resource_set_implementation(resource, &g_compositorInterface, data, nullptr);
        });

    m_client.eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_client.eglDisplay == EGL_NO_DISPLAY)
        return;

    eglInitialize(m_client.eglDisplay, nullptr, nullptr);

    eglBindWaylandDisplayWL(m_client.eglDisplay, m_client.display);

    m_client.source = g_source_new(&EventSource::sourceFuncs, sizeof(EventSource));
    auto* source = reinterpret_cast<EventSource*>(m_client.source);
    source->display = m_client.display;

    source->pfd.fd = wl_event_loop_get_fd(wl_display_get_event_loop(m_client.display));
    source->pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
    source->pfd.revents = 0;
    g_source_add_poll(m_client.source, &source->pfd);

    g_source_set_name(m_client.source, "[WPE] Nested WL display");
    g_source_set_priority(m_client.source, G_PRIORITY_HIGH + 30);
    g_source_set_can_recurse(m_client.source, TRUE);
    g_source_attach(m_client.source, g_main_context_get_thread_default());
}

BufferFactoryBCMRPi::~BufferFactoryBCMRPi()
{
    eglUnbindWaylandDisplayWL(m_client.eglDisplay, m_client.display);

    if (m_client.compositor)
        wl_global_destroy(m_client.compositor);
    if (m_client.display)
        wl_display_destroy(m_client.display);

    if (m_client.source)
        g_source_destroy(m_client.source);
}

uint32_t BufferFactoryBCMRPi::constructRenderingTarget(uint32_t, uint32_t)
{
    return 0;
}

#define ALIGN_UP(x,y)  ((x + (y)-1) & ~((y)-1))

std::pair<bool, std::pair<uint32_t, struct wl_buffer*>> BufferFactoryBCMRPi::createBuffer(int, const uint8_t* data, size_t size)
{
    std::pair<bool, std::pair<uint32_t, struct wl_buffer*>> result = { false, { 0, nullptr } };
    if (!data || size != sizeof(BufferDataBCMRPiBM))
        return result;

    auto& bufferData = *reinterpret_cast<const BufferDataBCMRPiBM*>(data);
    if (bufferData.magic != BufferDataBCMRPiBM::magicValue)
        return result;

    if (!m_client.surface.attachedBufferResource)
        return result;

    uint32_t handle = vc_dispmanx_get_handle_from_wl_buffer(m_client.surface.attachedBufferResource);
    result.first = true;
    auto it = m_buffers.find(handle);
    if (it == m_buffers.end()) {
        struct wl_buffer* buffer = wl_dispmanx_wrap_buffer(ViewBackend::WaylandDisplay::singleton().interfaces().dispmanx,
            handle, bufferData.width, bufferData.height, ALIGN_UP(bufferData.width * 4, 16), ALIGN_UP(bufferData.height, 16), WL_DISPMANX_FORMAT_ABGR8888);
        m_buffers.insert({ handle, buffer });
        result.second = { handle, buffer };
    } else
        result.second = { handle, nullptr };

    return result;
}

void BufferFactoryBCMRPi::frameComplete()
{
    if (m_client.surface.attachedBufferResource)
        wl_buffer_send_release(m_client.surface.attachedBufferResource);
}

void BufferFactoryBCMRPi::destroyBuffer(uint32_t, struct wl_buffer*)
{
}

} // namespace Graphics

} // namespace WPE

#endif // WPE_BUFFER_MANAGEMENT(BCM_RPI)
