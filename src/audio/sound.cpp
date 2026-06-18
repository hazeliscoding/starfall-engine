#include "engine/audio/sound.hpp"

#include <SDL3_mixer/SDL_mixer.h>

namespace Engine::Audio {

Sound::Sound(MIX_Audio* raw) noexcept : raw_(raw) {}

Sound::~Sound() {
    if (raw_ != nullptr) {
        MIX_DestroyAudio(raw_);
    }
}

}  // namespace Engine::Audio
