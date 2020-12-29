/*
*  实用代码工具类
*  d3dUtil.h
*  作者： Frank Luna (C)
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

// 转换成宽字符类型的字符串，wstring
// Windows平台使用wstring和wchar_t，用于UTF-16编码的字符，处理方式是在字符串前+L
inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

class d3dUtil
{
public:

    // 进行适当的运算
    // 使缓冲区的大小凑整为256B（硬件最小分配空间）的整数倍
    static UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        // 常量缓冲区的大小必须是硬件最小分配空间（通常为256B）的整数倍
        // 因此要将其凑整为满足要求的最小的256的整数倍
        // 通过为输入值byteSize加上255，再屏蔽求和结果的低2字节
        // （即计算结果中小于256的数据部分）来实现这一点

        // 例如：假设byteSize = 300
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

// 检查返回的HRESULT值，检查失败抛出异常
// 显示出错的错误码、函数名、文件名以及行号
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

// 用SubmeshGeometry来定义MeshGeometry中存储的单个几何体
struct SubmeshGeometry
{
    UINT IndexCount = 0;          // 索引（元素）数量
    UINT StartIndexLocation = 0;  // （全局）索引缓冲区中预读取的起始索引
    INT BaseVertexLocation = 0;   // 基准顶点地址

    // 通过此子网格来定义当前SubmeshGeometry结构体中所存几何体的包围盒
    DirectX::BoundingBox Bounds;
};

// MeshGeometry适用于将多个几何体数据存于一个顶点缓冲区和一个索引缓冲区的情况
// 它提供了对存于顶点缓冲区和索引缓冲区中的单个几何体进行绘制所需的数据和偏移量
struct MeshGeometry
{
    // 指定几何体网格集合的名称
    std::string Name;

    // 系统内存中的副本
    // 由于顶点/索引可以是泛型格式，用Blob类型表示，待用户在使用时再将其转换为适当的类型
    Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

    // 对应缓冲区GPU资源
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> VPosDataBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> VColorDataBufferGPU = nullptr;

    // 对应上传缓冲区
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

    // 相关数据
    UINT VertexByteStride = 0;                       // 每个顶点元素所占用的字节数
    UINT VertexBufferByteSize = 0;                   // 顶点缓冲区大小（字节）
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;  // 索引元素格式
    UINT IndexBufferByteSize = 0;                    // 索引缓冲区大小（字节）

    // 一个MeshGeometry结构体能够存储一组顶点/索引缓冲区中的多个几何体
    // 若利用下列容器来定义子网格几何体，我们就能单独地绘制出其中的子网格（单个几何体）
    // 通过名字找到相应子网格几何体
    std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

    // 顶点缓冲区视图
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
        vbv.StrideInBytes = VertexByteStride;
        vbv.SizeInBytes = VertexBufferByteSize;

        return vbv;
    }

    // 索引缓冲区视图
    D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
        ibv.Format = IndexFormat;
        ibv.SizeInBytes = IndexBufferByteSize;

        return ibv;
    }

    // 待数据上传GPU后，就可以释放内存
    void DisposeUploaders()
    {
        VertexBufferUploader = nullptr;
        IndexBufferUploader = nullptr;
    }
};

// ThrowIfFailed宏定义
// 对于x的调用结果用hr存储，之后转变为Unicode字符串，将错误信息输出到消息框中
#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

// ReleaseCom宏定义
// 调用Release方法，COM对象的引用计数为0时，自行释放占用的内存
#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

// 解决第6章习题2所用
struct MeshGeometryForExiercise6_2
{
    // 指定几何体网格集合的名称
    std::string Name;

    // 系统内存中的副本
    // 由于顶点/索引可以是泛型格式，用Blob类型表示，待用户在使用时再将其转换为适当的类型
    Microsoft::WRL::ComPtr<ID3DBlob> VPosDataBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> VColorDataBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

    // 对应缓冲区GPU资源
    Microsoft::WRL::ComPtr<ID3D12Resource> VPosDataBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> VColorDataBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

    // 对应上传缓冲区
    Microsoft::WRL::ComPtr<ID3D12Resource> VPosDataUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> VColorDataUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

    // 相关数据
    UINT VPosDataByteStride = 0;                       // 每个顶点位置元素所占用的字节数
    UINT VPosDataBufferByteSize = 0;                   // 顶点位置缓冲区大小（字节）
    UINT VColorDataByteStride = 0;                     // 每个顶点颜色元素所占用的字节数
    UINT VColorDataBufferByteSize = 0;                 // 顶点颜色缓冲区大小（字节）
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;    // 索引元素格式
    UINT IndexBufferByteSize = 0;                      // 索引缓冲区大小（字节）

    // 一个MeshGeometry结构体能够存储一组顶点/索引缓冲区中的多个几何体
    // 若利用下列容器来定义子网格几何体，我们就能单独地绘制出其中的子网格（单个几何体）
    // 通过名字找到相应子网格几何体
    std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

    // 顶点位置缓冲区视图
    D3D12_VERTEX_BUFFER_VIEW VPosDataBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vposbv;
        vposbv.BufferLocation = VPosDataBufferGPU->GetGPUVirtualAddress();
        vposbv.StrideInBytes = VPosDataByteStride;
        vposbv.SizeInBytes = VPosDataBufferByteSize;

        return vposbv;
    }

    // 顶点颜色缓冲区视图
    D3D12_VERTEX_BUFFER_VIEW VColorDataBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vcolorbv;
        vcolorbv.BufferLocation = VColorDataBufferGPU->GetGPUVirtualAddress();
        vcolorbv.StrideInBytes = VColorDataByteStride;
        vcolorbv.SizeInBytes = VColorDataBufferByteSize;

        return vcolorbv;
    }

    // 索引缓冲区视图
    D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
        ibv.Format = IndexFormat;
        ibv.SizeInBytes = IndexBufferByteSize;

        return ibv;
    }

    // 待数据上传GPU后，就可以释放内存
    void DisposeUploaders()
    {
        VPosDataUploader = nullptr;
        VColorDataUploader = nullptr;
        IndexBufferUploader = nullptr;
    }
};