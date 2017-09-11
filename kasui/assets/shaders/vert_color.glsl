uniform mat4 proj_modelview_matrix;

attribute vec2 position;
attribute vec4 color;

varying vec4 frag_color;

void main(void)
{
	gl_Position = proj_modelview_matrix*vec4(position, 0, 1);
	frag_color = color/255.;
}
