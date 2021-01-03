/*
*  ��6����ʾ������ɫ��
*  color.hlsl
*  ���ߣ� Frank Luna (C)
*/

cbuffer cbPerOject : register(b0)
{
    float4x4 gWorldViewProj;
    //float gTime;  // ϰ��6��14���ñ���
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
    
    // ϰ��6��Ӵ���
    //vin.PosL.xy += 0.5f * sin(vin.PosL.x) * sin(3.0f * gTime);
    //vin.PosL.z *= 0.6f + 0.4f * sin(2.0f * gTime);
    
    // �������ɾֲ��ռ�任����βü��ռ�
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    
    // ֱ�ӽ�������ɫ����������ɫ��   
    // ϰ��14
    //vin.Color.x += 0.5f * sin(vin.PosL.x) * sin(3.0f * gTime);
    //vin.Color.y += 0.5f * sin(vin.PosL.y) * sin(3.0f * gTime);
    //vin.Color.z *= 0.6f + 0.4f * sin(2.0f * gTime);
    vout.Color = vin.Color;
    
    return vout;
}

// ������ɫ��
float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}