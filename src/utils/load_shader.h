#pragma once
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "utils/load_file.h"

inline std::string& getShaderSearchPath()
{
    static std::string search_path = "shader/bin";
    return search_path;
}

inline void setShaderSearchPath(const std::string& path)
{
    auto& search_path = getShaderSearchPath();
    search_path       = path;
}

inline std::vector<char> loadShaderCode(const char* path)
{
    std::string file_path = getFilePathString(path, { getShaderSearchPath() });

    std::ifstream file(file_path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error(std::string("Failed to open file at path = ") + std::string(path));
    }

    size_t            file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    return buffer;
}
