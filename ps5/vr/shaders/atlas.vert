#version 450
// Source PC sprite mesh already reprojected into each independent eye.
layout(location=0) in vec4 inClip;
layout(location=1) in vec2 inUV;
layout(location=2) in vec4 inColor;
layout(location=3) in vec2 inUVNext;
layout(location=4) in float inFrameBlend;
layout(location=0) out vec2 uv;
layout(location=1) out vec4 tint;
layout(location=2) out vec2 uvNext;
layout(location=3) out float frameBlend;
void main(){
    gl_Position=inClip;
    uv=inUV;
    tint=inColor;
    uvNext=inUVNext;
    frameBlend=inFrameBlend;
}
