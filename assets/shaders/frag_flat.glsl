#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D texture;

uniform vec4 color;

varying vec2 frag_texcoord;

void main(void)
{
	gl_FragColor = color;
}
