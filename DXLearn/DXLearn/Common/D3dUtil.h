#pragma once
#include <string>
#include <windows.h>
#include <wrl.h>
#include <string>
#include <sstream>
#include <unordered_map>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <D3Dcompiler.h>
#include <DirectXCollision.h>


class D3dUtil
{
public:
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* data, uint64_t byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploaderBuffer);

    static UINT CalculateConstantBufferByteSize(UINT InByteSize);

    static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const std::wstring& fileName, const D3D_SHADER_MACRO* defines, const std::string& entryPoint, const std::string& target);
};

class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hresult, const std::wstring& functionName, const std::wstring& fileName, int lineNumber);

    std::wstring ToString() const;

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring FileName;
    int LineNumber;
};

class SubMeshGeometry
{
public:
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    UINT BaseVertexLocation = 0;

    DirectX::BoundingBox Bounds;
};

class MeshGeometry
{
public:
    std::string Name;
    Microsoft::WRL::ComPtr<ID3D10Blob> VertexBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

    // Data about the buffers
    UINT VertexByteStride = 0;
    UINT VertexBufferByteSize = 0;
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
    UINT IndexBufferByteSize = 0;

    // A MeshGeometry may contain multiple geometry in one vertex/index buffer.individually
    // Use this container to define the submesh geometry so we can draw teh submesh individualy
    std::unordered_map<std::string, SubMeshGeometry> DrawArgs;

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
        vbv.SizeInBytes = VertexBufferByteSize;
        vbv.StrideInBytes = VertexByteStride;
        return vbv;
    }

    D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.Format = IndexFormat;
        ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
        ibv.SizeInBytes = IndexBufferByteSize;

        return ibv;
    }

    // we can free this memory after we finishe upload to the gpu
    void DisposeUploaders()
    {
        VertexBufferUploader = nullptr;
        IndexBufferUploader = nullptr;
    }
};

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
HRESULT hr__ = (x);                                               \
std::wstring wfn = AnsiToWString(__FILE__);                       \
if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif
