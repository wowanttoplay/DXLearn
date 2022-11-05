#pragma once
#include "../../Common/FrameResource.h"

struct shapesObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
};

struct PassContants
{
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT3 EyePosW = {0.0f, 0.0f, 0.0f};
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = {0.0f, 0.0f};
    DirectX::XMFLOAT2 InvRenderTargetSize = {0.0f, 0.0f};
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;
};

struct ShapedVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

class ShapesFrameResource:public FrameResource
{
public:
    ShapesFrameResource(ID3D12Device* device, UINT passNum, UINT objectNum);
    virtual ~ShapesFrameResource();

    // We can't update constant buffer until the gpu is done processing the commands that reference it
    // so each frame needs their own cbuffers
    std::unique_ptr<UploadBuffer<PassContants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<shapesObjectConstants>> ObjectCb = nullptr;
};
