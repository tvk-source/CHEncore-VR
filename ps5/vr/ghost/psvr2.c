/*
 * psvr2 - the PS VR2 headset, its Sense controllers and the DualSense.
 *
 * The headset side is PS5VR's (ps5vr/app/src/app_vr.c and the probe's world
 * test): Hmd2 and VrTracker2 are system modules, loaded with
 * sceSysmoduleLoadModule and bound at the FW 13.60 export offsets after a
 * module-name and rodata check (an app may not dlsym them). Frames go out the
 * way Horizon Call of the Mountain sends them (research/hmd2-notes.md): one
 * side-by-side texture registered as a type 0x10 layer, then per frame
 * BeginFrame, SetParam (viewports, FOV tangents, the render pose, the
 * predicted display time) and a flip on the VR VideoOut port (bus 32).
 */
#include "gvr_stats.h"
#include "psvr2.h"
#include "xr_log.h"
#include "../probe_stereo.h"
#include "../probe_vulkan.h"
#include "../probe_timing.h"

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* -- console calls ---------------------------------------------------------- */
typedef struct { uint64_t size, align; } SizeAlign;

int sceSysmoduleLoadModule(uint32_t id);
const char *sceKernelGetFsSandboxRandomWord(void);
int sceKernelLoadStartModule(const char *path, size_t argc, const void *argv, uint32_t flags,
                             void *opt, int *res);
int sceKernelDlsym(int handle, const char *name, void **addr);
int sceKernelAllocateDirectMemory(int64_t start, int64_t end, size_t len, size_t align, int type,
                                  int64_t *offset);
int sceKernelMapDirectMemory(void **addr, size_t len, int prot, int flags, int64_t offset,
                             size_t align);
size_t sceKernelGetDirectMemorySize(void);
int sceKernelAvailableDirectMemorySize(int64_t start, int64_t end, size_t align, int64_t *start_out, size_t *size_out);
typedef struct { int64_t start, end; int type; } dmem_query_info;
int sceKernelDirectMemoryQuery(int64_t offset, int flags, dmem_query_info *info, size_t info_size);
int sceKernelUsleep(unsigned usec);
uint64_t sceKernelGetProcessTime(void);
int sceUserServiceInitialize(const void *params);
int sceUserServiceGetInitialUser(int32_t *user_id);

typedef struct { void *data, *metadata, *reserved0, *reserved1; } vo_buffer_t;
typedef struct { uint8_t reserved[80]; } vo_attribute_t;
int sceVideoOutOpen(int user_id, int bus_type, int index, const void *param);
int sceVideoOutClose(int handle);
void sceVideoOutSetBufferAttribute2(void *attr, uint64_t format, uint32_t tiling, uint32_t width,
                                    uint32_t height, uint64_t option, uint32_t reserved0,
                                    uint64_t reserved1);
int sceVideoOutRegisterBuffers2(int handle, int set_index, int start_index, void *buffers,
                                int count, void *attr, int category, void *option);
int sceVideoOutUnregisterBuffers(int handle, int set_index);
int sceVideoOutSubmitFlip(int handle, int index, int flip_mode, int64_t flip_arg);
int sceVideoOutConfigureOutput(int handle, uint64_t mode, const void *options, const void *r0,
                               uint64_t r1);

int scePadInit(void);
int scePadOpen(int user, int type, int index, const void *param);
int scePadGetHandle(int user, int type, int index);
int scePadReadState(int handle, void *data);
int scePadVrControllerReadState(int handle, void *state);
int scePadSetVibrationMode(int handle, int mode);
int scePadSetVibration(int handle, const void *vibration);
int scePadSetMotionSensorState(int handle, int enable);
int scePadResetOrientation(int handle);
static int pad_rumble(int h, int sense, uint8_t left, uint8_t right);

#define HMD2_FUNCS(X)                                                                        \
    X(int, sceHmd2Initialize, (const void *param))                                          \
    X(int, sceHmd2Open, (int user_id, int type, int index, const void *param))               \
    X(int, sceHmd2GetDeviceInformation, (void *info))                                        \
    X(int, sceHmd2GetFieldOfViewWithoutHandle, (void *fov))                                  \
    X(SizeAlign, sceHmd2ReprojectionQueryBufferSizeAlign, (void))                            \
    X(SizeAlign, sceHmd2ReprojectionQueryDisplayBufferSizeAlign, (void))                     \
    X(int, sceHmd2ReprojectionInitialize, (const void *param, const void *reserved))         \
    X(int, sceHmd2ReprojectionEnableVrMode, (uint64_t mode))                                 \
    X(int, sceHmd2ReprojectionDisableVrMode, (void))                                         \
    X(int, sceHmd2ReprojectionBeginFrame, (uint64_t frame))                                  \
    X(int, sceHmd2ReprojectionGetPredictedDisplayTime, (uint64_t frame, uint64_t *time))     \
    X(int, sceHmd2ReprojectionSetParam, (uint32_t buffer, uint64_t frame, const void *param, \
                                         const void *trace))                               \
    X(int, sceHmd2ReprojectionSetRenderConfig, (uint64_t half_rate, uint64_t r0, uint64_t r1)) \
    X(SizeAlign, sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign, (void))                \
    X(SizeAlign, sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign, (void))             \
    X(int, sceHmd2ReprojectionSetMirroringOption, (const void *opt))                       \
    X(int, sceHmd2ReprojectionEnableMirroring, (const void *param))                        \
    X(int, sceHmd2ReprojectionDisableMirroring, (void))                                    \
    X(int, sceHmd2ReprojectionTerminate, (void))                                           \
    X(int, sceHmd2Close, (int handle))                                                     \
    X(int, sceHmd2Terminate, (void))                                                       \
    X(int, sceHmd2SetVibration, (int handle, int strength))                                 \
    X(int, sceHmd2GazeGetResult, (const void *param, void *result))                         \
    X(int, sceHmd2InternalGazeStart, (void))                                                \
    X(int, sceHmd2InternalGazeStop, (void))
#define VRTRACKER2_FUNCS(X)                                                                  \
    X(int, sceVrTracker2QueryMemory, (const void *param, void *out))                         \
    X(int, sceVrTracker2Initialize, (const void *param))                                     \
    X(int, sceVrTracker2RegisterDevice, (int type, int handle))                              \
    X(int, sceVrTracker2GetResult, (const void *param, void *result))                        \
    X(int, sceVrTracker2UnregisterDevice, (int handle))                                      \
    X(int, sceVrTracker2Finalize, (void))
#define DECLARE_PTR(ret, name, args) static ret(*name) args;
HMD2_FUNCS(DECLARE_PTR)
VRTRACKER2_FUNCS(DECLARE_PTR)

#define SYSMODULE_HMD2 0x12a
#define SYSMODULE_VRTRACKER2 0x12b
#define VR_MODE_90HZ 0x2000c
#define VR_MODE_120HZ 0x2000f
#define VR_QUEUE_FULL ((int)0x8A72001A)
#define VO_BUSY ((int)0x80290009)
#define MAX_BUFFERS 4

/* -- module binding (PS5VR's) ----------------------------------------------- */
typedef struct { const char *name; uint32_t offset; } export_sig_t;
typedef struct { uint32_t va; const char *text; } export_anchor_t;
typedef struct {
    int fw;
    const export_sig_t *exports;
    size_t count;
    const export_anchor_t *anchor;
} export_table_t;
#define COUNT(a) (sizeof(a) / sizeof *(a))
#include "vr_exports.h"

typedef struct { const char *name; void **slot; } slot_t;
#define SLOT(ret, name, args) {#name, (void **)&name},
static slot_t kHmd2Slots[] = {HMD2_FUNCS(SLOT)};
static slot_t kVrTracker2Slots[] = {VRTRACKER2_FUNCS(SLOT)};

typedef struct {
    uint64_t size;
    char name[256];
    struct { uint64_t addr; uint32_t size; int32_t prot; } seg[4];
    uint32_t nseg;
    uint8_t fingerprint[20];
} module_info_t;
int sceKernelGetModuleInfo(int handle, module_info_t *info);
int sceKernelGetModuleList(int32_t *handles, size_t num, size_t *actual);
typedef struct { uint64_t size; char text[0x1c]; uint32_t version; } sw_version_t;
int sceKernelGetSystemSwVersion(sw_version_t *version);

/* The system's notification, so a player in the headset sees why PS VR2 didn't start instead of a
 * black screen (ivmreg's idea, HLA-PSVR2 pull request 12). Each start-up step fails once at most
 * (H.failed, H.output_failed), so this doesn't repeat; the output's own steps say it once per run. */
typedef struct { char reserved[45]; char message[3075]; } psvr2_notify_t;
int sceKernelSendNotificationRequest(int, psvr2_notify_t *, size_t, int);
static void psvr2_tell(const char *step, int rc, const char *advice)
{
    static psvr2_notify_t request;
    memset(&request, 0, sizeof request);
    snprintf(request.message, sizeof request.message, "PS VR2 didn't start (%s 0x%08x). %s", step, (unsigned)rc, advice);
    XRLOG("psvr2: told the player: %s", request.message);
    (void)sceKernelSendNotificationRequest(0, &request, sizeof request, 0);
}
#define TELL_HEADSET "Check the headset is connected and turned on, then start the game again."
#define TELL_REPORT "Start the game again, and if it keeps happening send a report from its page in Ghost VR."

static int load_sysmodule(uint32_t id, const char *name)
{
    int rc = sceSysmoduleLoadModule(id);
    char path[128];
    snprintf(path, sizeof path, "/%s/common/lib/%s.sprx", sceKernelGetFsSandboxRandomWord(), name);
    int res = 0;
    int h = sceKernelLoadStartModule(path, 0, NULL, 0, NULL, &res);
    if (h < 0) {
        /* Hosted in a title that has already changed its credentials (prospero-win's
         * /data grant): the sandbox path no longer resolves, but the title loaded the
         * module before that, so find it among the loaded ones by name. */
        char file[64];
        int32_t handles[256];
        size_t count = 0;
        snprintf(file, sizeof file, "%s.sprx", name);
        if (sceKernelGetModuleList(handles, 256, &count) == 0)
            for (size_t i = 0; i < count && h < 0; i++) {
                module_info_t mi;
                memset(&mi, 0, sizeof mi);
                mi.size = sizeof mi;
                if (sceKernelGetModuleInfo(handles[i], &mi) == 0 && !strcmp(mi.name, file))
                    h = handles[i];
            }
    }
    XRLOG("psvr2: sysmodule 0x%x %s -> 0x%08x, handle 0x%x", id, name, rc, h);
    return h;
}

static uintptr_t module_base(int h, const char *file, module_info_t *mi)
{
    memset(mi, 0, sizeof *mi);
    mi->size = sizeof *mi;
    if (sceKernelGetModuleInfo(h, mi) != 0 || strcmp(mi->name, file) != 0)
        return 0;
    uintptr_t base = 0;
    for (unsigned i = 0; i < mi->nseg && i < 4; i++)
        if (!base || mi->seg[i].addr < base)
            base = mi->seg[i].addr;
    return base;
}

static int anchor_matches(const module_info_t *mi, uintptr_t base, const export_anchor_t *anchor)
{
    const uintptr_t want = base + anchor->va;
    const size_t len = strlen(anchor->text) + 1;
    for (unsigned i = 0; i < mi->nseg && i < 4; i++) {
        const uintptr_t lo = mi->seg[i].addr, hi = lo + mi->seg[i].size;
        if (want >= lo && want + len <= hi && (mi->seg[i].prot & 1))
            return memcmp((const void *)want, anchor->text, len) == 0;
    }
    return 0;
}

/* inside one of the module's executable segments: an offset from the exports file is only
 * used where code can be */
static int in_code(const module_info_t *mi, uintptr_t at)
{
    for (unsigned i = 0; i < mi->nseg && i < 4; i++)
        if (at >= mi->seg[i].addr && at < mi->seg[i].addr + mi->seg[i].size && (mi->seg[i].prot & 4))
            return 1;
    return 0;
}

/* No table for this build: the offsets gv-tool read from this console's kernel (the app runs
 * "exports" once per firmware, engine/gv_tool.c). Used only when the file names this firmware. */
#define EXPORTS_FILE "/data/ghostvr/fw/exports.txt"
static uintptr_t exports_file_offset(const char *file, const char *name)
{
    static int fw_ok = -1;
    FILE *f = fopen(EXPORTS_FILE, "r");
    if (!f)
        return 0;
    char line[160], lib[64], sym[96];
    unsigned long off = 0, found = 0;
    unsigned fw = 0;
    while (fgets(line, sizeof line, f)) {
        if (fw_ok < 0 && sscanf(line, "fw %x", &fw) == 1) {
            sw_version_t v;
            memset(&v, 0, sizeof v);
            v.size = sizeof v;
            fw_ok = sceKernelGetSystemSwVersion(&v) == 0 && (v.version >> 16) == fw;
            XRLOG("psvr2: %s is for %x, this console is %x", EXPORTS_FILE, fw, v.version >> 16);
        } else if (fw_ok > 0 && sscanf(line, "%63s %95s %lx", lib, sym, &off) == 3 && !strcmp(lib, file) &&
                   !strcmp(sym, name)) {
            found = off;
            break;
        }
    }
    fclose(f);
    return fw_ok > 0 ? found : 0;
}

static int resolve_module(int h, const char *file, slot_t *slots, size_t n,
                          const export_table_t *tables, size_t tables_n)
{
    module_info_t mi;
    uintptr_t base = 0;
    const export_table_t *t = NULL;
    int tried_base = 0, ok = 1;
    for (size_t i = 0; i < n; i++) {
        if (sceKernelDlsym(h, slots[i].name, slots[i].slot) == 0 && *slots[i].slot)
            continue;
        if (!tried_base) {
            base = module_base(h, file, &mi);
            for (size_t k = 0; base && k < tables_n && !t; k++)
                if (
#if defined(ENCORE_VR_PROBE)
                    tables[k].fw == 761 &&
#endif
                    anchor_matches(&mi, base, tables[k].anchor))
                    t = &tables[k];
#ifdef PS5XR_TEST_EXPORTS_FILE
            t = NULL; /* test build: as on a firmware with no table (M7's proof on 13.60) */
#endif
            tried_base = 1;
            if (t)
                XRLOG("psvr2: %s by export offsets (table %d.%02d), base %p", file, t->fw / 100, t->fw % 100,
                      (void *)base);
            else
                XRLOG("psvr2: %s: no table for this build, using %s (base %p)", file, EXPORTS_FILE, (void *)base);
        }
        *slots[i].slot = NULL;
        for (size_t j = 0; t && j < t->count; j++)
            if (!strcmp(t->exports[j].name, slots[i].name) &&
                in_code(&mi, base + t->exports[j].offset))
                *slots[i].slot = (void *)(base + t->exports[j].offset);
#if !defined(ENCORE_VR_PROBE)
        if (!t && base) {
            const uintptr_t off = exports_file_offset(file, slots[i].name);
            if (off && in_code(&mi, base + off))
                *slots[i].slot = (void *)(base + off);
        }
#endif
        if (!*slots[i].slot) {
            XRLOG("psvr2: missing %s (no table for this firmware and no exports file from the app)", slots[i].name);
            ok = 0;
        }
    }
    return ok ? 0 : -1;
}

