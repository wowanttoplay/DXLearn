cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
};

struct VertexIn
{
    float3 Posl : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vIn)
{
    VertexOut vOut;
    float4 PosW = mul(float4(vIn.Posl, 1.0f), gWorld);
    vOut.PosH = mul(PosW, gViewProj);

    vOut.Color = vIn.Color;

    return vOut;
}

float4 PS(VertexOut pIn) : SV_Target
{
    return pIn.Color;
}
