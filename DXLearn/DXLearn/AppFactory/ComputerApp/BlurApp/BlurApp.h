#pragma once
#include "BlurFilter.h"
#include "../../TreeBillboardsApp/TreeBillboardsApp.h"

class BlurApp : public TreeBillboardsApp
{
public:
    explicit BlurApp(HINSTANCE hInsatnce)
        : TreeBillboardsApp(hInsatnce)
    {
    }

    bool Initialize() override;
    void Draw(const GameTimer& InGameTime) override;

protected:
    void BuildDescriptorHeaps() override;
    void BuildShadersAndInputLayout() override;
    virtual void BuildPostRootSignature();
    virtual void BuildPostPSOs();

    std::unique_ptr<BlurFilter> mBlurFilter;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mPostProcessRootSignature = nullptr;
};
