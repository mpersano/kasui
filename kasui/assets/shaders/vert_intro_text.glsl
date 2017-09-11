uniform mat4 proj_modelview_matrix;

attribute vec2 position;
attribute vec2 texcoord;

attribute float mix_factor;

varying vec2 frag_texcoord;
varying float frag_mix_factor;

void main(void)
{
	gl_Position = proj_modelview_matrix*vec4(position, 0, 1);
	frag_texcoord = texcoord;
	frag_mix_factor = mix_factor;
}
