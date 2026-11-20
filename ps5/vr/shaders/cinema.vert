#version 450
// CinemaVertex: homogeneous Vulkan clip position, followed by source UV.
// Preserve w for perspective-correct interpolation; no manual division.
layout(location=0) in vec4 inClip;
layout(location=1) in vec2 inUV;
layout(location=0) out vec2 uv;
void main() {
    gl_Position = inClip;
    uv = inUV;
}
