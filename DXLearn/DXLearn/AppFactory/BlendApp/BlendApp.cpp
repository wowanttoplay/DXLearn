#include "BlendApp.h"

#include "BlendFrameResource.h"
#include "../../Common/DDSTextureLoader.h"
#include "../../Common/GeometryGenerator.h"

using namespace std;
using namespace DirectX;

void BlendApp::Draw(const GameTimer& InGameTime)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs[EPSoType::Opaque].Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(RenderTargetView(), (float*)&mMainPassCB->FogColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &RenderTargetView(), true, &DepthStencilView());

    ID3D12DescriptorHeap* descHeaps[] = {mSrvheap.Get()};
    mCommandList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    auto passCb = dynamic_pointer_cast<BlendFrameResource>(mCurrFrameResource)->PassCB->GetResource();
    mCommandList->SetGraphicsRootConstantBufferView(2, passCb->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mRItemLayers[ERenderLayer::Opaque]);

    mCommandList->SetPipelineState(mPSOs[EPSoType::AlphaTest].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayers[ERenderLayer::AlphaTested]);

    mCommandList->SetPipelineState(mPSOs[EPSoType::Translucent].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayers[ERenderLayer::Translucent]);

    // Indicate a state transition on the resource usage.
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

void BlendApp::Update(const GameTimer& InGameTime)
{
    TextureApp::Update(InGameTime);
    UpdateWaves(InGameTime);
}

void BlendApp::BuildGeometry()
{
    TextureApp::BuildGeometry();
    mGeometries["landGeo"] = BuildLandGeometry();
    mGeometries["waveGeo"] = BuildWaveGeometry();
    mGeometries["boxGeo"] = BuildBoxGeometry();
}

void BlendApp::BuildRenderItems()
{
    auto waveRItem = make_unique<RenderItem>();
    waveRItem->World = MathHelper::Identity4x4();
    XMStoreFloat4x4(&waveRItem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
    waveRItem->ObjCBIndex = 0;
    waveRItem->Mat = mMaterials["water"].get();
    waveRItem->Geo = mGeometries["waveGeo"].get();
    waveRItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    waveRItem->IndexCount = waveRItem->Geo->DrawArgs["grid"].IndexCount;
    waveRItem->StartIndexLocation = waveRItem->Geo->DrawArgs["grid"].StartIndexLocation;
    waveRItem->BaseVertexLocation = waveRItem->Geo->DrawArgs["grid"].BaseVertexLocation;

    mWaveRItem = waveRItem.get();
    mRItemLayers[ERenderLayer::Translucent].push_back(mWaveRItem);

    auto gridRItem =std::make_unique<RenderItem>();
    gridRItem->World = MathHelper::Identity4x4();
    XMStoreFloat4x4(&gridRItem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
    gridRItem->ObjCBIndex = 1;
    gridRItem->Mat = mMaterials["grass"].get();
    gridRItem->Geo = mGeometries["landGeo"].get();
    gridRItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRItem->IndexCount = gridRItem->Geo->DrawArgs["grid"].IndexCount;
    gridRItem->StartIndexLocation = gridRItem->Geo->DrawArgs["grid"].StartIndexLocation;
    gridRItem->BaseVertexLocation = gridRItem->Geo->DrawArgs["grid"].BaseVertexLocation;

    mRItemLayers[ERenderLayer::Opaque].push_back(gridRItem.get());


    auto boxRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation(3.0f, 2.0f, -9.0f));
    boxRitem->ObjCBIndex = 2;
    boxRitem->Mat = mMaterials["wirefence"].get();
    boxRitem->Geo = mGeometries["boxGeo"].get();
    boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRitem->IndexCount = boxRitem->Geo->DrawArgs["grid"].IndexCount;
    boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

    mRItemLayers[ERenderLayer::AlphaTested].push_back(boxRitem.get());

    mAllRitems.push_back(move(waveRItem));
    mAllRitems.push_back(move(gridRItem));
    mAllRitems.push_back(move(boxRitem));
}

void BlendApp::BuildMaterials()
{
    auto grass = std::make_unique<Material>();
    grass->Name = "grass";
    grass->MatCBIndex = 0;
    grass->DiffuseSrvHeapIndex = 0;
    grass->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
    grass->Roughness = 0.125f;

    // This is not a good water material definition, but we do not have all the rendering
    // tools we need (transparency, environment reflection), so we fake it for now.
    auto water = std::make_unique<Material>();
    water->Name = "water";
    water->MatCBIndex = 1;
    water->DiffuseSrvHeapIndex = 1;
    water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
    water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    water->Roughness = 0.0f;

    auto wirefence = std::make_unique<Material>();
    wirefence->Name = "wirefence";
    wirefence->MatCBIndex = 2;
    wirefence->DiffuseSrvHeapIndex = 2;
    wirefence->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    wirefence->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    wirefence->Roughness = 0.25f;

    mMaterials["grass"] = std::move(grass);
    mMaterials["water"] = std::move(water);
    mMaterials["wirefence"] = std::move(wirefence);
}

void BlendApp::BuildFrameResources()
{
    for (int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(make_shared<BlendFrameResource>(
            mD3dDevice.Get(),
            1,
            static_cast<UINT>(mAllRitems.size()),
            static_cast<UINT>(mMaterials.size()),
            mWaves->VertexCount()));
    }
}

void BlendApp::BuildShadersAndInputLayout()
{
    const D3D_SHADER_MACRO fogDefine[] =
    {
        "FOG", "1",
        NULL, NULL
    };

    const D3D_SHADER_MACRO alphaTestDefine[] =
    {
        "FOG", "1",
        "ALPHA_TEST", "1",
        NULL, NULL
    };
    
    wstring shaderPath = TEXT(SHADER_PATH "blendShader.hlsl");
    mShaders["standardVS"] = D3dUtil::CompileShader(shaderPath, nullptr, "VS", "vs_5_0");
    mShaders["opaquePS"] = D3dUtil::CompileShader(shaderPath, fogDefine, "PS", "ps_5_0");
    mShaders["alphaTestedPS"] = D3dUtil::CompileShader(shaderPath, alphaTestDefine, "PS", "ps_5_0");

    mInputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}

void BlendApp::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 3;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(mD3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvheap)));

    CD3DX12_CPU_DESCRIPTOR_HANDLE hDesc(mSrvheap->GetCPUDescriptorHandleForHeapStart());

    auto grassTex = mTextures["grassTex"]->Resource;
    auto waterTex = mTextures["waterTex"]->Resource;
    auto fenceTex = mTextures["fenceTex"]->Resource;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping =D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = grassTex->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;
    mD3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDesc);

    hDesc.Offset(1, mCbvHandleSize);
    srvDesc.Format = waterTex->GetDesc().Format;
    mD3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDesc);

    hDesc.Offset(1, mCbvHandleSize);
    srvDesc.Format = fenceTex->GetDesc().Format;
    mD3dDevice->CreateShaderResourceView(fenceTex.Get(), &srvDesc, hDesc);
}

