#version 330 core
out vec3 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;
uniform vec3 campos;

uniform sampler2D tex;
uniform sampler2D tex2;

void main()
{
    color = vec3(abs(vertex_pos.x) / 128.0, -vertex_pos.z / 255.0, vertex_pos.y / 50.0);
}
