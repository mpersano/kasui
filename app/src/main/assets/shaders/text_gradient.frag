#version 300 es

precision highp float;

uniform sampler2D tex;

in vec2 frag_texcoord;
in vec4 frag_color0;
in vec4 frag_color1;

out vec4 out_color;

void main(void)
{
	vec4 texel = texture(tex, frag_texcoord);
	vec4 color = mix(frag_color0, frag_color1, texel.r);
	out_color = vec4(color.rgb, color.a*texel.a);
}
