#include "shader.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <glad/glad.h>

Shader::Shader(const std::string &path) : id(0), path(getResourcePath(path)) {}

bool Shader::compile(const std::string &defines) {
    if (id != 0) return true; // Already compiled
    if (path.empty()) {
        std::cerr << "ERROR: unspecified shader path" << std::endl;
        return false;
    }
    std::ifstream in(path);

    if (!in) {
        std::cerr << "ERROR: missing " << path << " shader" << std::endl;
        return false;
    }

    // Configure stream to throw exceptions on failure for better error handling
    in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        std::stringstream stream;

        // Read file buffer into stream
        stream << in.rdbuf();
        source = stream.str();

        in.close();
    } catch (std::ifstream::failure &e) {
        std::cerr << "ERROR: failed to read " << path << " shader " << e.what() << std::endl;
        return false;
    }

    unsigned int vertex;
    unsigned int fragment;

    // Compile both stages using the same source string but different preprocessor definitions
    vertex = generate("vertex", defines);
    fragment = generate("fragment", defines);

    if (fragment == 0 || vertex == 0) return false;

    // Create the final program object and link the stages
    id = glCreateProgram();

    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);

    bool success = checkCompileErrors(id, "program");

    // Delete intermediate shader objects; they are no longer needed after linking
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return success;
}

void Shader::free() {
    if (id != 0) {
        glDeleteProgram(id);
        id = 0;
    }
}

void Shader::use() const {
    glUseProgram(id);
}

void Shader::set(const char *name, int v0) const {
    int uniformId = getUniform(name);
    glUniform1i(uniformId, v0);
}

void Shader::set(const char *name, int v0, int v1) const {
    int uniformId = getUniform(name);
    glUniform2i(uniformId, v0, v1);
}

void Shader::set(const char *name, int v0, int v1, int v2) const {
    int uniformId = getUniform(name);
    glUniform3i(uniformId, v0, v1, v2);
}

void Shader::set(const char *name, int v0, int v1, int v2, int v3) const {
    int uniformId = getUniform(name);
    glUniform4i(uniformId, v0, v1, v2, v3);
}

void Shader::set(const char *name, float v0) const {
    int uniformId = getUniform(name);
    glUniform1f(uniformId, v0);
}

void Shader::set(const char *name, float v0, float v1) const {
    int uniformId = getUniform(name);
    glUniform2f(uniformId, v0, v1);
}

void Shader::set(const char *name, float v0, float v1, float v2) const {
    int uniformId = getUniform(name);
    glUniform3f(uniformId, v0, v1, v2);
}

void Shader::set(const char *name, float v0, float v1, float v2, float v3) const {
    int uniformId = getUniform(name);
    glUniform4f(uniformId, v0, v1, v2, v3);
}

void Shader::set(const char *name, const glm::vec1 &v0) const {
    int uniformId = getUniform(name);
    glUniform1fv(uniformId, 1, &v0[0]);
}

void Shader::set(const char *name, const glm::vec2 &v0) const {
    int uniformId = getUniform(name);
    glUniform2fv(uniformId, 1, &v0[0]);
}

void Shader::set(const char *name, const glm::vec3 &v0) const {
    int uniformId = getUniform(name);
    glUniform3fv(uniformId, 1, &v0[0]);
}

void Shader::set(const char *name, const glm::vec4 &v0) const {
    int uniformId = getUniform(name);
    glUniform4fv(uniformId, 1, &v0[0]);
}

// GLM Matrix overload
void Shader::set(const char *name, const glm::mat4 &v0) const {
    int uniformId = getUniform(name);
    glUniformMatrix4fv(uniformId, 1, GL_FALSE, &v0[0][0]);
}

unsigned int Shader::generate(const std::string &type, const std::string &defines) const {
    unsigned int shader;

    std::string pre = "#version 330 core\n" + defines;

    if (type == "vertex") {
        pre += "\n#define VERTEX_SHADER\n\n";
        shader = glCreateShader(GL_VERTEX_SHADER);
    } else if (type == "fragment") {
        pre += "\n#define FRAGMENT_SHADER\n\n";
        shader = glCreateShader(GL_FRAGMENT_SHADER);
    } else {
        return 0;
    }

    pre += source;
    const char *code = pre.c_str();

    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);
    checkCompileErrors(shader, type);

    return shader;
}

bool Shader::checkCompileErrors(unsigned int id, const std::string &type) const {
    int success;
    char infoLog[1024];
    if (type == "program") {
        glGetProgramiv(id, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(id, 1024, nullptr, infoLog);
            std::cerr << "ERROR: failed to link " << path << " shader program:\n" << infoLog;
        }
    } else {
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(id, 1024, nullptr, infoLog);
            std::cerr << "ERROR: failed to compile " << path << " " << type << " shader:\n" << infoLog;
        }
    }
    return success;
}

int Shader::getUniform(const char *name) const {
    static bool silenceWarning = false; // Prevents multiple prints in loops
    auto it = uniformLocations.find(name);
    if (it != uniformLocations.end()) {
        return it->second;
    }

    int uniformID = glGetUniformLocation(id, name);
    if (uniformID == -1) {
        if (!silenceWarning) {
            std::cerr << "WARNING: invalid uniform name \"" << name << "\"" << std::endl;
            silenceWarning = true;
        }
    } else {
        uniformLocations[name] = uniformID;
    }
    return uniformID;
}
