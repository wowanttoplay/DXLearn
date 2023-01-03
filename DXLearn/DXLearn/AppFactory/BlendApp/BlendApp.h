#pragma once
#include "BlendFrameResource.h"
#include "../LandAndWave/Waves.h"
#include "../Texture/TextureApp.h"

class BlendApp : public TextureApp
{
public:
    explicit BlendApp(HINSTANCE hInsatnce)
        : TextureApp(hInsatnce)
    {
        mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
    }
public:
    void Draw(const GameTimer& InGameTime) override;
    void Update(const GameTimer& InGameTime) override;

protected:

    void BuildGeometry() override;
    void BuildRenderItems() override;
    void BuildMaterials() override;
    void BuildFrameResources() override;
    void BuildShadersAndInputLayout() override;
    void BuildDescriptorHeaps() override;
    void BuildTextures() override;
    void BuildPSOs() override;
    void UpdateMainPassCB(const GameTimer& InGameTime) override;
    void UpdateObjectCBs(const GameTimer& InGameTime) override;
    void UpdateMaterialCBs(const GameTimer& InGameTime) override;
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems) override;
    void AnimateMaterials(const GameTimer& InGameTime) override;

    void UpdateWaves(const GameTimer& InGameTime);


    std::unique_ptr<MeshGeometry> BuildLandGeometry();
    std::unique_ptr<MeshGeometry> BuildWaveGeometry();
    std::unique_ptr<MeshGeometry> BuildBoxGeometry();
    std::unique_ptr<Waves> mWaves;
    RenderItem* mWaveRItem;
    std::unique_ptr<BlendPassConstants> mMainPassCB;
};
