#pragma once
#include <DirectXMath.h>

#include "../../Common/D3dUtil.h"
#include "../../Common/MathHelper.h"

constexpr int gNumFrameResource = 3;

class RenderItem
{
public:
    RenderItem() = default;

    virtual ~RenderItem() = default;

public:
    // World matrix of the shape that describe the object's local space
    // relative to the world space, which define the position, orientation,
    // and scale of the object in the world
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();

    // Dirty flag indicating the object data has changed and we need to update the constant buffer.
    // Beacause we have an object constant buffer for each frame resource, we have to apply the update  to each frame resource
    // Thus, when we modify object data we should set NumframeDirty = gNumFrameResource so that each frame resource gets the update
    int NumFrameDirty = gNumFrameResource;

    // Index into the GPU constant buffer correspoding to the objctCB for this render item
    UINT objectIndex = -1;

    MeshGeometry* Geo = nullptr;

    // Primitive topology
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexInsatnced parameter
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    UINT BaseVertexLcoation = 0;
};
