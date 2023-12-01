#pragma once
#include <exception>
#include <string>

class EngineDefaultException : public std::exception
{
public:
    EngineDefaultException(int line, const char* file) noexcept;

    const char* what() const noexcept override;

    virtual const char* getType() const noexcept;

    int                getLine() const noexcept;
    const std::string& getFile() const noexcept;
    std::string        getOriginString() const noexcept;

private:
    int         m_line;
    std::string m_file;

protected:
    mutable std::string m_what_buffer;
};
