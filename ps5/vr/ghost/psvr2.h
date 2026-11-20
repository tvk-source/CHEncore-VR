/*
 * psvr2 - the PS VR2 headset, its Sense controllers and the DualSense, for the
 * PS5XR OpenXR runtime.
 *
 * Everything here is in the tracker's space: metres, y up, -z forward, the
 * origin where the system's play area put it. Times are the console's process
 * time in microseconds (sceKernelGetProcessTime), the base Hmd2 predicts
 * display times in and VrTracker2 samples poses at.
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "warp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float pos[3];
    float quat[4]; /* x, y, z, w */
} psvr2_pose;

typedef struct {
    float fov[4];          /* tangents: out, in, top, bottom (left eye; mirrored for the right) */
    uint32_t panel_w;      /* per eye */
    uint32_t panel_h;
    float ipd;             /* metres, from the lens separation */
    int worn;
    int status;            /* GetDeviceInformation status: 0 = ready */
    /* Backends that stand for another headset (the Mac Quest 3 stand-in, the WiVRn remote
     * backend) fill these; zero / empty keeps PS VR2's own (PS5XR's render-size probe). */
    char system_name[64];  /* XrSystemProperties.systemName */
    uint32_t vendor_id;    /* XrSystemProperties.vendorId */
    uint32_t render_w;     /* recommendedImageRect per eye */
    uint32_t render_h;
    /* Backends that take the output at a fixed size per eye (the WiVRn remote backend: the
     * stream's) set these; zero = PS5XR chooses (output_scale). The output is then also
     * rectilinear (psvr2_set_warp answers -1). */
    uint32_t output_w;
    uint32_t output_h;
    /* The backend's encoder captures the TV (the remote backend on the PS5): PS5XR also
     * shows each output frame there (xr_tv.c). */
    int tv_output;
    /* A remote headset's foveation (WiVRn's band lists, remote/backend/gv_foveation.h): per eye,
     * x then y, fovea_n[eye][axis] (odd, up to 7) stored pixel counts summing to output_w / _h.
     * PS5XR draws the output with them (warp.h bands); zero = rectilinear. */
    uint16_t fovea[2][2][7];
    uint8_t fovea_n[2][2];
} psvr2_info;

/* Output modes, as PS VR2 games choose them (Horizon's sub_10e87b0). */
enum {
    PSVR2_MODE_90 = 0,      /* 90 Hz, every frame rendered */
    PSVR2_MODE_120 = 1,     /* 120 Hz, every frame rendered */
    PSVR2_MODE_120_HALF = 2 /* 120 Hz output, 60 Hz rendering, Hmd2 reprojects the rest */
};

/* Sense controller / DualSense state in one shape. */
enum {
    PSVR2_BTN_TRIGGER = 1u << 0, /* L2 / R2 pressed */
    PSVR2_BTN_GRIP = 1u << 1,    /* L1 / R1 */
    PSVR2_BTN_A = 1u << 2,       /* cross (right) */
    PSVR2_BTN_B = 1u << 3,       /* circle (right) */
    PSVR2_BTN_X = 1u << 4,       /* square (left) */
    PSVR2_BTN_Y = 1u << 5,       /* triangle (left) */
    PSVR2_BTN_MENU = 1u << 6,    /* options (right) / create (left) */
    PSVR2_BTN_STICK = 1u << 7,   /* L3 / R3 */
};

typedef struct {
    int connected;
    int tracked;           /* pose valid */
    psvr2_pose pose;       /* tracker space */
    float lin_vel[3];
    float ang_vel[3];
    uint32_t buttons;      /* PSVR2_BTN_* held */
    uint32_t touches;      /* PSVR2_BTN_* with a finger resting on them */
    float trigger;         /* 0..1 */
    float grip;            /* 0..1 */
    float stick[2];        /* -1..1, y up */
    int source;            /* 0 none, 1 Sense, 2 DualSense, 3 injected, 4 remote headset's (pose = grip) */
    psvr2_pose aim;        /* source 4: the aim pose, as the headset reports it */
} psvr2_controller;

/* Loads Hmd2 and VrTracker2, opens the headset, starts tracking and the
 * reprojection. Safe to call more than once. 0 on success. */
int psvr2_init(void);
/* psvr2_init, and in deferred mode also the tracker and reprojection. */
int psvr2_init_output(void);
void psvr2_set_defer_output(int on);
int psvr2_selftest(const char *tag, int frames);
void psvr2_set_clean_init_thread(int on);
void psvr2_shutdown(void);
int psvr2_ready(void);
void psvr2_get_info(psvr2_info *info);

/* VR output: eye_w x eye_h per eye, side by side in each buffer (linear RGBA8,
 * pitch = 2 * eye_w texels). buffers are GPU addresses (= CPU addresses on
 * this console). 0 on success. */
int psvr2_output_start(uint32_t eye_w, uint32_t eye_h, void *const *buffers, int count, int mode);
/* Before psvr2_output_start: the output warp per eye (warp.h), handed to Hmd2 as its
 * lookup tables with the buffers. Strength 0 = a rectilinear picture, as before.
 * -1: no tables (the output has to stay rectilinear). */
/* The TV's mode, for backends whose output is captured from the TV (tv_output): a remote
 * headset's stream size follows it. PS VR2 ignores it. */
