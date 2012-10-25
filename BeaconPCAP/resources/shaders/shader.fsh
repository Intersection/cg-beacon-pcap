uniform sampler2D tex;
uniform int pingCount;

void main( void )
{
	vec3 sample;
	vec2 st = gl_TexCoord[0].st;
	sample = texture2D( tex, st ).rgb;
	sample.r /= float(pingCount);
	gl_FragColor = vec4( sample, 1.0 );
	
}