void BlendApp::BuildTextures()
{
    auto grassTex = std::make_unique<Texture>();
    grassTex->Name = "grassTex";
    grassTex->Filename = L"AppFactory/Textures/grass.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mD3dDevice.Get(),
        mCommandList.Get(), grassTex->Filename.c_str(),
        grassTex->Resource, grassTex->UploadHeap));

    auto waterTex = std::make_unique<Texture>();
    waterTex->Name = "waterTex";
    waterTex->Filename = L"AppFactory/Textures/water1.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mD3dDevice.Get(),
        mCommandList.Get(), waterTex->Filename.c_str(),
        waterTex->Resource, waterTex->UploadHeap));

    auto fenceTex = std::make_unique<Texture>();
    fenceTex->Name = "fenceTex";
    fenceTex->Filename = L"AppFactory/Textures/WireFence.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mD3dDevice.Get(),
        mCommandList.Get(), fenceTex->Filename.c_str(),
        fenceTex->Resource, fenceTex->UploadHeap));

    mTextures[grassTex->Name] = std::move(grassTex);
    mTextures[waterTex->Name] = std::move(waterTex);
    mTextures[fenceTex->Name] = std::move(fenceTex);
}

void BlendApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
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
    ThrowIfFailed(mD3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[EPSoType::Opaque])));

    //PSO for alpah test
    D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestPsoDesc = opaquePsoDesc;
    alphaTestPsoDesc.PS =
    { 
        reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
        mShaders["alphaTestedPS"]->GetBufferSize()
    };
    alphaTestPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    ThrowIfFailed(mD3dDevice->CreateGraphicsPipelineState(&alphaTestPsoDesc, IID_PPV_ARGS(&mPSOs[EPSoType::AlphaTest])));

    // PSO for transparent objects

    D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;
    D3D12_RENDER_TARGET_BLEND_DESC blendDesc;
    blendDesc.BlendEnable = true;
    blendDesc.LogicOpEnable = false;
    blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    transparentPsoDesc.BlendState.RenderTarget[0] = blendDesc;
    ThrowIfFailed(mD3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs[EPSoType::Translucent])));
}

