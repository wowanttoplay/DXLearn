#include "D3dUtil.h"
#include <comdef.h>

#include "d3dx12.h"

using Microsoft::WRL::ComPtr;

Microsoft::WRL::ComPtr<ID3D12Resource> D3dUtil::CreateDefaultBuffer(ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList, const void* data, uint64_t byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploaderBuffer) const
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
