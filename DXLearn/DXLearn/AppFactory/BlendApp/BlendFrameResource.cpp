#include "BlendFrameResource.h"

BlendFrameResource::BlendFrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount,
    UINT WaveCount): FrameResource(device)
{
    WavesVB = std::make_unique<UploadBuffer<LightVertex>>(device, WaveCount,false);
    ObjectCB = std::make_unique<UploadBuffer<LightObjectConstants>>(device, objectCount, true);
    PassCB = std::make_unique<UploadBuffer<BlendPassConstants>>(device, passCount, true);
    MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);
}
