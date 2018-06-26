uniform mat4 proj_modelview_matrix;

attribute vec2 position;

void main(void)
{
	gl_Position = proj_modelview_matrix*vec4(position, 0, 1);
}
