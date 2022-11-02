#pragma once
#include "D3dUtil.h"
#include "d3dx12.h"

template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool bContantBuffer);
    UploadBuffer(const UploadBuffer& other) = delete;
    UploadBuffer& operator=(const UploadBuffer& other) = delete;

    virtual ~UploadBuffer();

public:
    ID3D12Resource* GetResource() const;

    void CopyData(int elementIndex, const T& data);

    UINT GetElementByteSize() const {return mElementByteSize;}

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploaderBuffer;
    BYTE* mMappedData = nullptr;
    UINT mElementByteSize = 0;
    bool mbConstantBuffer = false;
};

template <typename T>
UploadBuffer<T>::UploadBuffer(ID3D12Device* device, UINT elementCount, bool bContantBuffer)
{
    mbConstantBuffer = bContantBuffer;
    mElementByteSize = sizeof(T);
    // Constant buffer elements need to be multiples of 256 bytes.
    // This is because the hardware can only view constant data 
    // at m*256 byte offsets and of n*256 byte lengths. 
    // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
    // UINT64 OffsetInBytes; // multiple of 256
    // UINT   SizeInBytes;   // multiple of 256
    // } D3D12_CONSTANT_BUFFER_VIEW_DESC;
    if (mbConstantBuffer)
    {
        mElementByteSize = D3dUtil::CalculateConstantBufferByteSize(sizeof(T));
    }

    ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(elementCount * mElementByteSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mUploaderBuffer)
    ));

    ThrowIfFailed(mUploaderBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
}

template <typename T>
UploadBuffer<T>::~UploadBuffer()
{
    if (mUploaderBuffer)
    {
        mUploaderBuffer->Unmap(0 , nullptr);
        mUploaderBuffer = nullptr;
    }
}

template <typename T>
ID3D12Resource* UploadBuffer<T>::GetResource() const
{
    return mUploaderBuffer.Get();
}

template <typename T>
void UploadBuffer<T>::CopyData(int elementIndex, const T& data)
{
    memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
}
