#pragma once
#include <map>

#include "../BlendApp/BlendApp.h"



class TreeBillboardsApp : public BlendApp
{
public:
    explicit TreeBillboardsApp(HINSTANCE hInsatnce);

    TreeBillboardsApp(const TreeBillboardsApp& other) = delete;
    TreeBillboardsApp& operator=(const TreeBillboardsApp& other) = delete;
    virtual ~TreeBillboardsApp() = default;
    void Draw(const GameTimer& InGameTime) override;

protected:
    
    void BuildTextures() override;
    void BuildDescriptorHeaps() override;
    void BuildMaterials() override;

    void BuildShadersAndInputLayout() override;

    void BuildGeometry() override;
    void BuildRenderItems() override;

    void BuildPSOs() override;
    


    std::map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> mInputLayouts;

protected:
    std::unique_ptr<MeshGeometry> BuildTreeSpriteGeometry();
};

