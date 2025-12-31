// #version 330 core

#ifdef VERTEX_SHADER

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 Tangent;
out vec3 Bitangent;

uniform mat4 model = mat4(1.0);
uniform mat4 view = mat4(1.0);
uniform mat4 projection = mat4(1.0);

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));

    mat3 normalMatrix = mat3(transpose(inverse(model)));

    Normal = normalMatrix * aNormal;
    TexCoords = aTexCoords;
    Tangent = normalMatrix * aTangent;
    Bitangent = normalMatrix * aBitangent;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 Tangent;
in vec3 Bitangent;

out vec4 FragColor;

uniform float k_a = 0.05; // Ambient light reflection coefficient
uniform vec3 I_a = vec3(1.0); // Ambient light color

uniform float k_d = 0.8; // Diffuse light reflection coefficient
uniform vec3 I_d = vec3(1.0); // Diffuse light color
uniform vec3 lightPos = vec3(-0.3, 2.0, -0.3); // Diffuse light position // TODO: update on lights.

uniform float k_s = 0.5; // Specular coefficient
uniform vec3 I_s = vec3(1.0); // Specular light color
uniform vec3 viewPos;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D metalnessMap;

uniform bool hasDiffuse = false;
uniform bool hasNormal = false;
uniform bool hasRoughness = false;
uniform bool hasMetalness = false;

void main() {
    vec3 N = normalize(Normal);
    vec3 T = normalize(Tangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    vec3 L = normalize(lightPos - FragPos);
    vec3 V = normalize(viewPos - FragPos);
    vec3 H = normalize(L + V);

    vec3 baseColor = hasDiffuse ? texture(diffuseMap, TexCoords).rgb : vec3(1.0);

    vec3 normal = hasNormal ? texture(normalMap, TexCoords).rgb : vec3(0.5, 0.5, 1.0);

    float metalness = hasMetalness ? texture(metalnessMap, TexCoords).r : 0.0;

    // For metals, diffuse should be black (0.0). For non-metals, use baseColor.
    vec3 diffuseColor = mix(baseColor, vec3(0.0), metalness);

    // For metals, specular is tinted by the baseColor. For non-metals, it's white.
    vec3 specularColor = mix(vec3(1.0), baseColor, metalness);

    normal = normal * 2.0 - 1.0; // Map normal [0, 1] -> [-1, 1]
    mat3 TBN = mat3(T, B, N);
    N = normalize(TBN * normal);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    float roughness = texture(roughnessMap, TexCoords).r;
    float shininess = hasRoughness ? mix(256.0, 1.0, roughness) : 8.0; // Map roughness [0, 1] -> shininess (high, low)

    vec3 ambient = k_a * I_a * diffuseColor;
    vec3 diffuse = k_d * I_d * NdotL * diffuseColor;
    vec3 specular = k_s * I_s * pow(NdotH, shininess) * specularColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}

#endif