static void *direct_alloc(size_t size, size_t align)
{
    size = (size + 0xffff) & ~(size_t)0xffff;
    if (align < 0x10000)
        align = 0x10000;
    int64_t off = 0;
    void *va = NULL;
    if (sceKernelAllocateDirectMemory(0, (int64_t)16 << 30, size, align, 12 /* WB_ONION */, &off) != 0 ||
        sceKernelMapDirectMemory(&va, size, 0x33, 0, off, align) != 0) {
        XRLOG("psvr2: direct alloc 0x%zx failed", size);
        return NULL;
    }
    memset(va, 0, size);
    return va;
}

/* -- maths ------------------------------------------------------------------ */
static void qmul(const float a[4], const float b[4], float o[4])
{
    float r[4] = {
        a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
        a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
        a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3],
        a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2],
    };
    memcpy(o, r, sizeof r);
}

static void qrot(const float q[4], const float v[3], float o[3])
{
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    const float tx = 2 * (y * v[2] - z * v[1]), ty = 2 * (z * v[0] - x * v[2]), tz = 2 * (x * v[1] - y * v[0]);
    o[0] = v[0] + w * tx + (y * tz - z * ty);
    o[1] = v[1] + w * ty + (z * tx - x * tz);
    o[2] = v[2] + w * tz + (x * ty - y * tx);
}

static void qnorm(float q[4])
{
    float n = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (n < 1e-6f) {
        q[0] = q[1] = q[2] = 0;
        q[3] = 1;
        return;
    }
    for (int i = 0; i < 4; i++)
        q[i] /= n;
}

/* -- state ------------------------------------------------------------------ */
typedef struct { void *layers; uint32_t count; uint32_t pad; } layer_set_t;

static struct {
    int loaded, failed;
    /* Hosted (psvr2_set_defer_output): Hmd2 opened by xrGetSystem, the
     * tracker and reprojection started by the session. */
    int opened, output_failed, want_output;
    int hmd_initialized, tracker_initialized, tracker_registered;
    uint64_t last_display_us; /* the last frame's predicted display time */
    uint64_t last_begin_us;   /* when the last frame began */
    int live;                 /* Hmd2 predicts display times ahead of now */
    int user;
    int hmd;
    float fov[4];
    psvr2_info info;
    /* output */
    int active;
    int vh;
    int mode;
    uint32_t eye_w, eye_h, nbuf;
    uint8_t layers[MAX_BUFFERS][0x130] __attribute__((aligned(16)));
    layer_set_t sets[MAX_BUFFERS];
    warp_axis warp[2][2];
    float *warp_tables; /* GPU-visible: left across, left down, right across, right down, 64 KB apart */
    void *mirror_work, *mirror_disp;
    size_t mirror_disp_size;
    void *tracker_work, *reproj_work, *reproj_disp; /* kept: a session can restart in one process */
    int mirror_want, mirror_on;
    uint64_t worn_checked;
    /* Frames Hmd2 began that were not submitted yet (each holds a place in
     * its queue until it is), and the last image submitted. */
    struct { uint64_t frame, display_us; } pending[4];
    /* frames begun since the VR port's vblank count pace_vb (psvr2_begin_frame) */
    uint64_t pace_vb, pace_frames;
    int npending, have_last, last_buf;
    psvr2_pose last_pose;
    float last_tl[4], last_tr[4];
    /* stats */
    psvr2_stats stats;
} H = {.vh = -1, .hmd = -1, .mirror_want = 1};

/* 0 in hosted mode: the host title owns the DualSense. */
static int use_dualsense = 1;
/* 1 in hosted mode: the tracker and reprojection start with the session, after
 * the host's GPU driver has initialised AGC (prospero-win: RADV's
 * sceAgcInit(8) failed 0x8a6c0004 when they started first). */
static int defer_output;
static atomic_int exit_requested;
static pthread_mutex_t pad_lock = PTHREAD_MUTEX_INITIALIZER;

uint64_t psvr2_now_us(void)
{
    return sceKernelGetProcessTime();
}

int psvr2_user(void)
{
    return H.user;
}

int psvr2_exit_requested(void)
{
    return atomic_load(&exit_requested);
}

void psvr2_request_exit(void)
{
    atomic_store(&exit_requested, 1);
}

int psvr2_ready(void)
{
    return H.loaded;
}

void psvr2_get_stats(psvr2_stats *out)
{
    *out = H.stats;
}

int sceKernelGetAppInfo(int pid, void *info);
int getpid(void);

static void hexline(const char *what, const uint8_t *b, size_t n)
{
    char line[400];
    size_t o = 0;
    for (size_t i = 0; i < n && o + 3 < sizeof line; i++)
        o += (size_t)snprintf(line + o, sizeof line - o, "%02x", b[i]);
    XRLOG("psvr2: %s %s", what, line);
}

/* What Hmd2 sees of this process and the headset, for comparing a hosted
 * title with a native one. */
static void dump_identity(void)
{
    uint8_t ai[0x100], di[64];
    memset(ai, 0, sizeof ai);
    memset(di, 0, sizeof di);
    int r = sceKernelGetAppInfo(getpid(), ai);
    XRLOG("psvr2: appinfo rc 0x%08x", r);
    hexline("appinfo[0x00]", ai, 0x40);
    hexline("appinfo[0x40]", ai + 0x40, 0x40);
    r = sceHmd2GetDeviceInformation(di);
    XRLOG("psvr2: device info rc 0x%08x", r);
    hexline("devinfo", di, 64);
}

static void read_device_info(void)
{
    uint8_t info[64];
    memset(info, 0, sizeof info);
    if (sceHmd2GetDeviceInformation(info) == 0) {
        H.info.status = (int)*(uint32_t *)info;
        H.info.worn = info[20] != 0;
        if (info[21])
            H.info.ipd = info[21] / 1000.0f;
    }
}

/* -- controllers ------------------------------------------------------------ */
/* One pad sample (scePadReadState), the 120-byte layout PS5_vkQuake reads:
 * buttons, sticks, analog triggers, then the motion sensors. */
typedef struct {
    uint32_t buttons;
    uint8_t lx, ly, rx, ry, l2, r2;
    uint8_t pad0[2];
    float orientation[4];
    float accel[3];
    float angvel[3];
    uint8_t touch[24];
    int32_t connected;
    uint64_t timestamp;
    uint8_t rest[32];
} pad_data_t;
_Static_assert(sizeof(pad_data_t) == 120, "pad sample size");
_Static_assert(__builtin_offsetof(pad_data_t, connected) == 0x4c, "pad connected offset");

enum {
    PAD_L3 = 0x2, PAD_R3 = 0x4, PAD_OPTIONS = 0x8, PAD_UP = 0x10, PAD_RIGHT = 0x20,
    PAD_DOWN = 0x40, PAD_LEFT = 0x80, PAD_L2 = 0x100, PAD_R2 = 0x200, PAD_L1 = 0x400,
    PAD_R1 = 0x800, PAD_TRIANGLE = 0x1000, PAD_CIRCLE = 0x2000, PAD_CROSS = 0x4000,
    PAD_SQUARE = 0x8000, PAD_TOUCHPAD = 0x100000, PAD_CREATE = 0x1, PAD_INTERCEPTED = 0x80000000u
};

static struct {
    int init;
    /* Sense: pad handles (type 3 left, 4 right), tracker registration */
    int sense[2];
    int registered[2];
    int sense_connected[2];
    uint32_t sense_buttons[2];
    float sense_trigger[2], sense_grip[2], sense_stick[2][2];
    uint8_t last_raw[2][0x68];
    uint64_t next_open_try;
    /* DualSense fallback */
    int ds;
    int ds_connected;
    pad_data_t ds_data;
    int ds_motion;
    float ds_ref_inv[4];       /* inverse of the orientation at the last recentre */
    int ds_ref_set;
    /* injection */
    int inj_pose[2], inj_input[2];
    psvr2_pose inj_pose_v[2];
    psvr2_controller inj_input_v[2];
    uint64_t inj_until[2];
    /* haptics */
    uint64_t haptic_until[2];
    float haptic_amp[2];
    int vib_mode_set[3];
    /* menu button: a short press is a menu tap, a 1 s hold recentres
     * (0 left Sense Create, 1 right Sense Options, 2 DualSense Options) */
    uint64_t menu_down_us[3];
    int menu_held_fired[3];
    uint64_t menu_tap_until[2];
    int recenter_req;
    int ds_peek;               /* hosted: the host's DualSense, read for the hold only */
} P = {.sense = {-1, -1}, .ds = -1, .ds_peek = -1};

enum { MENU_HOLD_US = 1000000, MENU_TAP_US = 150000 };

/* Tracks one menu button; returns 1 on the frame a short press ends. */
static int menu_hold(int i, int down, uint64_t now)
{
    if (down) {
        if (!P.menu_down_us[i]) {
            P.menu_down_us[i] = now;
            P.menu_held_fired[i] = 0;
        } else if (!P.menu_held_fired[i] && now - P.menu_down_us[i] >= MENU_HOLD_US) {
            P.menu_held_fired[i] = 1;
            P.recenter_req = 1;
            XRLOG("psvr2: menu button held: recentre");
        }
        return 0;
    }
    const int tap = P.menu_down_us[i] && !P.menu_held_fired[i];
    P.menu_down_us[i] = 0;
    return tap;
}

static void pads_open(void)
{
    const uint64_t now = psvr2_now_us();
    if (P.init && now < P.next_open_try)
        return;
    if (!P.init) {
        scePadInit();
        P.init = 1;
    }
    P.next_open_try = now + 2000000; /* retry every 2 s for controllers switched on later */
    for (int side = 0; side < 2; side++) {
        if (P.sense[side] >= 0)
            continue;
        int h = scePadOpen(H.user, side ? 4 : 3, 0, NULL);
        if (h < 0)
            h = scePadGetHandle(H.user, side ? 4 : 3, 0);
        if (h < 0)
            continue;
        P.sense[side] = h;
        int rc = -1;
        if (H.loaded)
            rc = sceVrTracker2RegisterDevice(side ? 1 : 2, h);
        P.registered[side] = rc == 0;
        XRLOG("psvr2: %s Sense controller pad 0x%x, tracker register 0x%08x",
              side ? "right" : "left", h, rc);
    }
    if (P.ds < 0 && use_dualsense) {
        int h = scePadOpen(H.user, 0, 0, NULL);
        if (h < 0)
            h = scePadGetHandle(H.user, 0, 0);
        if (h >= 0) {
            P.ds = h;
            P.ds_motion = scePadSetMotionSensorState(h, 1) == 0;
            XRLOG("psvr2: DualSense pad 0x%x, motion %d", h, P.ds_motion);
        }
    }
}

static float stick_axis(uint8_t v)
{
    float f = ((float)v - 128.0f) / 127.0f;
    const float dead = 0.10f;
    if (f > -dead && f < dead)
        return 0.0f;
    f = f > 0 ? (f - dead) / (1 - dead) : (f + dead) / (1 - dead);
    return f > 1 ? 1 : f < -1 ? -1 : f;
}

static void log_bytes_changed(const char *what, int side, const uint8_t *now, uint8_t *last, size_t n)
{
    if (!memcmp(now, last, n))
        return;
    char line[512];
    int len = snprintf(line, sizeof line, "%s %d:", what, side);
    for (size_t i = 0; i < n && len < (int)sizeof line - 12; i++)
        if (now[i] != last[i])
            len += snprintf(line + len, sizeof line - len, " %zx=%02x", i, now[i]);
    memcpy(last, now, n);
    static uint64_t budget_reset;
    static int budget;
    uint64_t t = psvr2_now_us();
    if (t > budget_reset) {
        budget_reset = t + 1000000;
        budget = 0;
    }
    if (budget++ < 6)
        XRLOG("psvr2: %s", line);
}

/* A Sense controller's state (scePadVrControllerReadState, 0x68 bytes), read
 * the way Horizon Call of the Mountain reads it (its eboot, sub_1108000):
 *   +0x00 u32 buttons, the pad's own bits: left L3 0x2, L2 0x100, L1 0x400,
 *         triangle 0x1000, square 0x8000; right R3 0x4, options 0x8, R2 0x200,
 *         R1 0x800, cross 0x4000, circle 0x2000
 *   +0x04 u8 stick x, +0x05 u8 stick y (0..255, down-positive)
 *   +0x08 / +0x09 u8 analog triggers (L2, R2)
 *   +0x0c u32 finger-touch bits, the same bit per button
 *   +0x40 bit 0 connected */
static uint32_t pad_bits_to_btn(uint32_t b)
{
    uint32_t o = 0;
    if (b & (PAD_L2 | PAD_R2)) o |= PSVR2_BTN_TRIGGER;
    if (b & (PAD_L1 | PAD_R1)) o |= PSVR2_BTN_GRIP;
    if (b & PAD_CROSS) o |= PSVR2_BTN_A;
    if (b & PAD_CIRCLE) o |= PSVR2_BTN_B;
    if (b & PAD_SQUARE) o |= PSVR2_BTN_X;
    if (b & PAD_TRIANGLE) o |= PSVR2_BTN_Y;
    if (b & (PAD_OPTIONS | PAD_CREATE)) o |= PSVR2_BTN_MENU;
    if (b & (PAD_L3 | PAD_R3)) o |= PSVR2_BTN_STICK;
    return o;
}

static uint32_t sense_touch[2];

