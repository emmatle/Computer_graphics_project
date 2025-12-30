// #version 330 core

#ifdef VERTEX_SHADER

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model = mat4(1.0f);
uniform mat4 view = mat4(1.0f);
uniform mat4 projection = mat4(1.0f);

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
}

#endif

#ifdef FRAGMENT_SHADER

out int FragColor; // Must be an integer to write to the GL_R32I texture

uniform int id = 0;

void main() {
    FragColor = id;
}

#endif