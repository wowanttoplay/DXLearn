#pragma once
#include "../Light/LightApp.h"

class TextureApp: public LightApp
{
public:
    explicit TextureApp(HINSTANCE hInsatnce)
        : LightApp(hInsatnce)
    {
    }

    void Draw(const GameTimer& InGameTime) override;

protected:
    void BuildRootSignature() override;
    void BuildShadersAndInputLayout() override;
    void BuildDescriptorHeaps() override;
    void BuildTextures() override;
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems) override;

    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

protected:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvheap = nullptr;
};
