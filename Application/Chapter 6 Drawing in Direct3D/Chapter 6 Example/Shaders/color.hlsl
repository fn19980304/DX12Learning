/*
*  第6章演示程序着色器
*  color.hlsl
*  作者： Frank Luna (C)
*/

cbuffer cbPerOject : register(b0)
{
    float4x4 gWorldViewProj;
};

// 输入签名结构体
struct VertexIn
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
};

// 输出签名结构体
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

// 顶点着色器
VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    // 将顶点由局部空间变换到齐次裁剪空间
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    
    // 直接将顶点颜色传至像素着色器
    vout.Color = vin.Color;
    
    return vout;
}

// 像素着色器
float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}