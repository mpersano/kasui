#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D texture;

uniform vec3 color;

varying vec2 frag_texcoord;
varying float frag_alpha;

void main(void)
{
	float v = texture2D(texture, frag_texcoord).r;
	gl_FragColor = vec4(color, v*frag_alpha);
}
