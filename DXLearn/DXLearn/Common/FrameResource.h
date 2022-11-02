#pragma once
#include "D3dUtil.h"
#include "MathHelper.h"
#include "UploadBuffer.h"



class FrameResource
{
public:
    FrameResource(ID3D12Device* device);
    FrameResource(const FrameResource& other) = delete;
    FrameResource& operator=(const FrameResource& other) = delete;
    virtual ~FrameResource();

    // We can't reset the allocator unitl the gpu is done processing the commands
    // so each frame resource need have their own allocator
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    // Fence value to mark commands  up to this point. This lets us
    // check if these frame resource are still in use by the gpu
    uint64_t Fence = 0;
};
