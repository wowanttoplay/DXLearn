﻿#include "ShapesApp.h"

#include <DirectXColors.h>

#include "ShapesFrameResource.h"
#include "../../Common/d3dx12.h"
#include "../../Common/GeometryGenerator.h"

bool ShapesApp::Initialize()
{
    if (!D3dApp::Initialize())
    {
        return false;
    }
    ThrowIfFailed(mCommandList->Reset(mCommandAlloctor.Get(), nullptr));

    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildMeshGeometry();
    
    

    return true;
}

void ShapesApp::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE cbvTable0, cbvTable1;
    cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

    CD3DX12_ROOT_PARAMETER slotRootParameter[2];
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
    slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(2, slotRootParameter, 0, nullptr,
                                                  D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSignature = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> err = nullptr;
    HRESULT rst = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSignature.GetAddressOf(), err.GetAddressOf());

    if (err)
    {
        OutputDebugStringA(static_cast<char*>(err->GetBufferPointer()));
    }
    ThrowIfFailed(rst);

    ThrowIfFailed(mD3dDevice->CreateRootSignature(
        0,
        serializedRootSignature->GetBufferPointer(),
        serializedRootSignature->GetBufferSize(),
        IID_PPV_ARGS(&mRootSig)
    ));
}

void ShapesApp::BuildShadersAndInputLayout()
{
    const std::wstring ShaderPath = TEXT("AppFactory\\ShapesApp\\Shaders\\color.hlsl");

    mShaders["standardVS"] = D3dUtil::CompileShader(ShaderPath, nullptr, "VS", "vs_5_1");
    mShaders["opaquePS"] = D3dUtil::CompileShader(ShaderPath, nullptr, "PS", "ps_5_1");

    mInputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
}

void ShapesApp::BuildMeshGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(4.5f, 0.5f, 1.5f, 3);
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
    GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
    GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

    // We are concatenating all the geometry into one big vertex/index buffer. So define the regions in the buffer each submesh covers

    // Cache the vertex offsets to each object in the concatenated vertex buffer
    UINT boxVertexOffset = 0;
    UINT gridVertexOffset = static_cast<UINT>(box.Vertices.size());
    UINT sphereVertexOffset = static_cast<UINT>(gridVertexOffset + grid.Vertices.size());
    UINT cylinderVertexOffset = static_cast<UINT>(sphereVertexOffset + sphere.Vertices.size());

    // Cache the Starting index fro each object in the concatenated index buffer
    UINT boxIndexOffset = 0;
    UINT gridIndexOffset = static_cast<UINT>(box.Indices32.size());
    UINT sphereIndexOffset = static_cast<UINT>(gridIndexOffset + grid.Indices32.size());
    UINT cylinderIndexOffset = static_cast<UINT>(sphereIndexOffset + sphere.Indices32.size());

    // Define the submesh that cover different regions of the vertex/index buffers

    SubMeshGeometry boxSubMesh;
    boxSubMesh.IndexCount =  static_cast<UINT>(box.Indices32.size());
    boxSubMesh.BaseVertexLocation = boxVertexOffset;
    boxSubMesh.StartIndexLocation = boxIndexOffset;

    SubMeshGeometry gridSubMesh;
    gridSubMesh.IndexCount = static_cast<UINT>(grid.Indices32.size());
    gridSubMesh.BaseVertexLocation = gridVertexOffset;
    gridSubMesh.StartIndexLocation = gridIndexOffset;

    SubMeshGeometry sphereSubMesh;
    sphereSubMesh.IndexCount = static_cast<UINT>(sphere.Indices32.size());
    sphereSubMesh.BaseVertexLocation = sphereVertexOffset;
    sphereSubMesh.StartIndexLocation = sphereIndexOffset;

    SubMeshGeometry cylinderSubMesh;
    cylinderSubMesh.IndexCount = static_cast<UINT>(cylinder.Indices32.size());
    cylinderSubMesh.BaseVertexLocation = cylinderVertexOffset;
    cylinderSubMesh.StartIndexLocation = cylinderIndexOffset;

    // Extract teh vertex elements we are intersted in and pack the vertices of all the meshes into one vertex buffer
    auto totalVertexCount = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size();

    std::vector<ShapedVertex> TotalVertices(totalVertexCount);
    size_t TotalIndex = 0;
    for (size_t index = 0; index < box.Vertices.size(); ++index, ++TotalIndex)
    {
        TotalVertices[TotalIndex].Pos = box.Vertices[index].Position;
        TotalVertices[TotalIndex].Color = DirectX::XMFLOAT4(DirectX::Colors::DarkGreen);
    }

    for (size_t index = 0; index < grid.Vertices.size(); ++index, ++TotalIndex)
    {
        TotalVertices[TotalIndex].Pos = grid.Vertices[index].Position;
        TotalVertices[TotalIndex].Color = DirectX::XMFLOAT4(DirectX::Colors::ForestGreen);
    }

    for (size_t index = 0; index < sphere.Vertices.size(); ++index, ++TotalIndex)
    {
        TotalVertices[TotalIndex].Pos = sphere.Vertices[index].Position;
        TotalVertices[TotalIndex].Color = DirectX::XMFLOAT4(DirectX::Colors::Crimson);
    }

    for (size_t index = 0; index < cylinder.Vertices.size(); ++index, ++TotalIndex)
    {
        TotalVertices[TotalIndex].Pos = cylinder.Vertices[index].Position;
        TotalVertices[TotalIndex].Color = DirectX::XMFLOAT4(DirectX::Colors::SteelBlue);
    }

    std::vector<uint16_t> TotaleIndices;
    TotaleIndices.insert(TotaleIndices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
    TotaleIndices.insert(TotaleIndices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
    TotaleIndices.insert(TotaleIndices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
    TotaleIndices.insert(TotaleIndices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());


    const UINT vbBytesSize = sizeof(ShapedVertex) * static_cast<UINT>(TotalVertices.size());
    const UINT ibByteSize = sizeof(uint16_t) * static_cast<UINT>(TotaleIndices.size());

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "shapeGeo";

    ThrowIfFailed(D3DCreateBlob(vbBytesSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), TotalVertices.data(), vbBytesSize);
    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), TotaleIndices.data(), ibByteSize);

    geo->VertexBufferGPU = D3dUtil::CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), TotalVertices.data(), vbBytesSize, geo->VertexBufferUploader);
    geo->IndexBufferGPU = D3dUtil::CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), TotaleIndices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(ShapedVertex);
    geo->VertexBufferByteSize = vbBytesSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    geo->DrawArgs["box"] = boxSubMesh;
    geo->DrawArgs["grid"] = gridSubMesh;
    geo->DrawArgs["sphere"] = sphereSubMesh;
    geo->DrawArgs["cylinder"] = cylinderSubMesh;

    mMeshGeometry[geo->Name] = std::move(geo);
}
