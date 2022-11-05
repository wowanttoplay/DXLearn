#include "ShapesFrameResource.h"

ShapesFrameResource::ShapesFrameResource(ID3D12Device* device, UINT passNum, UINT objectNum)
    : FrameResource(device)
{
    PassCB = std::make_unique<UploadBuffer<PassContants>>(device, passNum, true);
    ObjectCb = std::make_unique<UploadBuffer<shapesObjectConstants>>(device, objectNum, true);
}

ShapesFrameResource::~ShapesFrameResource()
{
}