void psvr2_tv_mode(uint32_t w, uint32_t h);
int psvr2_set_warp(const warp_axis warp[2][2], uint32_t eye_w, uint32_t eye_h);
/* The process's direct memory: size, allocated, largest free block (bytes). */
void psvr2_direct_memory(uint64_t *pool, uint64_t *allocated, uint64_t *largest_free);
void psvr2_output_stop(void);
#if defined(ENCORE_VR_CINEMA)
void *psvr2_cinema_alloc(size_t bytes);
int psvr2_cinema_flip(uint64_t *shown);
#endif
int psvr2_output_mode(void);
int psvr2_output_set_mode(int mode);
float psvr2_refresh_hz(void);
const void *psvr2_mirror_buffer(size_t *size); /* the TV mirror's display buffer, NULL when off */   /* the headset's refresh */
float psvr2_frame_hz(void);     /* frames the app should render a second */

/* Frame pacing: begins frame `frame` with Hmd2 and returns its predicted
 * display time (us). 0 on success. */
int psvr2_begin_frame(uint64_t frame, uint64_t *display_us);
/* The VR port's last vblank: its count and process time (us). 0 on success. */
int psvr2_vblank(uint64_t *count, uint64_t *time_us);
/* Shows buffer `buf` for frame `frame`: rendered with the device pose `pose`
 * (tracker space) for display at `display_us`, each eye with the given
 * tangents {left, right, up, down} (all positive). */
int psvr2_submit(int buf, uint64_t frame, uint64_t display_us, const psvr2_pose *pose,
                 const float tan_left[4], const float tan_right[4]);

/* Head and eye poses at `time_us` (0 = now). 1 when tracked. Any out pointer
 * may be NULL. */
int psvr2_head(uint64_t time_us, psvr2_pose *device, psvr2_pose eyes[2], float lin_vel[3],
               float ang_vel[3]);

/* Controllers: side 0 = left, 1 = right. psvr2_poll reads the pads (call once a
 * frame); psvr2_get_controller locates them at time_us. */
void psvr2_poll(void);
void psvr2_get_controller(int side, uint64_t time_us, psvr2_controller *out);
/* The app ended frame `frame` without an image: show the last one again. */
void psvr2_repeat_frame(uint64_t frame);
void psvr2_haptic(int side, float amplitude, float seconds);
void psvr2_headset_haptic(float amplitude);
/* Sense adaptive trigger: libScePad's trigger effect mode (0 off, 1 feedback,
 * 2 weapon, ...) and its first four bytes. Kept and given again when the
 * controller comes back; off on psvr2_shutdown. */
void psvr2_set_trigger(int side, uint32_t mode, const uint8_t data[4]);
/* Weapon trigger effects on the Sense controllers: bit 0 left hand, bit 1 right. */
void psvr2_set_weapon_triggers(int hands);

/* Test overrides (from the debug server): replaces the real controller for
 * `side` until `until_us`; fields with pose_set/input_set 0 are left real. */
void psvr2_inject(int side, int pose_set, const psvr2_pose *pose, int input_set,
                  const psvr2_controller *input, uint64_t until_us);

int psvr2_worn(void);

/* Eye gaze pose in headset space (origin at the headset, looking along the
 * gaze direction); 1 when valid. The eye tracker starts on the first call. */
int psvr2_gaze(psvr2_pose *out);

/* Hand tracking (PS VR2 cameras through libSceVrHand). 26 joints per hand in
 * OpenXR order (palm, wrist, thumb 4, four fingers 5 each), poses in tracker
 * space. side 0 left, 1 right. 1 when the hand is tracked this call. The
 * cameras need raised credentials and the hand models in place, which only the
 * console glue (title_ps5.c) can do; the Mac stand-in simulates the joints. */
#define PSVR2_HAND_JOINTS 26
typedef struct {
    psvr2_pose joint[PSVR2_HAND_JOINTS];
    float radius[PSVR2_HAND_JOINTS];
} psvr2_hand;
int psvr2_hands_supported(void);     /* the library is present (per firmware) */
void psvr2_hands_enable(int on);     /* start/stop tracking (opens the cameras) */
int psvr2_hand_joints(int side, uint64_t time_us, psvr2_hand *out);
/* 1 once after a menu button was held for a second (the recentre gesture). */
int psvr2_take_recenter(void);
/* Averages (ms) since the last call: begin nap, BeginFrame, display ahead, display step. */
void psvr2_take_pace(float out[4]);
void psvr2_set_mirror(int on);
/* Hosted mode: leave the DualSense to the host (no stand-in hands). */
void psvr2_set_dualsense(int use);
uint64_t psvr2_now_us(void);
int psvr2_user(void);

/* Requests to end the session (debug server, a fatal error). */
int psvr2_exit_requested(void);
void psvr2_request_exit(void);

/* The last frames' numbers for the debug server. */
typedef struct {
    uint64_t frames, errors, late, queue_full, repeated;
    uint64_t paced; /* begins held back to the refresh count */
    float last_gpu_ms, last_frame_ms;
} psvr2_stats;
void psvr2_get_stats(psvr2_stats *out);
/* The compositor's GPU time for a frame, averaged into the stats line. */
void psvr2_note_gpu(uint64_t us);

#ifdef __cplusplus
}
#endif
