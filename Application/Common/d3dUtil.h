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

    // ����Ĭ�ϻ�����
    // Ĭ�ϻ�����Ϊ��D3D12_HEAP_TYPE_DEFAULT���ʹ����Ļ�����
    // ���ڴ�ž�̬�����壬�Ż�����
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);
};

// ת���ɿ��ַ����͵��ַ�����wstring
// Windowsƽ̨ʹ��wstring��wchar_t������UTF-16������ַ�������ʽ�����ַ���ǰ+L
inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

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
