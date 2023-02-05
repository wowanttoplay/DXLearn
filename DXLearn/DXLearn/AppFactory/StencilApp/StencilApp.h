#pragma once
#include "../BlendApp/BlendApp.h"

class StencilApp : public BlendApp
{
public:
    explicit StencilApp(HINSTANCE hInsatnce)
        : BlendApp(hInsatnce)
    {
    }

    void Draw(const GameTimer& InGameTime) override;
    void Update(const GameTimer& InGameTime) override;

    /**
     * 几何体、纹理、材质资源的准备
     */
    void BuildGeometry() override;
    void BuildMaterials() override;
    void BuildTextures() override;
    void BuildDescriptorHeaps() override;

    void BuildRenderItems() override;
    void BuildFrameResources() override;
    void BuildPSOs() override;

protected:
    void AnimateMaterials(const GameTimer& InGameTime) override;
    void UpdateWaves(const GameTimer& InGameTime) override;

    void BuildRoomGeometry();
    void OnKeyboardInput(const GameTimer& InGameTime) override;

private:
    RenderItem* mSkullRitem = nullptr;
    RenderItem* mReflectedSkullRitem = nullptr;
    RenderItem* mShadowedSkullRitem = nullptr;
private:
    void UpdateReflectedPassCB(const GameTimer& InGameTime);
private:
    std::unique_ptr<BlendPassConstants> mReflectedPassCB;
    DirectX::XMFLOAT3 mSkullTranslation = { 0.0f, 1.0f, -5.0f };
};
