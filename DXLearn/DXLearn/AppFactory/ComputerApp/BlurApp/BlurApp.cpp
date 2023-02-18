#include "BlurApp.h"

#include "../../../Common/FileManager.h"
using namespace std;
using namespace Microsoft::WRL;

bool BlurApp::Initialize()
{
    if (!D3dApp::Initialize())
    {
        return false;
    }

    ThrowIfFailed(mCommandList->Reset(mCommandAlloctor.Get(), nullptr));
    mWaves = make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
    mBlurFilter = make_unique<BlurFilter>(md3dDevice.Get(), mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);

    BuildTextures();
    BuildRootSignature();
    BuildPostRootSignature();
    BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
    BuildGeometry();
    BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();
    BuildPostPSOs();
    ExecuteCommandList();
    FlushCommandQueue();

    return true;
}

void BlurApp::Draw(const GameTimer& InGameTime)
{
    auto cmdAlloc = mCurrFrameResource->CmdListAlloc;

    ThrowIfFailed(cmdAlloc->Reset());
    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), mPSOs[EPSoType::Opaque].Get()));

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

    ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvheap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    auto passCB = dynamic_pointer_cast<BlendFrameResource>(mCurrFrameResource)->PassCB->GetResource();
    mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mRItemLayers[ERenderLayer::Opaque]);

    mCommandList->SetPipelineState(mPSOs[EPSoType::AlphaTest].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayers[ERenderLayer::AlphaTested]);

    mCommandList->SetPipelineState(mPSOs[EPSoType::Translucent].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayers[ERenderLayer::Translucent]);

    mBlurFilter->Execute(mCommandList.Get(), mPostProcessRootSignature.Get(), mPSOs[EPSoType::HorzBlur].Get(), mPSOs[EPSoType::VertBlur].Get(), CurrentRenderTargetBuffer(), 4);

    // Prepare to copy blurred output to the back buffer
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

    mCommandList->CopyResource(CurrentRenderTargetBuffer(), mBlurFilter->Output());

    //Transition to Present state
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));

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

void BlurApp::BuildDescriptorHeaps()
{
    const int textureDescriptorCount = 4;
    const int blurDescriptorCount = 4;

    //
    // Create the SRV heap.
    //
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = textureDescriptorCount + 
     blurDescriptorCount;
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
    mBlurFilter->BuildDescriptors(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvheap->GetCPUDescriptorHandleForHeapStart(), 3, mCbvHandleSize),
        CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvheap->GetGPUDescriptorHandleForHeapStart(), 3, mCbvHandleSize),
        mCbvHandleSize
    );
}


void BlurApp::BuildShadersAndInputLayout()
{
    TreeBillboardsApp::BuildShadersAndInputLayout();

    wstring BlurShaderPath = FileManager::GetShaderFullPath("Blur.hlsl");
    mShaders["horzBlurCS"] = D3dUtil::CompileShader(BlurShaderPath, nullptr, "HorzBlurCS", "cs_5_0");
    mShaders["vertBlurCS"] = D3dUtil::CompileShader(BlurShaderPath, nullptr, "VertBlurCS", "cs_5_0");
}

void BlurApp::BuildPostRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE srvTable;
    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE uavTable;
    uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[3];

    // Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter[0].InitAsConstants(12, 0);
    slotRootParameter[1].InitAsDescriptorTable(1, &srvTable);
    slotRootParameter[2].InitAsDescriptorTable(1, &uavTable);

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
        0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if(errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(mPostProcessRootSignature.GetAddressOf())
    ));
}

void BlurApp::BuildPostPSOs()
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC horzBlurPSO = {};
    horzBlurPSO.pRootSignature = mPostProcessRootSignature.Get();
    horzBlurPSO.CS =
    {
        reinterpret_cast<BYTE*>(mShaders["horzBlurCS"]->GetBufferPointer()),
        mShaders["horzBlurCS"]->GetBufferSize()
    };
    horzBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(md3dDevice->CreateComputePipelineState(&horzBlurPSO, IID_PPV_ARGS(&mPSOs[EPSoType::HorzBlur])));

    D3D12_COMPUTE_PIPELINE_STATE_DESC vertBlurPSO = {};
    vertBlurPSO.pRootSignature = mPostProcessRootSignature.Get();
    vertBlurPSO.CS =
    {
        reinterpret_cast<BYTE*>(mShaders["vertBlurCS"]->GetBufferPointer()),
        mShaders["vertBlurCS"]->GetBufferSize()
    };
    vertBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(md3dDevice->CreateComputePipelineState(&vertBlurPSO, IID_PPV_ARGS(&mPSOs[EPSoType::VertBlur])));
}