void BlendApp::UpdateMainPassCB(const GameTimer& InGameTime)
{
    if (!mMainPassCB)
    {
        mMainPassCB = make_unique<BlendPassConstants>();
    }
    
    XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&mMainPassCB->View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB->InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB->Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB->InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB->ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB->InvViewProj, XMMatrixTranspose(invViewProj));
    mMainPassCB->EyePosW = mEyePostion;
    mMainPassCB->RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
    mMainPassCB->InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
    mMainPassCB->NearZ = 1.0f;
    mMainPassCB->FarZ = 1000.0f;
    mMainPassCB->TotalTime = InGameTime.TotalTime();
    mMainPassCB->DeltaTime = InGameTime.DeltaTime();
    mMainPassCB->AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
    mMainPassCB->Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
    mMainPassCB->Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
    mMainPassCB->Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
    mMainPassCB->Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
    mMainPassCB->Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
    mMainPassCB->Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

    auto currPassCB = dynamic_pointer_cast<BlendFrameResource>(mCurrFrameResource)->PassCB.get();
    currPassCB->CopyData(0, *mMainPassCB);
}

void BlendApp::UpdateObjectCBs(const GameTimer& InGameTime)
{
    auto currObjectCB = dynamic_pointer_cast<BlendFrameResource>(mCurrFrameResource)->ObjectCB.get();
    for(auto& e : mAllRitems)
    {
        // Only update the cbuffer data if the constants have changed.  
        // This needs to be tracked per frame resource.
        if(e->NumFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&e->World);
            XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

            LightObjectConstants objConstants;
            XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
            XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

            currObjectCB->CopyData(e->ObjCBIndex, objConstants);

            // Next FrameResource need to be updated too.
            e->NumFramesDirty--;
        }
    }
}

void BlendApp::UpdateMaterialCBs(const GameTimer& InGameTime)
{
    auto currMaterialCB = dynamic_pointer_cast<BlendFrameResource>(mCurrFrameResource)->MaterialCB.get();
    for(auto& e : mMaterials)
    {
        // Only update the cbuffer data if the constants have changed.  If the cbuffer
        // data changes, it needs to be updated for each FrameResource.
        Material* mat = e.second.get();
        if(mat->NumFramesDirty > 0)
        {
            XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

            MaterialConstants matConstants;
            matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
            matConstants.FresnelR0 = mat->FresnelR0;
            matConstants.Roughness = mat->Roughness;
            XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

            currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

            // Next FrameResource need to be updated too.
            mat->NumFramesDirty--;
        }
    }
}

void BlendApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems)
{
    UINT objCBByteSize = D3dUtil::CalculateConstantBufferByteSize(sizeof(LightObjectConstants));
        UINT matCBByteSize = D3dUtil::CalculateConstantBufferByteSize(sizeof(MaterialConstants));
    
        auto objectCB = dynamic_pointer_cast<BlendFrameResource>(mCurrFrameResource)->ObjectCB->GetResource();
        auto matCB = dynamic_pointer_cast<BlendFrameResource>(mCurrFrameResource)->MaterialCB->GetResource();
        
        // For each render item...
        for(size_t i = 0; i < rItems.size(); ++i)
        {
            auto ri = rItems[i];
    
            cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
            cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
            cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
    
            CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvheap->GetGPUDescriptorHandleForHeapStart());
            tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvHandleSize);
    
            D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
            D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;
    
            cmdList->SetGraphicsRootDescriptorTable(0, tex);
            cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
            cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);
    
            cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
        }
}

void BlendApp::AnimateMaterials(const GameTimer& InGameTime)
{
    // Scroll the water material texture coordinates.
    auto waterMat = mMaterials["water"].get();

    float& tu = waterMat->MatTransform(3, 0);
    float& tv = waterMat->MatTransform(3, 1);

    tu += 0.1f * InGameTime.DeltaTime();
    tv += 0.02f * InGameTime.DeltaTime();

    if(tu >= 1.0f)
        tu -= 1.0f;

    if(tv >= 1.0f)
        tv -= 1.0f;

    waterMat->MatTransform(3, 0) = tu;
    waterMat->MatTransform(3, 1) = tv;

    // Material has changed, so need to update cbuffer.
    waterMat->NumFramesDirty = gNumFrameResources;
}

