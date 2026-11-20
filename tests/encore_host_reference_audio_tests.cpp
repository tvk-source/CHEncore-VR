// SPDX-License-Identifier: MIT
#include "../examples/cloneheronative/gameplay/host_reference_audio.hpp"
#include <cassert>
#include <cstdio>

int main(){
    rhythm::HostReferenceAudio audio;
    audio.playUiSound(rhythm::AudioStemMixer::UiSound::Confirm);
    audio.playUiSound(rhythm::AudioStemMixer::UiSound::StarDeployed);
    audio.seekSeconds(12.5);
    audio.pause();
    audio.clear();
    assert(!audio.ready());
    assert(audio.durationSeconds()==0.0);
    std::puts("PASS silent host reference audio adapter (no native mixer linkage)");
}
