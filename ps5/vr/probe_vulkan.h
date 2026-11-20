/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include "probe_stereo.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Initialize native RADV before Hmd2. No swapchain or TV output. */
int encore_probe_gpu_init(void);
/* Generate both eye images on Vulkan, copy back and verify before writing
 * the independent, not-yet-registered direct-memory headset buffers. */
int encore_probe_gpu_render(const encore_probe_eye eyes[2], unsigned size,
                            void *const *buffers, int count);
/* Only after headset shutdown. If a GPU fence did not complete, retain until
 * the title is closed; never destroy resources possibly still in use. */
void encore_probe_gpu_stop(void);
/* Replay private offscreen work. No registered memory is written. */
int encore_probe_gpu_replay(void);
/* Render tracked world-space marker projections into private GPU memory. */
int encore_probe_gpu_world(const float uv[2][3][2],const int visible[2][3]);
/* Up to 12 world-space markers per eye, private GPU targets only. */
int encore_probe_gpu_markers(const float uv[2][12][2],const int visible[2][12]);
/* Color-coded chart rendered to private GPU memory. */
int encore_probe_gpu_chart(const float uv[2][12][2],const int visible[2][12],unsigned lanes);
/* Single-use copy of the latest verified chart. Caller must enforce
 * experimental display-slot gating. This is not a release fence. */
int encore_probe_gpu_copy_live(void *target);
#ifdef __cplusplus
}
#endif
