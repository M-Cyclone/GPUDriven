#include "utils/exception.h"
#include <sstream>

EngineDefaultException::EngineDefaultException(int line, const char* file) noexcept
    : m_line(line)
    , m_file(file)
{}

const char* EngineDefaultException::what() const noexcept
{
    std::ostringstream oss;
    oss << getType() << "\n" << getOriginString();
    m_what_buffer = oss.str();
    return m_what_buffer.c_str();
}

const char* EngineDefaultException::getType() const noexcept
{
    return "Statard Exception";
}

int EngineDefaultException::getLine() const noexcept
{
    return m_line;
}

const std::string& EngineDefaultException::getFile() const noexcept
{
    return m_file;
}

std::string EngineDefaultException::getOriginString() const noexcept
{
    std::ostringstream oss;
    oss << "[File] " << getFile() << "\n"
        << "[Line] " << getLine();
    return oss.str();
}
