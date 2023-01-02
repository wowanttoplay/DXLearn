#pragma once
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

protected:
    
    void BuildGeometry() override;
    void BuildRenderItems() override;
    void BuildMaterials() override;
    void BuildFrameResources() override;
    void BuildShadersAndInputLayout() override;
    void BuildDescriptorHeaps() override;
    void BuildTextures() override;

protected:
    std::unique_ptr<MeshGeometry> BuildLandGeometry();
    std::unique_ptr<MeshGeometry> BuildWaveGeometry();
    std::unique_ptr<Waves> mWaves;
};
