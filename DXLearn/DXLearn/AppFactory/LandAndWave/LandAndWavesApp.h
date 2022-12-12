#pragma once
#include "Waves.h"
#include "../../Common/D3dApp.h"
#include "../../Common/RenderItem.h"

enum class RenderLayer : int
{
    Opaque = 0,
    Count
};

class LandAndWavesApp : public D3dApp
{
public:

    virtual bool Initialize() override;
    virtual void Draw(const GameTimer& InGameTime) override;
    virtual void Update(const GameTimer& InGameTime) override;
private:
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildLandGeometry();
    void BuildWaveGeomtry();
    void BuildRenderItem();
    void BuildFrameResource();
    void BuildPSO();

private:
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unique_ptr<Waves> mWaves;
    // List of all render items
    std::vector<std::unique_ptr<RenderItem>> mAllRenderItems;
    // Render Items divided by PSO
    std::vector<RenderItem*> mRitemLayer[static_cast<int>(RenderLayer::Count)];
    RenderItem* mWaveRenderItem = nullptr;
    

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    
};
