#include "D3dUtil.h"
#include <comdef.h>

#include "d3dx12.h"

using Microsoft::WRL::ComPtr;

Microsoft::WRL::ComPtr<ID3D12Resource> D3dUtil::CreateDefaultBuffer(ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList, const void* data, uint64_t byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploaderBuffer)
{
    // create uploader buffer
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploaderBuffer.GetAddressOf())
    ));

    // create default buffer
    ComPtr<ID3D12Resource> defaultBuffer;
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())
    ));

    // Transform the default state from common to copy dest
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

    // copy data from cpu to gpu
    D3D12_SUBRESOURCE_DATA subResourceData;
    subResourceData.pData = data;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploaderBuffer.Get(), 0, 0, 1, &subResourceData);

    // Transform the default buffer from copy dest state to read state
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    return defaultBuffer;
}

UINT D3dUtil::CalculateConstantBufferByteSize(UINT InByteSize)
{
    // Constant buffers must be a multiple of the minimum hardware
    // allocation size (usually 256 bytes).  So round up to nearest
    // multiple of 256.  We do this by adding 255 and then masking off
    // the lower 2 bytes which store all bits < 256.
    // Example: Suppose byteSize = 300.
    // (300 + 255) & ~255
    // 555 & ~255
    // 0x022B & ~0x00ff
    // 0x022B & 0xff00
    // 0x0200
    // 512
    return (InByteSize + 255) & ~255;
}

Microsoft::WRL::ComPtr<ID3DBlob> D3dUtil::CompileShader(const std::wstring& fileName, const D3D_SHADER_MACRO* defines,
    const std::string& entryPoint, const std::string& target)
{
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT rst = S_OK;
    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> err = nullptr;

    rst = D3DCompileFromFile(fileName.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &err);

    if (err)
    {
        OutputDebugStringA(static_cast<char*>(err->GetBufferPointer()));
    }

    ThrowIfFailed(rst);
    return byteCode;
}

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
