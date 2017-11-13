/*
 * Copyright (C) 2016 Igalia S.L.
 * Copyright (C) 2016 Metrological
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

#ifndef Extensions3DCache_h
#define Extensions3DCache_h

#include <mutex>
#include <wtf/NeverDestroyed.h>

namespace WebCore {
class Extensions3DCache {
public:
    static const Extensions3DCache& singleton();

    bool supportsUnpackSubimage() const { return m_supportsUnpackSubimage; }
#if 0
    bool GL_OES_packed_depth_stencil() const { return m_GL_OES_packed_depth_stencil; }
    bool GL_EXT_multisampled_render_to_texture() const { return m_GL_EXT_multisampled_render_to_texture; }
#endif

private:
    friend class LazyNeverDestroyed<Extensions3DCache>;
    Extensions3DCache();

    bool m_supportsUnpackSubimage;
#if 0
    bool m_GL_OES_packed_depth_stencil;
    bool m_GL_EXT_multisampled_render_to_texture;
#endif
};

} // namespace WebCore
#endif // Extensions3DCache_h
