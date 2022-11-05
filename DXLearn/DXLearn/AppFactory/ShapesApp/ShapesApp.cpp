#include "ShapesApp.h"

#include "../../Common/d3dx12.h"

bool ShapesApp::Initialize()
{
    if (!D3dApp::Initialize())
    {
        return false;
    }
    ThrowIfFailed(mCommandList->Reset(mCommandAlloctor.Get(), nullptr));

    BuildRootSignature();
    BuildShadersAndInputLayout();
    
    
    

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
    constexpr std::wstring ShaderPath = TEXT("AppFactory\\ShapesApp\\Shaders\\color.hlsl");

    mShaders["standardVS"] = D3dUtil::CompileShader(ShaderPath, nullptr, "VS", "vs_5_1");
    mShaders["opaquePS"] = D3dUtil::CompileShader(ShaderPath, nullptr, "PS", "ps_5_1");

    mInputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}
