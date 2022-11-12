#pragma once
#include "RenderItem.h"
#include "ShapesFrameResource.h"
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
    virtual void Update(const GameTimer& InGameTime) override;
    virtual void Draw(const GameTimer& InGameTime) override;

private:
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildMeshGeometry();
    void BuildRenderItems();
    void BuildFrameResource();
    void BuildDescriptorHeaps();
    void BuildContantBufferViews();
    void BuildPSO();
     

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSig;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mMeshGeometry;

    // List of all the render item
    std::vector<std::unique_ptr<RenderItem>> mAllRenderItems;
    std::vector<RenderItem*> mOpaqueRenderItems;
    std::vector<std::unique_ptr<ShapesFrameResource>> mFrameResources;
    int mCurrentFrameResourceIndex = 0;
    ShapesFrameResource* mCurrentFrameResource = nullptr;
    
    UINT mPassCBVOffset = 0; // pass constant buffer view offset in descriptor heaps
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;

    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

private:
    void OnKeyboardInput(const GameTimer& InGameTime);
    void UpdateObjectCBs(const GameTimer& InGameTime);
    void UpdateMainPassCB(const GameTimer& InGamTime);
private:
    bool mIsWireframe = false;
    ShapesPassContants mMainPassCB;
};
