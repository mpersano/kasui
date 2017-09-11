#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D texture;

varying vec2 frag_texcoord;
varying float frag_alpha;

void main(void)
{
	vec4 color = texture2D(texture, frag_texcoord);
	gl_FragColor = vec4(color.rgb, color.a*frag_alpha);
}
