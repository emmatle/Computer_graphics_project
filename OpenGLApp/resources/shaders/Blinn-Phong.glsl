// #version 330 core

#ifdef VERTEX_SHADER

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model = mat4(1.0);
uniform mat4 view = mat4(1.0);
uniform mat4 projection = mat4(1.0);

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    Normal = normalMatrix * aNormal;

    TexCoords = aTexCoords;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;


uniform float k_a = 0.05; // Ambient light reflection coefficient
uniform vec3 I_a = vec3(1.0); // Ambient light color

uniform float k_d = 0.8; // Diffuse light reflection coefficient
uniform vec3 I_d = vec3(1.0); // Diffuse light color
uniform vec3 lightPos = vec3(-0.3, 2.0, -0.3); // Diffuse light position // TODO: update on lights.

uniform float k_s = 0.5; // Specular coefficient
uniform vec3 I_s = vec3(1.0); // Specular light color
uniform vec3 viewPos;

// TODO: Add normal map.
uniform sampler2D diffuseMap;
uniform sampler2D roughnessMap;

void main() {
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
    vec3 V = normalize(viewPos - FragPos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    vec3 color = texture(diffuseMap, TexCoords).rgb;
    // float alpha = texture(diffuseMap, TexCoords).a; // TODO: Enable transparency.

    // Map roughness [0, 1] to shininess (high, low) // TODO: Use roughness maps.
    // float roughness = texture(roughnessMap, TexCoords).r;
    // float shininess = mix(256.0, 1.0, roughness);

    float shininess = 8.0;

    vec3 ambient = k_a * I_a;
    vec3 diffuse = k_d * I_d * NdotL;
    vec3 specular = k_s * I_s * pow(NdotH, shininess);

    FragColor = vec4((ambient + diffuse) * color + specular, 1.0);
}

#endif