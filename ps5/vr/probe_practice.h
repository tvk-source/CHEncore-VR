/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    float distance[5];
    unsigned notes, hit_flash, hits, misses, wrong, combo, best_combo, score;
} encore_practice_view;
/* Original CHEncore SongChart/InputEvaluator, with a self-contained practice chart. */
void encore_practice_begin(unsigned held);
int encore_practice_tick(uint64_t us,unsigned lanes,encore_practice_view *view);
int encore_practice_audio_start(void);
void encore_practice_audio_stop(void);
uint64_t encore_practice_audio_frames(void);
#ifdef __cplusplus
}
#endif
