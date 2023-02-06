#include "TreeBillboardsApp.h"

#include <array>
#include <iostream>

#include "../../Common/DDSTextureLoader.h"
#include "../../Common/FileManager.h"
using namespace DirectX;
using namespace std;


TreeBillboardsApp::TreeBillboardsApp(HINSTANCE hInsatnce)
    : BlendApp(hInsatnce)
{
    // Registe initialzie functions
}

void TreeBillboardsApp::BuildTextures()
{
    BlendApp::BuildTextures();

    auto treeArrayTex = std::make_unique<Texture>();
    treeArrayTex->Name = "treeArrayTex";
    treeArrayTex->Filename = FileManager::GetTextureFullPath("treeArray2.dds");
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), treeArrayTex->Filename.c_str(), treeArrayTex->Resource, treeArrayTex->UploadHeap));

    mTextures[treeArrayTex->Name] = std::move(treeArrayTex);
}

void TreeBillboardsApp::BuildDescriptorHeaps()
{
    //
    // Create the SRV heap.
    //
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 4;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvheap)));

    //
    // Fill out the heap with actual descriptors.
    //
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvheap->GetCPUDescriptorHandleForHeapStart());

    auto grassTex = mTextures["grassTex"]->Resource;
    auto waterTex = mTextures["waterTex"]->Resource;
    auto fenceTex = mTextures["fenceTex"]->Resource;
    auto treeArrayTex = mTextures["treeArrayTex"]->Resource;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = grassTex->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;
    md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvHandleSize);

    srvDesc.Format = waterTex->GetDesc().Format;
    md3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvHandleSize);

    srvDesc.Format = fenceTex->GetDesc().Format;
    md3dDevice->CreateShaderResourceView(fenceTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvHandleSize);

    auto desc = treeArrayTex->GetDesc();
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Format = treeArrayTex->GetDesc().Format;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = -1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
    md3dDevice->CreateShaderResourceView(treeArrayTex.Get(), &srvDesc, hDescriptor);
}

void TreeBillboardsApp::BuildMaterials()
{
    BlendApp::BuildMaterials();

    auto treeSprites = std::make_unique<Material>();
    treeSprites->Name = "treeSprites";
    treeSprites->MatCBIndex = 3;
    treeSprites->DiffuseSrvHeapIndex = 3;
    treeSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
    treeSprites->Roughness = 0.125f;

    mMaterials["treeSprites"] = std::move(treeSprites);
}

