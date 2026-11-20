/* PS5XR's exports of libSceHmd2 and libSceVrTracker2 at their offsets in each firmware's build,
 * for when sceKernelDlsym can't find them (the libraries keep them out of their symbol tables).
 * PS5VR's tables (ps5vr/tools/gen_export_table.py), cut to what PS5XR calls; 9.00 generated from
 * a community dump the same way. A table is used only when its anchor string sits at its offset
 * in the loaded library, so it can't be used on another build. Builds that several firmware share
 * (checked from their libraries):
 *   1360  13.20, 13.40 (VrTracker2), 13.60      1240  12.20, 12.40
 *   1000  10.00, 10.01, 10.20, 10.40, 10.60     1160  11.60
 *   900, 840, 761 (7.20, 7.61), 550             their own
 * Any other firmware: gv-tool's exports file (psvr2.c, exports_file_resolve). */
static const export_sig_t klibSceHmd2Exports_1360[] = {
  {"sceHmd2GazeGetResult", 0x35510},
  {"sceHmd2InternalGazeStart", 0x31b60},
  {"sceHmd2InternalGazeStop", 0x31b80},
  {"sceHmd2Initialize", 0x2a670},
  {"sceHmd2Open", 0x2a740},
  {"sceHmd2GetDeviceInformation", 0x2ab90},
  {"sceHmd2GetFieldOfViewWithoutHandle", 0x27940},
  {"sceHmd2ReprojectionQueryBufferSizeAlign", 0x218c0},
  {"sceHmd2ReprojectionQueryDisplayBufferSizeAlign", 0x21940},
  {"sceHmd2ReprojectionInitialize", 0x21a00},
  {"sceHmd2ReprojectionEnableVrMode", 0x21f00},
  {"sceHmd2ReprojectionDisableVrMode", 0x21f90},
  {"sceHmd2ReprojectionBeginFrame", 0x220d0},
  {"sceHmd2ReprojectionGetPredictedDisplayTime", 0x220e0},
  {"sceHmd2ReprojectionSetParam", 0x22110},
  {"sceHmd2ReprojectionSetRenderConfig", 0x22130},
  {"sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign", 0x221b0},
  {"sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign", 0x221c0},
  {"sceHmd2ReprojectionSetMirroringOption", 0x222c0},
  {"sceHmd2ReprojectionEnableMirroring", 0x221d0},
  {"sceHmd2ReprojectionDisableMirroring", 0x22250},
  {"sceHmd2ReprojectionTerminate", 0x21df0},
  {"sceHmd2Close", 0x2a8e0},
  {"sceHmd2Terminate", 0x2a710},
  {"sceHmd2SetVibration", 0x2ae00},
};
static const export_anchor_t klibSceHmd2Anchor_1360 = {0x7c686, "Not Initialized"};
static const export_sig_t klibSceVrTracker2Exports_1360[] = {
  {"sceVrTracker2QueryMemory", 0xe0},
  {"sceVrTracker2Initialize", 0x230},
  {"sceVrTracker2RegisterDevice", 0x11e0},
  {"sceVrTracker2GetResult", 0x2890},
  {"sceVrTracker2UnregisterDevice", 0x13d0},
  {"sceVrTracker2Finalize", 0x1140},
};
static const export_anchor_t klibSceVrTracker2Anchor_1360 = {0x1225e5, "MININLIERS-FOR-MULTICAMSTRICTGRAVITYBOOT"};
static const export_sig_t klibSceHmd2Exports_1240[] = {
  {"sceHmd2GazeGetResult", 0x35110},
  {"sceHmd2InternalGazeStart", 0x31760},
  {"sceHmd2InternalGazeStop", 0x31780},
  {"sceHmd2Initialize", 0x2a260},
  {"sceHmd2Open", 0x2a330},
  {"sceHmd2GetDeviceInformation", 0x2a780},
  {"sceHmd2GetFieldOfViewWithoutHandle", 0x27520},
  {"sceHmd2ReprojectionQueryBufferSizeAlign", 0x21470},
  {"sceHmd2ReprojectionQueryDisplayBufferSizeAlign", 0x214f0},
  {"sceHmd2ReprojectionInitialize", 0x215b0},
  {"sceHmd2ReprojectionEnableVrMode", 0x21ab0},
  {"sceHmd2ReprojectionDisableVrMode", 0x21b40},
  {"sceHmd2ReprojectionBeginFrame", 0x21c80},
  {"sceHmd2ReprojectionGetPredictedDisplayTime", 0x21c90},
  {"sceHmd2ReprojectionSetParam", 0x21cc0},
  {"sceHmd2ReprojectionSetRenderConfig", 0x21ce0},
  {"sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign", 0x21d60},
  {"sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign", 0x21d70},
  {"sceHmd2ReprojectionSetMirroringOption", 0x21e70},
  {"sceHmd2ReprojectionEnableMirroring", 0x21d80},
  {"sceHmd2ReprojectionDisableMirroring", 0x21e00},
  {"sceHmd2ReprojectionTerminate", 0x219a0},
  {"sceHmd2Close", 0x2a4d0},
  {"sceHmd2Terminate", 0x2a300},
  {"sceHmd2SetVibration", 0x2a9f0},
};
static const export_anchor_t klibSceHmd2Anchor_1240 = {0x7c67e, "Not Initialized"};
static const export_sig_t klibSceVrTracker2Exports_1240[] = {
  {"sceVrTracker2QueryMemory", 0xe0},
  {"sceVrTracker2Initialize", 0x230},
  {"sceVrTracker2RegisterDevice", 0x11b0},
  {"sceVrTracker2GetResult", 0x2870},
  {"sceVrTracker2UnregisterDevice", 0x13a0},
  {"sceVrTracker2Finalize", 0x1110},
};
static const export_anchor_t klibSceVrTracker2Anchor_1240 = {0x1232e5, "MININLIERS-FOR-MULTICAMSTRICTGRAVITYBOOT"};
static const export_sig_t klibSceHmd2Exports_1160[] = {
  {"sceHmd2GazeGetResult", 0x34f50},
  {"sceHmd2InternalGazeStart", 0x315a0},
  {"sceHmd2InternalGazeStop", 0x315c0},
  {"sceHmd2Initialize", 0x2a0c0},
  {"sceHmd2Open", 0x2a190},
  {"sceHmd2GetDeviceInformation", 0x2a5e0},
  {"sceHmd2GetFieldOfViewWithoutHandle", 0x27360},
  {"sceHmd2ReprojectionQueryBufferSizeAlign", 0x21340},
  {"sceHmd2ReprojectionQueryDisplayBufferSizeAlign", 0x213c0},
  {"sceHmd2ReprojectionInitialize", 0x21480},
  {"sceHmd2ReprojectionEnableVrMode", 0x21980},
  {"sceHmd2ReprojectionDisableVrMode", 0x21a10},
  {"sceHmd2ReprojectionBeginFrame", 0x21b50},
  {"sceHmd2ReprojectionGetPredictedDisplayTime", 0x21b60},
  {"sceHmd2ReprojectionSetParam", 0x21b90},
  {"sceHmd2ReprojectionSetRenderConfig", 0x21bb0},
  {"sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign", 0x21c30},
  {"sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign", 0x21c40},
  {"sceHmd2ReprojectionSetMirroringOption", 0x21d40},
  {"sceHmd2ReprojectionEnableMirroring", 0x21c50},
  {"sceHmd2ReprojectionDisableMirroring", 0x21cd0},
  {"sceHmd2ReprojectionTerminate", 0x21870},
  {"sceHmd2Close", 0x2a330},
  {"sceHmd2Terminate", 0x2a160},
  {"sceHmd2SetVibration", 0x2a850},
};
static const export_anchor_t klibSceHmd2Anchor_1160 = {0x136ab2, "Not Initialized"};
static const export_sig_t klibSceVrTracker2Exports_1160[] = {
  {"sceVrTracker2QueryMemory", 0xe0},
  {"sceVrTracker2Initialize", 0x230},
  {"sceVrTracker2RegisterDevice", 0x11a0},
  {"sceVrTracker2GetResult", 0x2860},
  {"sceVrTracker2UnregisterDevice", 0x1390},
  {"sceVrTracker2Finalize", 0x1100},
};
static const export_anchor_t klibSceVrTracker2Anchor_1160 = {0x11ae8d, "MININLIERS-FOR-MULTICAMSTRICTGRAVITYBOOT"};
static const export_sig_t klibSceHmd2Exports_1000[] = {
  {"sceHmd2GazeGetResult", 0x34de0},
  {"sceHmd2InternalGazeStart", 0x31400},
  {"sceHmd2InternalGazeStop", 0x31420},
  {"sceHmd2Initialize", 0x29f40},
  {"sceHmd2Open", 0x2a000},
  {"sceHmd2GetDeviceInformation", 0x2a450},
  {"sceHmd2GetFieldOfViewWithoutHandle", 0x27240},
  {"sceHmd2ReprojectionQueryBufferSizeAlign", 0x212a0},
  {"sceHmd2ReprojectionQueryDisplayBufferSizeAlign", 0x21320},
  {"sceHmd2ReprojectionInitialize", 0x213c0},
  {"sceHmd2ReprojectionEnableVrMode", 0x21890},
  {"sceHmd2ReprojectionDisableVrMode", 0x21920},
  {"sceHmd2ReprojectionBeginFrame", 0x21a70},
  {"sceHmd2ReprojectionGetPredictedDisplayTime", 0x21a80},
  {"sceHmd2ReprojectionSetParam", 0x21ab0},
  {"sceHmd2ReprojectionSetRenderConfig", 0x21ad0},
  {"sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign", 0x21b50},
  {"sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign", 0x21b60},
  {"sceHmd2ReprojectionSetMirroringOption", 0x21c60},
  {"sceHmd2ReprojectionEnableMirroring", 0x21b70},
  {"sceHmd2ReprojectionDisableMirroring", 0x21bf0},
  {"sceHmd2ReprojectionTerminate", 0x21780},
  {"sceHmd2Close", 0x2a1a0},
  {"sceHmd2Terminate", 0x29fd0},
  {"sceHmd2SetVibration", 0x2a6c0},
};
static const export_anchor_t klibSceHmd2Anchor_1000 = {0x136686, "Not Initialized"};
static const export_sig_t klibSceVrTracker2Exports_1000[] = {
  {"sceVrTracker2QueryMemory", 0xe0},
  {"sceVrTracker2Initialize", 0x230},
  {"sceVrTracker2RegisterDevice", 0x11a0},
  {"sceVrTracker2GetResult", 0x2880},
  {"sceVrTracker2UnregisterDevice", 0x13a0},
  {"sceVrTracker2Finalize", 0x1100},
};
static const export_anchor_t klibSceVrTracker2Anchor_1000 = {0x11aa7d, "MININLIERS-FOR-MULTICAMSTRICTGRAVITYBOOT"};
static const export_sig_t klibSceHmd2Exports_900[] = {
  {"sceHmd2GazeGetResult", 0x35ec0},
  {"sceHmd2InternalGazeStart", 0x31870},
  {"sceHmd2InternalGazeStop", 0x31890},
  {"sceHmd2Initialize", 0x2a310},
  {"sceHmd2Open", 0x2a3d0},
  {"sceHmd2GetDeviceInformation", 0x2a820},
  {"sceHmd2GetFieldOfViewWithoutHandle", 0x275b0},
  {"sceHmd2ReprojectionQueryBufferSizeAlign", 0x215a0},
  {"sceHmd2ReprojectionQueryDisplayBufferSizeAlign", 0x21620},
  {"sceHmd2ReprojectionInitialize", 0x216c0},
  {"sceHmd2ReprojectionEnableVrMode", 0x21ba0},
  {"sceHmd2ReprojectionDisableVrMode", 0x21c30},
  {"sceHmd2ReprojectionBeginFrame", 0x21d80},
  {"sceHmd2ReprojectionGetPredictedDisplayTime", 0x21d90},
  {"sceHmd2ReprojectionSetParam", 0x21dc0},
  {"sceHmd2ReprojectionSetRenderConfig", 0x21de0},
  {"sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign", 0x21e60},
  {"sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign", 0x21e70},
  {"sceHmd2ReprojectionSetMirroringOption", 0x21f70},
  {"sceHmd2ReprojectionEnableMirroring", 0x21e80},
  {"sceHmd2ReprojectionDisableMirroring", 0x21f00},
  {"sceHmd2ReprojectionTerminate", 0x21a90},
  {"sceHmd2Close", 0x2a570},
  {"sceHmd2Terminate", 0x2a3a0},
  {"sceHmd2SetVibration", 0x2aa90},
};
static const export_anchor_t klibSceHmd2Anchor_900 = {0x13b4e2, "Not Initialized"};
static const export_sig_t klibSceVrTracker2Exports_900[] = {
  {"sceVrTracker2QueryMemory", 0xe0},
  {"sceVrTracker2Initialize", 0x220},
  {"sceVrTracker2RegisterDevice", 0x1170},
  {"sceVrTracker2GetResult", 0x2870},
  {"sceVrTracker2UnregisterDevice", 0x1380},
  {"sceVrTracker2Finalize", 0x10e0},
};
static const export_anchor_t klibSceVrTracker2Anchor_900 = {0x115a3d, "MININLIERS-FOR-MULTICAMSTRICTGRAVITYBOOT"};
static const export_sig_t klibSceHmd2Exports_840[] = {
  {"sceHmd2GazeGetResult", 0x34950},
  {"sceHmd2InternalGazeStart", 0x30a60},
  {"sceHmd2InternalGazeStop", 0x30a80},
  {"sceHmd2Initialize", 0x29a30},
  {"sceHmd2Open", 0x29af0},
  {"sceHmd2GetDeviceInformation", 0x29f40},
  {"sceHmd2GetFieldOfViewWithoutHandle", 0x26d00},
  {"sceHmd2ReprojectionQueryBufferSizeAlign", 0x21060},
  {"sceHmd2ReprojectionQueryDisplayBufferSizeAlign", 0x210e0},
  {"sceHmd2ReprojectionInitialize", 0x21180},
  {"sceHmd2ReprojectionEnableVrMode", 0x21650},
  {"sceHmd2ReprojectionDisableVrMode", 0x216e0},
  {"sceHmd2ReprojectionBeginFrame", 0x21810},
  {"sceHmd2ReprojectionGetPredictedDisplayTime", 0x21820},
  {"sceHmd2ReprojectionSetParam", 0x21850},
  {"sceHmd2ReprojectionSetRenderConfig", 0x21870},
  {"sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign", 0x21890},
  {"sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign", 0x218a0},
  {"sceHmd2ReprojectionSetMirroringOption", 0x219a0},
  {"sceHmd2ReprojectionEnableMirroring", 0x218b0},
  {"sceHmd2ReprojectionDisableMirroring", 0x21930},
  {"sceHmd2ReprojectionTerminate", 0x21540},
  {"sceHmd2Close", 0x29c90},
  {"sceHmd2Terminate", 0x29ac0},
  {"sceHmd2SetVibration", 0x2a1b0},
};
static const export_anchor_t klibSceHmd2Anchor_840 = {0x13ae30, "warped depth mip chain R"};
static const export_sig_t klibSceVrTracker2Exports_840[] = {
  {"sceVrTracker2QueryMemory", 0xe0},
  {"sceVrTracker2Initialize", 0x220},
  {"sceVrTracker2RegisterDevice", 0x1170},
  {"sceVrTracker2GetResult", 0x2880},
  {"sceVrTracker2UnregisterDevice", 0x1390},
  {"sceVrTracker2Finalize", 0x10e0},
};
static const export_anchor_t klibSceVrTracker2Anchor_840 = {0x115421, "SceVrtMtxDepthMapWork"};
static const export_sig_t klibSceHmd2Exports_761[] = {
  {"sceHmd2GazeGetResult", 0x2e0a0},
  {"sceHmd2InternalGazeStart", 0x29b50},
  {"sceHmd2InternalGazeStop", 0x29b70},
  {"sceHmd2Initialize", 0x22f60},
  {"sceHmd2Open", 0x23020},
  {"sceHmd2GetDeviceInformation", 0x23470},
  {"sceHmd2GetFieldOfViewWithoutHandle", 0x20300},
  {"sceHmd2ReprojectionQueryBufferSizeAlign", 0x1ea90},
  {"sceHmd2ReprojectionQueryDisplayBufferSizeAlign", 0x1eb10},
  {"sceHmd2ReprojectionInitialize", 0x1eba0},
  {"sceHmd2ReprojectionEnableVrMode", 0x1f060},
  {"sceHmd2ReprojectionDisableVrMode", 0x1f0f0},
  {"sceHmd2ReprojectionBeginFrame", 0x1f220},
  {"sceHmd2ReprojectionGetPredictedDisplayTime", 0x1f230},
  {"sceHmd2ReprojectionSetParam", 0x1f260},
  {"sceHmd2ReprojectionSetRenderConfig", 0x1f280},
  {"sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign", 0x1f2a0},
  {"sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign", 0x1f2b0},
  {"sceHmd2ReprojectionSetMirroringOption", 0x1f3b0},
  {"sceHmd2ReprojectionEnableMirroring", 0x1f2c0},
  {"sceHmd2ReprojectionDisableMirroring", 0x1f340},
  {"sceHmd2ReprojectionTerminate", 0x1ef50},
  {"sceHmd2Close", 0x231c0},
  {"sceHmd2Terminate", 0x22ff0},
  {"sceHmd2SetVibration", 0x236e0},
};
static const export_anchor_t klibSceHmd2Anchor_761 = {0x1306c3, "Awarped depth mip chain R"};
static const export_sig_t klibSceVrTracker2Exports_761[] = {
  {"sceVrTracker2QueryMemory", 0xe0},
  {"sceVrTracker2Initialize", 0x230},
  {"sceVrTracker2RegisterDevice", 0xbf0},
  {"sceVrTracker2GetResult", 0x22f0},
  {"sceVrTracker2UnregisterDevice", 0xe00},
  {"sceVrTracker2Finalize", 0xb60},
};
static const export_anchor_t klibSceVrTracker2Anchor_761 = {0x116779, "SceVrtMtxDepthMapWork"};
static const export_sig_t klibSceHmd2Exports_550[] = {
  {"sceHmd2GazeGetResult", 0x24e60},
  {"sceHmd2InternalGazeStart", 0x21690},
  {"sceHmd2InternalGazeStop", 0x216b0},
  {"sceHmd2Initialize", 0x1bf90},
  {"sceHmd2Open", 0x1c050},
  {"sceHmd2GetDeviceInformation", 0x1c3b0},
  {"sceHmd2GetFieldOfViewWithoutHandle", 0x193a0},
  {"sceHmd2ReprojectionQueryBufferSizeAlign", 0x176b0},
  {"sceHmd2ReprojectionQueryDisplayBufferSizeAlign", 0x176d0},
  {"sceHmd2ReprojectionInitialize", 0x17760},
  {"sceHmd2ReprojectionEnableVrMode", 0x17c20},
  {"sceHmd2ReprojectionDisableVrMode", 0x17cb0},
  {"sceHmd2ReprojectionBeginFrame", 0x17d90},
  {"sceHmd2ReprojectionGetPredictedDisplayTime", 0x17da0},
  {"sceHmd2ReprojectionSetParam", 0x17dd0},
  {"sceHmd2ReprojectionSetRenderConfig", 0x17df0},
  {"sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign", 0x17e10},
  {"sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign", 0x17e20},
  {"sceHmd2ReprojectionSetMirroringOption", 0x17f20},
  {"sceHmd2ReprojectionEnableMirroring", 0x17e30},
  {"sceHmd2ReprojectionDisableMirroring", 0x17eb0},
  {"sceHmd2ReprojectionTerminate", 0x17b10},
  {"sceHmd2Close", 0x1c190},
  {"sceHmd2Terminate", 0x1c020},
  {"sceHmd2SetVibration", 0x1c640},
};
static const export_anchor_t klibSceHmd2Anchor_550 = {0x11685e, "SceHmd2ImageMutex"};
static const export_sig_t klibSceVrTracker2Exports_550[] = {
  {"sceVrTracker2QueryMemory", 0xe0},
  {"sceVrTracker2Initialize", 0x250},
  {"sceVrTracker2RegisterDevice", 0xba0},
  {"sceVrTracker2GetResult", 0x2280},
  {"sceVrTracker2UnregisterDevice", 0xd30},
  {"sceVrTracker2Finalize", 0xb10},
};
static const export_anchor_t klibSceVrTracker2Anchor_550 = {0xf8361, "SCALEDPOTHOLE-THRESHOLD"};
#define EXPORT_TABLE(fw, lib) {fw, k##lib##Exports_##fw, COUNT(k##lib##Exports_##fw), &k##lib##Anchor_##fw}
/* libSceVrHand (hand tracking): offsets read on the firmwares it was verified on
 * (13.60: PS5VR 10-03; 12.40: AstroBotVR2 1.0.7). The API differs before 12.20;
 * no table elsewhere means hand tracking stays off there. */
