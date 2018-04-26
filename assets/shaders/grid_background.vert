#version 300 es

precision highp float;

uniform mat4 proj_modelview;

layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec4 color;

uniform vec2 highlight_position;
uniform float highlight_fade_factor;

out vec2 frag_texcoord;
out vec4 frag_color;

void main(void)
{
	gl_Position = proj_modelview*vec4(position, 0., 1.);
	frag_texcoord = texcoord;
	float s = highlight_fade_factor/distance(position, highlight_position);
	frag_color = vec4(mix(color, vec4(1.), s));
}
