/*
 * PS5 Vulkan Template - the console: klog, the splash, the pad, sound, time, the exit.
 *
 * Copyright (C) 2026 Mihawk
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 *
 * Every console call here is one PS5_vkQuake, PS5_RetroArch or PS5_Vulkan's
 * smoke test runs with; no SDK header declares them.
 */
#include "platform.h"
#if defined(ENCORE_VR_CINEMA)
#include "../vr/cinema_pad_bridge.h"
#endif

#include <ps5platform/klog.h>

#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int sceSystemServiceHideSplashScreen(void);
void encore_apply_application_update(void);
int sceSystemServiceLoadExec(const char *path, const char *const *argv);
int sceUserServiceInitialize(const void *params);
int sceUserServiceGetInitialUser(int32_t *user_id);
/* The signed-in users, at most four; unused entries are -1. */
struct user_list {
   int32_t user_id[4];
};
int sceUserServiceGetLoginUserIdList(struct user_list *list);
int scePadInit(void);
int scePadOpen(int32_t user_id, int32_t port_type, int32_t index, const void *params);
int scePadGetHandle(int32_t user_id, int32_t port_type, int32_t index);
int scePadOpenExt(int32_t user_id, int32_t port_type, int32_t index, const void *params);
int scePadGetControllerInformation(int32_t handle, void *info);
int scePadGetExtControllerInformation(int32_t handle, void *info);
int scePadDeviceClassParseData(int32_t handle, const void *sample, void *data);
int scePadReadState(int32_t handle, void *sample);
int scePadClose(int32_t handle);
int scePadSetVibrationMode(int32_t handle, int32_t mode);
int scePadSetVibration(int32_t handle, const void *vibration);
int scePadSetLightBar(int32_t handle, const void *color);
int sceAudioOutInit(void);
int sceAudioOutOpen(int32_t user, int32_t type, int32_t index, uint32_t grain, uint32_t rate,
                    uint32_t format);
int sceAudioOutOutput(int32_t port, const void *samples);
int sceAudioOutClose(int32_t port);
int sceKernelUsleep(uint32_t microseconds);

void
platform_init(const char *title_name)
{
   static char prefix[64];
   snprintf(prefix, sizeof(prefix), "[%s] ", title_name);
   const int captured = ps5_klog_capture_stderr(prefix);
   say("starts (standard error to klog: %s)", captured == 0 ? "yes" : "no");
   /* The shell's splash covers the title's frames until it is dismissed. */
   say("splash dismissed: %d", sceSystemServiceHideSplashScreen());
}

void
say(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   vfprintf(stderr, format, args);
   va_end(args);
   fputc('\n', stderr);
   fflush(stderr);
}

