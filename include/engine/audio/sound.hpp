#pragma once

#include <memory>

struct MIX_Audio;

namespace Engine::Audio {

// Opaque SFX wrapper. Holds a MIX_Audio* loaded with predecode=true
// (fully decoded into memory for fast one-shot playback). Destructor
// releases the underlying audio.
class Sound {
public:
    explicit Sound(MIX_Audio* raw) noexcept;
    ~Sound();

    Sound(const Sound&)            = delete;
    Sound& operator=(const Sound&) = delete;
    Sound(Sound&&)                 = delete;
    Sound& operator=(Sound&&)      = delete;

    MIX_Audio* RawHandle() const noexcept { return raw_; }

private:
    MIX_Audio* raw_ = nullptr;
};

using SoundHandle = std::shared_ptr<Sound>;

}  // namespace Engine::Audio
