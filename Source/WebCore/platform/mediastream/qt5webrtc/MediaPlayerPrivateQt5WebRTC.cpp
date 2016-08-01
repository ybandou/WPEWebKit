#include "config.h"
#include "MediaPlayerPrivateQt5WebRTC.h"
#include "RealtimeMediaSourceCenterQt5WebRTC.h"

#include <wtf/HashSet.h>
#include <wtf/text/WTFString.h>
#include <wtf/NeverDestroyed.h>

#include "MediaPlayer.h"
#include "NotImplemented.h"
#include "TimeRanges.h"
#include "MediaStreamPrivate.h"

namespace WebCore {

void MediaPlayerPrivateQt5WebRTC::getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types)
{
    static NeverDestroyed<HashSet<String, ASCIICaseInsensitiveHash>> cache;
    types = cache;
}

MediaPlayer::SupportsType MediaPlayerPrivateQt5WebRTC::supportsType(const MediaEngineSupportParameters& parameters)
{
    if (parameters.isMediaStream)
        return MediaPlayer::IsSupported;
    return MediaPlayer::IsNotSupported;
}

void MediaPlayerPrivateQt5WebRTC::registerMediaEngine(MediaEngineRegistrar registrar)
{
    registrar([](MediaPlayer* player) { return std::make_unique<MediaPlayerPrivateQt5WebRTC>(player); },
            getSupportedTypes, supportsType, 0, 0, 0,
              [](const String&, const String&) { return false; });
}

MediaPlayerPrivateQt5WebRTC::MediaPlayerPrivateQt5WebRTC(MediaPlayer* player)
    : m_player(player)
{ }

MediaPlayerPrivateQt5WebRTC::~MediaPlayerPrivateQt5WebRTC()
{
    m_player = 0;
}

void MediaPlayerPrivateQt5WebRTC::load(MediaStreamPrivate& stream)
{
    m_stream = &stream;
    m_player->readyStateChanged();
}

FloatSize MediaPlayerPrivateQt5WebRTC::naturalSize() const
{
    if (!m_stream)
        return FloatSize();
    return m_stream->intrinsicSize();
}

void MediaPlayerPrivateQt5WebRTC::setSize(const IntSize& size)
{
    if (size == m_size)
        return;

    m_size = size;

    updateVideoRectangle();
}

void MediaPlayerPrivateQt5WebRTC::setPosition(const IntPoint& position)
{
    if (position == m_position)
        return;

    m_position = position;

    updateVideoRectangle();
}

void MediaPlayerPrivateQt5WebRTC::play()
{
    if (!m_stream || !m_stream->isProducingData())
        return;

    m_paused = false;

    MediaStreamTrackPrivate *videoTrack = m_stream->activeVideoTrack();
    if (videoTrack) {
        RealtimeVideoSourceQt5WebRTC& videoSource = static_cast<RealtimeVideoSourceQt5WebRTC&>(videoTrack->source());
        videoSource.startRenderer();
        updateVideoRectangle();
    }
}

void MediaPlayerPrivateQt5WebRTC::updateVideoRectangle()
{
    MediaStreamTrackPrivate *videoTrack = m_stream ? m_stream->activeVideoTrack() : nullptr;
    if (videoTrack) {
        RealtimeVideoSourceQt5WebRTC& videoSource = static_cast<RealtimeVideoSourceQt5WebRTC&>(videoTrack->source());
        videoSource.updateVideoRectangle(m_position.x(), m_position.y(), m_size.width(), m_size.height());
    }
}

PlatformLayer* MediaPlayerPrivateQt5WebRTC::platformLayer() const
{
    if (m_stream)
        return m_stream->platformLayer();
    return nullptr;
}

}
