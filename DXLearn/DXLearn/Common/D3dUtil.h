#pragma once
#include <string>
#include <windows.h>
#include <wrl.h>
#include <string>
#include <sstream>
#include <unordered_map>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <D3Dcompiler.h>
#include <DirectXCollision.h>


class D3dUtil
{
public:
    
};

class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hresult, const std::wstring& functionName, const std::wstring& fileName, int lineNumber);

    std::wstring ToString() const;

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring FileName;
    int LineNumber;
};

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
HRESULT hr__ = (x);                                               \
std::wstring wfn = AnsiToWString(__FILE__);                       \
if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif
