#pragma once
#include "../Light/LightFrameResource.h"

struct BlendObjectConstants : public LightObjectConstants
{
    
};

struct BlendPassConstants : public LightPassConstants
{
    
};

struct BlendVertex : public LightVertex
{
    
};


class BlendFrameResource : LightFrameResource
{
public:
    BlendFrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT WaveCount)
        : LightFrameResource(device, passCount, objectCount, materialCount)
    {
        WavesVB = std::make_unique<UploadBuffer<BlendVertex>>(device, WaveCount, false);
    }

    std::unique_ptr<UploadBuffer<BlendVertex>> WavesVB = nullptr;
};
