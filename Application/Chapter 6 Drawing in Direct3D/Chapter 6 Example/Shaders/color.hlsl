/*
*  第6章演示程序着色器
*  color.hlsl
*  作者： Frank Luna (C)
*/

cbuffer cbPerOject : register(b0)
{
    float4x4 gWorldViewProj;
    //float4 gPulseColor;  // 习题16
    //float gTime;         // 习题6、14、16
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
    
    // 习题6添加代码
    //vin.PosL.xy += 0.5f * sin(vin.PosL.x) * sin(3.0f * gTime);
    //vin.PosL.z *= 0.6f + 0.4f * sin(2.0f * gTime);
    
    // 将顶点由局部空间变换到齐次裁剪空间
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    
    // 直接将顶点颜色传至像素着色器   
    // 习题14
    //vin.Color.x += 0.5f * sin(vin.PosL.x) * sin(3.0f * gTime);
    //vin.Color.y += 0.5f * sin(vin.PosL.y) * sin(3.0f * gTime);
    //vin.Color.z *= 0.6f + 0.4f * sin(2.0f * gTime);
    vout.Color = vin.Color;
    
    return vout;
}

// 像素着色器
float4 PS(VertexOut pin) : SV_Target
{
    //clip(pin.Color.r - 0.5f);  // 习题15
    
    // 习题16
    //const float pi = 3.14159;
    
    //// 随着时间流逝，令正弦函数的值在[0,1]区间内周期性地变化
    //float s = 0.5f * sin(2 * gTime - 0.25f * pi) + 0.5f;
    
    //// 基于参数s在pin.color与gpulsecolor之间进行线性插值
    //float4 c = lerp(pin.Color, gPulseColor, s);
    
    //return c;
    return pin.Color;
}