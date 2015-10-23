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

#include "config.h"
#include "BCMRPiSurface.h"

#if PLATFORM(BCM_RPI)

#include "GLContextEGL.h"
#include "IntSize.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

namespace WebCore {

BCMRPiSurface::BCMRPiSurface(const IntSize& size, uint32_t elementHandle)
{
    printf("[%d]BCMRPiSurface::BCMRPiSurface() %dx%d\n", getpid(), size.width(), size.height());

    m_nativeWindow.element = elementHandle;
    m_nativeWindow.width = size.width();
    m_nativeWindow.height = size.height();
}


std::unique_ptr<GLContext> BCMRPiSurface::createGLContext()
{
    printf("[%d]BCMRPiSurface::createGLContext()\n", getpid());

    //return GLContextEGL::createWindowContext(&m_nativeWindow, GLContext::sharingContext());
    return GLContextEGL::createWindowContext((EGLNativeWindowType)0x07 /* GDL_PLANE_ID_UPP_C */, GLContext::sharingContext());
}

void BCMRPiSurface::resize(const IntSize& size)
{
    printf("[%d]BCMRPiSurface::resize() %dx%d\n", getpid(), size.width(), size.height());

    m_nativeWindow.width = size.width();
    m_nativeWindow.height = size.height();
}

BCMRPiSurface::BCMBufferExport BCMRPiSurface::lockFrontBuffer()
{
    #if 0
    printf("[%d]BCMRPiSurface::lockFrontBuffer()\n", getpid());
    #endif

    return BCMBufferExport{ m_nativeWindow.element, m_nativeWindow.width, m_nativeWindow.height };
}

void BCMRPiSurface::releaseBuffer(uint32_t)
{
    printf("[%d]BCMRPiSurface::releaseBuffer()\n", getpid());
}

} // namespace WebCore

#endif // PLATFORM(BCM_RPI)
