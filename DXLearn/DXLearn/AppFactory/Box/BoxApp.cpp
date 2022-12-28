#include "BoxApp.h"

#include <array>
#include <DirectXColors.h>
#include <iostream>
using namespace DirectX;

struct BoxVertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

bool BoxApp::Initialize()
{
    if (!D3dApp::Initialize())
    {
        return false;
    }

    ThrowIfFailed(mCommandList->Reset(mCommandAlloctor.Get(), nullptr));

    BuildBoxGeometry();

    BuildConstantBufferViewHeap();
    BuildConstantBuffersAndView();
    
    BuildRootSignature();
    
    BuildShadersAndInputLayout();
    
    BuildPSO();

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdList[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

    FlushCommandQueue();

    return true;
}

void BoxApp::Update(const GameTimer& InGameTime)
{
    D3dApp::Update(InGameTime);
    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    XMMATRIX worldViewProj = world * view * proj;

    BoxObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    mConstantBuffer->CopyData(0, objConstants);
}

void BoxApp::Draw(const GameTimer& InGameTime)
{
    ThrowIfFailed(mCommandAlloctor->Reset());
    ThrowIfFailed(mCommandList->Reset(mCommandAlloctor.Get(), mPSO.Get()));

    // 设置视图
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // 转换render target resource state
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                      CurrentRenderTargetBuffer(), D3D12_RESOURCE_STATE_PRESENT,
                                      D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the depth and stencil
    mCommandList->ClearRenderTargetView(RenderTargetView(), Colors::SkyBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // set render target and depth stencil
    mCommandList->OMSetRenderTargets(1, &RenderTargetView(), true, &DepthStencilView());

    // bind root signature and descriptor heaps
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    ID3D12DescriptorHeap* descHeap[] = {mCbvHeap.Get()};
    mCommandList->SetDescriptorHeaps(_countof(descHeap), descHeap);

    // set vertex and index data
    mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
    mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // set descriptor pointer
    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    // draw index
    for (auto& subMesh : mBoxGeo->DrawArgs)
    {
        const SubMeshGeometry& mesh = subMesh.second;
        mCommandList->DrawIndexedInstanced(mesh.IndexCount, 1, mesh.StartIndexLocation, mesh.BaseVertexLocation, 0);
    }

    // Transition the render target resource state
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentRenderTargetBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(mCommandList->Close());

    // Add command list into the command queue
    ID3D12CommandList* cmdList[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(1, cmdList);

    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrentSwapChainIndex = (mCurrentSwapChainIndex + 1) % mSwapChainBufferNumber;
    
    FlushCommandQueue();
}

void BoxApp::OnResize()
{
    D3dApp::OnResize();
}

void BoxApp::BuildBoxGeometry()
{
    std::array<BoxVertex, 8> vertices =
    {
        BoxVertex({XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)}),
        BoxVertex({XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)}),
        BoxVertex({XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)}),
        BoxVertex({XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)}),
        BoxVertex({XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)}),
        BoxVertex({XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)}),
        BoxVertex({XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)}),
        BoxVertex({XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta)})
    };

    std::array<std::uint16_t, 36> indices =
    {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };

    const UINT vbByteSize = static_cast<UINT>(vertices.size() * sizeof(BoxVertex));
    const UINT ibByteSize = static_cast<UINT>(indices.size() * sizeof(std::uint16_t));

    mBoxGeo = std::make_unique<MeshGeometry>();
    mBoxGeo->Name = "BoxGeometry";

    // copy data to cpu
    ThrowIfFailed(D3DCreateBlob(vbByteSize, mBoxGeo->VertexBufferCPU.GetAddressOf()));
    ThrowIfFailed(D3DCreateBlob(ibByteSize, mBoxGeo->IndexBufferCPU.GetAddressOf()));
    CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
    CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    // copy data to gpu
    mBoxGeo->VertexBufferGPU = D3dUtil::CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);
    mBoxGeo->IndexBufferGPU = D3dUtil::CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

    mBoxGeo->VertexByteStride = sizeof(BoxVertex);
    mBoxGeo->VertexBufferByteSize = vbByteSize;
    mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
    mBoxGeo->IndexBufferByteSize = ibByteSize;

    // create box submesh
    SubMeshGeometry subMesh;
    subMesh.IndexCount = static_cast<UINT>(indices.size());
    subMesh.BaseVertexLocation = 0;
    subMesh.StartIndexLocation = 0;

    mBoxGeo->DrawArgs["box"] = subMesh;
}

void BoxApp::BuildConstantBufferViewHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors  = 1;
    cbvHeapDesc.NodeMask = 0;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(mD3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void BoxApp::BuildConstantBuffersAndView()
{
    mConstantBuffer = std::make_unique<UploadBuffer<BoxObjectConstants>>(mD3dDevice.Get(), 1, true);

    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mConstantBuffer->GetResource()->GetGPUVirtualAddress();
    UINT boxConstantIndex = 0;
    cbAddress += boxConstantIndex * mConstantBuffer->GetElementByteSize();

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = mConstantBuffer->GetElementByteSize();

    mD3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildShadersAndInputLayout()
{
    const std::wstring FileName = TEXT("AppFactory\\Box\\Shaders\\color.hlsl");
    mVsByteCode = D3dUtil::CompileShader(FileName, nullptr, "VS", "vs_5_0");
    mPsByteCode = D3dUtil::CompileShader(FileName, nullptr, "PS", "ps_5_0");

    mInputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}

void BoxApp::BuildRootSignature()
{
    // Shader programs typically require resources as input (constant buffers,
    // textures, samplers).  The root signature defines the resources the shader
    // programs expect.  If we think of the shader programs as a function, and
    // the input resources as function parameters, then the root signature can be
    // thought of as defining the function signature.  

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    // Create a single descriptor table of CBVs.
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if(errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(mD3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&mRootSignature)));
}

void BoxApp::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = {mInputLayout.data(), static_cast<UINT>(mInputLayout.size())};
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = {static_cast<BYTE*>(mVsByteCode->GetBufferPointer()), mVsByteCode->GetBufferSize()};
    psoDesc.PS = {static_cast<byte*>(mPsByteCode->GetBufferPointer()), mPsByteCode->GetBufferSize()};
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = mMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = mMsaaState ? mMsaaQuality - 1 : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(mD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}