void BlendApp::UpdateWaves(const GameTimer& InGameTime)
{
    // Every quarter second, generate a random wave.
    static float t_base = 0.0f;
    if((mTimer.TotalTime() - t_base) >= 0.25f)
    {
        t_base += 0.25f;

        int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
        int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

        float r = MathHelper::RandF(0.2f, 0.5f);

        mWaves->Disturb(i, j, r);
    }

    // Update the wave simulation.
    mWaves->Update(InGameTime.DeltaTime());

    // Update the wave vertex buffer with the new solution.
    auto currWavesVB = dynamic_pointer_cast<BlendFrameResource>(mCurrFrameResource)->WavesVB.get();
    for(int i = 0; i < mWaves->VertexCount(); ++i)
    {
        LightVertex v;

        v.Pos = mWaves->Position(i);
        v.Normal = mWaves->Normal(i);
		
        // Derive tex-coords from position by 
        // mapping [-w/2,w/2] --> [0,1]
        v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
        v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

        currWavesVB->CopyData(i, v);
    }

    // Set the dynamic VB of the wave renderitem to the current frame VB.
    mWaveRItem->Geo->VertexBufferGPU = currWavesVB->GetResource();
}

std::unique_ptr<MeshGeometry> BlendApp::BuildLandGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

    //
    // Extract the vertex elements we are interested and apply the height function to
    // each vertex.  In addition, color the vertices based on their height so we have
    // sandy looking beaches, grassy low hills, and snow mountain peaks.
    //

    std::vector<LightVertex> vertices(grid.Vertices.size());
    for(size_t i = 0; i < grid.Vertices.size(); ++i)
    {
        auto& p = grid.Vertices[i].Position;
        vertices[i].Pos = p;
        vertices[i].Pos.y = LandUtil::GetHillsHeight(p.x, p.z);
        vertices[i].Normal = LandUtil::GetHillsNormal(p.x, p.z);
        vertices[i].TexC = grid.Vertices[i].TexC;
    }

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(LightVertex);

    std::vector<std::uint16_t> indices = grid.GetIndices16();
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "landGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = D3dUtil::CreateDefaultBuffer(mD3dDevice.Get(),
        mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = D3dUtil::CreateDefaultBuffer(mD3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(LightVertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubMeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs["grid"] = submesh;

    return geo;
}

std::unique_ptr<MeshGeometry> BlendApp::BuildWaveGeometry()
{
    std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
    assert(mWaves->VertexCount() < 0x0000ffff);

    // Iterate over each quad.
    int m = mWaves->RowCount();
    int n = mWaves->ColumnCount();
    int k = 0;
    for(int i = 0; i < m - 1; ++i)
    {
        for(int j = 0; j < n - 1; ++j)
        {
            indices[k] = i*n + j;
            indices[k + 1] = i*n + j + 1;
            indices[k + 2] = (i + 1)*n + j;

            indices[k + 3] = (i + 1)*n + j;
            indices[k + 4] = i*n + j + 1;
            indices[k + 5] = (i + 1)*n + j + 1;

            k += 6; // next quad
        }
    }

    UINT vbByteSize = mWaves->VertexCount()*sizeof(LightVertex);
    UINT ibByteSize = (UINT)indices.size()*sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "waterGeo";

    // Set dynamically.
    geo->VertexBufferCPU = nullptr;
    geo->VertexBufferGPU = nullptr;

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->IndexBufferGPU = D3dUtil::CreateDefaultBuffer(mD3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(LightVertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubMeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs["grid"] = submesh;
    return geo;
}

std::unique_ptr<MeshGeometry> BlendApp::BuildBoxGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3);

    std::vector<LightVertex> vertices(box.Vertices.size());
    for (size_t i = 0; i < box.Vertices.size(); ++i)
    {
        auto& p = box.Vertices[i].Position;
        vertices[i].Pos = p;
        vertices[i].Normal = box.Vertices[i].Normal;
        vertices[i].TexC = box.Vertices[i].TexC;
    }

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(LightVertex);

    std::vector<std::uint16_t> indices = box.GetIndices16();
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "boxGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = D3dUtil::CreateDefaultBuffer(mD3dDevice.Get(),
        mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = D3dUtil::CreateDefaultBuffer(mD3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(LightVertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubMeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs["grid"] = submesh;

    return geo;
}
