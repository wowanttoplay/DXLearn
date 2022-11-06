#pragma once
#include "RenderItem.h"
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
    void BuildRenderItems();

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSig;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mMeshGeometry;

    // List of all the render item
    std::vector<std::unique_ptr<RenderItem>> mAllRenderItems;
    std::vector<RenderItem*> mOpaqueRenderItems;
};
