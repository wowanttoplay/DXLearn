#pragma once
#include "../../Common/D3dApp.h"

class LandAndWavesApp : public D3dApp
{
public:

    virtual bool Initialize() override;
    virtual void Draw(const GameTimer& InGameTime) override;
    virtual void Update(const GameTimer& InGameTime) override;
private:
    
    
};
