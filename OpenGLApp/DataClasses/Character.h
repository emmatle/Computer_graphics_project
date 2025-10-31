#ifndef CHARACTER_H
#define CHARACTER_H

#include <glm/glm.hpp>

struct Character {
    unsigned int TextureID;  // ID della texture del glifo
    glm::ivec2   Size;       // Dimensione del glifo
    glm::ivec2   Bearing;    // Offset dal baseline al glifo
    unsigned int Advance;    // Offset alla lettera successiva (in 1/64 pixel, come da FreeType)
};

#endif // CHARACTER_H