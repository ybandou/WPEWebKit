/*
 * Copyright (C) 2015, 2016 Igalia S.L.
 * Copyright (C) 2015, 2016 Metrological
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

#include <wpe/renderer-host.h>

#include <westeros-compositor.h>
#include <cstdio>
#include <cstdlib>

namespace Westeros {

static void compositorDestroyedCallback(WstCompositor*, void*)
{
    exit(1);
}

struct RendererHost {
    RendererHost();
    ~RendererHost();

    WstCompositor* compositor;
};

RendererHost::RendererHost()
{
    compositor = WstCompositorCreate();
    if (!compositor)
        return;

    const char* nestedTargetDisplay = std::getenv("WAYLAND_DISPLAY");
#if 0
    if (nestedTargetDisplay) {
        fprintf(stderr, "ViewBackendWesteros: running as the nested compositor\n");
        WstCompositorSetIsNested(compositor, true);
        WstCompositorSetIsRepeater(compositor, true);
        WstCompositorSetNestedDisplayName(compositor, nestedTargetDisplay);
        WstCompositorSetTerminatedCallback(compositor, &compositorDestroyedCallback, this);
        //Register for all the necessary callback before starting the compositor
        // input_handler->initializeNestedInputHandler(compositor);
        // output_handler->initializeNestedOutputHandler(compositor);
        const char * nestedDisplayName = WstCompositorGetDisplayName(compositor);
        setenv("WAYLAND_DISPLAY", nestedDisplayName, 1);
    }
#endif
    {
        WstCompositorSetRendererModule(compositor, "libwesteros_render_dispmanx.so");
        WstCompositorSetTerminatedCallback(compositor, &compositorDestroyedCallback, this);
        const char * nestedDisplayName = WstCompositorGetDisplayName(compositor);
        setenv("WAYLAND_DISPLAY", nestedDisplayName, 1);
    }

    if (!WstCompositorStart(compositor))
    {
        fprintf(stderr, "ViewBackendWesteros: failed to start the compositor: %s\n",
            WstCompositorGetLastErrorDetail(compositor));
        WstCompositorDestroy(compositor);
        compositor = nullptr;
    }
}

RendererHost::~RendererHost()
{
    if (compositor) {
        WstCompositorStop(compositor);
        WstCompositorDestroy(compositor);
        compositor = nullptr;
    }
}

} // namespace Westeros

extern "C" {

struct wpe_renderer_host_interface westeros_renderer_host_interface = {
    // create
    []() -> void*
    {
        return new Westeros::RendererHost;
    },
    // destroy
    [](void* data)
    {
        auto* host = static_cast<Westeros::RendererHost*>(data);
        delete host;
    },
    // create_client
    [](void* data) -> int
    {
        return -1;
    },
};

}