double
now_seconds(void)
{
   struct timespec t;
   clock_gettime(CLOCK_MONOTONIC, &t);
   return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

void
platform_sleep_us(uint32_t microseconds)
{
   if (microseconds != 0)
      (void)sceKernelUsleep(microseconds);
}

/* The title ends by asking the shell to close it. exit() and returning from
 * _start both kill it, and the console then reports a crash over a run that
 * worked. The CRT calls this after main returns; the shell closes the title
 * asynchronously, so it must wait and never return. */
void
catchReturnFromMain(int status)
{
   say("exit status %d: asking the shell to close the title", status);
   fflush(NULL);
   /* All game/audio/storage workers and Vulkan resources are gone here. */
   encore_apply_application_update();
   (void)sceSystemServiceLoadExec("exit", NULL);
   for (;;)
      sceKernelUsleep(100000);
}

/* ----------------------------------------------------------------- the pad */

/* One sample as the console writes it: 120 bytes, the layout PS5_vkQuake and
 * ProsperoLight read. */
struct pad_sample {
   uint32_t buttons;
   uint8_t left_x, left_y, right_x, right_y, l2, r2;
   uint8_t reserved[66];
   int32_t connected;
   uint64_t timestamp_us;
   uint8_t extension[16];
   uint8_t connected_count;
   uint8_t remaining[15];
};
_Static_assert(sizeof(struct pad_sample) == 120, "the console's pad samples are 120 bytes");
_Static_assert(offsetof(struct pad_sample, connected) == 0x4c, "connection state sits at 0x4c");
_Static_assert(offsetof(struct pad_sample, timestamp_us) == 0x50, "the timestamp sits at 0x50");

/* The PS4-compatible instrument ABI exposed by libScePad on PS5. These are
 * declarations of the binary interface, not HID packet layouts. References:
 * OpenOrbis include/orbis/_types/pad.h and AnyPS5 core/libs/SceTypes.hpp. */
struct instrument_open_params {
   uint16_t vendor, product, product_extra;
   uint8_t reserved[10];
};
struct instrument_info {
   float density;
   uint16_t touch_x, touch_y;
   uint8_t dead_left, dead_right, connection_type, connected_count, connected;
   uint8_t padding[3];
   int32_t device_class;
   uint8_t reserved[8];
};
struct instrument_data {
   int32_t device_class;
   uint8_t valid, padding[3];
   union {
      struct { uint8_t tone, whammy, tilt, frets, solo_frets, reserved[11]; } guitar;
      uint8_t bytes[16];
   } data;
};
_Static_assert(sizeof(struct instrument_open_params) == 16, "instrument open ABI");
_Static_assert(sizeof(struct instrument_info) == 28, "instrument information ABI");
_Static_assert(offsetof(struct instrument_info, device_class) == 16, "instrument class ABI");
_Static_assert(sizeof(struct instrument_data) == 24, "instrument data ABI");
_Static_assert(offsetof(struct instrument_data, data.guitar.frets) == 11, "guitar fret ABI");
#define PAD_PORT_STANDARD 0
#define PAD_PORT_INSTRUMENT 2
#define PAD_CLASS_GUITAR 1

/* The two-motor rumble, as PS5_vkQuake and ProsperoLight drive it (the other
 * mode is the DualSense's haptics). */
#define PAD_VIBRATION_COMPATIBLE 2

/* A player: a signed-in user's controller, PS5_ProsperoEden's way. The console
 * pairs each controller with a user, and scePadOpen opens a user's. Player 0 is
 * the user who started the title and stays theirs; about once a second
 * pad_poll reads the signed-in users again, a new user's controller takes the
 * first free player, and a player is let go when their user signs out. */
struct pad_player_state {
   int32_t user;
   int32_t handle;
   int32_t port;
   bool connected;
   bool class_error;
   bool input_intercepted;
   bool instrument;
   bool owned;
   bool waiting_first_sample;
   double first_sample_deadline;
   struct pad_sample last;
   struct pad_reading taken[64];
   int taken_count;
};

static struct pad_player_state pad_states[PAD_PLAYERS];
static bool pad_service;
static double pad_next_scan;
#if defined(ENCORE_VR_CINEMA)
static struct pad_player_state *pad_state(int player);
static int cinema_virtual;
static struct pad_reading cinema_virtual_sample;
static int cinema_last_logged=-1;
static void cinema_bridge_input(void){
    psvr2_cinema_input in={0};
    const int available=psvr2_cinema_input_snapshot(&in);
    cinema_virtual=0;
    if(available!=cinema_last_logged){
        if(available)say("VR_CINEMA Sense input bridge active (short right Options=menu, short left Create=touchpad, hold Options or both grips=recentre)");
        cinema_last_logged=available;
    }
    if(!available)return;
    struct pad_player_state* s=pad_state(0);
    if(s&&s->connected&&s->instrument)return; // never corrupt original physical instrument
    struct pad_reading r={0};
    r.left_x=r.left_y=r.right_x=r.right_y=128;
    r.connected=true;
    if(s&&s->connected){
        r.buttons=s->last.buttons;
        r.left_x=s->last.left_x;r.left_y=s->last.left_y;
        r.right_x=s->last.right_x;r.right_y=s->last.right_y;
        r.l2=s->last.l2;r.r2=s->last.r2;
        r.timestamp_us=s->last.timestamp_us;
    }
    if(!encore_cinema_merge(&in,&r))return;
    if(s&&s->connected){
        s->last.buttons=r.buttons;
        s->last.left_x=r.left_x;s->last.left_y=r.left_y;
        s->last.right_x=r.right_x;s->last.right_y=r.right_y;
        s->last.l2=r.l2;s->last.r2=r.r2;
        s->taken[0]=r;s->taken_count=1;
    }else{
        cinema_virtual_sample=r;
        cinema_virtual=1;
    }
}
#endif
/* Bounded event queue: the game's existing asynchronous tester log drains this
 * after initialization. No filesystem access in the pad/input polling path. */
static char pad_events[16][256];
static unsigned pad_event_head, pad_event_count;
static void pad_event(const char *format, ...)
{
   char text[256]; va_list args; va_start(args, format);
   vsnprintf(text, sizeof(text), format, args); va_end(args);
   say("%s", text);
   if (pad_event_count == 16) { pad_event_head = (pad_event_head + 1) % 16; --pad_event_count; }
   snprintf(pad_events[(pad_event_head + pad_event_count++) % 16], 256, "%s", text);
}
bool pad_diagnostic(char *text, uint32_t capacity)
{
   if (!text || !capacity || !pad_event_count) return false;
   snprintf(text, capacity, "%s", pad_events[pad_event_head]);
   pad_event_head = (pad_event_head + 1) % 16; --pad_event_count;
   return true;
}

/* A user whose controller would not open, said once rather than every scan. */
static int32_t pad_unopened = -1;
#define PAD_SCAN_SECONDS 1.0

/* A player's state while they have a controller, else NULL (and always NULL
 * before pad_open has set the players up). */
static struct pad_player_state *
pad_state(int player)
{
   if (!pad_service || player < 0 || player >= PAD_PLAYERS || pad_states[player].handle < 0)
      return NULL;
   return &pad_states[player];
}

static void
pad_rest(struct pad_player_state *state)
{
   memset(state, 0, sizeof(*state));
   state->user = state->handle = -1;
   state->last.left_x = state->last.left_y = state->last.right_x = state->last.right_y = 128;
}

/* Both information queries use the documented pad-information ABI. Preserve
 * the ordinary result unless the extended query supplies the missing class. */
static int
pad_controller_info(int handle, struct instrument_info *info)
{
   const int basic = scePadGetControllerInformation(handle, info);
   if (basic >= 0 && info->device_class == PAD_CLASS_GUITAR) return basic;
   struct instrument_info extended = {0};
   const int result = scePadGetExtControllerInformation(handle, &extended);
   if (result >= 0 && (basic < 0 || extended.device_class == PAD_CLASS_GUITAR)) {
      *info = extended;
      if (extended.device_class == PAD_CLASS_GUITAR) {
         static bool reported;
         if (!reported) { pad_event("pad: RB4 class information available through extended API"); reported = true; }
      }
      return result;
   }
   return basic;
}

/* Device class belongs to the controller, not its port number. Some paired
 * instruments are exposed on the standard port. Refresh this on discovery,
 * never in the ordinary per-frame input path. */
static void
pad_refresh_class(int player)
{
   struct pad_player_state *state = &pad_states[player];
   if (state->handle < 0) return;
   struct instrument_info info = {0};
   if (pad_controller_info(state->handle, &info) < 0) return;
   const bool guitar = info.device_class == PAD_CLASS_GUITAR;
   if (guitar != state->instrument) {
      pad_event("pad: native source=%d port=%d class=%d instrument=%d", player,
          (int)state->port, (int)info.device_class, guitar);
      state->instrument = guitar;
      state->class_error = false;
   }
}

static bool
pad_player_open(int player, int32_t user, int attempts)
{
   struct pad_player_state *state = &pad_states[player];
   pad_rest(state);
   /* A title can start before the pad service has published the device. */
   int32_t handle = -1;
   for (int attempt = 0; attempt < attempts && handle < 0; attempt++) {
      handle = scePadOpen(user, 0, 0, NULL);
      if (handle < 0 && attempt + 1 < attempts)
         sceKernelUsleep(100000);
   }
   /* A handle this process already holds for the user is reused. Borrowed
    * handles are never closed by this input backend. */
   const bool owned = handle >= 0;
   if (handle < 0)
      handle = scePadGetHandle(user, 0, 0);
   if (handle < 0) {
      if (user != pad_unopened)
         pad_event("pad: scePadOpen failed for user %d (player %d)", (int)user, player);
      pad_unopened = user;
      return false;
   }
   state->user = user;
   state->handle = handle;
   state->port = PAD_PORT_STANDARD;
   state->owned = owned;
   pad_refresh_class(player);
   struct pad_sample initial = {0};
   state->connected = scePadReadState(handle, &initial) >= 0 && initial.connected;
   pad_event("pad: player %d opened for user %d, rumble mode %d", player, (int)user,
       state->instrument ? 0 : scePadSetVibrationMode(handle, PAD_VIBRATION_COMPATIBLE));
   return true;
}

static void
pad_player_close(int player, const char *why)
{
   struct pad_player_state *state = &pad_states[player];
   if (state->handle < 0)
      return;
   const int result = state->owned ? scePadClose(state->handle) : 0;
   pad_event("pad: player %d let go (user %d %s), owned=%d close=%d", player,
       (int)state->user, why, state->owned, result);
   pad_rest(state);
}

/* Adopt only a class-verified guitar. The first sample can lag a successful
 * open; allow a bounded connection window instead of closing the new handle
 * before the service has published input. Other native sources remain intact. */
static bool
pad_adopt_instrument(int slot, int32_t user, int handle, bool owned)
{
   if (handle < 0) return false;
   for (int i = 0; i < PAD_PLAYERS; ++i)
      if (pad_states[i].handle == handle) return pad_states[i].instrument;
   struct instrument_info info = {0};
   const int information = pad_controller_info(handle, &info);
   if (information < 0 || info.device_class != PAD_CLASS_GUITAR) {
      if (owned) {
         static bool reported; static int last_information, last_class;
         if (!reported || last_information != information || last_class != info.device_class) {
            pad_event("pad: RB4 handle rejected info=%08x class=%d", (unsigned)information,
                (int)info.device_class);
            reported = true; last_information = information; last_class = info.device_class;
         }
         (void)scePadClose(handle);
      }
      return false;
   }
   struct pad_sample sample = {0};
   const int read = scePadReadState(handle, &sample);
   // A disconnected placeholder must not occupy a source needed by a real pad.
   // Wait only when either the sample or device information confirms a kit.
   if (!(read >= 0 && sample.connected) && !info.connected) {
      if (owned) (void)scePadClose(handle);
      return false;
   }
   pad_player_close(slot, "replaced disconnected standard port");
   struct pad_player_state *state = &pad_states[slot];
   pad_rest(state);
   state->handle = handle;
   state->user = user;
   state->port = PAD_PORT_INSTRUMENT;
   state->instrument = true;
   state->owned = owned;
   state->connected = read >= 0 && sample.connected;
   state->waiting_first_sample = !state->connected;
   state->first_sample_deadline = now_seconds() + 3.0;
   pad_event("pad: RB4 guitar source=%d port=%d owned=%d read=%08x connected=%d connection=%u waiting=%d",
       slot, state->port, owned, (unsigned)read, state->connected,
       (unsigned)info.connection_type, state->waiting_first_sample);
   return true;
}

static void
pad_scan_instrument(int32_t user)
{
   for (int i = 0; i < PAD_PLAYERS; ++i)
      if (pad_states[i].handle >= 0 && pad_states[i].user == user && pad_states[i].instrument &&
          (pad_states[i].connected || pad_states[i].waiting_first_sample))
         return;
   int slot = -1;
   for (int i = 0; i < PAD_PLAYERS; ++i)
      if (pad_states[i].handle < 0) { slot = i; break; }
   if (slot < 0)
      for (int i = PAD_PLAYERS - 1; i >= 0; --i)
         if (!pad_states[i].connected && pad_states[i].port == PAD_PORT_STANDARD) {
            slot = i; break;
         }
   if (slot < 0) return;
   /* OpenExt may report that a special port is already open. Reuse its
    * verified guitar handle before requesting a new one. */
   const int existing = scePadGetHandle(user, PAD_PORT_INSTRUMENT, 0);
   if (existing >= 0 && pad_adopt_instrument(slot, user, existing, false)) return;
   int result = -1;
   /* OpenOrbis exposes a second product field. Preserve the original form and
    * support the duplicated-product form without guessing privilege APIs or
    * opening unrelated special devices. */
   for (int variant = 0; variant < 2; ++variant) {
      const struct instrument_open_params params = {0x0738, 0x8261,
          variant ? 0x8261 : 0, {0}};
      result = scePadOpenExt(user, PAD_PORT_INSTRUMENT, 0, &params);
      if (result >= 0 && pad_adopt_instrument(slot, user, result, true)) return;
   }
   static int32_t reported_users[4] = {-1,-1,-1,-1};
   static int reported_results[4];
   int report = 0;
   for (; report < 3 && reported_users[report] != user && reported_users[report] >= 0; ++report) {}
   const int status = result >= 0 ? 0 : result;
   if (reported_users[report] != user || reported_results[report] != status) {
      pad_event("pad: RB4 instrument probe user=%d result=%08x existing=%08x",
          (int)user, (unsigned)status, (unsigned)existing);
      reported_users[report] = user; reported_results[report] = status;
   }
}

static void
pad_scan(void)
{
   struct user_list list = {{-1, -1, -1, -1}};
   if (sceUserServiceGetLoginUserIdList(&list) < 0)
      return;
   for (int player = 0; player < PAD_PLAYERS; player++) {
      if (pad_states[player].handle < 0)
         continue;
      bool signed_in = false;
      for (int i = 0; i < 4; i++)
         signed_in |= list.user_id[i] >= 0 && list.user_id[i] == pad_states[player].user;
      if (!signed_in)
         pad_player_close(player, "signed out");
      else if (pad_states[player].port == PAD_PORT_INSTRUMENT && !pad_states[player].connected &&
               (!pad_states[player].waiting_first_sample || now_seconds() >= pad_states[player].first_sample_deadline))
         pad_player_close(player, "instrument disconnected or first sample timed out");
      else if (pad_states[player].port == PAD_PORT_STANDARD)
         pad_refresh_class(player);
   }
   /* Discover instruments before reserving slots for disconnected standard
    * handles belonging to the other signed-in users. */
   for (int i = 0; i < 4; ++i)
      if (list.user_id[i] >= 0)
         pad_scan_instrument(list.user_id[i]);
   for (int i = 0; i < 4; i++) {
      const int32_t user = list.user_id[i];
      bool known = user < 0;
      for (int player = 0; player < PAD_PLAYERS && !known; player++)
         known = pad_states[player].handle >= 0 && pad_states[player].user == user &&
                 pad_states[player].port == PAD_PORT_STANDARD;
      for (int player = 0; player < PAD_PLAYERS && !known; player++) {
         if (pad_states[player].handle < 0) {
            (void)pad_player_open(player, user, 1);
            break;
         }
      }
   }
}

/* Convert both fret banks to the same canonical five-fret buttons used by the
 * existing controller profiles. Strum, Options, Share and the OS interception
 * bit stay in the native button mask; no synthetic edges or repeat filter. */
static void
pad_decode_instrument(struct pad_player_state *state, struct pad_sample *sample)
{
   if (!state->instrument || !sample->connected ||
       (sample->buttons & PAD_INTERCEPTED))
      return;
   struct instrument_data data = {0};
   data.device_class = PAD_CLASS_GUITAR;
   const int parsed = scePadDeviceClassParseData(state->handle, sample, &data);
   const bool valid = parsed >= 0 && data.valid && data.device_class == PAD_CLASS_GUITAR &&
                      !(data.data.guitar.frets & ~31u) && !(data.data.guitar.solo_frets & ~31u);
   if (!valid) {
      if (!state->class_error)
         pad_event("pad: RB4 fret data unavailable result=%08x class=%d valid=%u",
             (unsigned)parsed, (int)data.device_class, (unsigned)data.valid);
      state->class_error = true;
      /* Preserve genuine native buttons when class decoding is unavailable. */
      return;
   }
   state->class_error = false;
   const unsigned frets = data.data.guitar.frets | data.data.guitar.solo_frets;
   sample->buttons &= ~(PAD_CROSS | PAD_CIRCLE | PAD_SQUARE | PAD_TRIANGLE | PAD_L1);
   const uint32_t buttons[5] = {PAD_CROSS, PAD_CIRCLE, PAD_SQUARE, PAD_TRIANGLE, PAD_L1};
   for (int i = 0; i < 5; ++i)
      if (frets & (1u << i)) sample->buttons |= buttons[i];
}

bool
pad_open(void)
{
   for (int player = 0; player < PAD_PLAYERS; player++)
      pad_rest(&pad_states[player]);
   (void)sceUserServiceInitialize(NULL);
   int32_t user = -1;
   if (sceUserServiceGetInitialUser(&user) < 0 || scePadInit() < 0) {
      pad_event("pad: no user or no pad service");
      return false;
   }
   pad_service = true;
   pad_event("pad: native guitar discovery revision=2");
   const bool opened = pad_player_open(0, user, 10);
   pad_scan();
   pad_next_scan = now_seconds() + PAD_SCAN_SECONDS;
   return opened;
}

static float
stick(uint8_t value)
{
   const float v = ((float)value - 128.0f) / 127.0f;
   const float dead = 0.12f;
   if (v > -dead && v < dead)
      return 0.0f;
   return v > 0 ? (v - dead) / (1.0f - dead) : (v + dead) / (1.0f - dead);
}

static void
pad_fill(const struct pad_player_state *state, struct pad *pad)
{
   const uint32_t before = pad->held;
   pad->held = state->handle >= 0 ? state->last.buttons : 0;
   pad->pressed = pad->held & ~before;
   pad->left_x = stick(state->last.left_x);
   pad->left_y = stick(state->last.left_y);
   pad->right_x = stick(state->last.right_x);
   pad->right_y = stick(state->last.right_y);
   pad->l2 = state->last.l2 / 255.0f;
   pad->r2 = state->last.r2 / 255.0f;
}

void
pad_poll(struct pad *pad)
{
   if (pad_service && now_seconds() >= pad_next_scan) {
      pad_scan();
      pad_next_scan = now_seconds() + PAD_SCAN_SECONDS;
   }
   static struct pad_sample samples[64];
   for (int player = 0; player < PAD_PLAYERS; player++) {
      struct pad_player_state *state = pad_state(player);
      if (state == NULL)
         continue;
      state->taken_count = 0;
      const int result = scePadReadState(state->handle, &samples[0]);
      const int count = result < 0 ? -1 : 1;
      memset(&state->last, 0, sizeof(state->last));
      state->last.left_x = state->last.left_y = 128;
      state->last.right_x = state->last.right_y = 128;
      if (count < 0)
         state->connected = false;
      for (int i = 0; i < count && i < 64; i++) {
         const bool intercepted = (samples[i].buttons & PAD_INTERCEPTED) != 0;
         if (state->instrument && intercepted != state->input_intercepted) {
            pad_event("pad: guitar source=%d intercepted=%d", player, intercepted);
            state->input_intercepted = intercepted;
         }
         pad_decode_instrument(state, &samples[i]);
         const struct pad_sample *s = &samples[i];
         state->taken[state->taken_count++] = (struct pad_reading){
            s->buttons, s->left_x, s->left_y, s->right_x, s->right_y, s->l2, s->r2,
            s->connected != 0, s->timestamp_us};
         state->connected = s->connected != 0;
         if (state->connected) state->waiting_first_sample = false;
         if (s->connected && !(s->buttons & PAD_INTERCEPTED))
            state->last = *s;
      }
   }
#if defined(ENCORE_VR_CINEMA)
   cinema_bridge_input();
#endif
   (void)pad_player(0, pad);
}

bool
pad_player_instrument(int player)
{
   const struct pad_player_state *state = pad_state(player);
   return state && state->connected && state->instrument;
}

uint32_t
pad_players(void)
{
   uint32_t players = 0;
   for (int player = 0; player < PAD_PLAYERS; player++) {
      const struct pad_player_state *state = pad_state(player);
      if (state != NULL && state->connected)
         players |= 1u << player;
   }
#if defined(ENCORE_VR_CINEMA)
   if(cinema_virtual)players|=1u;
#endif
   return players;
}

bool
pad_player(int player, struct pad *pad)
{
#if defined(ENCORE_VR_CINEMA)
   if(player==0&&cinema_virtual){
      struct pad_player_state synth={0};
      synth.handle=0;
      synth.last.buttons=cinema_virtual_sample.buttons;
      synth.last.left_x=cinema_virtual_sample.left_x;
      synth.last.left_y=cinema_virtual_sample.left_y;
      synth.last.right_x=cinema_virtual_sample.right_x;
      synth.last.right_y=cinema_virtual_sample.right_y;
      synth.last.l2=cinema_virtual_sample.l2;
      synth.last.r2=cinema_virtual_sample.r2;
      pad_fill(&synth,pad);
      return true;
   }
#endif
   const struct pad_player_state *state = pad_state(player);
   if (state == NULL || !state->connected) {
      struct pad_player_state rest;
      pad_rest(&rest);
      pad_fill(&rest, pad);
      return false;
   }
   pad_fill(state, pad);
   return true;
}

int
pad_player_readings(int player, const struct pad_reading **readings)
{
#if defined(ENCORE_VR_CINEMA)
   if(player==0&&cinema_virtual){
      *readings=&cinema_virtual_sample;
      return 1;
   }
#endif
   const struct pad_player_state *state = pad_state(player);
   *readings = state != NULL ? state->taken : NULL;
   return state != NULL ? state->taken_count : 0;
}

int
pad_readings(const struct pad_reading **readings)
{
   return pad_player_readings(0, readings);
}

void
pad_player_vibrate(int player, float large, float small)
{
   const struct pad_player_state *state = pad_state(player);
   if (state == NULL || state->instrument)
      return;
   const float l = large < 0.0f ? 0.0f : large > 1.0f ? 1.0f : large;
   const float s = small < 0.0f ? 0.0f : small > 1.0f ? 1.0f : small;
   const uint8_t motors[2] = {(uint8_t)(l * 255.0f), (uint8_t)(s * 255.0f)};
   (void)scePadSetVibration(state->handle, motors);
}

void
pad_vibrate(float large, float small)
{
   pad_player_vibrate(0, large, small);
}

void
pad_player_light_bar(int player, uint8_t r, uint8_t g, uint8_t b)
{
   const struct pad_player_state *state = pad_state(player);
   if (state == NULL)
      return;
   const uint8_t color[4] = {r, g, b, 0};
   (void)scePadSetLightBar(state->handle, color);
}

void
pad_light_bar(uint8_t r, uint8_t g, uint8_t b)
{
   pad_player_light_bar(0, r, g, b);
}

/* --------------------------------------------------------------- the sound */

#define AUDIO_GRAIN 256
#define AUDIO_ALREADY_INITIALISED ((int)0x8026000e)

static int32_t audio_port = -1;
static pthread_t audio_thread;
static atomic_bool audio_running;
static audio_fill_fn audio_fill;
static void *audio_user;

static void *
audio_main(void *unused)
{
   (void)unused;
   static int16_t grain[AUDIO_GRAIN * 2] __attribute__((aligned(64)));
   unsigned errors = 0;
   while (atomic_load(&audio_running)) {
      audio_fill(grain, AUDIO_GRAIN, audio_user);
      /* Blocks for one grain: this is what paces the thread. An output that
       * stops taking grains is said once, not left to stall the program. */
      if (sceAudioOutOutput(audio_port, grain) < 0) {
         if (errors++ == 0)
            say("audio: sceAudioOutOutput failed");
         sceKernelUsleep(5000);
      }
   }
   (void)sceAudioOutOutput(audio_port, NULL); /* drain the grain still queued */
   return NULL;
}

bool
audio_start(audio_fill_fn fill, void *user)
{
   if (audio_port >= 0 || fill == NULL)
      return false;
   const int init = sceAudioOutInit();
   if (init < 0 && init != AUDIO_ALREADY_INITIALISED) {
      say("audio: sceAudioOutInit 0x%08x", (unsigned)init);
      return false;
   }
   /* The system user's main port: 256 frames, 48 kHz, 16-bit stereo. */
   audio_port = sceAudioOutOpen(0xff, 0, 0, AUDIO_GRAIN, 48000, 1);
   if (audio_port < 0) {
      say("audio: sceAudioOutOpen 0x%08x", (unsigned)audio_port);
      audio_port = -1;
      return false;
   }
   audio_fill = fill;
   audio_user = user;
   atomic_store(&audio_running, true);
   if (pthread_create(&audio_thread, NULL, audio_main, NULL) != 0) {
      say("audio: no thread");
      atomic_store(&audio_running, false);
      (void)sceAudioOutClose(audio_port);
      audio_port = -1;
      return false;
   }
   say("audio: 48 kHz stereo on port %d", (int)audio_port);
   return true;
}

void
audio_stop(void)
{
   if (audio_port < 0)
      return;
   atomic_store(&audio_running, false);
   pthread_join(audio_thread, NULL);
   (void)sceAudioOutClose(audio_port);
   audio_port = -1;
}
