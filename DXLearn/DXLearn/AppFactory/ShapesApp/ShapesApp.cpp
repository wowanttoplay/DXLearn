#include "ShapesApp.h"

#include <DirectXColors.h>

#include "ShapesFrameResource.h"
#include "../../Common/d3dx12.h"
#include "../../Common/GeometryGenerator.h"

const std::string GeoName = "shapeGeo";

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
    BuildRenderItems();
    BuildFrameResource();
    BuildDescriptorHeaps();
    BuildContantBufferViews();
    BuildPSO();

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdList[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

    FlushCommandQueue();
    
    return true;
}

void ShapesApp::Update(const GameTimer& InGameTime)
{
    OnKeyboardInput(InGameTime);
    D3dApp::Update(InGameTime);

    mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % gNumFrameResources;
    mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

    // Has the GPu finished processing the commands of the current frame resource ?
    // If not , wait until the GPU has completed commmands up to this fence point
    if (mCurrentFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    UpdateObjectCBs(InGameTime);
    UpdateMainPassCB(InGameTime);
}

void ShapesApp::Draw(const GameTimer& InGameTime)
{
    auto cmdAlloc = mCurrentFrameResource->CmdListAlloc;
    ThrowIfFailed(cmdAlloc->Reset());

    Microsoft::WRL::ComPtr<ID3D12PipelineState> currentPiplineState = mIsWireframe ? mPSOs["opaque_wrieframe"] : mPSOs["opaque"];
    ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), currentPiplineState.Get()));

    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    mCommandList->ClearRenderTargetView(RenderTargetView(), DirectX::Colors::Aqua, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    mCommandList->OMSetRenderTargets(1, &RenderTargetView(), true, &DepthStencilView());

    ID3D12DescriptorHeap* dsvHeaps[] = {mDescriptorHeap.Get()};
    mCommandList->SetDescriptorHeaps(_countof(dsvHeaps), dsvHeaps);
    mCommandList->SetGraphicsRootSignature(mRootSig.Get());


    // set pass constant pointer
    UINT passCBVIndex = mPassCBVOffset + mCurrentFrameResourceIndex;
    auto passCBVHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    passCBVHandle.Offset(passCBVIndex, mCbvHandleSize);
    mCommandList->SetGraphicsRootDescriptorTable(1, passCBVHandle);
    // draw render items
    DrawRenderItems(mCommandList.Get(), mOpaqueRenderItems);
    
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdList[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

    ThrowIfFailed(mSwapChain->Present(0,0));
    mCurrentSwapChainIndex = (mCurrentSwapChainIndex + 1) % mSwapChainBufferNumber;

    mCurrentFrameResource->Fence = ++mCurrentFence;
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
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
    geo->Name = GeoName;

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

void ShapesApp::BuildRenderItems()
{
    // box render item
    auto boxItem = std::make_unique<RenderItem>();
    DirectX::XMStoreFloat4x4(&boxItem->World, DirectX::XMMatrixScaling(2.0, 2.0, 2.0) * DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    boxItem->objectIndex = 0;
    boxItem->Geo = mMeshGeometry[GeoName].get();
    boxItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxItem->IndexCount = boxItem->Geo->DrawArgs["box"].IndexCount;
    boxItem->BaseVertexLocation = boxItem->Geo->DrawArgs["box"].BaseVertexLocation;
    boxItem->StartIndexLocation = boxItem->Geo->DrawArgs["box"].StartIndexLocation;
    mAllRenderItems.push_back(std::move(boxItem));

    // grid render item
    auto gridItem = std::make_unique<RenderItem>();
    gridItem->World = MathHelper::Identity4x4();
    gridItem->objectIndex = 1;
    gridItem->Geo = mMeshGeometry[GeoName].get();
    gridItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridItem->IndexCount = gridItem->Geo->DrawArgs["grid"].IndexCount;
    gridItem->BaseVertexLocation = gridItem->Geo->DrawArgs["grid"].BaseVertexLocation;
    gridItem->StartIndexLocation = gridItem->Geo->DrawArgs["grid"].StartIndexLocation;
    mAllRenderItems.push_back(std::move(gridItem));

    UINT objectIndex = 2;
    for (int index = 0; index < 5; ++index)
    {
        auto leftCylinderRnderItem = std::make_unique<RenderItem>();
        auto leftSphereRenderItem = std::make_unique<RenderItem>();
        auto rightCylinderRenderItem = std::make_unique<RenderItem>();
        auto rightSphereRenderItem = std::make_unique<RenderItem>();

        DirectX::XMMATRIX leftCylinderWorld = DirectX::XMMatrixTranslation(-5.0f, 1.5f, -10.0f + index * 5.0f);
        DirectX::XMMATRIX rightCylinderWorld = DirectX::XMMatrixTranslation(5.0f, 1.5f, -10.0f + index * 5.0f);
        DirectX::XMMATRIX leftSphereWorld = DirectX::XMMatrixTranslation(-5.0f, 3.5f, -10.0f + index * 5.0f);
        DirectX::XMMATRIX rightSphereWorld = DirectX::XMMatrixTranslation(5.0f, 3.5f, -10.0f + index * 5.0f);

        DirectX::XMStoreFloat4x4(&leftCylinderRnderItem->World, leftCylinderWorld);
        leftCylinderRnderItem->objectIndex = objectIndex++;
        leftCylinderRnderItem->Geo = mMeshGeometry[GeoName].get();
        leftCylinderRnderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftCylinderRnderItem->IndexCount = leftCylinderRnderItem->Geo->DrawArgs["cylinder"].IndexCount;
        leftCylinderRnderItem->BaseVertexLocation = leftCylinderRnderItem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
        leftCylinderRnderItem->StartIndexLocation = leftCylinderRnderItem->Geo->DrawArgs["cylinder"].StartIndexLocation;
        mAllRenderItems.push_back(std::move(leftCylinderRnderItem));

        DirectX::XMStoreFloat4x4(&rightCylinderRenderItem->World, rightCylinderWorld);
        rightCylinderRenderItem->objectIndex = objectIndex++;
        rightCylinderRenderItem->Geo = mMeshGeometry[GeoName].get();
        rightCylinderRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightCylinderRenderItem->IndexCount = rightCylinderRenderItem->Geo->DrawArgs["cylinder"].IndexCount;
        rightCylinderRenderItem->BaseVertexLocation = rightCylinderRenderItem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
        rightCylinderRenderItem->StartIndexLocation = rightCylinderRenderItem->Geo->DrawArgs["cylinder"].StartIndexLocation;
        mAllRenderItems.push_back(std::move(rightCylinderRenderItem));

        DirectX::XMStoreFloat4x4(&leftSphereRenderItem->World, leftSphereWorld);
        leftSphereRenderItem->objectIndex = objectIndex++;
        leftSphereRenderItem->Geo = mMeshGeometry[GeoName].get();
        leftSphereRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftSphereRenderItem->IndexCount = leftSphereRenderItem->Geo->DrawArgs["sphere"].IndexCount;
        leftSphereRenderItem->BaseVertexLocation = leftSphereRenderItem->Geo->DrawArgs["sphere"].BaseVertexLocation;
        leftSphereRenderItem->StartIndexLocation = leftSphereRenderItem->Geo->DrawArgs["sphere"].StartIndexLocation;
        mAllRenderItems.push_back(std::move(leftSphereRenderItem));

        DirectX::XMStoreFloat4x4(&rightSphereRenderItem->World, rightSphereWorld);
        rightSphereRenderItem->objectIndex = objectIndex++;
        rightSphereRenderItem->Geo = mMeshGeometry[GeoName].get();
        rightSphereRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightSphereRenderItem->IndexCount = rightSphereRenderItem->Geo->DrawArgs["sphere"].IndexCount;
        rightSphereRenderItem->BaseVertexLocation = rightSphereRenderItem->Geo->DrawArgs["sphere"].BaseVertexLocation;
        rightSphereRenderItem->StartIndexLocation = rightSphereRenderItem->Geo->DrawArgs["sphere"].StartIndexLocation;
        mAllRenderItems.push_back(std::move(rightSphereRenderItem));
    }

    // All the render items are opaque
    for (auto& e : mAllRenderItems)
    {
        mOpaqueRenderItems.push_back(e.get());
    }
    
}

void ShapesApp::BuildFrameResource()
{
    for (size_t index = 0; index < gNumFrameResources; ++index)
    {
        mFrameResources.emplace_back(std::move(
            std::make_unique<ShapesFrameResource>(mD3dDevice.Get(), 1, static_cast<UINT>(mAllRenderItems.size()))));
    }
}

void ShapesApp::BuildDescriptorHeaps()
{
    UINT objCount = static_cast<UINT>(mOpaqueRenderItems.size());
    // Need a constant buffer view descriptor for each frame resource,
    // +1 for the perpass for each fraeme resource
    UINT numDescriptors = (objCount + 1)  * gNumFrameResources;

    // Save an offset to the start of the pass CBVs. There are the last 3 descriptors
    mPassCBVOffset = objCount * gNumFrameResources;

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.NodeMask = 0u;
    cbvHeapDesc.NumDescriptors = numDescriptors;

    ThrowIfFailed(mD3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mDescriptorHeap)));
}

void ShapesApp::BuildContantBufferViews()
{
    UINT objCBByteSize = D3dUtil::CalculateConstantBufferByteSize(sizeof(shapesObjectConstants));
    UINT objCount = static_cast<UINT>(mAllRenderItems.size());

    // Need a CBV descriptor fro each object fro each frame resource
    for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
    {
        auto objectCB = mFrameResources[frameIndex]->ObjectCb->GetResource();
        for (UINT objIndex = 0; objIndex < objCount; ++objIndex)
        {
            D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = objectCB->GetGPUVirtualAddress();

            // Offset the gpu address in buffer
            cbvAddress += objIndex * objCBByteSize;

            // Offset to the object cbv in the descriptor heap
            int heapIndex = frameIndex * objCount + objIndex;
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(heapIndex, mCbvHandleSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
            cbvDesc.BufferLocation = cbvAddress;
            cbvDesc.SizeInBytes = objCBByteSize;

            mD3dDevice->CreateConstantBufferView(&cbvDesc, handle);
        }
    }

    UINT passCBByteSize = D3dUtil::CalculateConstantBufferByteSize(sizeof(ShapesPassContants));
    // Last three descriptor are the pass CBVs for each frame resource
    for (UINT frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
    {
        auto passCB = mFrameResources[frameIndex]->PassCB->GetResource();
        D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = passCB->GetGPUVirtualAddress();

        // Offset to the pass cbv in the descriptor heap
        int heapIndex = mPassCBVOffset + frameIndex;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, mCbvHandleSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbvAddress;
        cbvDesc.SizeInBytes = passCBByteSize;

        mD3dDevice->CreateConstantBufferView(&cbvDesc, handle);
    }
}

void ShapesApp::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

    //
    // PSO for opaque objects.
    //
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    opaquePsoDesc.pRootSignature = mRootSig.Get();
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
    opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    opaquePsoDesc.SampleMask = UINT_MAX;
    opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    opaquePsoDesc.NumRenderTargets = 1;
    opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
    opaquePsoDesc.SampleDesc.Count = mMsaaState ? 4 : 1;
    opaquePsoDesc.SampleDesc.Quality = mMsaaState ? (mMsaaQuality - 1) : 0;
    opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(mD3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));


    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
    opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(mD3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["opaque_wrieframe"])));
}

