#pragma once
#include "../../Common/D3dApp.h"

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

private:
    
};
