#pragma once
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
};
