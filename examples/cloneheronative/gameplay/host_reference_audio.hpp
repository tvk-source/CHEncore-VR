// SPDX-License-Identifier: MIT
#pragma once
#include "audio_player.hpp"

namespace rhythm {
// A deliberately silent adapter for the Linux graphics reference build.
// Keep the original cue enum and shared gameplay control flow (including
// scoring side effects), but never claim decoded audio or playback.
// Native builds continue using AudioStemMixer, not this adapter.
class HostReferenceAudio {
public:
    void playUiSound(AudioStemMixer::UiSound) noexcept {}
    void seekSeconds(double) noexcept {}
    void pause() noexcept {}
    void clear() noexcept {}
    [[nodiscard]] bool ready() const noexcept { return false; }
    [[nodiscard]] double durationSeconds() const noexcept { return 0.0; }
};
}
