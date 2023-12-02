#pragma once
#include <type_traits>
#include <cassert>
#include <span>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

namespace vertex
{

#define DEFINE_ELEMENT_TYPES                                                                                                               \
    DEF(Pos2d)                                                                                                                             \
    DEF(Pos3d)                                                                                                                             \
    DEF(Color3)                                                                                                                            \
    DEF(Color4)                                                                                                                            \
    DEF(Normal)                                                                                                                            \
    DEF(Tangent)                                                                                                                           \
    DEF(Bitangent)                                                                                                                         \
    DEF(TexCoords)                                                                                                                         \
    DEF(Count)

enum class AttributeType
{
#define DEF(et) et,
    DEFINE_ELEMENT_TYPES
#undef DEF
};

class Layout
{
public:
    template <AttributeType>
    struct Map
    {};

    template <>
    struct Map<AttributeType::Pos2d>
    {
        using DataType = glm::vec2;

        static constexpr size_t   k_dimension = 2;
        static constexpr VkFormat k_format    = VK_FORMAT_R32G32_SFLOAT;
    };

    template <>
    struct Map<AttributeType::Pos3d>
    {
        using DataType = glm::vec3;

        static constexpr size_t   k_dimension = 3;
        static constexpr VkFormat k_format    = VK_FORMAT_R32G32B32_SFLOAT;
    };

    template <>
    struct Map<AttributeType::Color3>
    {
        using DataType = glm::vec3;

        static constexpr size_t   k_dimension = 3;
        static constexpr VkFormat k_format    = VK_FORMAT_R32G32B32_SFLOAT;
    };

    template <>
    struct Map<AttributeType::Color4>
    {
        using DataType = glm::vec4;

        static constexpr size_t   k_dimension = 4;
        static constexpr VkFormat k_format    = VK_FORMAT_R32G32B32A32_SFLOAT;
    };

    template <>
    struct Map<AttributeType::Normal>
    {
        using DataType = glm::vec3;

        static constexpr size_t   k_dimension = 3;
        static constexpr VkFormat k_format    = VK_FORMAT_R32G32B32_SFLOAT;
    };

    template <>
    struct Map<AttributeType::Tangent>
    {
        using DataType = glm::vec3;

        static constexpr size_t   k_dimension = 3;
        static constexpr VkFormat k_format    = VK_FORMAT_R32G32B32_SFLOAT;
    };

    template <>
    struct Map<AttributeType::Bitangent>
    {
        using DataType = glm::vec3;

        static constexpr size_t   k_dimension = 3;
        static constexpr VkFormat k_format    = VK_FORMAT_R32G32B32_SFLOAT;
    };

    template <>
    struct Map<AttributeType::TexCoords>
    {
        using DataType = glm::vec2;

        static constexpr size_t   k_dimension = 2;
        static constexpr VkFormat k_format    = VK_FORMAT_R32G32_SFLOAT;
    };

    template <>
    struct Map<AttributeType::Count>
    {
        using DataType = long double;

        static constexpr size_t   k_dimension = 4;
        static constexpr VkFormat k_format    = VK_FORMAT_UNDEFINED;
    };


    template <template <AttributeType> class Func, typename... Args>
    static constexpr auto visit(AttributeType type, Args&&... args) noexcept
    {
        using T = AttributeType;
        switch (type)
        {
#define DEF(et)                                                                                                                            \
    case T::et: return Func<T::et>::exec(std::forward<Args>(args)...);
            DEFINE_ELEMENT_TYPES
#undef DEF
        }
        assert(false && "Invalid element type.");
        return Func<AttributeType::Count>::exec(std::forward<Args>(args)...);
    }

    class Attribute
    {
    private:
        template <AttributeType AT>
        struct SizeOf
        {
            static constexpr size_t exec() noexcept { return sizeof(typename Map<AT>::DataType); }
        };

        template <AttributeType AT>
        struct Dimension
        {
            static constexpr size_t exec() noexcept { return Map<AT>::k_dimension; }
        };

        template <AttributeType AT>
        struct Format
        {
            static constexpr VkFormat exec() noexcept { return Map<AT>::k_format; }
        };

    public:
        Attribute(AttributeType type, size_t offset)
            : m_type(type)
            , m_offset(offset)
        {}

        AttributeType type() const noexcept { return m_type; }

        size_t   size() const noexcept { return visit<SizeOf>(m_type); }
        size_t   offset() const noexcept { return m_offset; }
        size_t   offsetAfter() const noexcept { return m_offset + size(); }
        size_t   dimension() const noexcept { return visit<Dimension>(m_type); }
        VkFormat format() const noexcept { return visit<Format>(m_type); }

    private:
        AttributeType m_type;
        size_t        m_offset;
    };

public:
    Layout() noexcept = default;

    Layout& append(AttributeType type) noexcept
    {
        if (!hasElement(type))
        {
            m_elements.emplace_back(type, getStride());
        }
        return *this;
    }

    template <AttributeType AT>
    const Attribute& resolve() const noexcept
    {
        for (const Attribute& e : m_elements)
        {
            if (e.type() == AT)
            {
                return e;
            }
        }
        assert(false && "Cannot find element type in this Vertex type.");
        return m_elements.front();
    }
    const Attribute& resolve(size_t i) const noexcept { return m_elements[i]; }

