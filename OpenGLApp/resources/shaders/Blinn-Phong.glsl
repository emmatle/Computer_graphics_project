// #version 330 core

#ifdef VERTEX_SHADER

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;

uniform mat4 model = mat4(1.0);
uniform mat4 view = mat4(1.0);
uniform mat4 projection = mat4(1.0);

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));

    mat3 normalMatrix = mat3(transpose(inverse(model)));

    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt orthogonalization
    vec3 B = cross(N, T);

    TBN = mat3(T, B, N);

    Normal = N;
    TexCoords = aTexCoords;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

#define MAX_LIGHTS 8

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

out vec4 FragColor;

// --- Material uniforms ---

// Illumination model: 0 No shading (Base color), 1 Ambient + Diffuse (Lambert), 2 Ambient + Diffuse + Specular (Blinn-Phong)
uniform vec3 ambientLight = vec3(1.0, 0.82, 0.62); // 4000K white;
vec3 k_a = vec3(0.1); // No ambient reflectivity uniform for simplicity
uniform vec3 k_d = vec3(1.0);
uniform vec3 k_s = vec3(0.5);
uniform int illum = 2;
uniform float shininess = 32.0;

uniform sampler2D diffuseMap;
uniform sampler2D ambientOcclusionMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D metalnessMap;
uniform bool hasDiffuseMap = false;
uniform bool hasAmbientOcclusionMap = false;
uniform bool hasNormalMap = false;
uniform bool hasRoughnessMap = false;
uniform bool hasMetalnessMap = false;

// --- Light uniforms ---

struct Light {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};

uniform Light lights[MAX_LIGHTS];
uniform int numLights;
uniform vec3 viewPos;

void main() {
    vec3 V = normalize(viewPos - FragPos);

    vec3 baseColor = hasDiffuseMap ? texture(diffuseMap, TexCoords).rgb: k_d;
    vec3 ambientColor = hasAmbientOcclusionMap ? vec3(texture(ambientOcclusionMap, TexCoords).r) * ambientLight: ambientLight;
    vec3 normal = hasNormalMap ? texture(normalMap, TexCoords).rgb : vec3(0.5, 0.5, 1.0);
    float roughness = hasRoughnessMap ? texture(roughnessMap, TexCoords).r : 0.125;
    float metalness = hasMetalnessMap ? texture(metalnessMap, TexCoords).r : 0.0;

    if (illum == 0) {
        FragColor = vec4(baseColor, 1.0);
        return;
    }

    // Map normals [0, 1] -> [-1, 1]
    normal = normalize(normal * 2.0 - 1.0);
    vec3 N = hasNormalMap ? normalize(TBN * normal) : normalize(Normal);

    // Metals have no diffuse, their color is in the specular reflection
    vec3 diffuseColor = mix(baseColor, vec3(0.0), metalness);
    vec3 specularColor = mix(vec3(1.0), baseColor, metalness);

    // Map roughness to shininess exponent
    float exponent = hasRoughnessMap ? mix(256.0, 2.0, roughness) : shininess;

    // Lighting Loop
    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);

    for(int i = 0; i < numLights; i++) {
        vec3 L = normalize(lights[i].position - FragPos);
        vec3 H = normalize(L + V);

        // Attenuation
        float distance = length(lights[i].position - FragPos);
        float attenuation = 1.0 / (lights[i].constant +
                                   lights[i].linear * distance +
                                   lights[i].quadratic * (distance * distance));

        // Diffuse
        float diff = max(dot(N, L), 0.0);
        totalDiffuse += diff * lights[i].color * attenuation;

        if (illum < 2) continue;
        // Specular
        float spec = pow(max(dot(N, H), 0.0), exponent);
        totalSpecular += spec * lights[i].color * attenuation;
    }

    vec3 ambient = k_a * baseColor * ambientColor; // Basic global ambient
    vec3 diffuse = k_d * totalDiffuse * diffuseColor;
    vec3 specular = k_s * totalSpecular * specularColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}

#endif