uniform mat4 proj_modelview_matrix;

attribute vec2 position;
attribute vec2 texcoord;
attribute vec4 color;

varying vec2 frag_texcoord;
varying vec4 frag_color;

void main(void)
{
	gl_Position = proj_modelview_matrix*vec4(position, 0, 1);
	frag_texcoord = texcoord;
	frag_color = color/255.;
}
