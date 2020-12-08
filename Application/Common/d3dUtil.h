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

class d3dUtil
{
public:

    // 创建默认缓冲区
    // 默认缓冲区为用D3D12_HEAP_TYPE_DEFAULT类型创建的缓冲区
    // 用于存放静态几何体，优化性能
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);
};

// 转换成宽字符类型的字符串，wstring
// Windows平台使用wstring和wchar_t，用于UTF-16编码的字符，处理方式是在字符串前+L
inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

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
