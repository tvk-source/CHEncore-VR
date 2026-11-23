#version 450
// Mask the source game's black/charcoal background when sampling its live HUD.
// This removes the conspicuous black rectangles from peripheral cinema crops.
layout(set=0,binding=0) uniform sampler2D gameImage;
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
void main(){
    vec3 rgb=texture(gameImage,uv).rgb;
    float signal=max(rgb.r,max(rgb.g,rgb.b));
    float a=smoothstep(0.035,0.17,signal);
    outColor=vec4(rgb,a);
}
