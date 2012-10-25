uniform sampler2D tex;

// Brightness of pixels
uniform float bright;

// 'LED' resolution
uniform float ledScale;

vec4 pixelize(vec2 uv, float sca) {
    float dx = 1.0/sca;
    float dy = 1.0/sca;
    vec2 coord = vec2(dx * ceil(uv.x/dx),
                      dy * ceil(uv.y/dy));
    return texture2D(tex, coord);
}

void main()
{
    vec2 uv = gl_TexCoord[0].st;
    
    vec2 coor = gl_TexCoord[0].st * ledScale;
    vec4 resColor = pixelize(uv, ledScale) * bright;
    
    float mvx = abs(sin(coor.s * 3.1415)) * 1.5;
    float mvy = abs(sin(coor.t * 3.1415)) * 1.5;
    if (mvx * mvy < 0.9)
        resColor = resColor * (mvx * mvy);
    
    gl_FragColor = resColor;
}