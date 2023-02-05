#include "FileManager.h"

#include <codecvt>
#include <locale>
#define SHADER_PATH "AppFactory/Shaders/"
#define MODELS_PATH "AppFactory/Models/"
#define TEXTURES_PATH "AppFactory/Textures/"

std::wstring to_wide_string(const std::string& input)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(input);
}

std::string to_byte_string(const std::wstring& input)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(input);
}

std::wstring FileManager::GetShaderFullPath(const std::string& fileName)
{
    static std::string shaderFolder(SHADER_PATH);
    return to_wide_string(shaderFolder + fileName);
}

std::wstring FileManager::GetTextureFullPath(const std::string& fileName)
{
    static std::string TextureFolder(TEXTURES_PATH);
    return to_wide_string(TextureFolder + fileName);
}

std::wstring FileManager::GetModelFullPath(const std::string& fileName)
{
    static std::string ModelFolder(MODELS_PATH);
    return to_wide_string(ModelFolder + fileName);
}
