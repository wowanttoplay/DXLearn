#include "BlurFilter.h"

#include "../../../Common/D3dUtil.h"

BlurFilter::BlurFilter(ID3D12Device* device, UINT32 width, UINT32 height, DXGI_FORMAT format)
    : mD3dDevice(device), mWidth(width), mHeight(height),
    mFormat(format)
{
    BuildResource();
}

ID3D12Resource* BlurFilter::Output()
{
    return mBlurMap[0].Get();
}

void BlurFilter::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
    CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor, UINT8 descriptorSize)
{
    mBlurCpuSrv[0] = hCpuDescriptor;
    mBlurCpuUav[0] = hCpuDescriptor.Offset(1, descriptorSize);
    mBlurCpuSrv[1] = hCpuDescriptor.Offset(1, descriptorSize);
    mBlurCpuUav[1] = hCpuDescriptor.Offset(1, descriptorSize);

    mBlurGpuSrv[0] = hGpuDescriptor;
    mBlurGpuUav[0] = hGpuDescriptor.Offset(1, descriptorSize);
    mBlurGpuSrv[1] = hGpuDescriptor.Offset(1, descriptorSize);
    mBlurGpuUav[1] = hGpuDescriptor.Offset(1, descriptorSize);

    BuildDescriptors();
}

void BlurFilter::OnResize(UINT32 width, UINT32 height)
{
    if (mWidth != width || mHeight != height)
    {
        mWidth = width;
        mHeight = height;

        BuildResource();

        BuildDescriptors();
    }
}

void BlurFilter::Execute(ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSig,
    ID3D12PipelineState* horzBlurPSO, ID3D12PipelineState* vertBlurPSO, ID3D12Resource* input, int blurCount)
{
    auto weights = CalcGaussWeights(2.5f);
    int blurRadius = (int)weights.size() / 2;

    cmdList->SetComputeRootSignature(rootSig);
    cmdList->SetComputeRoot32BitConstants(0, 1, &blurRadius, 0);
    cmdList->SetComputeRoot32BitConstants(0, (UINT32)weights.size(), weights.data(), 1);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap[0].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

    cmdList->CopyResource(mBlurMap[0].Get(), input);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap[0].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition( mBlurMap[1].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    for (int i = 0; i < blurCount; ++i)
    {
        /**
         * Horizontal Blur pass
         */
        cmdList->SetPipelineState(horzBlurPSO);
        cmdList->SetComputeRootDescriptorTable(1, mBlurGpuSrv[0]);
        cmdList->SetComputeRootDescriptorTable(2, mBlurGpuUav[0]);
        UINT32 numGroupX = (UINT32)ceilf(mWidth / 256.0f);
        cmdList->Dispatch(numGroupX, mHeight, 1);

        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap[0].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

        /**
         *  Vertical Blur pass
         */
        cmdList->SetPipelineState(vertBlurPSO);
        cmdList->SetComputeRootDescriptorTable(1, mBlurGpuSrv[1]);
        cmdList->SetComputeRootDescriptorTable(2, mBlurGpuUav[1]);

        UINT32 numGroupsY = (UINT32)ceilf(mHeight) / 256.0f;
        cmdList->Dispatch(mWidth, numGroupsY, 1);
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap[1].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    }
}

std::vector<float> BlurFilter::CalcGaussWeights(float sigma)
{
    float twoSigma2 = 2.0f*sigma*sigma;

    // Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
    // For example, for sigma = 3, the width of the bell curve is 
    int blurRadius = (int)ceil(2.0f * sigma);

    assert(blurRadius <= MaxBlurRadius);

    std::vector<float> weights;
    weights.resize(2 * blurRadius + 1);
	
    float weightSum = 0.0f;

    for(int i = -blurRadius; i <= blurRadius; ++i)
    {
        float x = (float)i;

        weights[i+blurRadius] = expf(-x*x / twoSigma2);

        weightSum += weights[i+blurRadius];
    }

    // Divide by the sum so all the weights add up to 1.0.
    for(int i = 0; i < weights.size(); ++i)
    {
        weights[i] /= weightSum;
    }

    return weights;
}

void BlurFilter::BuildDescriptors()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = mFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    mD3dDevice->CreateShaderResourceView(mBlurMap[0].Get(), &srvDesc, mBlurCpuSrv[0]);
    mD3dDevice->CreateUnorderedAccessView(mBlurMap[0].Get(), nullptr, &uavDesc, mBlurCpuUav[0]);

    mD3dDevice->CreateShaderResourceView(mBlurMap[1].Get(), &srvDesc, mBlurCpuSrv[1]);
    mD3dDevice->CreateUnorderedAccessView(mBlurMap[1].Get(), nullptr, &uavDesc, mBlurCpuUav[1]);
}

void BlurFilter::BuildResource()
{
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = mWidth;
    texDesc.Height = mHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = mFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    ThrowIfFailed(mD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&mBlurMap[0])
    ));

    ThrowIfFailed(mD3dDevice->CreateCommittedResource(
       &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
       D3D12_HEAP_FLAG_NONE,
       &texDesc,
       D3D12_RESOURCE_STATE_COMMON,
       nullptr,
       IID_PPV_ARGS(&mBlurMap[1])
   ));
}
