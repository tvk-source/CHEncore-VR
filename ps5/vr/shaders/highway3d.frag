#version 450
// Palette-indexed solid 3D stage, not an image of the original game.
// inUV.x = logical lane/palette, inUV.y = intensity.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
void main(){
    int i=int(uv.x+.5);
    vec3 c=vec3(.018,.025,.046);
    if(i==1)c=vec3(.10,.82,.22);       // G
    else if(i==2)c=vec3(1.0,.10,.13);  // R
    else if(i==3)c=vec3(1.,.79,.06);   // Y
    else if(i==4)c=vec3(.10,.33,1.);   // B
    else if(i==5)c=vec3(1.,.37,.06);   // O
    else if(i==6)c=vec3(.50,.64,.81);
    outColor=vec4(clamp(c*uv.y,0.,1.),1.);
}
