#pragma once
#include "../../Common/D3dApp.h"

class ShapesApp : public D3dApp
{
public:
    explicit ShapesApp(HINSTANCE hInsatnce)
        : D3dApp(hInsatnce)
    {
    }

protected:
    virtual bool Initialize() override;

private:
    void BuildRootSignature();
    void BuildShadersAndInputLayout();

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSig;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
    Microsoft::WRL::ComPtr<D3D12_INPUT_LAYOUT_DESC> mInputLayout;
};
