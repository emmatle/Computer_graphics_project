// #version 330 core

#ifdef VERTEX_SHADER

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec4 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model = mat4(1.f);
uniform mat4 view = mat4(1.f);
uniform mat4 projection = mat4(1.f);

void main() {
    FragPos = model * vec4(aPos, 1.f);
    Normal = aNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * FragPos;
}

#endif

#ifdef FRAGMENT_SHADER

in vec4 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform sampler2D diffuse;
uniform sampler2D normal;
uniform sampler2D specular;
uniform sampler2D displacement;

void main() {
    vec3 normalMap = texture(normal, TexCoords).rgb; // TODO: Use normals for the lighting model.
    FragColor = texture(diffuse, TexCoords);
}

#endif