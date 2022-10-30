#include "BoxApp.h"

#include <array>
#include <DirectXColors.h>
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
}

void BoxApp::Update(const GameTimer& InGameTime)
{
}

void BoxApp::Draw(const GameTimer& InGameTime)
{
    
}

void BoxApp::OnResize()
{
    D3dApp::OnResize();
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    D3dApp::OnMouseUp(btnState, x, y);
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    D3dApp::OnMouseDown(btnState, x, y);
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    D3dApp::OnMouseMove(btnState, x, y);
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
    mConstantBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(mD3dDevice.Get(), 1, true);

    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mConstantBuffer->GetResource()->GetGPUVirtualAddress();
    UINT boxConstantIndex = 0;
    cbAddress += boxConstantIndex * mConstantBuffer->GetElementByteSize();

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = mConstantBuffer->GetElementByteSize();

    mD3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    // create a single descriptor table of CBVs
    CD3DX12_DESCRIPTOR_RANGE cbvTables;
    cbvTables.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTables);

    // A root signature is an array of root  parameters
    CD3DX12_ROOT_SIGNATURE_DESC rootSignaturedesc(1, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // Create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    Microsoft::WRL::ComPtr<ID3DBlob> serializeRootSig = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errBlob = nullptr;
    HRESULT rst = D3D12SerializeRootSignature(&rootSignaturedesc, D3D_ROOT_SIGNATURE_VERSION_1, serializeRootSig.GetAddressOf(), errBlob.GetAddressOf());

    if (errBlob)
    {
        OutputDebugStringA(static_cast<char*>(errBlob->GetBufferPointer()));
    }
    ThrowIfFailed(rst);

    ThrowIfFailed(mD3dDevice->CreateRootSignature(0, serializeRootSig->GetBufferPointer(), serializeRootSig->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
}
