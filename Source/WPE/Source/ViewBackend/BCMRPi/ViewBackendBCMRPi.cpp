/*
 * Copyright (C) 2015 Igalia S.L.
 * Copyright (C) 2015 Metrological
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Config.h"
#include "ViewBackendBCMRPi.h"

#include <unistd.h>
#include <sys/types.h>


#if WPE_BACKEND(BCM_RPI)


#include <libgdl.h>

// Plane size and position
#define ORIGIN_X 0
#define ORIGIN_Y 0
#define WIDTH 1280
#define HEIGHT 720
#define ASPECT ((GLfloat)WIDTH / (GLfloat)HEIGHT)

// Initializes a plane for the graphics to be rendered to
static gdl_ret_t setup_plane(gdl_plane_id_t plane) {
    gdl_pixel_format_t pixelFormat = GDL_PF_ARGB_32;
    gdl_color_space_t colorSpace = GDL_COLOR_SPACE_RGB;
    gdl_rectangle_t srcRect;
    gdl_rectangle_t dstRect;
    gdl_ret_t rc = GDL_SUCCESS;

    dstRect.origin.x = ORIGIN_X;
    dstRect.origin.y = ORIGIN_Y;
    dstRect.width = WIDTH;
    dstRect.height = HEIGHT;

    srcRect.origin.x = 0;
    srcRect.origin.y = 0;
    srcRect.width = WIDTH;
    srcRect.height = HEIGHT;

    rc = gdl_plane_reset(plane);
    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_config_begin(plane);
    }

    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_set_attr(GDL_PLANE_SRC_COLOR_SPACE, &colorSpace);
    }

    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_set_attr(GDL_PLANE_PIXEL_FORMAT, &pixelFormat);
    }

    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_set_attr(GDL_PLANE_DST_RECT, &dstRect);
    }

    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_set_attr(GDL_PLANE_SRC_RECT, &srcRect);
    }

    if (GDL_SUCCESS == rc) {
        rc = gdl_plane_config_end(GDL_FALSE);
    } else {
        gdl_plane_config_end(GDL_TRUE);
    }

    if (GDL_SUCCESS != rc) {
        fprintf(stderr, "GDL configuration failed! GDL error code is 0x%x\n",
                rc);
    }

    return rc;
}



#include "LibinputServer.h"

namespace WPE {

namespace ViewBackend {

ViewBackendBCMRPi::ViewBackendBCMRPi()
    : m_elementHandle(0 /*DISPMANX_NO_HANDLE*/)
    , m_width(0)
    , m_height(0)
{

    printf("[%d]ViewBackendBCMRPi::ViewBackendBCMRPi()\n", getpid());

    #if 0
    bcm_host_init();
    m_displayHandle = vc_dispmanx_display_open(0);
    #endif

    printf("[%d]+gdl-init\n", getpid());

    gdl_plane_id_t plane = GDL_PLANE_ID_UPP_C;
    gdl_init(0);
    setup_plane(plane);

    printf("[%d]+gdl-init\n", getpid());
}

ViewBackendBCMRPi::~ViewBackendBCMRPi()
{
    printf("[%d]ViewBackendBCMRPi::~ViewBackendBCMRPi()\n", getpid());

    LibinputServer::singleton().setClient(nullptr);
}

void ViewBackendBCMRPi::setClient(Client* client)
{
    printf("[%d]ViewBackendBCMRPi::setClient()\n", getpid());

    m_client = client;
}

uint32_t ViewBackendBCMRPi::createBCMElement(int32_t width, int32_t height)
{
    printf("[%d]ViewBackendBCMRPi::createBCMElement() %dx%d\n", getpid(), width, height);

    #if 0
    static VC_DISPMANX_ALPHA_T alpha = {
        static_cast<DISPMANX_FLAGS_ALPHA_T>(DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS),
        255, 0
    };

    if (m_elementHandle != DISPMANX_NO_HANDLE)
        return 0;

    m_width = std::max(width, 0);
    m_height = std::max(height, 0);

    DISPMANX_UPDATE_HANDLE_T updateHandle = vc_dispmanx_update_start(0);

    VC_RECT_T srcRect, destRect;
    vc_dispmanx_rect_set(&srcRect, 0, 0, m_width << 16, m_height << 16);
    vc_dispmanx_rect_set(&destRect, 0, 0, m_width, m_height);

    m_elementHandle = vc_dispmanx_element_add(updateHandle, m_displayHandle, 0,
        &destRect, DISPMANX_NO_HANDLE, &srcRect, DISPMANX_PROTECTION_NONE,
        &alpha, nullptr, DISPMANX_NO_ROTATE);

    vc_dispmanx_update_submit_sync(updateHandle);
    return m_elementHandle;
    #endif

    return 0;
}

void ViewBackendBCMRPi::commitBCMBuffer(uint32_t elementHandle, uint32_t width, uint32_t height)
{
    #if 0
    printf("[%d]ViewBackendBCMRPi::commitBCMBuffer() %dx%d\n", getpid(), width, height);
    #endif

    #if 0
    DISPMANX_UPDATE_HANDLE_T updateHandle = vc_dispmanx_update_start(0);

    m_width = width;
    m_height = height;

    VC_RECT_T srcRect, destRect;
    vc_dispmanx_rect_set(&srcRect, 0, 0, m_width << 16, m_height << 16);
    vc_dispmanx_rect_set(&destRect, 0, 0, m_width, m_height);

    vc_dispmanx_element_change_attributes(updateHandle, m_elementHandle, 1 << 3 | 1 << 2, 0, 0, &destRect, &srcRect, 0, DISPMANX_NO_ROTATE);

    vc_dispmanx_update_submit_sync(updateHandle);

    #endif

    if (m_client)
        m_client->frameComplete();

}

void ViewBackendBCMRPi::setInputClient(Input::Client* client)
{
    printf("[%d]ViewBackendBCMRPi::setInputClient()\n", getpid());

    LibinputServer::singleton().setClient(client);
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(BCM_RPI)
