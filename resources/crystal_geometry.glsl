#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

in vec3 vertex_pos[];
in vec2 vertex_tex[];

out vec3 position;
out vec2 texture;
out vec3 normal;
 
void main()
{
    vec3 ba = (gl_in[1].gl_Position - gl_in[0].gl_Position).xyz;
    vec3 ca = (gl_in[2].gl_Position - gl_in[0].gl_Position).xyz;
    normal = normalize(cross(ba, ca));
    for (int i = 0; i < gl_in.length(); i++) {
        gl_Position = gl_in[i].gl_Position;
        position = vertex_pos[i];
        texture = vertex_tex[i];
        EmitVertex();
    }
    EndPrimitive();
}