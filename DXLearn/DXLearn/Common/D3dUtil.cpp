#include "D3dUtil.h"
#include <comdef.h>

using Microsoft::WRL::ComPtr;

DxException::DxException(HRESULT hresult, const std::wstring& functionName, const std::wstring& fileName,
    int lineNumber)
        :ErrorCode(hresult),
FunctionName(functionName),
FileName(fileName),
LineNumber(lineNumber)
{
}

std::wstring DxException::ToString() const
{
     _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + TEXT("failed in ") + FileName + TEXT("; line ") + std::to_wstring(LineNumber) + TEXT("; error : ") + msg;
}
