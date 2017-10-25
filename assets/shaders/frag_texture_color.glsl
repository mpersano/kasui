#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D texture;

varying vec2 frag_texcoord;
varying vec4 frag_color;

void main(void)
{
	gl_FragColor = texture2D(texture, frag_texcoord)*frag_color;
}
