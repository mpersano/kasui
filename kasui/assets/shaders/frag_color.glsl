#ifdef GL_ES
precision mediump float;
#endif

varying vec4 frag_color;

void main(void)
{
	gl_FragColor = frag_color;
}
