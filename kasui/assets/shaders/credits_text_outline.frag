#version 300 es

precision highp float;

uniform sampler2D tex;

in vec2 frag_texcoord;
in vec4 frag_color;

out vec4 out_color;

void main(void)
{
    float v = texture(tex, frag_texcoord).a;
    out_color = vec4(frag_color.rgb, frag_color.a*v);
}