static void sense_read(int side)
{
    uint8_t raw[0x68];
    memset(raw, 0, sizeof raw);
    const int rc = scePadVrControllerReadState(P.sense[side], raw);
    if (rc == 0)
        log_bytes_changed("sense", side, raw, P.last_raw[side], 0x10);
    const int connected = rc == 0 && (raw[0x40] & 1);
    if (connected != P.sense_connected[side])
        XRLOG("psvr2: %s Sense controller %s", side ? "right" : "left", connected ? "connected" : "gone");
    P.sense_connected[side] = connected;
    if (!connected)
        return;
    uint32_t b;
    memcpy(&b, raw, 4);
    uint32_t t;
    memcpy(&t, raw + 0x0c, 4);
    const int intercepted = (b & PAD_INTERCEPTED) != 0;
    if (intercepted) {   /* the system has the controller (home menu) */
        b = 0;
        t = 0;
    }
    P.sense_buttons[side] = pad_bits_to_btn(b);
    sense_touch[side] = pad_bits_to_btn(t);
    /* The menu button reaches the app as a tap when it is let go early; held
     * for a second it recentres instead. */
    const uint64_t now = psvr2_now_us();
    if (menu_hold(side, (b & (PAD_OPTIONS | PAD_CREATE)) != 0, now))
        P.menu_tap_until[side] = now + MENU_TAP_US;
    P.sense_buttons[side] &= ~PSVR2_BTN_MENU;
    if (now < P.menu_tap_until[side])
        P.sense_buttons[side] |= PSVR2_BTN_MENU;
    float trig = intercepted ? 0.0f : (raw[8] > raw[9] ? raw[8] : raw[9]) / 255.0f;
    if ((P.sense_buttons[side] & PSVR2_BTN_TRIGGER) && trig < 0.5f)
        trig = 1.0f;
    P.sense_trigger[side] = trig;
    P.sense_grip[side] = (P.sense_buttons[side] & PSVR2_BTN_GRIP) ? 1.0f : 0.0f;
    P.sense_stick[side][0] = intercepted ? 0.0f : stick_axis(raw[4]);
    P.sense_stick[side][1] = intercepted ? 0.0f : -stick_axis(raw[5]);
}

static void dualsense_read(void)
{
    if (P.ds < 0)
        return;
    pad_data_t d;
    memset(&d, 0, sizeof d);
    if (scePadReadState(P.ds, &d) != 0) {
        P.ds_connected = 0;
        return;
    }
    P.ds_connected = d.connected != 0;
    if (!P.ds_connected || (d.buttons & PAD_INTERCEPTED))
        return;
    /* Options alone held for a second recentres (Options + Create is the
     * host's close combo). */
    menu_hold(2, (d.buttons & PAD_OPTIONS) && !(d.buttons & PAD_CREATE), psvr2_now_us());
    /* Touch pad click recentres the DualSense hands' orientation. */
    if ((d.buttons & PAD_TOUCHPAD) && !(P.ds_data.buttons & PAD_TOUCHPAD))
        P.ds_ref_set = 0;
    P.ds_data = d;
    if (!P.ds_ref_set && P.ds_motion) {
        float q[4] = {d.orientation[0], d.orientation[1], d.orientation[2], d.orientation[3]};
        qnorm(q);
        P.ds_ref_inv[0] = -q[0];
        P.ds_ref_inv[1] = -q[1];
        P.ds_ref_inv[2] = -q[2];
        P.ds_ref_inv[3] = q[3];
        P.ds_ref_set = 1;
    }
}

/* Hosted: the host has the DualSense; its state is only looked at for the
 * recentre hold. */
static void dualsense_peek(void)
{
    if (use_dualsense)
        return;
    if (P.ds_peek < 0) {
        static uint64_t next_try;
        const uint64_t now = psvr2_now_us();
        if (now < next_try)
            return;
        next_try = now + 2000000;
        P.ds_peek = scePadGetHandle(H.user, 0, 0);
        if (P.ds_peek < 0)
            return;
        XRLOG("psvr2: host DualSense pad 0x%x (recentre hold only)", P.ds_peek);
    }
    pad_data_t d;
    memset(&d, 0, sizeof d);
    if (scePadReadState(P.ds_peek, &d) != 0 || !d.connected || (d.buttons & PAD_INTERCEPTED)) {
        P.menu_down_us[2] = 0;
        return;
    }
    menu_hold(2, (d.buttons & PAD_OPTIONS) && !(d.buttons & PAD_CREATE), psvr2_now_us());
}

int psvr2_take_recenter(void)
{
    pthread_mutex_lock(&pad_lock);
    const int r = P.recenter_req;
    P.recenter_req = 0;
    pthread_mutex_unlock(&pad_lock);
    return r;
}

static void triggers_poll(void);

void psvr2_poll(void)
{
    pthread_mutex_lock(&pad_lock);
    pads_open();
    for (int side = 0; side < 2; side++)
        if (P.sense[side] >= 0)
            sense_read(side);
    dualsense_read();
    dualsense_peek();
    /* Injected menu presses (debug server) go through the hold too. */
    for (int side = 0; side < 2; side++)
        if (P.inj_input[side] && P.inj_until[side] > psvr2_now_us())
            menu_hold(side, (P.inj_input_v[side].buttons & PSVR2_BTN_MENU) != 0, psvr2_now_us());
    /* Injected menu presses (debug server) go through the hold too. */
    for (int side = 0; side < 2; side++)
        if (P.inj_input[side] && P.inj_until[side] > psvr2_now_us())
            menu_hold(side, (P.inj_input_v[side].buttons & PSVR2_BTN_MENU) != 0, psvr2_now_us());
    /* Haptics that ran out. */
    const uint64_t now = psvr2_now_us();
    for (int side = 0; side < 2; side++) {
        if (P.haptic_until[side] && now >= P.haptic_until[side]) {
            P.haptic_until[side] = 0;
            if (P.sense[side] >= 0 && P.sense_connected[side])
                pad_rumble(P.sense[side], 1, 0, 0);
            else if (P.ds >= 0)
                pad_rumble(P.ds, 0, 0, 0);
        }
    }
    triggers_poll();
    pthread_mutex_unlock(&pad_lock);
}

static int tracker_get(int handle, uint64_t time_us, uint8_t *res, size_t size)
{
    uint8_t gp[0x20];
    memset(gp, 0, sizeof gp);
    *(uint32_t *)(gp + 0x00) = 0x20;
    *(int32_t *)(gp + 0x04) = handle;
    *(uint64_t *)(gp + 0x10) = time_us;
    memset(res, 0, size);
    return sceVrTracker2GetResult(gp, res);
}

int psvr2_head(uint64_t time_us, psvr2_pose *device, psvr2_pose eyes[2], float lin_vel[3],
               float ang_vel[3])
{
    if (!H.loaded)
        return 0;
    uint8_t res[0x400] __attribute__((aligned(16)));
    if (tracker_get(H.hmd, time_us, res, sizeof res) != 0)
        return 0;
    if (device) {
        memcpy(device->pos, res + 0x60, 12);
        memcpy(device->quat, res + 0x70, 16);
        qnorm(device->quat);
    }
    if (eyes) {
        memcpy(eyes[0].pos, res + 0xa0, 12);
        memcpy(eyes[0].quat, res + 0xb0, 16);
        memcpy(eyes[1].pos, res + 0xe0, 12);
        memcpy(eyes[1].quat, res + 0xf0, 16);
        qnorm(eyes[0].quat);
        qnorm(eyes[1].quat);
    }
    if (lin_vel)
        memcpy(lin_vel, res + 0x80, 12);
    if (ang_vel)
        memcpy(ang_vel, res + 0x90, 12);
    return *(uint32_t *)(res + 4) != 0;
}

/* The DualSense stands in for both hands: they float in front of the head
 * (where a hand holding a pad would be when looking forward), turn with the
 * pad's motion sensors, and the face buttons, sticks and triggers are split
 * between them the way a Sense pair has them. */
static void dualsense_hand(int side, uint64_t time_us, psvr2_controller *c)
{
    psvr2_pose head;
    if (!psvr2_head(time_us, &head, NULL, NULL, NULL))
        return;
    /* Yaw-only body frame from the head. */
    const float *q = head.quat;
    const float fx = -2 * (q[0] * q[2] + q[1] * q[3]), fz = -(1 - 2 * (q[0] * q[0] + q[1] * q[1]));
    const float yaw = atan2f(-fx, -fz);
    const float yq[4] = {0, sinf(yaw * 0.5f), 0, cosf(yaw * 0.5f)};
    /* In front of the face, following where the head looks so a hand can be
     * steered without arm tracking. */
    const float off[3] = {side ? 0.12f : -0.12f, -0.12f, -0.42f};
    float o[3];
    qrot(head.quat, off, o);
    c->pose.pos[0] = head.pos[0] + o[0];
    c->pose.pos[1] = head.pos[1] + o[1];
    c->pose.pos[2] = head.pos[2] + o[2];
    float rel[4] = {0, 0, 0, 1};
    if (P.ds_motion && P.ds_ref_set) {
        float qd[4] = {P.ds_data.orientation[0], P.ds_data.orientation[1], P.ds_data.orientation[2],
                       P.ds_data.orientation[3]};
        qnorm(qd);
        qmul(P.ds_ref_inv, qd, rel);
    }
    /* Pitch the hand down a little, the way a held controller rests. */
    const float tilt[4] = {sinf(-0.35f * 0.5f), 0, 0, cosf(-0.35f * 0.5f)};
    float t[4];
    qmul(yq, rel, t);
    qmul(t, tilt, c->pose.quat);
    qnorm(c->pose.quat);
    c->tracked = 1;
    c->connected = 1;
    c->source = 2;
    const pad_data_t *d = &P.ds_data;
    uint32_t b = d->buttons, o2 = 0;
    if (side) {
        if (d->r2 > 100 || (b & PAD_R2)) o2 |= PSVR2_BTN_TRIGGER;
        if (b & PAD_R1) o2 |= PSVR2_BTN_GRIP;
        if (b & PAD_CROSS) o2 |= PSVR2_BTN_A;
        if (b & PAD_CIRCLE) o2 |= PSVR2_BTN_B;
        if (b & PAD_OPTIONS) o2 |= PSVR2_BTN_MENU;
        if (b & PAD_R3) o2 |= PSVR2_BTN_STICK;
        c->trigger = d->r2 / 255.0f;
        c->stick[0] = stick_axis(d->rx);
        c->stick[1] = -stick_axis(d->ry);
    } else {
        if (d->l2 > 100 || (b & PAD_L2)) o2 |= PSVR2_BTN_TRIGGER;
        if (b & PAD_L1) o2 |= PSVR2_BTN_GRIP;
        if (b & PAD_SQUARE) o2 |= PSVR2_BTN_X;
        if (b & PAD_TRIANGLE) o2 |= PSVR2_BTN_Y;
        if (b & PAD_CREATE) o2 |= PSVR2_BTN_MENU;
        if (b & PAD_L3) o2 |= PSVR2_BTN_STICK;
        c->trigger = d->l2 / 255.0f;
        c->stick[0] = stick_axis(d->lx);
        c->stick[1] = -stick_axis(d->ly);
    }
    c->buttons = o2;
    c->grip = (o2 & PSVR2_BTN_GRIP) ? 1.0f : 0.0f;
}

/* How fast a Sense controller moves and turns, from one tracked pose to the next (as Astro Bot
 * VR2 does for the head). VrTracker2's own fields after the pose aren't known for sure: in the PS4
 * tracker's order (velocity, acceleration, angular velocity, ...) +0x90 would be acceleration.
 * Games throw with this (Half-Life: Alyx). Tracker space, per second; under pad_lock. */
static struct {
    float pos[3], rot[4], v[3], w[3];
    uint64_t t;
    int ok;
} vel_state[2];

static void pose_velocity(int side, uint64_t t, psvr2_controller *c)
{
    typeof(vel_state[0]) *s = &vel_state[side];
    if (s->ok) {
        const int64_t d = (int64_t)(t - s->t);
        if (d < 2000)
            goto out; /* the same moment again, or an earlier one: keep the last estimate */
        const float dt = (float)d * 1e-6f;
        if (dt > 0.1f) {
            memset(s->v, 0, sizeof s->v);
            memset(s->w, 0, sizeof s->w);
        } else {
            /* the turn over dt: q_now q_prev* = (sin(a/2) n, cos(a/2)); small angles */
            const float conj[4] = {-s->rot[0], -s->rot[1], -s->rot[2], s->rot[3]};
            float dq[4];
            qmul(c->pose.quat, conj, dq);
            const float sgn = dq[3] < 0 ? -1.0f : 1.0f;
            for (int i = 0; i < 3; i++) {
                s->v[i] += ((c->pose.pos[i] - s->pos[i]) / dt - s->v[i]) * 0.5f;
                s->w[i] += (2.0f * sgn * dq[i] / dt - s->w[i]) * 0.5f;
            }
        }
    }
    memcpy(s->pos, c->pose.pos, sizeof s->pos);
    memcpy(s->rot, c->pose.quat, sizeof s->rot);
    s->t = t;
    s->ok = 1;
out:
    memcpy(c->lin_vel, s->v, sizeof c->lin_vel);
    memcpy(c->ang_vel, s->w, sizeof c->ang_vel);
}

void psvr2_get_controller(int side, uint64_t time_us, psvr2_controller *c)
{
    memset(c, 0, sizeof *c);
    c->pose.quat[3] = 1;
    if (side < 0 || side > 1)
        return;
    pthread_mutex_lock(&pad_lock);
    if (P.sense[side] >= 0 && P.sense_connected[side]) {
        c->connected = 1;
        c->source = 1;
        c->buttons = P.sense_buttons[side];
        c->touches = sense_touch[side];
        c->trigger = P.sense_trigger[side];
        c->grip = P.sense_grip[side];
        c->stick[0] = P.sense_stick[side][0];
        c->stick[1] = P.sense_stick[side][1];
        if (P.registered[side] && H.loaded) {
            uint8_t res[0x400] __attribute__((aligned(16)));
            if (tracker_get(P.sense[side], time_us, res, sizeof res) == 0 && *(uint32_t *)(res + 4)) {
                memcpy(c->pose.pos, res + 0x60, 12);
                memcpy(c->pose.quat, res + 0x70, 16);
                qnorm(c->pose.quat);
                c->tracked = 1;
                pose_velocity(side, time_us, c);
            }
            static uint64_t next_log[2];
            const uint64_t now = psvr2_now_us();
            if (now >= next_log[side]) {
                next_log[side] = now + 5000000;
                XRLOG("psvr2: sense %d tracked %d pos %.3f %.3f %.3f q %.3f %.3f %.3f %.3f btn %x trig %.2f",
                      side, c->tracked, (double)c->pose.pos[0], (double)c->pose.pos[1],
                      (double)c->pose.pos[2], (double)c->pose.quat[0], (double)c->pose.quat[1],
                      (double)c->pose.quat[2], (double)c->pose.quat[3], c->buttons,
                      (double)c->trigger);
            }
        }
    } else if (P.ds >= 0 && P.ds_connected) {
        dualsense_hand(side, time_us, c);
    }
    /* Test injection overrides. */
    const uint64_t now = psvr2_now_us();
    if (P.inj_until[side] && now < P.inj_until[side]) {
        if (P.inj_pose[side]) {
            c->pose = P.inj_pose_v[side];
            c->tracked = 1;
            c->connected = 1;
            memset(c->lin_vel, 0, sizeof c->lin_vel);
            memset(c->ang_vel, 0, sizeof c->ang_vel);
        }
        if (P.inj_input[side]) {
            const psvr2_controller *i = &P.inj_input_v[side];
            c->buttons = i->buttons;
            c->touches = i->touches;
            c->trigger = i->trigger;
            c->grip = i->grip;
            c->stick[0] = i->stick[0];
            c->stick[1] = i->stick[1];
            c->connected = 1;
        }
        if (!c->source || P.inj_pose[side])
            c->source = 3;
    }
    pthread_mutex_unlock(&pad_lock);
}

