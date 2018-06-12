#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

uniform int currIndex;
uniform int xtex;
uniform float scaling;

uniform sampler2D tex;
uniform sampler2D tex2;

out vec3 vertex_pos;
out vec3 vertex_normal;
out vec2 vertex_tex;

void main()
{
    int xoff = gl_InstanceID % xtex;
    int zoff = gl_InstanceID / xtex;
    float x = float(xoff) / xtex;
    float y = (float(zoff) + float(currIndex + 1)) / xtex;
    if (y > 1.0)
        y -= 1.0;
    float yoff = texture(tex, vec2(x, y)).r * 250;

	vertex_normal = vec4(M * vec4(vertNor,0.0)).xyz;

	vec4 tpos =  M * vec4(vertPos, 1.0) + vec4(2.0 * scaling * xoff - scaling * xtex, yoff - 1.0, zoff * scaling * -2.0, 1.0);
	vertex_pos = tpos.xyz;
	gl_Position = P * V * tpos;
	vertex_tex = vertTex;
}
