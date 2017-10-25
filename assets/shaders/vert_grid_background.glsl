uniform mat4 proj_modelview_matrix;

uniform vec3 theme_color;
uniform vec2 highlight_position;
uniform float highlight_fade_factor;

attribute vec2 position;
attribute vec2 texcoord;

varying vec2 frag_texcoord;
varying vec4 frag_color;

void main(void)
{
	gl_Position = proj_modelview_matrix*vec4(position, 0, 1);
	frag_texcoord = texcoord;

	float s = highlight_fade_factor/distance(position, highlight_position);
	frag_color = vec4(mix(theme_color, vec3(1, 1, 1), s), 1);
}
