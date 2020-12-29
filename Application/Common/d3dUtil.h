/*
*  ʵ�ô��빤����
*  d3dUtil.h
*  ���ߣ� Frank Luna (C)
*/

#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include <comdef.h>
#include "d3dx12.h"

// ת���ɿ��ַ����͵��ַ�����wstring
// Windowsƽ̨ʹ��wstring��wchar_t������UTF-16������ַ�������ʽ�����ַ���ǰ+L
inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

class d3dUtil
{
public:

    // �����ʵ�������
    // ʹ�������Ĵ�С����Ϊ256B��Ӳ����С����ռ䣩��������
    static UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        // �����������Ĵ�С������Ӳ����С����ռ䣨ͨ��Ϊ256B����������
        // ���Ҫ�������Ϊ����Ҫ�����С��256��������
        // ͨ��Ϊ����ֵbyteSize����255����������ͽ���ĵ�2�ֽ�
        // ������������С��256�����ݲ��֣���ʵ����һ��

        // ���磺����byteSize = 300
        // (300 + 255) & ~255
        // 555 & ~255
        // 0x022b & ~0x00ff
        // 0x022b & 0xff00
        // 0x0200
        // 512

        return (byteSize + 255) & ~255;
    }

    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

    static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
        const std::wstring& filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& entrypoint,
        const std::string& target);
};

// ��鷵�ص�HRESULTֵ�����ʧ���׳��쳣
// ��ʾ����Ĵ����롢���������ļ����Լ��к�
class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

    std::wstring ToString()const;

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring Filename;
    int LineNumber = -1;
};

// ��SubmeshGeometry������MeshGeometry�д洢�ĵ���������
struct SubmeshGeometry
{
    UINT IndexCount = 0;          // ������Ԫ�أ�����
    UINT StartIndexLocation = 0;  // ��ȫ�֣�������������Ԥ��ȡ����ʼ����
    INT BaseVertexLocation = 0;   // ��׼�����ַ

    // ͨ���������������嵱ǰSubmeshGeometry�ṹ�������漸����İ�Χ��
    DirectX::BoundingBox Bounds;
};

// MeshGeometry�����ڽ�������������ݴ���һ�����㻺������һ�����������������
// ���ṩ�˶Դ��ڶ��㻺�����������������еĵ�����������л�����������ݺ�ƫ����
struct MeshGeometry
{
    // ָ�����������񼯺ϵ�����
    std::string Name;

    // ϵͳ�ڴ��еĸ���
    // ���ڶ���/���������Ƿ��͸�ʽ����Blob���ͱ�ʾ�����û���ʹ��ʱ�ٽ���ת��Ϊ�ʵ�������
    Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

    // ��Ӧ������GPU��Դ
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> VPosDataBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> VColorDataBufferGPU = nullptr;

    // ��Ӧ�ϴ�������
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

    // �������
    UINT VertexByteStride = 0;                       // ÿ������Ԫ����ռ�õ��ֽ���
    UINT VertexBufferByteSize = 0;                   // ���㻺������С���ֽڣ�
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;  // ����Ԫ�ظ�ʽ
    UINT IndexBufferByteSize = 0;                    // ������������С���ֽڣ�

    // һ��MeshGeometry�ṹ���ܹ��洢һ�鶥��/�����������еĶ��������
    // �������������������������񼸺��壬���Ǿ��ܵ����ػ��Ƴ����е������񣨵��������壩
    // ͨ�������ҵ���Ӧ�����񼸺���
    std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

    // ���㻺������ͼ
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
        vbv.StrideInBytes = VertexByteStride;
        vbv.SizeInBytes = VertexBufferByteSize;

        return vbv;
    }

    // ������������ͼ
    D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
        ibv.Format = IndexFormat;
        ibv.SizeInBytes = IndexBufferByteSize;

        return ibv;
    }

    // �������ϴ�GPU�󣬾Ϳ����ͷ��ڴ�
    void DisposeUploaders()
    {
        VertexBufferUploader = nullptr;
        IndexBufferUploader = nullptr;
    }
};

// ThrowIfFailed�궨��
// ����x�ĵ��ý����hr�洢��֮��ת��ΪUnicode�ַ�������������Ϣ�������Ϣ����
#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

// ReleaseCom�궨��
// ����Release������COM��������ü���Ϊ0ʱ�������ͷ�ռ�õ��ڴ�
#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

// �����6��ϰ��2����
struct MeshGeometryForExiercise6_2
{
    // ָ�����������񼯺ϵ�����
    std::string Name;

    // ϵͳ�ڴ��еĸ���
    // ���ڶ���/���������Ƿ��͸�ʽ����Blob���ͱ�ʾ�����û���ʹ��ʱ�ٽ���ת��Ϊ�ʵ�������
    Microsoft::WRL::ComPtr<ID3DBlob> VPosDataBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> VColorDataBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

    // ��Ӧ������GPU��Դ
    Microsoft::WRL::ComPtr<ID3D12Resource> VPosDataBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> VColorDataBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

    // ��Ӧ�ϴ�������
    Microsoft::WRL::ComPtr<ID3D12Resource> VPosDataUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> VColorDataUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

    // �������
    UINT VPosDataByteStride = 0;                       // ÿ������λ��Ԫ����ռ�õ��ֽ���
    UINT VPosDataBufferByteSize = 0;                   // ����λ�û�������С���ֽڣ�
    UINT VColorDataByteStride = 0;                     // ÿ��������ɫԪ����ռ�õ��ֽ���
    UINT VColorDataBufferByteSize = 0;                 // ������ɫ��������С���ֽڣ�
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;    // ����Ԫ�ظ�ʽ
    UINT IndexBufferByteSize = 0;                      // ������������С���ֽڣ�

    // һ��MeshGeometry�ṹ���ܹ��洢һ�鶥��/�����������еĶ��������
    // �������������������������񼸺��壬���Ǿ��ܵ����ػ��Ƴ����е������񣨵��������壩
    // ͨ�������ҵ���Ӧ�����񼸺���
    std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

    // ����λ�û�������ͼ
    D3D12_VERTEX_BUFFER_VIEW VPosDataBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vposbv;
        vposbv.BufferLocation = VPosDataBufferGPU->GetGPUVirtualAddress();
        vposbv.StrideInBytes = VPosDataByteStride;
        vposbv.SizeInBytes = VPosDataBufferByteSize;

        return vposbv;
    }

    // ������ɫ��������ͼ
    D3D12_VERTEX_BUFFER_VIEW VColorDataBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vcolorbv;
        vcolorbv.BufferLocation = VColorDataBufferGPU->GetGPUVirtualAddress();
        vcolorbv.StrideInBytes = VColorDataByteStride;
        vcolorbv.SizeInBytes = VColorDataBufferByteSize;

        return vcolorbv;
    }

    // ������������ͼ
    D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
        ibv.Format = IndexFormat;
        ibv.SizeInBytes = IndexBufferByteSize;

        return ibv;
    }

    // �������ϴ�GPU�󣬾Ϳ����ͷ��ڴ�
    void DisposeUploaders()
    {
        VPosDataUploader = nullptr;
        VColorDataUploader = nullptr;
        IndexBufferUploader = nullptr;
    }
};