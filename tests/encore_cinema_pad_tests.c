/* SPDX-License-Identifier: MIT */
#include "../ps5/vr/cinema_pad_bridge.h"
#include <assert.h>
#include <stdio.h>
static struct pad_reading neutral(void) {
    struct pad_reading r={0};
    r.left_x=r.left_y=r.right_x=r.right_y=128;
    return r;
}
int main(void){
    psvr2_cinema_input in={0};struct pad_reading r=neutral();
    assert(!encore_cinema_merge(&in,&r));
    in.side[0].connected=in.side[1].connected=1;
    in.side[0].buttons=PSVR2_BTN_X|PSVR2_BTN_Y|PSVR2_BTN_GRIP;
    in.side[1].buttons=PSVR2_BTN_A|PSVR2_BTN_B|PSVR2_BTN_GRIP;
    in.side[0].stick[1]=.9f;in.side[1].stick[0]=-.8f;
    in.side[0].trigger=.6f;in.side[1].trigger=1.f;
    r.buttons=PAD_LEFT;r.l2=200;
    assert(encore_cinema_merge(&in,&r));
    assert((r.buttons&(PAD_SQUARE|PAD_TRIANGLE|PAD_CROSS|PAD_CIRCLE|PAD_L1|PAD_R1|PAD_UP|PAD_LEFT))==
             (PAD_SQUARE|PAD_TRIANGLE|PAD_CROSS|PAD_CIRCLE|PAD_L1|PAD_R1|PAD_UP|PAD_LEFT));
    assert(r.l2==200&&r.r2==255);
    assert(r.left_y<30&&r.right_x<40);
    in.side[0].buttons=PSVR2_BTN_MENU;in.side[1].buttons=PSVR2_BTN_MENU;
    r=neutral();assert(encore_cinema_merge(&in,&r));
    assert((r.buttons&(PAD_TOUCH_PAD|PAD_OPTIONS))==(PAD_TOUCH_PAD|PAD_OPTIONS));
    in.side[0].connected=0;in.side[1].connected=0;
    r=neutral();assert(!encore_cinema_merge(&in,&r));
    assert(encore_cinema_axis(NAN,0)==128);
    assert(encore_cinema_trigger(NAN)==0);
    assert(encore_cinema_axis(2.f,0)==255);
    puts("CINEMA_PAD_TEST PASS Sense face, grip, analog, D-pad, menu tap, disconnection, original-pad merging");
    return 0;
}