void psvr2_inject(int side, int pose_set, const psvr2_pose *pose, int input_set,
                  const psvr2_controller *input, uint64_t until_us)
{
    if (side < 0 || side > 1)
        return;
    pthread_mutex_lock(&pad_lock);
    if (pose_set >= 0) {
        P.inj_pose[side] = pose_set && pose;
        if (pose)
            P.inj_pose_v[side] = *pose;
    }
    if (input_set >= 0) {
        P.inj_input[side] = input_set && input;
        if (input)
            P.inj_input_v[side] = *input;
    }
    P.inj_until[side] = until_us;
    pthread_mutex_unlock(&pad_lock);
}

/* -- Sense rumble ----------------------------------------------------------------- */
/* libScePad's DualSense setters refuse a Sense controller: pad types 3/4 fail the handle check in
 * scePadSetVibrationMode and scePadSetVibration with 0x80920003 (sub_13e0 on 13.60; Kayak 231 logged
 * "vibration mode (pad 0x3720701) -> 0x80920003" and no Sense ever rumbled). The Sense's own setters:
 * scePadVrControllerSetVibrationMode (Wf6-PNCyY20, 13.60 +0x100b0, types 3/4 only) and
 * scePadVrControllerSetVibration (DGCwN1Lmmys, +0x5610: handle and one intensity byte, an ioctl to
 * the controller). The SDK stubs have neither, so they are found as the trigger effect is. */
static int (*vr_vib_mode)(int handle, int mode);
static int (*vr_vib)(int handle, uint8_t intensity);

static void *pad_vr_export(int h, const char *name, const char *nid, uint32_t off1360, int *dlsym_rc,
                           uintptr_t *base_out)
{
    void *fn = NULL;
    *dlsym_rc = sceKernelDlsym(h, name, &fn);
    if (*dlsym_rc == 0 && fn)
        return fn;
    static const export_anchor_t anchor = {0x1f650, "scePadCtrlpSyncInit Error : cannot open [%08x]"};
    module_info_t mi;
    memset(&mi, 0, sizeof mi);
    mi.size = sizeof mi;
    const uintptr_t base = module_base(h, "libScePad.sprx", &mi);
    *base_out = base;
    if (!base)
        return NULL;
    uintptr_t off = anchor_matches(&mi, base, &anchor) ? off1360 : 0;
    if (!off) {
        off = exports_file_offset("libScePad.sprx", name);
        if (!off)
            off = exports_file_offset("libScePad.sprx", nid);
        if (off && !in_code(&mi, base + off))
            off = 0;
    }
    return off ? (void *)(base + off) : NULL;
}

static void bind_vr_vibration(void)
{
    static int bound;
    if (bound)
        return;
    bound = 1;
    int32_t handles[256];
    size_t n = 0;
    int rc_mode = -1, rc_vib = -1, found = 0;
    uintptr_t base = 0;
    if (sceKernelGetModuleList(handles, 256, &n) != 0)
        n = 0;
    for (size_t i = 0; i < n && !found; i++) {
        module_info_t mi;
        memset(&mi, 0, sizeof mi);
        mi.size = sizeof mi;
        if (sceKernelGetModuleInfo(handles[i], &mi) != 0 || strcmp(mi.name, "libScePad.sprx"))
            continue;
        found = 1;
        vr_vib_mode = pad_vr_export(handles[i], "scePadVrControllerSetVibrationMode", "Wf6-PNCyY20", 0x100b0,
                                    &rc_mode, &base);
        vr_vib = pad_vr_export(handles[i], "scePadVrControllerSetVibration", "DGCwN1Lmmys", 0x5610, &rc_vib, &base);
    }
    XRLOG("psvr2: Sense vibration setters: mode %s, intensity %s (libScePad %s, dlsym 0x%08x 0x%08x, base %p)",
          vr_vib_mode ? "found" : "not found", vr_vib ? "found" : "not found", found ? "loaded" : "missing",
          rc_mode, rc_vib, (void *)base);
}

/* sets a Sense controller's motor (side 0/1) or the DualSense's; under pad_lock */
static int pad_rumble(int h, int sense, uint8_t left, uint8_t right)
{
    if (!sense) {
        const uint8_t m[2] = {left, right};
        return scePadSetVibration(h, m);
    }
    bind_vr_vibration();
    return vr_vib ? vr_vib(h, left > right ? left : right) : -1;
}

void psvr2_haptic(int side, float amplitude, float seconds)
{
    if (side < 0 || side > 1)
        return;
    pthread_mutex_lock(&pad_lock);
    int h = -1, slot = 2;
    if (P.sense[side] >= 0 && P.sense_connected[side]) {
        h = P.sense[side];
        slot = side;
    } else if (P.ds >= 0 && P.ds_connected) {
        h = P.ds;
    }
    if (h >= 0) {
        const int sense = slot < 2;
        if (!P.vib_mode_set[slot]) {
            int rc = -1;
            if (sense) {
                bind_vr_vibration();
                rc = vr_vib_mode ? vr_vib_mode(h, 2) : -1;
            } else {
                rc = scePadSetVibrationMode(h, 2);
            }
            XRLOG("psvr2: vibration mode (%s pad 0x%x) -> 0x%08x", sense ? "Sense" : "DualSense", h, rc);
            P.vib_mode_set[slot] = 1;
        }
        float a = amplitude < 0 ? 0 : amplitude > 1 ? 1 : amplitude;
        const uint8_t v = (uint8_t)(a * 255);
        int rc;
        if (h == P.ds) /* A DualSense rumbles on the side the hand is. */
            rc = pad_rumble(h, 0, side ? 0 : v, side ? v : 0);
        else
            rc = pad_rumble(h, sense, v, v);
        static unsigned nfail, nok;
        if (rc != 0 && nfail < 4) {
            nfail++;
            XRLOG("psvr2: vibration (%s pad 0x%x, %u) -> 0x%08x", sense ? "Sense" : "DualSense", h, v, rc);
        } else if (rc == 0 && v && nok < 2) {
            nok++;
            XRLOG("psvr2: vibration (%s pad 0x%x, %u, %.0f ms) -> 0", sense ? "Sense" : "DualSense", h, v,
                  seconds * 1000.0f);
        }
        if (seconds <= 0)
            seconds = 0.05f;
        P.haptic_until[side] = a > 0 ? psvr2_now_us() + (uint64_t)(seconds * 1e6f) : 0;
        P.haptic_amp[side] = a;
    }
    pthread_mutex_unlock(&pad_lock);
}

/* -- adaptive triggers ----------------------------------------------------------- */
/* libScePad's trigger effect for the Sense controllers (13.60: export 0xa730, 12.40: 0x84c0,
 * other firmware from the exports file):
 * the two controllers' handles and the effects, laid out as the DualSense's
 * scePadSetTriggerEffect takes them (PSVR2Toolkit's ScePadTriggerEffectCommand).
 * It is looked up in the loaded libScePad; without it there are no trigger
 * effects. */
typedef struct {
    uint32_t mode;    /* 0 off, 1 feedback, 2 weapon, 3 vibration, ... */
    uint32_t pad;
    uint8_t data[48]; /* weapon: start position (2-7), end position (start+1-8), strength (0-8) */
} trigger_command_t;
typedef struct {
    uint8_t mask;     /* 1 left, 2 right */
    uint8_t pad[7];
    trigger_command_t command[2];
} trigger_effect_t;
_Static_assert(sizeof(trigger_effect_t) == 120, "libScePad's trigger effect layout");
enum { TRIGGER_OFF = 0, TRIGGER_WEAPON = 2 };

static int (*set_vr_trigger_effects)(int left, int right, const trigger_effect_t *effect);

static int bind_trigger_effects(void)
{
    static int bound;
    if (bound)
        return set_vr_trigger_effects ? 0 : -1;
    bound = 1;
    int32_t handles[256];
    size_t n = 0;
    if (sceKernelGetModuleList(handles, 256, &n) != 0)
        return -1;
    /* By name, or else at its 13.60 or 12.40 offset once a string of that firmware's
     * libScePad is where it should be (its code is execute-only: never read it), or
     * else from gv-tool's exports file on other firmware. 12.40's offset and anchor are
     * milpugeda's (HLA-PSVR2 pull request 5): its read-only data holds only three
     * strings, and "/dev/ctrlp_sync" at 0x1b630 is unique there. */
    static const struct { export_anchor_t anchor; uint32_t offset; } known[] = {
        {{0x1f650, "scePadCtrlpSyncInit Error : cannot open [%08x]"}, 0xa730}, /* 13.60 */
        {{0x1b630, "/dev/ctrlp_sync"}, 0x84c0},                                /* 12.40 */
    };
    int found = 0, rc = -1;
    uintptr_t base = 0, bound_at = 0;
    for (size_t i = 0; i < n && !found; i++) {
        module_info_t mi;
        memset(&mi, 0, sizeof mi);
        mi.size = sizeof mi;
        if (sceKernelGetModuleInfo(handles[i], &mi) != 0 || strcmp(mi.name, "libScePad.sprx"))
            continue;
        found = 1;
        rc = sceKernelDlsym(handles[i], "scePadVrControllerSetTriggerEffects", (void **)&set_vr_trigger_effects);
        if (rc || !set_vr_trigger_effects) {
            set_vr_trigger_effects = NULL;
            base = module_base(handles[i], "libScePad.sprx", &mi);
            uintptr_t off = 0;
            for (size_t k = 0; base && !off && k < COUNT(known); k++)
                if (anchor_matches(&mi, base, &known[k].anchor) && in_code(&mi, base + known[k].offset))
                    off = known[k].offset;
            if (base && !off) {
                off = exports_file_offset("libScePad.sprx", "scePadVrControllerSetTriggerEffects");
                if (off && !in_code(&mi, base + off))
                    off = 0;
            }
            if (off)
                set_vr_trigger_effects = (void *)(base + off);
            bound_at = off;
        }
    }
    XRLOG("psvr2: Sense trigger effects %s (libScePad %s, dlsym 0x%08x, base %p, offset 0x%lx)",
          set_vr_trigger_effects ? "available" : "not found", found ? "loaded" : "missing", rc, (void *)base,
          (unsigned long)bound_at);
    return set_vr_trigger_effects ? 0 : -1;
}

/* What each trigger should do, and the controller each was last given to
 * (-1: not yet, or the controller went away). Under pad_lock. */
static struct {
    int used;
    trigger_command_t want[2];
    int dirty[2];
    int given[2];
    int last_rc;
    unsigned logged;
} T = {.given = {-1, -1}};

/* Gives both controllers what they should do when it changed or a controller
 * came back; libScePad takes the pair, so both have to be there. A failed call
 * isn't retried until the next change. */
static void triggers_apply(void)
{
    const int left = P.sense_connected[0] ? P.sense[0] : -1, right = P.sense_connected[1] ? P.sense[1] : -1;
    if (left < 0 || right < 0) {
        T.given[0] = T.given[1] = -1;
        return;
    }
    int mask = 0;
    for (int side = 0; side < 2; side++)
        if (T.dirty[side] || T.given[side] != (side ? right : left))
            mask |= 1 << side;
    if (!mask || bind_trigger_effects() != 0)
        return;
    trigger_effect_t e;
    memset(&e, 0, sizeof e);
    e.mask = (uint8_t)mask;
    e.command[0] = T.want[0];
    e.command[1] = T.want[1];
    const int rc = set_vr_trigger_effects(left, right, &e);
    if (T.logged < 60 || rc != T.last_rc) {
        T.logged++;
        XRLOG("psvr2: triggers %s%s left %u %u %u %u right %u %u %u %u -> 0x%08x", mask & 1 ? "L" : "",
              mask & 2 ? "R" : "", T.want[0].mode, T.want[0].data[0], T.want[0].data[1], T.want[0].data[2],
              T.want[1].mode, T.want[1].data[0], T.want[1].data[1], T.want[1].data[2], rc);
    }
    T.last_rc = rc;
    for (int side = 0; side < 2; side++)
        if (mask & (1 << side)) {
            T.dirty[side] = 0;
            T.given[side] = side ? right : left;
        }
}

/* Under pad_lock, once a frame: effects for controllers that came (back)
 * since they were set. */
static void triggers_poll(void)
{
    if (T.used)
        triggers_apply();
}

void psvr2_set_trigger(int side, uint32_t mode, const uint8_t data[4])
{
    if (side < 0 || side > 1)
        return;
    trigger_command_t c;
    memset(&c, 0, sizeof c);
    c.mode = mode;
    memcpy(c.data, data, 4);
    pthread_mutex_lock(&pad_lock);
    T.used = 1;
    if (memcmp(&c, &T.want[side], sizeof c)) {
        T.want[side] = c;
        T.dirty[side] = 1;
    }
    triggers_apply();
    pthread_mutex_unlock(&pad_lock);
}

/* hands: bit 0 left, bit 1 right. A hand that holds a weapon gets a trigger
 * that resists from about 45% of its travel and gives way with a click at
 * about 70%, as a gun's does; the other hand's trigger is free. */
void psvr2_set_weapon_triggers(int hands)
{
    static const uint8_t gun[4] = {4, 6, 5, 0}, none[4] = {0, 0, 0, 0}; /* start, end, strength */
    for (int side = 0; side < 2; side++)
        psvr2_set_trigger(side, hands & (1 << side) ? TRIGGER_WEAPON : TRIGGER_OFF, hands & (1 << side) ? gun : none);
}

void psvr2_headset_haptic(float amplitude)
{
    if (H.loaded && sceHmd2SetVibration) {
        int s = (int)(amplitude * 25.0f + 0.5f);
        sceHmd2SetVibration(H.hmd, s < 0 ? 0 : s > 25 ? 25 : s);
    }
}

/* -- headset ---------------------------------------------------------------- */
void psvr2_set_defer_output(int on)
{
    defer_output = on;
}

int psvr2_init_output(void)
{
    H.want_output = 1;
    return psvr2_init();
}

static int psvr2_init_impl(void);

/* Hosted: the headset is started from a fresh thread with every signal
 * unblocked and default scheduling. Hmd2's reprojection thread, created by
 * sceHmd2ReprojectionInitialize, otherwise inherits the calling thread's
 * state, which in prospero-win is a Wine thread's. */
static int clean_init_thread;
void psvr2_set_clean_init_thread(int on)
{
    clean_init_thread = on;
}

