/* SPDX-License-Identifier: MIT
 * Pure Sense -> original gamepad bridge. No console calls, no input injection.
 * Merge only into the currently rendered game's player 0 (never an instrument).
 */
#pragma once
#include <math.h>
#include "ghost/psvr2.h"
#include "../src/platform.h"
static inline uint8_t encore_cinema_axis(float v, int invert) {
    if (!__builtin_isfinite(v)) return 128;
    if (v > 1.f) v=1.f;
    if (v < -1.f) v=-1.f;
    float x=128.f+(invert?-127.f:127.f)*v;
    return (uint8_t)(x<0?0:x>255?255:(int)(x+.5f));
}
static inline uint8_t encore_cinema_trigger(float v) {
    if (!__builtin_isfinite(v)||v<0.f)return 0;
    if(v>1.f)v=1.f;
    return (uint8_t)(v*255.f+.5f);
}
/* Preserve physical pad keys/axes; only use Sense sticks when displaced.
 * Menu is a synthesized *short release* from psvr2.c. Long hold is recenter,
 * never a game menu press. Left Create = touch pad, right Options = Options.
 */
static inline int encore_cinema_merge(const psvr2_cinema_input *v,struct pad_reading *r) {
    if(!v||!r||(!v->side[0].connected&&!v->side[1].connected))return 0;
    const psvr2_cinema_hand *l=&v->side[0],*h=&v->side[1];
    uint32_t b=0;
    if(l->connected) {
        // Default five-fret mappings: RED = Circle (lane 2), BLUE =
        // Triangle (lane 4). Both accessible from the left Sense pad.
        if(l->buttons&PSVR2_BTN_X)b|=PAD_CIRCLE;
        if(l->buttons&PSVR2_BTN_Y)b|=PAD_TRIANGLE;
        if(l->buttons&PSVR2_BTN_GRIP)b|=PAD_L1;
        if(l->buttons&PSVR2_BTN_STICK)b|=PAD_L3;
        if(l->buttons&PSVR2_BTN_MENU)b|=PAD_TOUCH_PAD;
        uint8_t t=encore_cinema_trigger(l->trigger);
        if(t>r->l2)r->l2=t;
        if(fabsf(l->stick[0])>.1f)r->left_x=encore_cinema_axis(l->stick[0],0);
        if(fabsf(l->stick[1])>.1f)r->left_y=encore_cinema_axis(l->stick[1],1);
        if(l->stick[1]>.65f)b|=PAD_UP;
        if(l->stick[1]<-.65f)b|=PAD_DOWN;
        if(l->stick[0]>.65f)b|=PAD_RIGHT;
        if(l->stick[0]<-.65f)b|=PAD_LEFT;
    }
    if(h->connected) {
        // Right hand covers GREEN (Cross) and YELLOW (Square).
        // The dedicated grip remains the fifth ORANGE (L1) fret.
        if(h->buttons&PSVR2_BTN_A)b|=PAD_CROSS;
        if(h->buttons&PSVR2_BTN_B)b|=PAD_SQUARE;
        if(h->buttons&PSVR2_BTN_GRIP)b|=PAD_R1;
        if(h->buttons&PSVR2_BTN_STICK)b|=PAD_R3;
        if(h->buttons&PSVR2_BTN_MENU)b|=PAD_OPTIONS;
        uint8_t t=encore_cinema_trigger(h->trigger);
        if(t>r->r2)r->r2=t;
        if(fabsf(h->stick[0])>.1f)r->right_x=encore_cinema_axis(h->stick[0],0);
        if(fabsf(h->stick[1])>.1f)r->right_y=encore_cinema_axis(h->stick[1],1);
    }
    r->buttons|=b;
    r->connected=true;
    return 1;
}