    size_t getElementCount() const noexcept { return m_elements.size(); }
    size_t getStride() const noexcept { return m_elements.empty() ? 0 : m_elements.back().offsetAfter(); }

    bool hasElement(AttributeType type) const noexcept
    {
        for (const auto& e : m_elements)
        {
            if (e.type() == type)
            {
                return true;
            }
        }
        return false;
    }

    void getAttributeDescs(uint32_t binding, std::vector<VkVertexInputAttributeDescription>& descs)
    {
        for (size_t i = 0, n = m_elements.size(); i < n; ++i)
        {
            size_t   dimension = m_elements[i].dimension();
            VkFormat format    = m_elements[i].format();
            size_t   offset    = m_elements[i].offset();

            auto& desc    = descs.emplace_back();
            desc.location = static_cast<uint32_t>(i);
            desc.binding  = binding;
            desc.format   = format;
            desc.offset   = offset;
        }
    }

    Attribute operator[](size_t idx) const { return m_elements[idx]; }

private:
    std::vector<Attribute> m_elements;
};

class Vertex
{
public:
    Vertex(std::byte* data, const Layout& layout) noexcept
        : m_data_ref(data)
        , m_layout_ref(layout)
    {}
    Vertex(const Vertex&)            = default;
    Vertex& operator=(const Vertex&) = delete;

    template <typename T>
    void setAttribute(size_t i, T&& val) noexcept
    {
        const auto& element = m_layout_ref.resolve(i);
        auto        attrib  = m_data_ref + element.offset();

        Layout::visit<SetAttrb>(element.type(), this, attrib, std::forward<T>(val));
    }

    template <AttributeType AT, typename DataType>
    void setAttribute(std::byte* attrib, DataType&& data) noexcept
    {
        using ElementDataType = typename Layout::Map<AT>::DataType;
        if constexpr (std::is_assignable_v<ElementDataType, DataType>)
        {
            *reinterpret_cast<ElementDataType*>(attrib) = data;
        }
        else
        {
            assert(false && "Mismatch attribute type.");
        }
    }

    template <AttributeType AT>
    auto& attr() noexcept
    {
        auto attrib = m_data_ref + m_layout_ref.resolve<AT>().offset();
        return *reinterpret_cast<typename Layout::Map<AT>::DataType*>(attrib);
    }

private:
    template <AttributeType AT>
    struct SetAttrb
    {
        template <typename T>
        static constexpr auto exec(Vertex* vertex, std::byte* attrib, T&& val) noexcept
        {
            return vertex->setAttribute<T>(attrib, std::forward<T>(val));
        }
    };

    template <typename T, typename... TRest>
    void setAttribute(size_t i, T&& val, TRest&&... rest) noexcept
    {
        setAttribute(i, std::forward<T>(val));
        setAttribute(i + 1, std::forward<TRest>(rest)...);
    }

private:
    std::byte*    m_data_ref = nullptr;
    const Layout& m_layout_ref;
};

class ConstVertex
{
public:
    ConstVertex(const Vertex& v) noexcept
        : m_vertex(v)
    {}

    template <AttributeType AT>
    const auto& attr() const noexcept
    {
        return const_cast<Vertex&>(m_vertex).attr<AT>();
    }

private:
    Vertex m_vertex;
};

class Buffer
{
public:
    Buffer(Layout layout, size_t count = 0) noexcept
        : m_layout(std::move(layout))
    {
        resize(count);
    }
    Buffer(const Buffer&)            = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&)                 = default;
    Buffer& operator=(Buffer&&)      = default;

    std::span<const std::byte> data() const noexcept { return m_data; }
    const std::byte*           dataPtr() const noexcept { return m_data.data(); }
    const Layout&              layout() const noexcept { return m_layout; }

    size_t count() const noexcept { return m_data.size() / m_layout.getStride(); }
    size_t sizeOf() const noexcept { return m_data.size(); }

    void resize(size_t size)
    {
        const size_t curr_count = count();
        if (curr_count < size)
        {
            m_data.resize(m_data.size() + m_layout.getStride() * (size - curr_count));
        }
    }

    Vertex front()
    {
        assert(m_data.size() != 0);
        return Vertex{ m_data.data(), m_layout };
    }
    Vertex back()
    {
        assert(m_data.size() != 0);
        return Vertex{ m_data.data() + sizeOf() - m_layout.getStride(), m_layout };
    }
    Vertex operator[](size_t i)
    {
        assert(m_data.size() != 0);
        return Vertex{ m_data.data() + m_layout.getStride() * i, m_layout };
    }

    ConstVertex front() const { return const_cast<Buffer*>(this)->front(); }
    ConstVertex back() const { return const_cast<Buffer*>(this)->back(); }
    ConstVertex operator[](size_t i) const { return const_cast<Buffer&>(*this)[i]; }

private:
    std::vector<std::byte> m_data;
    Layout                 m_layout;
};

#undef DEFINE_ELEMENT_TYPES

}  // namespace vertex
