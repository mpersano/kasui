#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D texture;

uniform vec4 color;

varying vec2 frag_texcoord;

void main(void)
{
	vec4 tex_color = texture2D(texture, frag_texcoord);
	gl_FragColor = vec4(tex_color.r*color.r, tex_color.g*color.g, tex_color.b*color.b, tex_color.a*color.a);
}
