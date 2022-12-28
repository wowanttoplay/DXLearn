#pragma once
#include "LightFrameResource.h"
#include "../../Common/D3dApp.h"
#include "../../Common/RenderItem.h"

class LightApp : public D3dApp
{
public:
    explicit LightApp(HINSTANCE hInsatnce)
        : D3dApp(hInsatnce)
    {
    }

    virtual ~LightApp() = default;

public:
    bool Initialize() override;
    void Update(const GameTimer& InGameTime) override;
    void Draw(const GameTimer& InGameTime) override;

protected:
    virtual void BuildRootSignature();
    virtual void BuildShadersAndInputLayout();
    virtual void BuildGeometry();
    virtual void BuildRenderItems();
    virtual void BuildMaterials();
    virtual void BuildFrameResources();
    virtual void BuildPSOs();
protected:
    virtual void OnKeyboardInput(const GameTimer& InGameTime);
    virtual void AnimateMaterials(const GameTimer& InGameTime);
    virtual void UpdateObjectCBs(const GameTimer& InGameTime);
    virtual void UpdateMaterialCBs(const GameTimer& InGameTime);
    virtual void UpdateMainPassCB(const GameTimer& InGameTime);
protected:
    virtual void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems);

protected:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

protected:
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;
    std::vector<RenderItem*> mOpaqueRitems;

protected:
    std::unordered_map<EPSoType, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;
    
protected:
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
    std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

protected:
    std::vector<std::unique_ptr<LightFrameResource>> mFrameResources;
    LightFrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;
    
protected:
    std::unique_ptr<LightPassConstants> mMainPassCB = nullptr;

protected:
    bool mIsWireframe = false;

private:
    void BuildShapeGeometry();
    void BuildSkullGeometry();
};

