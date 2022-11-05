#pragma once
#include "../../Common/D3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"

struct BoxObjectConstants
{
    DirectX::XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class BoxApp : public D3dApp
{
public:
    explicit BoxApp(HINSTANCE hInsatnce)
        : D3dApp(hInsatnce)
    {
    }

    BoxApp(const BoxApp& other) = delete;
    BoxApp& operator=(const BoxApp& other) = delete;
    virtual ~BoxApp() = default;

   virtual bool Initialize() override;
protected:
    virtual void Update(const GameTimer& InGameTime) override;
    virtual void Draw(const GameTimer& InGameTime) override;
    virtual void OnResize() override;

private:
    void BuildBoxGeometry();
    void BuildConstantBufferViewHeap();
    void BuildConstantBuffersAndView();
    void BuildShadersAndInputLayout();
    void BuildRootSignature();
    void BuildPSO();

private:
    std::unique_ptr<MeshGeometry> mBoxGeo;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
    std::unique_ptr<UploadBuffer<BoxObjectConstants>> mConstantBuffer = nullptr;

    Microsoft::WRL::ComPtr<ID3DBlob> mVsByteCode = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> mPsByteCode = nullptr;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO = nullptr;
};
