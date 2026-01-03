// #version 330 core

#ifdef VERTEX_SHADER

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoords = aTex;
}

#endif

#ifdef FRAGMENT_SHADER

in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D text;
uniform vec3 textColor;
void main() {
    float alpha = texture(text, TexCoords).r;
    FragColor = vec4(textColor, alpha);
}

#endif