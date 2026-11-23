#version 450
// Match Encore's authored two-frame note animation, optional lane tint and
// purple tap sheen. Transparent texture edges keep original rounded shapes.
layout(set=0,binding=0) uniform sampler2D gameImage;
layout(location=0) in vec2 uv;
layout(location=1) in vec4 tint;
layout(location=2) in vec2 uvNext;
layout(location=3) in float frameBlend;
layout(location=0) out vec4 outColor;
void main(){
    bool laneTint=frameBlend<0.0;
    float tagged=laneTint?(-frameBlend-1.0):frameBlend;
    bool tapSheen=tagged>=2.0;
    float blend=clamp(tapSheen?tagged-2.0:tagged,0.0,1.0);
    vec4 texel=texture(gameImage,uv);
    if(blend>0.001)texel=mix(texel,texture(gameImage,uvNext),blend);
    if(laneTint)texel.rgb=vec3(max(texel.r,max(texel.g,texel.b)));
    vec4 color=texel*tint;
    if(tapSheen)color.rgb=mix(color.rgb,vec3(.68,.35,1.)*max(color.r,max(color.g,color.b)),.14);
    outColor=clamp(color,0.0,1.0);
}
