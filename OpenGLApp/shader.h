#pragma once

#include <string>
#include <glm/glm.hpp>

/**
 * @brief Utility class for loading, compiling, and managing GLSL shaders.
 */
class Shader {
    mutable std::unordered_map<std::string, int> uniformLocations; // Cached uniform locations
    std::string source;

public:
    unsigned int id;
    std::string path;

    Shader(const std::string &path = "");

    /**
    * @brief Compiles the shaders with custom defines and checks errors.
    */
    bool compile(const std::string &defines = "");

    void free();

    /**
     * @brief Activates the shader program.
     */
    void use() const;

    // --- Uniform Setters -----------------------------------------
    // Wrappers for glUniform* functions. They query the location by
    // name and print an error if the uniform is not found/active
    // --------------------------------------------------------------

    // Integer overloads
    void set(const char *name, int v0) const;

    void set(const char *name, int v0, int v1) const;

    void set(const char *name, int v0, int v1, int v2) const;

    void set(const char *name, int v0, int v1, int v2, int v3) const;

    // Float overloads
    void set(const char *name, float v0) const;

    void set(const char *name, float v0, float v1) const;

    void set(const char *name, float v0, float v1, float v2) const;

    void set(const char *name, float v0, float v1, float v2, float v3) const;

    // GLM Vector overloads
    void set(const char *name, const glm::vec1 &v0) const;

    void set(const char *name, const glm::vec2 &v0) const;

    void set(const char *name, const glm::vec3 &v0) const;

    void set(const char *name, const glm::vec4 &v0) const;

    // GLM Matrix overload
    void set(const char *name, const glm::mat4 &v0) const;

private:
    // Compiles a specific shader type
    unsigned int generate(const std::string &type, const std::string &defines = "") const;

    // Checks for shader compiling or program linking errors
    bool checkCompileErrors(unsigned int id, const std::string &type) const;

    // Retrieves the uniform location from cached values
    int getUniform(const char *name) const;
};