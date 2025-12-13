#pragma once

// TODO: remove when importing using models.

// Vertex data structure
struct Vertex {
    float position[3];
    float texCoords[2];
};

// Constant vertex data for a cube
static constexpr float VERTEX_DATA[180] = {
        // pos.x, pos.y, pos.z, tex.u, tex.v (repeated for 6 faces * 6 vertices)
        -0.5f, -0.5f, -0.5f, 0.f, 0.f,
        0.5f, -0.5f, -0.5f, 1.f, 0.f,
        0.5f, 0.5f, -0.5f, 1.f, 1.f,
        0.5f, 0.5f, -0.5f, 1.f, 1.f,
        -0.5f, 0.5f, -0.5f, 0.f, 1.f,
        -0.5f, -0.5f, -0.5f, 0.f, 0.f,

        -0.5f, -0.5f, 0.5f, 0.f, 0.f,
        0.5f, -0.5f, 0.5f, 1.f, 0.f,
        0.5f, 0.5f, 0.5f, 1.f, 1.f,
        0.5f, 0.5f, 0.5f, 1.f, 1.f,
        -0.5f, 0.5f, 0.5f, 0.f, 1.f,
        -0.5f, -0.5f, 0.5f, 0.f, 0.f,

        -0.5f, 0.5f, 0.5f, 1.f, 0.f,
        -0.5f, 0.5f, -0.5f, 1.f, 1.f,
        -0.5f, -0.5f, -0.5f, 0.f, 1.f,
        -0.5f, -0.5f, -0.5f, 0.f, 1.f,
        -0.5f, -0.5f, 0.5f, 0.f, 0.f,
        -0.5f, 0.5f, 0.5f, 1.f, 0.f,

        0.5f, 0.5f, 0.5f, 1.f, 0.f,
        0.5f, 0.5f, -0.5f, 1.f, 1.f,
        0.5f, -0.5f, -0.5f, 0.f, 1.f,
        0.5f, -0.5f, -0.5f, 0.f, 1.f,
        0.5f, -0.5f, 0.5f, 0.f, 0.f,
        0.5f, 0.5f, 0.5f, 1.f, 0.f,

        -0.5f, -0.5f, -0.5f, 0.f, 1.f,
        0.5f, -0.5f, -0.5f, 1.f, 1.f,
        0.5f, -0.5f, 0.5f, 1.f, 0.f,
        0.5f, -0.5f, 0.5f, 1.f, 0.f,
        -0.5f, -0.5f, 0.5f, 0.f, 0.f,
        -0.5f, -0.5f, -0.5f, 0.f, 1.f,

        -0.5f, 0.5f, -0.5f, 0.f, 1.f,
        0.5f, 0.5f, -0.5f, 1.f, 1.f,
        0.5f, 0.5f, 0.5f, 1.f, 0.f,
        0.5f, 0.5f, 0.5f, 1.f, 0.f,
        -0.5f, 0.5f, 0.5f, 0.f, 0.f,
        -0.5f, 0.5f, -0.5f, 0.f, 1.f
};