void ShapesApp::OnKeyboardInput(const GameTimer& InGameTime)
{
    if (GetAsyncKeyState('1') & 0x8000)
    {
        mIsWireframe = true;
    }
    else
    {
        mIsWireframe  = false;
    }
}

void ShapesApp::UpdateObjectCBs(const GameTimer& IngameTime)
{
    auto objCBBuffer = mCurrentFrameResource->ObjectCb.get();
    for (auto& e : mAllRenderItems)
    {
        if (e->NumFrameDirty > 0)
        {
            DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&e->World);

            shapesObjectConstants objConstant;
            DirectX::XMStoreFloat4x4(&objConstant.World, DirectX::XMMatrixTranspose(world));

            objCBBuffer->CopyData(e->objectIndex, objConstant);

            e->NumFrameDirty--;
        }
    }
}

void ShapesApp::UpdateMainPassCB(const GameTimer& InGamTime)
{
    DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&mView);
    DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&mProj);

    DirectX::XMMATRIX viewPorj = DirectX::XMMatrixMultiply(view, proj);
    DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(view), view);
    DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(proj), proj);
    DirectX::XMMATRIX invViewPorj = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(viewPorj), viewPorj);

    DirectX::XMStoreFloat4x4(&mMainPassCB.View, DirectX::XMMatrixTranspose(view));
    DirectX::XMStoreFloat4x4(&mMainPassCB.Proj, DirectX::XMMatrixTranspose(proj));
    DirectX::XMStoreFloat4x4(&mMainPassCB.InvProj, DirectX::XMMatrixTranspose(invProj));
    DirectX::XMStoreFloat4x4(&mMainPassCB.InvView, DirectX::XMMatrixTranspose(invView));
    DirectX::XMStoreFloat4x4(&mMainPassCB.ViewPorj, DirectX::XMMatrixTranspose(viewPorj));
    DirectX::XMStoreFloat4x4(&mMainPassCB.InvViewProj, DirectX::XMMatrixTranspose(invViewPorj));
    mMainPassCB.EyePosW = mEyePostion;
    mMainPassCB.RenderTargetSize = DirectX::XMFLOAT2(static_cast<float>(mClientWidth), static_cast<float>(mClientHeight));
    mMainPassCB.InvRenderTargetSize = DirectX::XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
    mMainPassCB.NearZ = mNearZ;
    mMainPassCB.FarZ = mFarZ;
    mMainPassCB.TotalTime = InGamTime.TotalTime();
    mMainPassCB.DeltaTime = InGamTime.DeltaTime();

    auto currPassCB = mCurrentFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

void ShapesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& renderItems)
{
    for (size_t index = 0; index < renderItems.size(); ++index)
    {
        auto renderItem = renderItems.at(index);
        cmdList->IASetVertexBuffers(0, 1, &renderItem->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&renderItem->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(renderItem->PrimitiveType);

        UINT cbvIndex = mCurrentFrameResourceIndex * static_cast<UINT>(mOpaqueRenderItems.size()) + renderItem->objectIndex;
        auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(cbvIndex, mCbvHandleSize);

        cmdList->SetGraphicsRootDescriptorTable(0, handle);

        cmdList->DrawIndexedInstanced(renderItem->IndexCount, 1, renderItem->StartIndexLocation, renderItem->BaseVertexLocation, 0);
    }
}