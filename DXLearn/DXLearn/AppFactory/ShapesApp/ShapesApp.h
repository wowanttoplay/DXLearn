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
    void BuildMeshGeometry();

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSig;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mMeshGeometry;
};