static void log_thread_state(const char *who)
{
    sigset_t mask;
    int policy = -1;
    struct sched_param sp = {0};
    memset(&mask, 0, sizeof mask);
    pthread_sigmask(SIG_BLOCK, NULL, &mask);
    pthread_getschedparam(pthread_self(), &policy, &sp);
    const uint32_t *m = (const uint32_t *)&mask;
    XRLOG("psvr2: %s thread: blocked signals %08x %08x %08x %08x, policy %d, priority %d", who, m[0], m[1], m[2], m[3],
          policy, sp.sched_priority);
}

static void *init_thread_main(void *arg)
{
    sigset_t all;
    sigfillset(&all);
    pthread_sigmask(SIG_UNBLOCK, &all, NULL);
    log_thread_state("init");
    *(int *)arg = psvr2_init_impl();
    return NULL;
}

int psvr2_init(void)
{
    if (!clean_init_thread || H.loaded || H.failed || (H.opened && defer_output && !H.want_output))
        return psvr2_init_impl();
    log_thread_state("caller");
    int rc = -1;
    pthread_t t;
    if (pthread_create(&t, NULL, init_thread_main, &rc) != 0)
        return psvr2_init_impl();
    pthread_join(t, NULL);
    return rc;
}

static int psvr2_init_impl(void)
{
    if (H.loaded)
        return 0;
    if (H.failed)
        return -1;
    if (H.opened)
        goto output;
    H.failed = 1;
    sceUserServiceInitialize(NULL);
    int32_t user = -1;
    sceUserServiceGetInitialUser(&user);
    H.user = user;

    int h = load_sysmodule(SYSMODULE_HMD2, "libSceHmd2");
    if (h < 0 || resolve_module(h, "libSceHmd2.sprx", kHmd2Slots, COUNT(kHmd2Slots),
                                klibSceHmd2Tables, COUNT(klibSceHmd2Tables))) {
        psvr2_tell(h < 0 ? "libSceHmd2" : "libSceHmd2 functions", h < 0 ? h : 0, TELL_REPORT);
        return -1;
    }
    h = load_sysmodule(SYSMODULE_VRTRACKER2, "libSceVrTracker2");
    if (h < 0 || resolve_module(h, "libSceVrTracker2.sprx", kVrTracker2Slots,
                                COUNT(kVrTracker2Slots), klibSceVrTracker2Tables,
                                COUNT(klibSceVrTracker2Tables))) {
        psvr2_tell(h < 0 ? "libSceVrTracker2" : "libSceVrTracker2 functions", h < 0 ? h : 0, TELL_REPORT);
        return -1;
    }

    uint8_t init[16] = {0};
    int rc = sceHmd2Initialize(init);
    XRLOG("psvr2: sceHmd2Initialize -> 0x%08x", rc);
    if (rc != 0) {
        psvr2_tell("sceHmd2Initialize", rc, TELL_HEADSET);
        return -1;
    }
    H.hmd_initialized = 1;
    dump_identity();
    float fov[16];
    memset(fov, 0, sizeof fov);
    rc = sceHmd2GetFieldOfViewWithoutHandle(fov);
    memcpy(H.fov, fov, sizeof H.fov);
    XRLOG("psvr2: FOV rc 0x%08x out %.4f in %.4f top %.4f bottom %.4f", rc, (double)fov[0],
          (double)fov[1], (double)fov[2], (double)fov[3]);
    if (rc != 0 || H.fov[0] <= 0.0f || H.fov[2] <= 0.0f) {
        psvr2_tell("sceHmd2GetFieldOfView", rc, TELL_HEADSET);
        return -1;
    }
    H.hmd = sceHmd2Open(H.user, 0, 0, NULL);
    H.info.panel_w = 2000;
    H.info.panel_h = 2040;
    H.info.ipd = 0.064f;
    memcpy(H.info.fov, H.fov, sizeof H.fov);
    read_device_info();
    XRLOG("psvr2: sceHmd2Open -> 0x%08x; status %d worn %d ipd %.3f", H.hmd, H.info.status,
          H.info.worn, (double)H.info.ipd);
    if (H.hmd < 0) {
        psvr2_tell("sceHmd2Open", H.hmd, TELL_HEADSET);
        return -1;
    }
    H.opened = 1;
    H.failed = 0;
output:
    if (defer_output && !H.want_output)
        return 0;
    if (H.output_failed)
        return -1;
    H.output_failed = 1;

    uint32_t qparam[8] = {0x20, 0};
    uint32_t qout[8] = {0};
    if (sceVrTracker2QueryMemory(qparam, qout) != 0)
        return -1;
    if (!H.tracker_work)
        H.tracker_work = direct_alloc(qout[1], qout[2]);
    if (!H.tracker_work)
        return -1;
    uint8_t ip[0x48];
    memset(ip, 0, sizeof ip);
    *(uint32_t *)(ip + 0x00) = 0x48;
    *(int32_t *)(ip + 0x08) = 700;
    *(int32_t *)(ip + 0x0c) = 700;
    *(uint64_t *)(ip + 0x10) = 0x1f00;
    *(uint64_t *)(ip + 0x18) = 0x1f00;
    *(void **)(ip + 0x20) = H.tracker_work;
    *(uint32_t *)(ip + 0x28) = qout[1];
    *(uint32_t *)(ip + 0x2c) = qout[2];
    rc = sceVrTracker2Initialize(ip);
    H.tracker_initialized = rc == 0;
    int rc2 = rc == 0 ? sceVrTracker2RegisterDevice(0, H.hmd) : -1;
    H.tracker_registered = rc2 == 0;
    XRLOG("psvr2: tracker init 0x%08x register 0x%08x", rc, rc2);
    if (rc != 0 || rc2 != 0) {
        psvr2_tell(rc ? "sceVrTracker2Initialize" : "sceVrTracker2RegisterDevice", rc ? rc : rc2, TELL_HEADSET);
        return -1;
    }

    SizeAlign wb = sceHmd2ReprojectionQueryBufferSizeAlign();
    SizeAlign db = sceHmd2ReprojectionQueryDisplayBufferSizeAlign();
    if (!H.reproj_work)
        H.reproj_work = direct_alloc(wb.size, 0x10000);
    if (!H.reproj_disp)
        H.reproj_disp = direct_alloc(db.size, 0x10000);
    if (!H.reproj_work || !H.reproj_disp)
        return -1;
    uint8_t rp[72];
    memset(rp, 0, sizeof rp);
    *(void **)(rp + 0x00) = H.reproj_work;
    *(void **)(rp + 0x08) = H.reproj_disp;
    *(uint32_t *)(rp + 0x10) = 256;
    *(int32_t *)(rp + 0x14) = 0x1f00;
    *(uint32_t *)(rp + 0x18) = 1;
    /* No positional reprojection (sceHmd2ReprojectionSetAllowPositionalReprojection(2), as
     * Horizon Call of the Mountain sets it): CotM's frames carry the depth and foveation tables it
     * works from, and ours carry neither. With it the headset showed only red with a correct TV
     * mirror (Half-Life: Alyx at half rate, RE7 at 120 Hz, 11-01; Rush of Blood on the PS4 track,
     * 10-31), though one RE7 run had it on and looked right, so a clean run doesn't clear it. */
    rc = sceHmd2ReprojectionInitialize(rp, NULL);
    XRLOG("psvr2: sceHmd2ReprojectionInitialize -> 0x%08x", rc);
    if (rc != 0) {
        psvr2_tell("sceHmd2ReprojectionInitialize", rc, TELL_REPORT);
        return -1;
    }
    H.loaded = 1;
    H.output_failed = 0;
    pthread_mutex_lock(&pad_lock);
    pads_open();
    pthread_mutex_unlock(&pad_lock);
    return 0;
}

void psvr2_get_info(psvr2_info *info)
{
    if (H.loaded)
        read_device_info();
    *info = H.info;
}

int psvr2_worn(void)
{
    if (!H.loaded)
        return 0;
    const uint64_t now = psvr2_now_us();
    if (now - H.worn_checked >= 500000) {
        H.worn_checked = now;
        int was = H.info.worn;
        read_device_info();
        if (was != H.info.worn)
            XRLOG("psvr2: headset %s", H.info.worn ? "put on" : "taken off");
    }
    return H.info.worn;
}

/* Eye gaze (PS VR2's eye tracker through Hmd2). The result is a 0x38-byte block:
 * +4 status (0 ok), +0x10 valid (1), +0x14 a unit gaze direction in headset
 * space (-z forward), the way Horizon Call of the Mountain reads it (PS5VR
 * app_vr_gaze_dir, hardware-validated). The pose is the headset origin looking
 * along that direction, in headset space: a session turns it to tracker space
 * with the head pose, as OpenXR locates the gaze space against its base.
 * Tracking runs only after InternalGazeStart (an ioctl on the Hmd2 device);
 * without it the result stays "not valid". Returns 1 when the pose is valid. */
int psvr2_gaze(psvr2_pose *out)
{
    if (!H.loaded || !sceHmd2GazeGetResult || H.hmd < 0)
        return 0;
    static int started;
    if (!started && sceHmd2InternalGazeStart) {
        started = 1;
        XRLOG("psvr2: eye tracking start -> 0x%08x", sceHmd2InternalGazeStart());
    }
    uint8_t param[0x10], r[0x38];
    memset(param, 0, sizeof param);
    *(uint32_t *)param = sizeof param;
    *(int32_t *)(param + 4) = H.hmd;
    memset(r, 0, sizeof r);
    *(uint32_t *)r = sizeof r;
    if (sceHmd2GazeGetResult(param, r) != 0 || *(int32_t *)(r + 4) != 0 || *(uint32_t *)(r + 0x10) != 1)
        return 0;
    const float *d = (const float *)(r + 0x14); /* unit, -z forward, headset space */
    /* The quaternion turning -z onto d: (cross(-z, d), 1 + dot(-z, d)), normalised. */
    const float zx = 0, zy = 0, zz = -1.0f;
    float ax = zy * d[2] - zz * d[1];
    float ay = zz * d[0] - zx * d[2];
    float az = zx * d[1] - zy * d[0];
    float dot = zx * d[0] + zy * d[1] + zz * d[2]; /* = -d[2] */
    float s = sqrtf((1.0f + dot) * 2.0f);
    out->pos[0] = out->pos[1] = out->pos[2] = 0.0f; /* headset origin */
    if (s < 1e-6f) {
        /* d points at +z (opposite -z): 180 deg about y. */
        out->quat[0] = 0;
        out->quat[1] = 1;
        out->quat[2] = 0;
        out->quat[3] = 0;
        return 1;
    }
    float inv = 1.0f / s;
    out->quat[0] = ax * inv;
    out->quat[1] = ay * inv;
    out->quat[2] = az * inv;
    out->quat[3] = s * 0.5f;
    return 1;
}

/* Hand tracking (PS VR2 cameras through libSceVrHand). The joint read is PS5VR's
 * hardware-validated one (vr_system.c, AstroBotVR2 1.0.7): result 0xdb8 bytes,
 * +0xc state (3 tracked), then 26 joints of 0x60 (pos +0, quat +0x10, radius
 * +0x30) in OpenXR order. Two console pieces are NOT wired here and are done by
 * the title glue in a supervised console session (see WIVRN-CLIENT-HANDOFF C7):
 *   - raising the title's credentials so /dev/hmd2_image and _leddet open
 *     (0x8a721003 otherwise), through the payload loader as the Ghost VR app does;
 *   - pointing libSceVrHand's model import at /system/priv/vr_hand_ro.
 * Until that glue runs, psvr2_hands_enable finds the cameras refused and reports
 * the hand unsupported, so the title still runs with gaze, video, sound and
 * controllers. gv_hand_cameras_ready() is the title's hook: it asks its helper
 * payload to raise the title's credentials and returns 1 once /dev/hmd2_image
 * opens. The title overrides this (a strong definition wins over this weak 0). */
__attribute__((weak)) int gv_hand_cameras_ready(void) { return 0; }

static int g_hands_on, g_hands_ok;
static void *g_hand_work;
static uint8_t g_hand_res[0xdb8];
static int (*sceVrHandInitialize)(const void *);
static int (*sceVrHandFinalize)(void);
static int (*sceVrHandStart)(const void *);
static int (*sceVrHandStop)(void);
static int (*sceVrHandGetResult)(const void *, void *);
static slot_t kVrHandSlots[] = {
    {"sceVrHandInitialize", (void **)&sceVrHandInitialize},
    {"sceVrHandFinalize", (void **)&sceVrHandFinalize},
    {"sceVrHandStart", (void **)&sceVrHandStart},
    {"sceVrHandStop", (void **)&sceVrHandStop},
    {"sceVrHandGetResult", (void **)&sceVrHandGetResult},
};
static uintptr_t g_hand_base, g_tracker_base;
static int g_hand_fw;

/* libSceVrHand asks the system tracker daemon for the hand models through a
 * libSceVrTracker2 export (a "set VR model" command over a domain socket the
 * daemon does not answer us). So its import of that export is redirected here,
 * which serves the files from /system/priv/vr_hand_ro: model 2v+1 is
 * handCenter_%02d, 2v+2 handPose_%02d. Same contract as the export: 0 and *out
 * = bytes, 0x8a740006 bad args, 0x8a740009 buffer too small. PS5VR's code. */
static int gv_hand_model_read(int id, void *buf, size_t size, size_t *out)
{
    if (!buf || !out || id < 1 || id > 6 || !((0x46 >> id) & 1))
        return (int)0x8a740006;
    char path[96];
    snprintf(path, sizeof path, "/system/priv/vr_hand_ro/%s_%02d.bin",
             (id & 1) ? "handCenter" : "handPose", (id & 1) ? (id - 1) / 2 : (id - 2) / 2);
    const int fd = open(path, O_RDONLY);
    if (fd < 0) {
        XRLOG("psvr2: hand model %s: open failed (0x%x)", path, errno);
        *out = 0;
        return 0;
    }
    size_t got = 0;
    for (;;) {
        const ssize_t n = read(fd, (uint8_t *)buf + got, size - got);
        if (n <= 0)
            break;
        got += (size_t)n;
        if (got == size)
            break;
    }
    char more;
    const int too_big = got == size && read(fd, &more, 1) == 1;
    close(fd);
    *out = got;
    return too_big ? (int)0x8a740009 : 0;
}

/* The GOT slot in libSceVrHand that holds VrTracker2's model export, and the
 * export's offset, per firmware (PS5VR's). */
static const struct { int fw; uint32_t got, fn; } kModelImport[] = {
    {1360, 0xf4e68, 0x7f50},
    {1240, 0xf4e68, 0x7f70},
};
static int g_models_patched;

