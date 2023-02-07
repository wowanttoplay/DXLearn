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

void TreeBillboardsApp::Draw(const GameTimer& InGameTime)
{
    auto cmdAlloc = mCurrFrameResource->CmdListAlloc;
    ThrowIfFailed(cmdAlloc->Reset());

    ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), mPSOs[EPSoType::Opaque].Get()));
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    mCommandList->ClearRenderTargetView(RenderTargetView(), (float*)(&mMainPassCB->FogColor), 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f , 0, 0 , nullptr);
    mCommandList->OMSetRenderTargets(1, &RenderTargetView(), true, &DepthStencilView());

    ID3D12DescriptorHeap* descHeaps[] = {mSrvheap.Get()};
    mCommandList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    auto passCB = dynamic_pointer_cast<BlendFrameResource>(mCurrFrameResource)->PassCB->GetResource();
    mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mRItemLayers[ERenderLayer::Opaque]);

    mCommandList->SetPipelineState(mPSOs[EPSoType::AlphaTest].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayers[ERenderLayer::AlphaTested]);

    mCommandList->SetPipelineState(mPSOs[EPSoType::TreeSprite].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayers[ERenderLayer::AlphaTestedTreeSprites]);

    mCommandList->SetPipelineState(mPSOs[EPSoType::Translucent].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayers[ERenderLayer::Translucent]);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ExecuteCommandList();
    
    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrentSwapChainIndex = (mCurrentSwapChainIndex + 1) % mSwapChainBufferNumber;

    // Advance the fence value to mark commands up to this fence point.
    mCurrFrameResource->Fence = ++mCurrentFence;

    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
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

   D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayouts["blend"].data(), (UINT)mInputLayouts["blend"].size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), 
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[EPSoType::Opaque])));

	//
	// PSO for transparent objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs[EPSoType::Translucent])));

	//
	// PSO for alpha tested objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		mShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs[EPSoType::AlphaTest])));

	//
	// PSO for tree sprites
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
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
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mInputLayouts["treeSprite"].data(), (UINT)mInputLayouts["treeSprite"].size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

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



