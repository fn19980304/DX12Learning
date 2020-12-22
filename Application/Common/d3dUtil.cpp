/*
*  实用代码工具类实现
*  d3dUtil.cpp
*  作者： Frank Luna (C)
*/

#include "d3dUtil.h"
#include <comdef.h>

using Microsoft::WRL::ComPtr;

// DxException类构造函数定义，利用初始化列表来初始化字段
DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

// 创建默认缓冲区
// 默认缓冲区为用D3D12_HEAP_TYPE_DEFAULT类型创建的缓冲区
// 用于存放静态几何体，优化性能
Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;  // 最终得到的默认缓冲区
    
    // 创建实际的默认缓冲区资源
    // CD3DX12_RESOURCE_DESC类提供了多种便于使用的构造函数以及方法
    // 这里使用了其提供的简化缓冲区描述过程的D3D12_RESOURCE_DESC的构造函数
    // 参数为width，即缓冲区所占的字节数
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),          // 简化方法
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // 创建处于中介位置，帮助CPU将内存中的数据复制到默认缓冲区的上传堆
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),  // 堆类型为上传堆
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

    // 描述我们想要复制到默认缓冲区中的数据
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;     // 指向有初始化缓冲区所用数据的内存块指针
    subResourceData.RowPitch = byteSize;  // 欲复制数据的字节数
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // 将数据复制到默认缓冲区资源的流程
    
    // Step1:转换资源状态（初始状态 -> 复制目标状态（官方文档定义为复制中被写入的状态））
    cmdList->ResourceBarrier(
        1, 
        &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    // Step2:调用UpdateSubresources辅助函数将数据从CPU端的内存中复制到上传堆
    UpdateSubresources<1>(
        cmdList, 
        defaultBuffer.Get(), 
        uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    // Step3:转换资源状态（复制目标状态 -> 始读状态（官方文档定义为上传堆使用前的起始状态））
    cmdList->ResourceBarrier(
        1, 
        &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
    // Step4:调用ID3D12CommandList::CopySubresourceRegion函数把上传堆内的数据复制到mBuffer中（实际程序中实现）

    // 注意：待调用者得知复制完成的消息后，才能释放uploadBuffer
    return defaultBuffer;
}

// 编译着色器
ComPtr<ID3DBlob> d3dUtil::CompileShader(
    const std::wstring& filename,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
    const std::string& target)
{
    // 若处于调试模式，则使用调试标志
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = S_OK;

    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(
        filename.c_str(),    // 希望编译的以.hlsl作为扩展名的HLSL源代码文件
        defines,            
        D3D_COMPILE_STANDARD_FILE_INCLUDE,  
        entrypoint.c_str(),  // 着色器入口点函数名
        target.c_str(),      // 指定所用着色器类型和版本的字符串
        compileFlags,        // 指定应如何编译的标志
        0, 
        &byteCode,           // 存储编译好的着色器对象字节码
        &errors);            // 存储报错的字符串

    // 将错误信息输出到调试窗口
    // 在使用GetBufferPointer()获得的指针时需要将它转换为适当的类型
    if(errors!=nullptr)
        OutputDebugStringA((char*)errors->GetBufferPointer());  

    ThrowIfFailed(hr);

    return byteCode;
}

// DxException类成员函数ToString的定义
std::wstring DxException::ToString()const
{
    // 获得错误代码的字符串描述
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}