static void gv_hand_patch_models(void)
{
    if (g_models_patched || !g_hand_base || !g_tracker_base)
        return;
    size_t k = 0;
    while (k < COUNT(kModelImport) && kModelImport[k].fw != g_hand_fw)
        k++;
    if (k == COUNT(kModelImport)) {
        XRLOG("psvr2: hand model import: no offsets for firmware %d, left alone", g_hand_fw);
        return;
    }
    uint64_t *slot = (uint64_t *)(g_hand_base + kModelImport[k].got);
    if (*slot != g_tracker_base + kModelImport[k].fn) {
        XRLOG("psvr2: hand model import not as expected (%p), left alone", (void *)*slot);
        return;
    }
    void *page = (void *)((uintptr_t)slot & ~(uintptr_t)0x3fff);
    if (mprotect(page, 0x4000, PROT_READ | PROT_WRITE) != 0) {
        XRLOG("psvr2: hand model import: mprotect failed (0x%x)", errno);
        return;
    }
    *slot = (uint64_t)(uintptr_t)gv_hand_model_read;
    mprotect(page, 0x4000, PROT_READ);
    g_models_patched = 1;
    XRLOG("psvr2: hand models read from /system/priv/vr_hand_ro");
}

/* Load libSceVrHand (sandbox path), resolve its five functions and record its
 * base and the loaded VrTracker2's base for the model-import patch. */
static int gv_hand_load(void)
{
    if (sceVrHandGetResult)
        return 0;
    int h = load_sysmodule(0 /* not a sysmodule id */, "libSceVrHand");
    if (h < 0 || resolve_module(h, "libSceVrHand.sprx", kVrHandSlots, COUNT(kVrHandSlots),
                                klibSceVrHandTables, COUNT(klibSceVrHandTables))) {
        XRLOG("psvr2: libSceVrHand not available on this firmware");
        return -1;
    }
    module_info_t mi;
    memset(&mi, 0, sizeof mi);
    g_hand_base = module_base(h, "libSceVrHand.sprx", &mi);
    g_hand_fw = 0;
    for (size_t k = 0; g_hand_base && k < COUNT(klibSceVrHandTables); k++)
        if (anchor_matches(&mi, g_hand_base, klibSceVrHandTables[k].anchor))
            g_hand_fw = klibSceVrHandTables[k].fw;
    /* VrTracker2's base, loaded already (psvr2_init). Find it by name. */
    int32_t handles[256];
    size_t count = 0;
    g_tracker_base = 0;
    if (sceKernelGetModuleList(handles, 256, &count) == 0)
        for (size_t i = 0; i < count && !g_tracker_base; i++) {
            module_info_t ti;
            memset(&ti, 0, sizeof ti);
            ti.size = sizeof ti;
            if (sceKernelGetModuleInfo(handles[i], &ti) == 0 && !strcmp(ti.name, "libSceVrTracker2.sprx"))
                g_tracker_base = (uintptr_t)ti.seg[0].addr;
        }
    return sceVrHandInitialize && sceVrHandStart && sceVrHandGetResult ? 0 : -1;
}

int psvr2_hands_supported(void)
{
    /* The library is known only for 13.60 and 12.40; a load+resolve tells us. */
    static int checked, ok;
    if (!checked) {
        checked = 1;
        ok = gv_hand_load() == 0;
    }
    return ok;
}

void psvr2_hands_enable(int on)
{
    if (on == g_hands_on)
        return;
    g_hands_on = on;
    if (!on) {
        if (g_hands_ok && sceVrHandStop) {
            sceVrHandStop();
            sceVrHandFinalize();
        }
        g_hands_ok = 0;
        return;
    }
    if (!psvr2_hands_supported())
        return;
    /* The title (its helper payload) raises the credentials the cameras need. */
    if (!gv_hand_cameras_ready()) {
        XRLOG("psvr2: hand tracking asked for, but the cameras are not open (helper not run?)");
        return;
    }
    gv_hand_patch_models();
    if (!g_hand_work)
        g_hand_work = direct_alloc(64u << 20, 0x10000); /* >= 60 MB work memory */
    uint8_t p[0x40];
    memset(p, 0, sizeof p);
    *(uint32_t *)(p + 0x00) = sizeof p;
    *(int32_t *)(p + 0x08) = 0x1c0;   /* priority */
    *(uint64_t *)(p + 0x10) = 0x1fff; /* CPUs */
    *(uint32_t *)(p + 0x18) = 5;      /* compute pipe */
    *(uint32_t *)(p + 0x1c) = 0;      /* queue */
    *(void **)(p + 0x20) = g_hand_work;
    *(uint32_t *)(p + 0x28) = 64u << 20;
    *(uint32_t *)(p + 0x2c) = 0x10000;
    int rc = sceVrHandInitialize(p);
    XRLOG("psvr2: sceVrHandInitialize -> 0x%08x", rc);
    if (rc == 0) {
        uint8_t sp[0x10];
        memset(sp, 0, sizeof sp);
        *(uint32_t *)sp = sizeof sp;
        rc = sceVrHandStart(sp);
        XRLOG("psvr2: sceVrHandStart -> 0x%08x", rc);
        if (rc == 0)
            g_hands_ok = 1;
        else
            sceVrHandFinalize();
    }
}

int psvr2_hand_joints(int side, uint64_t time_us, psvr2_hand *out)
{
    if (!g_hands_ok || !sceVrHandGetResult)
        return 0;
    uint8_t gp[0x30];
    memset(gp, 0, sizeof gp);
    *(uint32_t *)gp = sizeof gp;
    *(uint64_t *)(gp + 0x10) = time_us ? time_us : psvr2_now_us();
    *(uint32_t *)(gp + 0x18) = (uint32_t)(side ? 1 : 0);
    if (sceVrHandGetResult(gp, g_hand_res) != 0)
        return 0;
    if (*(const uint32_t *)(g_hand_res + 0xc) != 3) /* 3 = tracked */
        return 0;
    for (int j = 0; j < PSVR2_HAND_JOINTS; j++) {
        const uint8_t *jt = g_hand_res + 0x10 + (size_t)j * 0x60;
        const float *pos = (const float *)(jt + 0x00);
        const float *quat = (const float *)(jt + 0x10);
        out->joint[j].pos[0] = pos[0];
        out->joint[j].pos[1] = pos[1];
        out->joint[j].pos[2] = pos[2];
        out->joint[j].quat[0] = quat[0];
        out->joint[j].quat[1] = quat[1];
        out->joint[j].quat[2] = quat[2];
        out->joint[j].quat[3] = quat[3];
        out->radius[j] = *(const float *)(jt + 0x30);
    }
    return 1;
}

/* GFX10.3 image descriptor (T#) for linear RGBA8 with pitch == width: PS5VR's
 * probe encoding (hardware-validated). */
static void tsharp_rgba8(uint32_t out[8], uint64_t gpu, uint32_t w, uint32_t h)
{
    memset(out, 0, 32);
    out[0] = (uint32_t)(gpu >> 8);
    out[1] = (uint32_t)(gpu >> 40) | (56u << 20) | (((w - 1) & 3u) << 30);
    out[2] = (((w - 1) >> 2) & 0xfffu) | ((h - 1) << 14);
    out[3] = 4u | (5u << 3) | (6u << 6) | (7u << 9) | (9u << 28);
}

/* GFX10.3 1D 32_FLOAT T# for an Hmd2 lookup table, word for word as Horizon Call of
 * the Mountain's (its Hmd2 frame records, research/06): only the address and size differ. */
static void tsharp_table(uint32_t out[8], uint64_t gpu, uint32_t n)
{
    memset(out, 0, 32);
    out[0] = (uint32_t)(gpu >> 8);
    out[1] = (uint32_t)(gpu >> 40) | (22u << 20) | (((n - 1) & 3u) << 30);
    out[2] = ((n - 1) >> 2) & 0xfffu;
    out[3] = 4u | (8u << 28);
    out[5] = 0x00700000;
}

/* The process's direct memory (CPU and GPU share it): its size, what is allocated (walking
 * the allocations), and the largest free block at 64 KB alignment. */
void psvr2_direct_memory(uint64_t *pool, uint64_t *allocated, uint64_t *largest_free)
{
    *pool = sceKernelGetDirectMemorySize();
    *allocated = *largest_free = 0;
    dmem_query_info q;
    int64_t at = 0;
    for (int n = 0; n < 100000 && at < (int64_t)*pool; n++) {
        memset(&q, 0, sizeof q);
        if (sceKernelDirectMemoryQuery(at, 1 /* the next allocation */, &q, sizeof q) != 0 || q.end <= at)
            break;
        *allocated += (uint64_t)(q.end - (q.start > at ? q.start : at));
        at = q.end;
    }
    int64_t start = 0;
    size_t size = 0;
    if (sceKernelAvailableDirectMemorySize(0, (int64_t)*pool, 0x10000, &start, &size) == 0)
        *largest_free = size;
}

void psvr2_tv_mode(uint32_t w, uint32_t h)
{
    (void)w;
    (void)h;
}

int psvr2_set_warp(const warp_axis warp[2][2], uint32_t eye_w, uint32_t eye_h)
{
    memset(H.warp, 0, sizeof H.warp);
    if (warp[0][0].strength <= 0.0f)
        return 0;
    if (eye_w > 16384 || eye_h > 16384 || (!H.warp_tables && !(H.warp_tables = direct_alloc(0x40000, 0x10000)))) {
        XRLOG("psvr2: output warp off (%ux%u eye, tables %p)", eye_w, eye_h, (void *)H.warp_tables);
        return -1;
    }
    memcpy(H.warp, warp, sizeof H.warp);
    const uint32_t n[2] = {eye_w, eye_h};
    for (int e = 0; e < 2; e++)
        for (int axis = 0; axis < 2; axis++)
            warp_table(&H.warp[e][axis], H.warp_tables + (e * 2 + axis) * 16384, n[axis]);
    XRLOG("psvr2: output warp on (strength %.2f: %.2fx the detail across, %.2fx down in the middle; tables at %p)",
          (double)warp[0][0].strength, warp_centre_gain(&warp[0][0]), warp_centre_gain(&warp[0][1]),
          (void *)H.warp_tables);
    return 0;
}

/* The registered layer's tables (libSceHmd2 sub_1bba0 / sub_1e980): +0x44 fsrEn, then per
 * eye 0x60 bytes from +0x48: the across table's T#, the down table's T#, 32 unused bytes.
 * Hmd2 checks each table holds at least the eye's width (height) and samples it with the
 * eye's 0..1 rectilinear position. */
static void warp_layer(uint8_t *layer)
{
    if (H.warp[0][0].strength <= 0.0f)
        return;
    const uint32_t n[2] = {H.eye_w, H.eye_h};
    for (int e = 0; e < 2; e++)
        for (int axis = 0; axis < 2; axis++)
            tsharp_table((uint32_t *)(layer + 0x48 + e * 0x60 + axis * 0x20),
                         (uint64_t)(uintptr_t)(H.warp_tables + (e * 2 + axis) * 16384), n[axis]);
    layer[0x44] = 1;
}

static void mirror_apply(void)
{
    if (!H.loaded || H.mirror_want == H.mirror_on)
        return;
    if (H.mirror_want) {
        if (!H.mirror_work) {
            SizeAlign w = sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign();
            SizeAlign d = sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign();
            H.mirror_work = direct_alloc(w.size, w.align);
            H.mirror_disp = direct_alloc(d.size, d.align);
            H.mirror_disp_size = H.mirror_disp ? d.size : 0;
            XRLOG("psvr2: TV mirror buffers: work %zu bytes, display %zu bytes", (size_t)w.size, (size_t)d.size);
        }
        if (!H.mirror_work || !H.mirror_disp)
            return;
        uint32_t opt[8] = {1, 0, 0, 0, 0, 0, 0, 0};
        int r1 = sceHmd2ReprojectionSetMirroringOption(opt);
        void *param[2] = {H.mirror_work, H.mirror_disp};
        int r2 = sceHmd2ReprojectionEnableMirroring(param);
        XRLOG("psvr2: TV mirror on: option 0x%08x enable 0x%08x", r1, r2);
        H.mirror_on = r2 == 0;
    } else {
        int r = sceHmd2ReprojectionDisableMirroring();
        XRLOG("psvr2: TV mirror off: 0x%08x", r);
        H.mirror_on = 0;
    }
}

void psvr2_set_dualsense(int use)
{
    use_dualsense = use != 0;
}

/* The TV mirror's display buffer (ours, written by Hmd2), for the debug port's "mirror". */
const void *psvr2_mirror_buffer(size_t *size)
{
    *size = H.mirror_on ? H.mirror_disp_size : 0;
    return H.mirror_on ? H.mirror_disp : NULL;
}

void psvr2_set_mirror(int on)
{
    H.mirror_want = on != 0;
    mirror_apply();
}

static int apply_mode(int mode)
{
    const uint64_t vr = mode == PSVR2_MODE_90 ? VR_MODE_90HZ : VR_MODE_120HZ;
    int c1 = sceVideoOutConfigureOutput(H.vh, vr, NULL, NULL, 0);
    int c2 = 0;
    if (mode == PSVR2_MODE_120_HALF)
        c2 = sceHmd2ReprojectionSetRenderConfig(1, 0, 0);
    else if (mode == PSVR2_MODE_120 || H.mode == PSVR2_MODE_120_HALF)
        c2 = sceHmd2ReprojectionSetRenderConfig(0, 0, 0);
    XRLOG("psvr2: output mode %d (0x%lx): configure 0x%08x, render config 0x%08x", mode,
          (unsigned long)vr, c1, c2);
    H.mode = mode;
    return c1 ? c1 : c2;
}

/* After EnableVrMode the headset's link retrains (USB-C DP, HDCP) for about a
 * second. Flips sent before its display scans out wedged Hmd2's queue for
 * the rest of the process (every BeginFrame queue-full: seen in a hosted
 * title that sent its first frames 1 ms after EnableVrMode). Wait until the
 * VR port counts vblanks, at most 5 s. */
int sceVideoOutGetVblankStatus(int handle, void *status);
int sceVideoOutGetFlipStatus(int handle, void *status);
static int wait_display_live(void)
{
    uint64_t st[32] = {0};
    const uint64_t t0 = psvr2_now_us();
    uint64_t first = UINT64_MAX, count = 0;
    int rc = -1;
    while (psvr2_now_us() - t0 < 5000000) {
        memset(st, 0, sizeof st);
        rc = sceVideoOutGetVblankStatus(H.vh, st);
        count = st[0];
        if (rc == 0 && first == UINT64_MAX)
            first = count;
        if (rc == 0 && first != UINT64_MAX && count >= first && count - first >= 3)
            break;
        sceKernelUsleep(5000);
    }
    XRLOG("psvr2: VR display live after %.0f ms (vblank rc 0x%08x count %lu -> %lu)",
          (double)(psvr2_now_us() - t0) / 1000.0, rc, (unsigned long)(first == UINT64_MAX ? 0 : first),
          (unsigned long)count);
    return rc == 0 && first != UINT64_MAX && count >= first && count - first >= 3 ? 0 : -1;
}

