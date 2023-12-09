#include "graphics/drawable/box.h"
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader_header/device.h"
#include "shader_header/vertex_info.h"

#include "graphics/bindable/vertex_buffer.h"
#include "graphics/bindable/index_buffer.h"

Box::Box(Graphics& gfx, vertex::Layout& layout)
{
    if (!layout.hasElement(vertex::AttributeType::Pos3d))
    {
        layout.append(vertex::AttributeType::Pos3d);
    }


    //    1________ 2        y+
    //    /|      /|         ^
    //   /_|_____/ |         |
    //  5|0|_ _ 6|_|3        |----->x+
    //   | /     | /        /
    //   |/______|/        /
    //  4       7         z+


    vertex::Buffer vb(layout, 24);

    //         y+
    // 2----1  ^
    // |    |  |
    // |    |  |
    // 3----0  +---> x-
    vb[+0].attr<vertex::AttributeType::Pos3d>() = { -1.0f, -1.0f, -1.0f };
    vb[+1].attr<vertex::AttributeType::Pos3d>() = { -1.0f, +1.0f, -1.0f };
    vb[+2].attr<vertex::AttributeType::Pos3d>() = { +1.0f, +1.0f, -1.0f };
    vb[+3].attr<vertex::AttributeType::Pos3d>() = { +1.0f, -1.0f, -1.0f };

    //         y+
    // 6----2  ^
    // |    |  |
    // |    |  |
    // 7----3  +---> z-
    vb[+4].attr<vertex::AttributeType::Pos3d>() = { +1.0f, +1.0f, -1.0f };
    vb[+5].attr<vertex::AttributeType::Pos3d>() = { +1.0f, -1.0f, -1.0f };
    vb[+6].attr<vertex::AttributeType::Pos3d>() = { +1.0f, +1.0f, +1.0f };
    vb[+7].attr<vertex::AttributeType::Pos3d>() = { +1.0f, -1.0f, +1.0f };

    //         y+
    // 5----6  ^
    // |    |  |
    // |    |  |
    // 4----7  +---> x+
    vb[+8].attr<vertex::AttributeType::Pos3d>() = { -1.0f, -1.0f, +1.0f };
    vb[+9].attr<vertex::AttributeType::Pos3d>() = { -1.0f, +1.0f, +1.0f };
    vb[10].attr<vertex::AttributeType::Pos3d>() = { +1.0f, +1.0f, +1.0f };
    vb[11].attr<vertex::AttributeType::Pos3d>() = { +1.0f, -1.0f, +1.0f };

    //         y+
    // 1----5  ^
    // |    |  |
    // |    |  |
    // 0----4  +---> z+
    vb[12].attr<vertex::AttributeType::Pos3d>() = { -1.0f, -1.0f, -1.0f };
    vb[13].attr<vertex::AttributeType::Pos3d>() = { -1.0f, +1.0f, -1.0f };
    vb[14].attr<vertex::AttributeType::Pos3d>() = { -1.0f, -1.0f, +1.0f };
    vb[15].attr<vertex::AttributeType::Pos3d>() = { -1.0f, +1.0f, +1.0f };

    //         y+
    // 1----2  ^
    // |    |  |
    // |    |  |
    // 5----6  +---> z+
    vb[16].attr<vertex::AttributeType::Pos3d>() = { -1.0f, +1.0f, -1.0f };
    vb[17].attr<vertex::AttributeType::Pos3d>() = { +1.0f, +1.0f, -1.0f };
    vb[18].attr<vertex::AttributeType::Pos3d>() = { -1.0f, +1.0f, +1.0f };
    vb[19].attr<vertex::AttributeType::Pos3d>() = { +1.0f, +1.0f, +1.0f };

    //         y+
    // 4----7  ^
    // |    |  |
    // |    |  |
    // 0----3  +---> z+
    vb[20].attr<vertex::AttributeType::Pos3d>() = { -1.0f, -1.0f, -1.0f };
    vb[21].attr<vertex::AttributeType::Pos3d>() = { +1.0f, -1.0f, -1.0f };
    vb[22].attr<vertex::AttributeType::Pos3d>() = { -1.0f, -1.0f, +1.0f };
    vb[23].attr<vertex::AttributeType::Pos3d>() = { +1.0f, -1.0f, +1.0f };

    if (layout.hasElement(vertex::AttributeType::TexCoords))
    {
        vb[+0].attr<vertex::AttributeType::TexCoords>() = { 1.0f, 0.0f };
        vb[+1].attr<vertex::AttributeType::TexCoords>() = { 1.0f, 1.0f };
        vb[+2].attr<vertex::AttributeType::TexCoords>() = { 0.0f, 1.0f };
        vb[+3].attr<vertex::AttributeType::TexCoords>() = { 0.0f, 0.0f };

        vb[+4].attr<vertex::AttributeType::TexCoords>() = { 1.0f, 1.0f };
        vb[+5].attr<vertex::AttributeType::TexCoords>() = { 1.0f, 0.0f };
        vb[+6].attr<vertex::AttributeType::TexCoords>() = { 0.0f, 1.0f };
        vb[+7].attr<vertex::AttributeType::TexCoords>() = { 0.0f, 0.0f };

        vb[+8].attr<vertex::AttributeType::TexCoords>() = { 0.0f, 0.0f };
        vb[+9].attr<vertex::AttributeType::TexCoords>() = { 0.0f, 1.0f };
        vb[10].attr<vertex::AttributeType::TexCoords>() = { 1.0f, 1.0f };
        vb[11].attr<vertex::AttributeType::TexCoords>() = { 1.0f, 0.0f };

        vb[12].attr<vertex::AttributeType::TexCoords>() = { 0.0f, 0.0f };
        vb[13].attr<vertex::AttributeType::TexCoords>() = { 0.0f, 1.0f };
        vb[14].attr<vertex::AttributeType::TexCoords>() = { 1.0f, 0.0f };
        vb[15].attr<vertex::AttributeType::TexCoords>() = { 1.0f, 1.0f };

        vb[16].attr<vertex::AttributeType::TexCoords>() = { 0.0f, 1.0f };
        vb[17].attr<vertex::AttributeType::TexCoords>() = { 1.0f, 1.0f };
        vb[18].attr<vertex::AttributeType::TexCoords>() = { 0.0f, 0.0f };
        vb[19].attr<vertex::AttributeType::TexCoords>() = { 1.0f, 0.0f };

        vb[20].attr<vertex::AttributeType::TexCoords>() = { 0.0f, 0.0f };
        vb[21].attr<vertex::AttributeType::TexCoords>() = { 1.0f, 0.0f };
        vb[22].attr<vertex::AttributeType::TexCoords>() = { 0.0f, 1.0f };
        vb[23].attr<vertex::AttributeType::TexCoords>() = { 1.0f, 1.0f };
    }

    setVertexBuffer(std::make_unique<VertexBuffer>(gfx, vb));


    const std::array<uint16_t, 36> ib = { 0,  1,  2,  0,  2,  3,  5,  4,  6,  5,  6,  7,  11, 10, 9,  11, 9,  8,
                                          14, 15, 13, 14, 13, 12, 19, 17, 16, 19, 16, 18, 21, 23, 22, 21, 22, 20 };
    setIndexBuffer(std::make_unique<IndexBuffer>(gfx, ib));
}

void Box::update(float dt, float tt) noexcept
{
    (void)dt;
    m_total_time = tt;
}

glm::mat4 Box::getModelMatrix() const noexcept
{
    glm::mat4 matrix = glm::mat4(1.0f);

    matrix = glm::rotate(matrix, m_total_time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    matrix = glm::scale(matrix, glm::vec3(0.5f, 0.5f, 0.5f));

    return matrix;
}
