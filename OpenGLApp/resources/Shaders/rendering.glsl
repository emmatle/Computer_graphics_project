// #version 330 core

#ifdef VERTEX_SHADER

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model = mat4(1.0f);
uniform mat4 view = mat4(1.0f);
uniform mat4 projection = mat4(1.0f);

out vec2 TexCoord;

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	TexCoord = aTexCoord;
}

#endif

#ifdef FRAGMENT_SHADER

in vec2 TexCoord;

uniform sampler2D texture1;

out vec4 FragColor;

void main() {
    FragColor = texture(texture1, TexCoord);
}

#endif