#version 300 es

precision highp float;

uniform mat4 proj_modelview;

layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec4 color0;
layout(location=3) in vec4 color1;

out vec2 frag_texcoord;
out vec4 frag_color0;
out vec4 frag_color1;

void main(void)
{
    gl_Position = proj_modelview*vec4(position, 0., 1.);
    frag_texcoord = texcoord;
    frag_color0 = color0;
    frag_color1 = color1;
}
