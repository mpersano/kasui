#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D texture;

uniform vec4 color;

varying vec2 frag_texcoord;

void main(void)
{
	float v = texture2D(texture, frag_texcoord).a;
	gl_FragColor = vec4(color.rgb, color.a*v);
}
