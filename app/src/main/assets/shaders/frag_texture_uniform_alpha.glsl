#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D texture;

uniform float alpha;

varying vec2 frag_texcoord;

void main(void)
{
	vec4 color = texture2D(texture, frag_texcoord);
	gl_FragColor = vec4(color.rgb, color.a*alpha);
}
