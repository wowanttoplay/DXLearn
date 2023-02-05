#pragma once
#include <string>


class FileManager
{
public:
    static std::wstring GetShaderFullPath(const std::string& fileName);
    static std::wstring GetTextureFullPath(const std::string& fileName);
    static std::wstring GetModelFullPath(const std::string& fileName);
};
