#pragma once
#include <string>
#include <windows.h>
#include <wrl.h>
#include <string>
#include <sstream>
#include <unordered_map>

class D3dUtil
{
public:
    
};

class DxExpection
{
public:
    DxExpection() = default;
    DxExpection(HRESULT hresult, const std::wstring& functionName, const std::wstring& fileName, int lineNumber);

    std::wstring ToString() const;

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring FileName;
    int LineNumber;
};