void TreeBillboardsApp::BuildShadersAndInputLayout()
{
    const D3D_SHADER_MACRO defines[] =
    {
        "FOG", "1",
        NULL, NULL
    };

    const D3D_SHADER_MACRO alphaTestDefines[] =
    {
        "FOG", "1",
        "ALPHA_TEST", "1",
        NULL, NULL
    };

    wstring defaultShaderPath = FileManager::GetShaderFullPath("blendShader.hlsl");
    wstring treeSpriteShaderPath = FileManager::GetShaderFullPath("TreeSprite.hlsl");

    mShaders["standardVS"] = D3dUtil::CompileShader(defaultShaderPath, nullptr, "VS", "vs_5_0");
    mShaders["opaquePS"] = D3dUtil::CompileShader(defaultShaderPath, defines, "PS", "ps_5_0");
    mShaders["alphaTestedPS"] = D3dUtil::CompileShader(defaultShaderPath, alphaTestDefines, "PS", "ps_5_0");
	
    mShaders["treeSpriteVS"] = D3dUtil::CompileShader(treeSpriteShaderPath, nullptr, "VS", "vs_5_0");
    mShaders["treeSpriteGS"] = D3dUtil::CompileShader(treeSpriteShaderPath, nullptr, "GS", "gs_5_0");
    mShaders["treeSpritePS"] = D3dUtil::CompileShader(treeSpriteShaderPath, alphaTestDefines, "PS", "ps_5_0");

    mInputLayouts["blend"] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    mInputLayouts["treeSprite"] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

void TreeBillboardsApp::BuildGeometry()
{
    BlendApp::BuildGeometry();

    mGeometries["treeSpritesGeo"] = BuildTreeSpriteGeometry();
}

void TreeBillboardsApp::BuildRenderItems()
{
    BlendApp::BuildRenderItems();

    auto treeSpriteRitem = make_unique<RenderItem>();
    treeSpriteRitem->World = MathHelper::Identity4x4();
    treeSpriteRitem->ObjCBIndex = 3;
    treeSpriteRitem->Mat = mMaterials["treeSprites"].get();
    treeSpriteRitem->Geo = mGeometries["treeSpritesGeo"].get();
    treeSpriteRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    treeSpriteRitem->IndexCount = treeSpriteRitem->Geo->DrawArgs["points"].IndexCount;
    treeSpriteRitem->StartIndexLocation = treeSpriteRitem->Geo->DrawArgs["points"].StartIndexLocation;
    treeSpriteRitem->BaseVertexLocation = treeSpriteRitem->Geo->DrawArgs["points"].BaseVertexLocation;

    mRItemLayers[ERenderLayer::AlphaTestedTreeSprites].push_back(treeSpriteRitem.get());

    mAllRitems.push_back(move(treeSpriteRitem));
}

void TreeBillboardsApp::BuildPSOs()
{
    BlendApp::BuildPSOs();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc;

    //
    // PSO for opaque objects.
    //
    ZeroMemory(&treeSpritePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    treeSpritePsoDesc.InputLayout = { mInputLayouts["treeSprite"].data(), (UINT)mInputLayouts["treeSprite"].size() };
    treeSpritePsoDesc.pRootSignature = mRootSignature.Get();
    treeSpritePsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["treeSpriteVS"]->GetBufferPointer()),
        mShaders["treeSpriteVS"]->GetBufferSize()
    };
    treeSpritePsoDesc.GS =
    {
        reinterpret_cast<BYTE*>(mShaders["treeSpriteGS"]->GetBufferPointer()),
        mShaders["treeSpriteGS"]->GetBufferSize()
    };
    treeSpritePsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["treeSpritePS"]->GetBufferPointer()),
        mShaders["treeSpritePS"]->GetBufferSize()
    };
    treeSpritePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    treeSpritePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    treeSpritePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    treeSpritePsoDesc.SampleMask = UINT_MAX;
    treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    treeSpritePsoDesc.NumRenderTargets = 1;
    treeSpritePsoDesc.RTVFormats[0] = mBackBufferFormat;
    treeSpritePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    treeSpritePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    treeSpritePsoDesc.DSVFormat = mDepthStencilFormat;

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs[EPSoType::TreeSprite])));
}

std::unique_ptr<MeshGeometry> TreeBillboardsApp::BuildTreeSpriteGeometry()
{
    struct TreeSpriteVertex
    {
        XMFLOAT3 Pos;
        XMFLOAT2 Size;
    };

    static const int treeCount = 16;
    std::array<TreeSpriteVertex, 16> vertices;
    for(UINT i = 0; i < treeCount; ++i)
    {
        float x = MathHelper::RandF(-45.0f, 45.0f);
        float z = MathHelper::RandF(-45.0f, 45.0f);
        float y = LandUtil::GetHillsHeight(x, z);

        // Move tree slightly above land height.
        y += 8.0f;

        vertices[i].Pos = XMFLOAT3(x, y, z);
        vertices[i].Size = XMFLOAT2(20.0f, 20.0f);
    }

    std::array<std::uint16_t, 16> indices =
    {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15
    };


    const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "treeSpritesGeo";
    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);


    geo->VertexBufferGPU = D3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
    mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = D3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(TreeSpriteVertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubMeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs["points"] = submesh;
    return geo;
}