int psvr2_vblank(uint64_t *count, uint64_t *time_us)
{
    uint64_t st[32] = {0};
    if (!H.active || sceVideoOutGetVblankStatus(H.vh, st) != 0 || !st[0] || !st[1])
        return -1;
    *count = st[0];
    *time_us = st[1];
    return 0;
}

int psvr2_output_start(uint32_t eye_w, uint32_t eye_h, void *const *buffers, int count, int mode)
{
    if (!H.loaded || H.active || count < 2 || count > MAX_BUFFERS)
        return -1;
    H.eye_w = eye_w;
    H.eye_h = eye_h;
    H.nbuf = (uint32_t)count;
    mirror_apply(); /* before VR mode, as Horizon does */
    const uint64_t vr = mode == PSVR2_MODE_90 ? VR_MODE_90HZ : VR_MODE_120HZ;
    int rc = sceHmd2ReprojectionEnableVrMode(vr);
    if (rc != 0) {
        XRLOG("psvr2: EnableVrMode failed 0x%08x", rc);
        return -1;
    }
    H.vh = sceVideoOutOpen(0xff, 32, 0, NULL);
    XRLOG("psvr2: EnableVrMode(0x%lx) -> 0x%08x, VR port 0x%08x", (unsigned long)vr, rc, H.vh);
    static int told_output;
    if (H.vh < 0) {
        sceHmd2ReprojectionDisableVrMode();
        if (!told_output++)
            psvr2_tell("sceVideoOutOpen", H.vh, TELL_HEADSET);
        return -1;
    }
    const uint32_t tw = 2 * eye_w, th = eye_h;
    vo_buffer_t bufs[MAX_BUFFERS];
    memset(bufs, 0, sizeof bufs);
    memset(H.layers, 0, sizeof H.layers);
    for (int b = 0; b < count; b++) {
        *(uint32_t *)H.layers[b] = 0x10; /* the same side-by-side texture twice */
        tsharp_rgba8((uint32_t *)(H.layers[b] + 0x04), (uint64_t)(uintptr_t)buffers[b], tw, th);
        tsharp_rgba8((uint32_t *)(H.layers[b] + 0x24), (uint64_t)(uintptr_t)buffers[b], tw, th);
        warp_layer(H.layers[b]);
        H.sets[b].layers = H.layers[b];
        H.sets[b].count = 1;
        bufs[b].data = &H.sets[b];
    }
    vo_attribute_t attr;
    memset(&attr, 0, sizeof attr);
    sceVideoOutSetBufferAttribute2(&attr, 0x8000000000000000ull, 0, tw, th, 0, 0, 0);
    rc = sceVideoOutRegisterBuffers2(H.vh, 0, 0, bufs, count, &attr, 0x10, NULL);
    XRLOG("psvr2: RegisterBuffers2 %ux%u x%d -> 0x%08x", tw, th, count, rc);
    if (rc != 0) {
        sceVideoOutClose(H.vh);
        H.vh = -1;
        sceHmd2ReprojectionDisableVrMode();
        if (!told_output++)
            psvr2_tell("sceVideoOutRegisterBuffers2", rc, TELL_REPORT);
        return -1;
    }
    H.mode = PSVR2_MODE_90;
    H.active = 1;
    if (apply_mode(mode) != 0 || wait_display_live() != 0) {
        XRLOG("psvr2: output configuration or vblank validation failed");
        psvr2_output_stop();
        return -1;
    }
    H.last_display_us = H.last_begin_us = 0;
    H.npending = H.have_last = 0;
    H.pace_vb = H.pace_frames = 0;
    H.live = 0;
    H.active = 1;
    return 0;
}

int psvr2_output_mode(void)
{
    return H.mode;
}

int psvr2_output_set_mode(int mode)
{
    if (!H.active || mode == H.mode)
        return 0;
    return apply_mode(mode);
}

float psvr2_refresh_hz(void)
{
    return H.mode == PSVR2_MODE_90 ? 90.0f : 120.0f;
}

float psvr2_frame_hz(void)
{
    return H.mode == PSVR2_MODE_90 ? 90.0f : H.mode == PSVR2_MODE_120 ? 120.0f : 60.0f;
}

void psvr2_output_stop(void)
{
    if (!H.active)
        return;
    H.active = 0;
    /* Leave VR mode first: the reprojection keeps sampling the last buffer. */
    int rc3 = sceHmd2ReprojectionDisableVrMode();
    int rc = 0;
    for (int i = 0; i < 50; i++) {
        rc = sceVideoOutUnregisterBuffers(H.vh, 0);
        if (rc != VO_BUSY)
            break;
        sceKernelUsleep(10000);
    }
    if (rc != 0)
        XRLOG("psvr2: warning: unregister not confirmed (0x%08x); no buffer reuse before process closure", rc);
    int rc2 = sceVideoOutClose(H.vh);
    H.vh = -1;
    XRLOG("psvr2: output stopped after %lu frames (unregister 0x%08x close 0x%08x disable 0x%08x)",
          (unsigned long)H.stats.frames, rc, rc2, rc3);
}

/* Begin pacing for the timing log: nap before BeginFrame, the call itself,
 * how far ahead the predicted display was, and its step from the last one. */
static struct { uint64_t nap, call, ahead, step, n; } pace;

static int submit_flip(int buf, uint64_t frame, uint64_t display_us, const psvr2_pose *pose,
                       const float tan_left[4], const float tan_right[4]);
static void show_last(uint64_t frame, uint64_t display_us);

int psvr2_begin_frame(uint64_t frame, uint64_t *display_us)
{
    if (!H.active)
        return -1;
    /* One frame per headset refresh at most. Until Hmd2 is live it predicts
     * "now" for every frame, and frames begun faster than the refresh then
     * (a hosted D3D12 test drew in 0.5 ms) filled its queue before it went
     * live; it never drained for the rest of the process (every BeginFrame
     * queue-full, 15 fps). A native app's first frames were ~9 ms apart and
     * Hmd2 went live at the 4th. So: before it is live, one frame per period;
     * after, begin once the last frame's display is under a period away. */
    const uint64_t period = (uint64_t)(1e6f / psvr2_frame_hz());
    uint64_t napped = 0;
    const uint64_t entered = psvr2_now_us();
    /* Waiting for the last frame's display here (as up to 10-24) fed back on
     * itself with a game slower than the refresh: Hmd2 spaces its predicted
     * display times by the game's frame interval, so every wait lengthened
     * the next (RE7: 18 ms of work, 50 ms waits, 14.6 fps). Pacing against
     * the display is xrWaitFrame's job, with the game's measured frame cost. */
    uint64_t start = 0;
    if (H.last_begin_us)
        start = H.last_begin_us + period - 300;
    if (start > entered) {
        uint64_t nap = start - entered;
        if (nap > 3 * period)
            nap = 3 * period;
        sceKernelUsleep((unsigned)nap);
        napped = nap;
    }
    /* Live, a frame may begin at most one ahead of the refreshes that have
     * passed. period - 300 above lets a game faster than the refresh begin
     * 124 frames a second at 120 Hz (xrtest: 123), one more than the headset
     * shows every ~40 frames, and that frame was dropped (or repeated and
     * refused, 0x8a720019). Counted on the port's vblanks, so a slower game
     * (fewer frames than refreshes) never waits here; its unused refreshes
     * don't build up into a burst later. The port's vblanks come at the frame
     * rate, not the headset's: 60 a second at half rate (Half-Life: Alyx,
     * 11-01: 600 in 10 s), so one frame per vblank in every mode. Counting
     * them as 120 Hz refreshes allowed half a frame each and held it to 30. */
    uint64_t vc = 0, vt = 0;
    if (H.live && psvr2_vblank(&vc, &vt) == 0) {
        const double vperiod = (double)period;
        if (!H.pace_vb || vc < H.pace_vb) {
            H.pace_vb = vc;
            H.pace_frames = 0;
        }
        double allowed = (double)(vc - H.pace_vb);
        if ((double)H.pace_frames > allowed + 1.0) {
            const double refreshes = (double)H.pace_frames - 1.0 - allowed;
            const uint64_t now = psvr2_now_us(), since = now > vt ? now - vt : 0;
            double wait = refreshes * vperiod - (double)since + 200.0;
            if (wait > 2.0 * vperiod)
                wait = 2.0 * vperiod;
            if (wait > 0) {
                sceKernelUsleep((unsigned)wait);
                napped += (uint64_t)wait;
                H.stats.paced++;
            }
        } else if ((double)H.pace_frames + 2.0 < allowed) {
            H.pace_vb = vc; /* behind: start counting again from here */
            H.pace_frames = 0;
        }
        H.pace_frames++;
    }
    H.last_begin_us = psvr2_now_us();
    pace.nap += napped;
    /* A frame two or more behind this one that was never submitted was
     * abandoned (the frame before may still be rendering on another thread):
     * it gets the last image, or its place in the queue is never freed. */
    for (int i = 0; i < H.npending;)
        if (H.pending[i].frame + 2 <= frame)
            show_last(H.pending[i].frame, H.pending[i].display_us);
        else
            i++;
    int rc = sceHmd2ReprojectionBeginFrame(frame);
    for (int tries = 0; rc == VR_QUEUE_FULL && tries < 60; tries++) {
        H.stats.queue_full++;
        sceKernelUsleep(1000);
        rc = sceHmd2ReprojectionBeginFrame(frame);
    }
    if (rc != 0) {
        if (H.stats.errors++ < 20)
            XRLOG("psvr2: BeginFrame(%lu) -> 0x%08x", (unsigned long)frame, rc);
        return -1;
    }
    uint64_t t = 0;
    sceHmd2ReprojectionGetPredictedDisplayTime(frame, &t);
    if (!t)
        t = psvr2_now_us() + 20000;
    if (frame <= 12)
        XRLOG("psvr2: frame %lu: entered %lu, last display %lu, napped %lu us, predicted %lu (%+ld us)",
              (unsigned long)frame, (unsigned long)entered, (unsigned long)H.last_display_us, (unsigned long)napped,
              (unsigned long)t, (long)(t - psvr2_now_us()));
    if (!H.live && t > psvr2_now_us() + 2000) {
        H.live = 1;
        XRLOG("psvr2: Hmd2 live at frame %lu (display %+ld us)", (unsigned long)frame, (long)(t - psvr2_now_us()));
    }
    const uint64_t done = psvr2_now_us();
    pace.call += done - H.last_begin_us;
    pace.ahead += t > done ? t - done : 0;
    pace.step += H.last_display_us && t > H.last_display_us ? t - H.last_display_us : 0;
    pace.n++;
    H.last_display_us = t;
    if (H.npending == (int)(sizeof H.pending / sizeof H.pending[0]))
        show_last(H.pending[0].frame, H.pending[0].display_us);
    H.pending[H.npending].frame = frame;
    H.pending[H.npending].display_us = t;
    H.npending++;
    *display_us = t;
    return 0;
}

void psvr2_take_pace(float out[4])
{
    const double n = pace.n ? (double)pace.n : 1.0;
    out[0] = (float)(pace.nap / n / 1000.0);
    out[1] = (float)(pace.call / n / 1000.0);
    out[2] = (float)(pace.ahead / n / 1000.0);
    out[3] = (float)(pace.step / n / 1000.0);
    memset(&pace, 0, sizeof pace);
}

/* The frame is submitted: it no longer holds a place in Hmd2's queue. */
static void drop_pending(uint64_t frame)
{
    for (int i = 0; i < H.npending; i++)
        if (H.pending[i].frame == frame) {
            H.pending[i] = H.pending[--H.npending];
            return;
        }
}

static uint64_t gpu_sum_us, gpu_n;

void psvr2_note_gpu(uint64_t us)
{
    gpu_sum_us += us;
    gpu_n++;
}

/* Every 10 s, the stats line both tracks log (core/gvr_stats.h). "again" is a
 * frame the app ended without an image, shown again; gpu is the compositor's
 * GPU time per frame (xr_vulkan.c timestamps); never-shown and late are not
 * measured here yet. */
static void stats_line(void)
{
    static uint64_t t0, fr0, rep0, qf0, err0, vb0;
    uint64_t now = psvr2_now_us(), vb = 0, vt = 0;
    if (t0 && now - t0 < 10000000)
        return;
    if (psvr2_vblank(&vb, &vt) != 0)
        vb = 0;
    if (t0) {
        gvr_stats st;
        gvr_stats_init(&st);
        st.window_ms = (now - t0) / 1000;
        st.refresh_hz = (uint64_t)(psvr2_refresh_hz() + 0.5f);
        if (vb && vb0 && vb >= vb0)
            st.refreshes = vb - vb0;
        st.handed = H.stats.frames - fr0;
        st.again = H.stats.repeated - rep0;
        st.qfull = H.stats.queue_full - qf0;
        st.err = H.stats.errors - err0;
        if (gpu_n)
            st.gpu_us = gpu_sum_us / gpu_n;
        char line[192];
        gvr_stats_format(line, sizeof line, &st);
        XRLOG("%s", line);
    }
    t0 = now;
    fr0 = H.stats.frames;
    rep0 = H.stats.repeated;
    qf0 = H.stats.queue_full;
    err0 = H.stats.errors;
    vb0 = vb;
    gpu_sum_us = gpu_n = 0;
}

int psvr2_submit(int buf, uint64_t frame, uint64_t display_us, const psvr2_pose *pose,
                 const float tan_left[4], const float tan_right[4])
{
    const int rc = submit_flip(buf, frame, display_us, pose, tan_left, tan_right);
    drop_pending(frame);
    if (!rc) {
        H.have_last = 1;
        H.last_buf = buf;
        H.last_pose = *pose;
        memcpy(H.last_tl, tan_left, sizeof H.last_tl);
        memcpy(H.last_tr, tan_right, sizeof H.last_tr);
    }
    return rc;
}

/* A frame the app ended without an image (OpenComposite while the game
 * loads) shows the last one again, with the pose it was drawn for, so Hmd2
 * reprojects it. Left in the queue, such frames fill it and it stays full:
 * every BeginFrame then waits out its retries (HL:A: 66 ms, 12 fps). */
static void show_last(uint64_t frame, uint64_t display_us)
{
    static const psvr2_pose still = {.quat = {0.0f, 0.0f, 0.0f, 1.0f}};
    drop_pending(frame);
    H.stats.repeated++;
    if (H.have_last) {
        submit_flip(H.last_buf, frame, display_us, &H.last_pose, H.last_tl, H.last_tr);
        return;
    }
    /* Nothing drawn yet: an empty buffer, with the panel's own field of view
     * (Hmd2 refuses tangents that are not positive). */
    const float tl[4] = {H.fov[0], H.fov[1], H.fov[2], H.fov[3]}; /* out, in, top, bottom */
    const float tr[4] = {H.fov[1], H.fov[0], H.fov[2], H.fov[3]};
    submit_flip(0, frame, display_us, &still, tl, tr);
}

