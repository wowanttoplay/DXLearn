#pragma once
#include <d3d12.h>
#include <vector>
#include <wrl/client.h>

#include "../../../Common/d3dx12.h"

class BlurFilter
{
public:
    BlurFilter(ID3D12Device* device, UINT32 width, UINT32 height, DXGI_FORMAT format);

    BlurFilter(const BlurFilter& other) = delete;
    BlurFilter& operator=(const BlurFilter& other) = delete;
    ~BlurFilter() = default;

    ID3D12Resource* Output();

    void BuildDescriptors(
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
        CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
        UINT8 descriptorSize
    );

    void OnResize(UINT32 width, UINT32 height);

    void Execute(
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSig,
        ID3D12PipelineState* horzBlurPSO,
        ID3D12PipelineState* vertBlurPSO,
        ID3D12Resource* input,
        int blurCount);

private:
    std::vector<float> CalcGaussWeights(float sigma);

    void BuildDescriptors();
    void BuildResource();

private:
    const int MaxBlurRadius = 5;

    ID3D12Device* mD3dDevice = nullptr;

    UINT32 mWidth = 0;
    UINT32 mHeight = 0;
    DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mBlurCpuSrv[2];
    CD3DX12_CPU_DESCRIPTOR_HANDLE mBlurCpuUav[2];

    CD3DX12_GPU_DESCRIPTOR_HANDLE mBlurGpuSrv[2];
    CD3DX12_GPU_DESCRIPTOR_HANDLE mBlurGpuUav[2];

    // Two for ping-ponging the textures
    Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap[2];
};
