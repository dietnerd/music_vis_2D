#version 330 core
out vec3 color;

in vec3 position;
in vec2 texture;
in vec3 normal;

uniform vec3 campos;
uniform sampler2D tex;
uniform sampler2D tex2;

void main()
{
    vec3 n = normalize(normal);
    vec3 lp=vec3(10,-20,-100);
    vec3 ld = normalize(position - lp);
    float diffuse = dot(n,ld);

    color = vec3(1, 1, 1);//texture(tex, texture).rgb;

    color *= pow(diffuse, 3);
}
