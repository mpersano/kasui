uniform mat4 proj_modelview_matrix;

attribute vec2 position;
attribute vec2 texcoord;

varying vec2 frag_texcoord;

void main(void)
{
	gl_Position = proj_modelview_matrix*vec4(position, 0, 1);
	frag_texcoord = texcoord;
}
