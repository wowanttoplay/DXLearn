#pragma once
#include "../Common/D3dApp.h"

class BaseApp : public D3dApp
{
public:
    explicit BaseApp(HINSTANCE hInsatnce)
        : D3dApp(hInsatnce)
    {
    }

    virtual void Update(const GameTimer& InGameTime) override;

    virtual void Draw(const GameTimer& InGameTime) override;
};
