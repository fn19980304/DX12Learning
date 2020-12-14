/*
*  ��6����ʾ������ɫ��
*  color.hlsl
*  ���ߣ� Frank Luna (C)
*/

cbuffer cbPerOject : register(b0)
{
    float4x4 gWorldViewProj;
};

// ����ǩ���ṹ��
struct VertexIn
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
};

// ���ǩ���ṹ��
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

// ������ɫ��
VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    // �������ɾֲ��ռ�任����βü��ռ�
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    
    // ֱ�ӽ�������ɫ����������ɫ��
    vout.Color = vin.Color;
    
    return vout;
}

// ������ɫ��
float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}