#pragma once

#include <memory>

struct MIX_Audio;

namespace Engine::Audio {

// Opaque streaming-music wrapper. Holds a MIX_Audio* loaded with
// predecode=false (decoded on the fly during playback). Destructor
// releases the underlying audio.
class Music {
public:
    explicit Music(MIX_Audio* raw) noexcept;
    ~Music();

    Music(const Music&)            = delete;
    Music& operator=(const Music&) = delete;
    Music(Music&&)                 = delete;
    Music& operator=(Music&&)      = delete;

    MIX_Audio* RawHandle() const noexcept { return raw_; }

private:
    MIX_Audio* raw_ = nullptr;
};

using MusicHandle = std::shared_ptr<Music>;

}  // namespace Engine::Audio
