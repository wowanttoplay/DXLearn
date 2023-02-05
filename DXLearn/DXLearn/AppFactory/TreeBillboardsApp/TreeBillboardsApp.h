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

protected:
    
    void BuildTextures() override;
    void BuildDescriptorHeaps() override;
};

