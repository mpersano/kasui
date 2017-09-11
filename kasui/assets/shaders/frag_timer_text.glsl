#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D texture;

uniform vec4 text_color;
uniform vec4 outline_color;

varying vec2 frag_texcoord;

void main(void)
{
	vec4 texel = texture2D(texture, frag_texcoord);
	vec4 color = mix(outline_color, text_color, texel.r);
	gl_FragColor = vec4(color.rgb, color.a*texel.a);
}
