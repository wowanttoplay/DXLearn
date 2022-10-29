#pragma once
#include "../../Common/D3dApp.h"

class BoxApp : public D3dApp
{
public:
    explicit BoxApp(HINSTANCE hInsatnce)
        : D3dApp(hInsatnce)
    {
    }

    BoxApp(const BoxApp& other) = delete;
    BoxApp& operator=(const BoxApp& other) = delete;
    virtual ~BoxApp() = default;

private:
    virtual bool Initialize() override;
    
    virtual void Update(const GameTimer& InGameTime) override;
    virtual void Draw(const GameTimer& InGameTime) override;
    virtual void OnResize() override;

    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
    virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
    void BuildBoxGeometry();

private:
    std::unique_ptr<MeshGeometry> mBoxGeo;
    
};