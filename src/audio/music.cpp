#include "engine/audio/music.hpp"

#include <SDL3_mixer/SDL_mixer.h>

namespace Engine::Audio {

Music::Music(MIX_Audio* raw) noexcept : raw_(raw) {}

Music::~Music() {
    if (raw_ != nullptr) {
        MIX_DestroyAudio(raw_);
    }
}

}  // namespace Engine::Audio
