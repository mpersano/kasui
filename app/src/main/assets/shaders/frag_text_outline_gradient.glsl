#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D texture;

uniform vec4 top_color;
uniform vec4 bottom_color;

varying vec2 frag_texcoord;
varying float frag_mix_factor;

void main(void)
{
	float v = texture2D(texture, frag_texcoord).a;
	vec4 color = mix(top_color, bottom_color, frag_mix_factor);
	gl_FragColor = vec4(color.rgb, color.a*v);
}
