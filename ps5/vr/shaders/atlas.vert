#version 450
// PC source vertex in per-eye homogeneous clip coordinates.
layout(location=0) in vec4 inClip;
layout(location=1) in vec2 inUV;
layout(location=2) in vec4 inColor;
layout(location=0) out vec2 uv;
layout(location=1) out vec4 tint;
void main(){
    gl_Position=inClip;
    uv=inUV;
    tint=inColor;
}