void psvr2_repeat_frame(uint64_t frame)
{
    for (int i = 0; i < H.npending; i++)
        if (H.pending[i].frame == frame) {
            show_last(frame, H.pending[i].display_us);
            return;
        }
}

static int submit_flip(int buf, uint64_t frame, uint64_t display_us, const psvr2_pose *pose,
                       const float tan_left[4], const float tan_right[4])
{
    if (!H.active || buf < 0 || (uint32_t)buf >= H.nbuf)
        return -1;
    static uint8_t lp[0x148] __attribute__((aligned(16)));
    struct { void *layers; int32_t count; int32_t pad; void *ext; } param = {lp, 1, 0, NULL};
    const uint32_t tw = 2 * H.eye_w, th = H.eye_h;
    memset(lp, 0, sizeof lp);
    uint32_t *u = (uint32_t *)lp;
    u[0x00 / 4] = 0x10;
    u[0x04 / 4] = 0;
    u[0x08 / 4] = tw, u[0x0c / 4] = th, u[0x10 / 4] = tw, u[0x14 / 4] = th;
    u[0x18 / 4] = 0x92, u[0x1c / 4] = 0x00fff000, u[0x20 / 4] = 0x05500000;    /* sampler */
    u[0x28 / 4] = H.eye_w, u[0x2c / 4] = H.eye_h, u[0x30 / 4] = 0, u[0x34 / 4] = 0;
    u[0x38 / 4] = H.eye_w, u[0x3c / 4] = H.eye_h, u[0x40 / 4] = H.eye_w, u[0x44 / 4] = 0;
    memcpy(lp + 0x48, tan_left, 16);
    memcpy(lp + 0x58, tan_right, 16);
    memcpy(lp + 0x68, pose->pos, 12);
    memcpy(lp + 0x74, pose->quat, 16);
    *(uint64_t *)(lp + 0x88) = display_us;
    *(uint64_t *)(lp + 0x90) = frame;
    u[0x98 / 4] = 100;
    u[0xa0 / 4] = H.eye_w, u[0xa4 / 4] = H.eye_h, u[0xa8 / 4] = H.eye_w, u[0xac / 4] = H.eye_h;
    int r1 = sceHmd2ReprojectionSetParam((uint32_t)buf, frame, &param, NULL);
    int r2 = sceVideoOutSubmitFlip(H.vh, buf, 2, (int64_t)frame);
    H.stats.frames++;
    if ((r1 || r2) && H.stats.errors++ < 20)
        XRLOG("psvr2: frame %lu setparam 0x%08x flip 0x%08x", (unsigned long)frame, r1, r2);
    if (H.stats.frames % 900 == 1)
        XRLOG("psvr2: frame %lu submitted (buf %d, queue-full waits %lu, repeated %lu)", (unsigned long)frame,
              buf, (unsigned long)H.stats.queue_full, (unsigned long)H.stats.repeated);
    stats_line();
    return (r1 || r2) ? -1 : 0;
}

void psvr2_shutdown(void)
{
    psvr2_output_stop();
    if (!H.loaded) {
        const int r1 = H.tracker_registered ? sceVrTracker2UnregisterDevice(H.hmd) : 0;
        const int r2 = H.tracker_initialized ? sceVrTracker2Finalize() : 0;
        const int r4 = H.opened ? sceHmd2Close(H.hmd) : 0;
        const int r5 = H.hmd_initialized ? sceHmd2Terminate() : 0;
        H.opened = H.hmd_initialized = H.tracker_initialized = H.tracker_registered = 0;
        XRLOG("psvr2: partial shutdown (tracker 0x%08x 0x%08x, hmd 0x%08x 0x%08x)", r1, r2, r4, r5);
        return;
    }
    pthread_mutex_lock(&pad_lock);
    /* Triggers left stiff would stay so on the home screen. */
    if (T.used) {
        memset(T.want, 0, sizeof T.want);
        T.dirty[0] = T.dirty[1] = 1;
        triggers_apply();
    }
    for (int side = 0; side < 2; side++) {
        if (P.registered[side]) {
            sceVrTracker2UnregisterDevice(P.sense[side]);
            P.registered[side] = 0;
        }
        P.sense[side] = -1; /* a later session registers them with its tracker */
    }
    P.next_open_try = 0;
    pthread_mutex_unlock(&pad_lock);
    const int r1 = sceVrTracker2UnregisterDevice(H.hmd);
    const int r2 = sceVrTracker2Finalize();
    /* Terminate refuses while the TV mirror is on (0x8a72001e, libSceHmd2 0x21e3d) and the next
     * ReprojectionInitialize then says already initialized (0x8a72000c). OpenComposite ends its
     * first session to start the real one, so that one never came up. */
    if (H.mirror_on) {
        const int r = sceHmd2ReprojectionDisableMirroring();
        XRLOG("psvr2: TV mirror off: 0x%08x", r);
        H.mirror_on = 0;
    }
    int r3 = 0;
    for (int i = 0; i < 50; i++) {
        r3 = sceHmd2ReprojectionTerminate();
        if (r3 != VO_BUSY)
            break;
        sceKernelUsleep(10000);
    }
    const int r4 = sceHmd2Close(H.hmd);
    const int r5 = sceHmd2Terminate();
    H.loaded = 0;
    H.opened = H.hmd_initialized = H.tracker_initialized = H.tracker_registered = 0;
    XRLOG("psvr2: shut down (tracker 0x%08x 0x%08x, reprojection 0x%08x, hmd 0x%08x 0x%08x)", r1,
          r2, r3, r4, r5);
}

/* -- self test ---------------------------------------------------------------- */
/* Native bounded diagnostic: Vulkan-generated immutable stereo reference.
 * No game assets or changing credentials. Returns number of successful submissions. */
int psvr2_selftest(const char *tag, int frames)
{
    enum { EYE = 1024, NBUF = 3 };
    static void *buf[NBUF];
    if (frames < 1 || frames > ENCORE_PROBE_FRAMES) return -1;
    if (psvr2_init_output() != 0) {
        XRLOG("selftest %s: init failed", tag);
        return -1;
    }
    encore_probe_eye optical[2];
    for (int e=0; e<2; ++e) {
        if (!encore_probe_eye_init(&optical[e],H.fov,H.info.ipd,e)) {
            XRLOG("VR_PROBE invalid optical calibration");
            return -1;
        }
        float u,v;
        encore_probe_project(&optical[e],0.f,0.f,&u,&v);
        XRLOG("VR_PROBE stereo eye=%d center_uv=%.5f %.5f eye_x=%.5f depth=%.2f",
              e,(double)u,(double)v,(double)optical[e].eye_x,(double)optical[e].depth);
    }
    for (int b = 0; b < NBUF; b++) {
        if (!buf[b])
            buf[b] = direct_alloc((size_t)2 * EYE * EYE * 4, 0x10000);
        if (!buf[b])
            return -1;
    }
    // Render before registration; never write these buffers while Hmd2 can read them.
    if (encore_probe_gpu_render(optical,EYE,buf,NBUF)!=0) {
        XRLOG("VR_PROBE GPU validation failed; no headset output registered");
        return -1;
    }
    XRLOG("VR_PROBE scene: square left 1m, cross/ring center 2m, diamond right 4m; head-relative");
    const uint64_t qf0 = H.stats.queue_full, err0 = H.stats.errors, fr0 = H.stats.frames;
    if (psvr2_output_start(EYE, EYE, buf, NBUF, PSVR2_MODE_90) != 0) {
        XRLOG("selftest %s: output start failed", tag);
        return -1;
    }
    const float tl[4] = {H.fov[0], H.fov[1], H.fov[2], H.fov[3]};
    const float tr[4] = {H.fov[1], H.fov[0], H.fov[2], H.fov[3]};
    const uint64_t t0 = psvr2_now_us();
    int ok = 0;
    encore_origin_gate gate={0};
    encore_flip_observation flips={0};
    uint64_t gpu_total=0,gpu_max=0,gpu_count=0,gpu_over=0;
    // Give the tracker up to 3 seconds to settle before beginning any frame.
    psvr2_pose dev = {{0, 0, 0}, {0, 0, 0, 1}}, eyes[2];
    float lv[3], av[3];
    int tracked = 0;
    while (psvr2_now_us() - t0 < 3000000u && psvr2_worn() && !psvr2_exit_requested()) {
        if (psvr2_head(psvr2_now_us(), &dev, eyes, lv, av)) { tracked = 1; break; }
        sceKernelUsleep(10000);
    }
    if (!tracked) XRLOG("selftest %s: tracking not ready", tag);
    for (int i = 1; tracked && i <= frames; i++) {
        if (!psvr2_worn() || psvr2_exit_requested() || psvr2_now_us() - t0 >= ENCORE_PROBE_DEADLINE_US) {
            XRLOG("selftest %s: stopped (wear/exit/deadline)", tag);
            break;
        }
        uint64_t disp = 0;
        if (psvr2_begin_frame((uint64_t)i, &disp) != 0) {
            XRLOG("selftest %s: begin failed at %d", tag, i);
            break;
        }
        if (!psvr2_head(disp, &dev, eyes, lv, av)) {
            psvr2_repeat_frame((uint64_t)i);
            XRLOG("selftest %s: tracking lost at %d", tag, i);
            break;
        }
        // After the initial three seconds, exercise the actual GPU on each
        // frame, in resources entirely separate from the headset buffers.
        const uint64_t elapsed=psvr2_now_us()-t0;
        if(encore_origin_sample(&gate,elapsed,dev.pos,dev.quat)) {
            XRLOG("VR_PROBE origin candidate frame=%d elapsed_us=%lu pos=(%.4f %.4f %.4f) quat=(%.5f %.5f %.5f %.5f)",
                  i,(unsigned long)elapsed,(double)dev.pos[0],(double)dev.pos[1],(double)dev.pos[2],
                  (double)dev.quat[0],(double)dev.quat[1],(double)dev.quat[2],(double)dev.quat[3]);
        }
        if(i>1){
            // The pinned PS5_Vulkan TV WSI uses word 3 as the shown flip
            // argument. Verify its behavior on this VR port, read-only.
            uint64_t status[32]={0};
            int rc=sceVideoOutGetFlipStatus(H.vh,status);
            int valid=encore_flip_sample(&flips,rc,status[3],(uint64_t)(i-1));
            if(i<=12||i%450==0||i==frames)
                XRLOG("VR_PROBE flip frame=%d rc=0x%08x shown_word3=%lu last_submitted=%d valid=%d raw0=%lu raw1=%lu raw2=%lu",
                      i,rc,(unsigned long)status[3],i-1,valid,(unsigned long)status[0],
                      (unsigned long)status[1],(unsigned long)status[2]);
        }
        if(elapsed>=3000000u){
            const uint64_t gpu_start=psvr2_now_us();
            if(encore_probe_gpu_replay()!=0){
                psvr2_repeat_frame((uint64_t)i);
                XRLOG("VR_PROBE GPU replay failed at %d",i);
                break;
            }
            const uint64_t cost=psvr2_now_us()-gpu_start;
            gpu_total+=cost;++gpu_count;
            if(cost>gpu_max)gpu_max=cost;
            if(cost>11111u)++gpu_over;
            if(i%450==0||i==frames)
                XRLOG("VR_PROBE GPU replay count=%lu mean_us=%.1f max_us=%lu over_11ms=%lu (submit+fence+sample wall time)",
                      (unsigned long)gpu_count,(double)gpu_total/(double)gpu_count,(unsigned long)gpu_max,(unsigned long)gpu_over);
        }
        if (!psvr2_worn() || psvr2_exit_requested() || psvr2_now_us()-t0>=ENCORE_PROBE_DEADLINE_US) {
            psvr2_repeat_frame((uint64_t)i);
            break;
        }
        if (psvr2_submit((i-1) % NBUF, (uint64_t)i, disp, &dev, tl, tr) != 0) {
            XRLOG("selftest %s: submission failed at %d", tag, i);
            break;
        }
        if (i==1 || i%450==0 || i==frames) {
            // Diagnostic only: measured eye baseline and head-local offsets.
            // No change to the submitted pose or runtime projection.
            float inv[4]={-dev.quat[0],-dev.quat[1],-dev.quat[2],dev.quat[3]};
            float off[2][3];
            for(int side=0;side<2;++side) {
                float delta[3];
                for(int k=0;k<3;++k) delta[k]=eyes[side].pos[k]-dev.pos[k];
                qrot(inv,delta,off[side]);
            }
            float dx=off[1][0]-off[0][0],dy=off[1][1]-off[0][1],dz=off[1][2]-off[0][2];
            float baseline=sqrtf(dx*dx+dy*dy+dz*dz);
            float dot=0.f;
            for(int k=0;k<4;++k) dot+=eyes[0].quat[k]*eyes[1].quat[k];
            XRLOG("VR_PROBE pose frame=%d baseline_mm=%.2f eyeL=(%.4f %.4f %.4f) eyeR=(%.4f %.4f %.4f) eye_quat_absdot=%.6f",
                  i,(double)(1000.f*baseline),(double)off[0][0],(double)off[0][1],(double)off[0][2],
                  (double)off[1][0],(double)off[1][1],(double)off[1][2],(double)fabsf(dot));
            XRLOG("VR_PROBE head frame=%d pos=(%.4f %.4f %.4f) quat=(%.5f %.5f %.5f %.5f)",
                  i,(double)dev.pos[0],(double)dev.pos[1],(double)dev.pos[2],(double)dev.quat[0],
                  (double)dev.quat[1],(double)dev.quat[2],(double)dev.quat[3]);
        }
        ok++;
    }
    const double secs = (double)(psvr2_now_us() - t0) / 1e6;
    XRLOG("selftest %s: %d of %d frames in %.2f s (%.1f fps), queue-full waits %lu, errors %lu, worn %d", tag, ok,
          frames, secs, secs > 0 ? ok / secs : 0.0, (unsigned long)(H.stats.queue_full - qf0),
          (unsigned long)(H.stats.errors - err0), psvr2_worn());
    (void)fr0;
    XRLOG("VR_PROBE V7 summary origin_candidate=%d replay_count=%lu replay_over_11ms=%lu flip_valid=%lu flip_invalid=%lu flip_regressed=%lu flip_max_lag=%lu",
          gate.anchored,(unsigned long)gpu_count,(unsigned long)gpu_over,(unsigned long)flips.valid,
          (unsigned long)flips.invalid,(unsigned long)flips.regressed,(unsigned long)flips.max_lag);
    psvr2_output_stop();
    return ok;
}
