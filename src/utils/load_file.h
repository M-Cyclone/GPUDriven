#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

inline std::string getFilePathString(const std::string& path, const std::vector<std::string>& directories)
{
    std::ifstream file;

    file.open(path);
    if (file.is_open())
    {
        return path;
    }

    for (const std::string& dir : directories)
    {
        std::string new_path = dir + "/" + path;
        file.open(new_path);
        if (file.is_open())
        {
            return new_path;
        }
    }

    std::cerr << "Failed to open file at path = [" << path << "]" << std::endl;

    return "";
}
