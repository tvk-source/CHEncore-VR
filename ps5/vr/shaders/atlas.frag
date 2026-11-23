#version 450
// Source game atlas is GPU-owned and shared read-only by the VR renderer.
layout(set=0,binding=0) uniform sampler2D gameImage;
layout(location=0) in vec2 uv;
layout(location=1) in vec4 tint;
layout(location=0) out vec4 outColor;
void main(){
    vec4 texel=texture(gameImage,uv);
    outColor=vec4(texel.rgb*tint.rgb,texel.a*tint.a);
}
