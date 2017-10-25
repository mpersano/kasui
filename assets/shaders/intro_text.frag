#version 300 es

precision highp float;

uniform sampler2D tex;

uniform vec3 top_color_text;
uniform vec3 bottom_color_text;

uniform vec3 top_color_outline;
uniform vec3 bottom_color_outline;

in vec2 frag_texcoord;
in vec4 frag_color;

out vec4 out_color;

void main(void)
{
	vec4 texel = texture(tex, frag_texcoord);

	float frag_mix_factor = frag_color.r;

	vec3 color_text = mix(top_color_text, bottom_color_text, frag_mix_factor);
	vec3 color_outline = mix(top_color_outline, bottom_color_outline, frag_mix_factor);
	vec3 color = mix(color_outline, color_text, texel.r);

	out_color = vec4(color, frag_color.a*texel.a);
}
