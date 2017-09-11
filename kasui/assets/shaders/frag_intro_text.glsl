#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D texture;

uniform vec4 top_color_text;
uniform vec4 bottom_color_text;

uniform vec4 top_color_outline;
uniform vec4 bottom_color_outline;

varying vec2 frag_texcoord;
varying float frag_mix_factor;

void main(void)
{
	vec4 texel = texture2D(texture, frag_texcoord);

	vec4 color_text = mix(top_color_text, bottom_color_text, frag_mix_factor);
	vec4 color_outline = mix(top_color_outline, bottom_color_outline, frag_mix_factor);

	vec4 color = mix(color_outline, color_text, texel.r);

	gl_FragColor = vec4(color.rgb, color.a*texel.a);
}
