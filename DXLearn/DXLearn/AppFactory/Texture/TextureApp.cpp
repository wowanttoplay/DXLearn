#include "TextureApp.h"

#include <array>
#include <DirectXColors.h>

#include "../../Common/DDSTextureLoader.h"
using namespace std;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

void TextureApp::Draw(const GameTimer& InGameTime)
{
    auto cmdAlloc = mCurrFrameResource->CmdListAlloc;
    ThrowIfFailed(cmdAlloc->Reset());

    if (mIsWireframe)
    {
        ThrowIfFailed((mCommandList->Reset(cmdAlloc.Get(), mPSOs[EPSoType::Opaque_wire].Get())));
    }
    else
    {
        ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), mPSOs[EPSoType::Opaque].Get()));
    }
    

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    mCommandList->ClearRenderTargetView(RenderTargetView(), Colors::LightBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);

    mCommandList->OMSetRenderTargets(1, &RenderTargetView(), true, &DepthStencilView());
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    ID3D12DescriptorHeap* descHeaps[] = {mSrvheap.Get()};
    mCommandList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

    auto passCB = dynamic_pointer_cast<LightFrameResource>(mCurrFrameResource)->PassCB->GetResource();
    mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mRItemLayers[ERenderLayer::Opaque]);

    // Indicate a state transition on the resource usage.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ExecuteCommandList();

    ThrowIfFailed(mSwapChain->Present(0,0));
    mCurrentSwapChainIndex = (mCurrentSwapChainIndex + 1) % mSwapChainBufferNumber;

    mCurrFrameResource->Fence = ++mCurrentFence;
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void TextureApp::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_ROOT_PARAMETER slotParameters[4];
    slotParameters[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    slotParameters[1].InitAsConstantBufferView(0);
    slotParameters[2].InitAsConstantBufferView(1);
    slotParameters[3].InitAsConstantBufferView(2);

    auto staticSampler = GetStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotParameters, static_cast<UINT>(staticSampler.size()), staticSampler.data(),
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
    IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void TextureApp::BuildShadersAndInputLayout()
{
    std::wstring ShaderPath = TEXT("AppFactory/Shaders/TexSamplerDefault.hlsl");
    mShaders["standardVS"] = D3dUtil::CompileShader(ShaderPath, nullptr, "VS", "vs_5_0");
    mShaders["opaquePS"] = D3dUtil::CompileShader(ShaderPath, nullptr, "PS", "ps_5_0");

    mInputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}

void TextureApp::BuildDescriptorHeaps()
{
    //Create SRV heap
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 4;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvheap)));

    // Fill the srv heap by texture desc
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDesc(mSrvheap->GetCPUDescriptorHandleForHeapStart());
    auto bricksTex = mTextures["bricksTex"]->Resource;
    auto stoneTex = mTextures["stoneTex"]->Resource;
    auto tileTex = mTextures["tileTex"]->Resource;
    auto skullTex = mTextures["skullTex"]->Resource;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = bricksTex->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = bricksTex->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    md3dDevice->CreateShaderResourceView(bricksTex.Get(), &srvDesc, hDesc);

    hDesc.Offset(1, mCbvHandleSize);
    srvDesc.Format = stoneTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = stoneTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(stoneTex.Get(), &srvDesc, hDesc);
    
    hDesc.Offset(1, mCbvHandleSize);
    srvDesc.Format = tileTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = tileTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(tileTex.Get(), &srvDesc, hDesc);

    hDesc.Offset(1, mCbvHandleSize);
    srvDesc.Format = skullTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = skullTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(skullTex.Get(), &srvDesc, hDesc);
}

void TextureApp::BuildTextures()
{
    auto bricksTex = std::make_unique<Texture>();
    bricksTex->Name = "bricksTex";
    bricksTex->Filename = TEXT("AppFactory/Textures/bricks.dds");
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), bricksTex->Filename.c_str(),
        bricksTex->Resource, bricksTex->UploadHeap));

    auto stoneTex = std::make_unique<Texture>();
    stoneTex->Name = "stoneTex";
    stoneTex->Filename = TEXT("AppFactory/Textures/stone.dds");
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), stoneTex->Filename.c_str(),
        stoneTex->Resource, stoneTex->UploadHeap));

    auto tileTex = std::make_unique<Texture>();
    tileTex->Name = "tileTex";
    tileTex->Filename = TEXT("AppFactory/Textures/tile.dds");
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), tileTex->Filename.c_str(),
        tileTex->Resource, tileTex->UploadHeap));

    auto skullTex = std::make_unique<Texture>();
    skullTex->Name = "skullTex";
    skullTex->Filename = TEXT("AppFactory/Textures/ice.dds");
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), skullTex->Filename.c_str(),
        skullTex->Resource, skullTex->UploadHeap));

    mTextures[bricksTex->Name] = move(bricksTex);
    mTextures[stoneTex->Name] = move(stoneTex);
    mTextures[tileTex->Name] = move(tileTex);
    mTextures[skullTex->Name] = move(skullTex);
}

void TextureApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems)
{
    UINT objCBByteSize = D3dUtil::CalculateConstantBufferByteSize(sizeof(LightObjectConstants));
    UINT matCBByteSize = D3dUtil::CalculateConstantBufferByteSize(sizeof(MaterialConstants));

    auto objectCB = dynamic_pointer_cast<LightFrameResource>(mCurrFrameResource)->ObjectCB->GetResource();
    auto matCB = dynamic_pointer_cast<LightFrameResource>(mCurrFrameResource)->MaterialCB->GetResource();
    
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

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> TextureApp::GetStaticSamplers()
{
    // Application usually only need a handle of samplers. So just define them all up front
    // and keep them avaliable as part of the root signature

    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
        0,
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
        1,
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
        2,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        3,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
        4,
        D3D12_FILTER_ANISOTROPIC,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0.0f,
        8);

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
        5,
        D3D12_FILTER_ANISOTROPIC,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        0.0f,
        8);

    return {pointWrap, pointClamp,
    linearWrap, linearClamp,
    anisotropicWrap, anisotropicClamp};
}

