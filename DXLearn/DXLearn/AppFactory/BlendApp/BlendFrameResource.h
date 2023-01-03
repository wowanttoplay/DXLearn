#pragma once
#include "../Light/LightFrameResource.h"



struct BlendPassConstants : public LightPassConstants
{
    DirectX::XMFLOAT4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
    float gFogStart = 5.0f;
    float gFogRange = 150.0f;
    DirectX::XMFLOAT2 cbPerObjectPad2;
};


class BlendFrameResource : public FrameResource
{
public:
    BlendFrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT WaveCount);

    std::unique_ptr<UploadBuffer<LightVertex>> WavesVB = nullptr;
    std::unique_ptr<UploadBuffer<BlendPassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;
    std::unique_ptr<UploadBuffer<LightObjectConstants>> ObjectCB = nullptr;
    
};