static const export_sig_t klibSceVrHandExports_1360[] = {
  {"sceVrHandInitialize", 0x197c0},
  {"sceVrHandFinalize", 0x1a540},
  {"sceVrHandStart", 0x1a750},
  {"sceVrHandStop", 0x1a6c0},
  {"sceVrHandGetResult", 0x1a9a0},
};
static const export_anchor_t klibSceVrHandAnchor_1360 = {0xb84e0, "handPose_%02d"};
static const export_sig_t klibSceVrHandExports_1240[] = {
  {"sceVrHandInitialize", 0x19510},
  {"sceVrHandFinalize", 0x1a290},
  {"sceVrHandStart", 0x1a4a0},
  {"sceVrHandStop", 0x1a410},
  {"sceVrHandGetResult", 0x1a6f0},
};
static const export_anchor_t klibSceVrHandAnchor_1240 = {0xb8600, "handPose_%02d"};

static const export_table_t klibSceHmd2Tables[] = {
    EXPORT_TABLE(1360, libSceHmd2),
    EXPORT_TABLE(1240, libSceHmd2),
    EXPORT_TABLE(1160, libSceHmd2),
    EXPORT_TABLE(1000, libSceHmd2),
    EXPORT_TABLE(900, libSceHmd2),
    EXPORT_TABLE(840, libSceHmd2),
    EXPORT_TABLE(761, libSceHmd2),
    EXPORT_TABLE(550, libSceHmd2),
};
static const export_table_t klibSceVrTracker2Tables[] = {
    EXPORT_TABLE(1360, libSceVrTracker2),
    EXPORT_TABLE(1240, libSceVrTracker2),
    EXPORT_TABLE(1160, libSceVrTracker2),
    EXPORT_TABLE(1000, libSceVrTracker2),
    EXPORT_TABLE(900, libSceVrTracker2),
    EXPORT_TABLE(840, libSceVrTracker2),
    EXPORT_TABLE(761, libSceVrTracker2),
    EXPORT_TABLE(550, libSceVrTracker2),
};
static const export_table_t klibSceVrHandTables[] = {
    EXPORT_TABLE(1360, libSceVrHand),
    EXPORT_TABLE(1240, libSceVrHand),
};
