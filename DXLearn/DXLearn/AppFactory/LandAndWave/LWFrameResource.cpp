#include "LWFrameResource.h"

LWFrameResource::LWFrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT waveVertexCount):
    FrameResource(device)
{
    ThrowIfFailed(
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

    PassCB = std::make_unique<UploadBuffer<LWPassConstants>>(device, passCount, true);
    ObjectCB = std::make_unique<UploadBuffer<LWObjectConstants>>(device, objectCount, true);

    WavesVB = std::make_unique<UploadBuffer<LWVertex>>(device, waveVertexCount, false);
}

LWFrameResource::~LWFrameResource()
{
}
