#include "LightFrameResource.h"

LightFrameResource::LightFrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount): FrameResource(device)
{
    PassCB = std::make_unique<UploadBuffer<LightPassConstants>>(device, passCount, true);
    MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);
    ObjectCB = std::make_unique<UploadBuffer<LightObjectConstants>>(device, objectCount, true);
}
