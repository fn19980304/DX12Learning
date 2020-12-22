/*
*  ʵ�ô��빤����ʵ��
*  d3dUtil.cpp
*  ���ߣ� Frank Luna (C)
*/

#include "d3dUtil.h"
#include <comdef.h>

using Microsoft::WRL::ComPtr;

// DxException�๹�캯�����壬���ó�ʼ���б�����ʼ���ֶ�
DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

// ����Ĭ�ϻ�����
// Ĭ�ϻ�����Ϊ��D3D12_HEAP_TYPE_DEFAULT���ʹ����Ļ�����
// ���ڴ�ž�̬�����壬�Ż�����
Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;  // ���յõ���Ĭ�ϻ�����
    
    // ����ʵ�ʵ�Ĭ�ϻ�������Դ
    // CD3DX12_RESOURCE_DESC���ṩ�˶��ֱ���ʹ�õĹ��캯���Լ�����
    // ����ʹ�������ṩ�ļ򻯻������������̵�D3D12_RESOURCE_DESC�Ĺ��캯��
    // ����Ϊwidth������������ռ���ֽ���
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),          // �򻯷���
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // ���������н�λ�ã�����CPU���ڴ��е����ݸ��Ƶ�Ĭ�ϻ��������ϴ���
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),  // ������Ϊ�ϴ���
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

    // ����������Ҫ���Ƶ�Ĭ�ϻ������е�����
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;     // ָ���г�ʼ���������������ݵ��ڴ��ָ��
    subResourceData.RowPitch = byteSize;  // ���������ݵ��ֽ���
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // �����ݸ��Ƶ�Ĭ�ϻ�������Դ������
    
    // Step1:ת����Դ״̬����ʼ״̬ -> ����Ŀ��״̬���ٷ��ĵ�����Ϊ�����б�д���״̬����
    cmdList->ResourceBarrier(
        1, 
        &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    // Step2:����UpdateSubresources�������������ݴ�CPU�˵��ڴ��и��Ƶ��ϴ���
    UpdateSubresources<1>(
        cmdList, 
        defaultBuffer.Get(), 
        uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    // Step3:ת����Դ״̬������Ŀ��״̬ -> ʼ��״̬���ٷ��ĵ�����Ϊ�ϴ���ʹ��ǰ����ʼ״̬����
    cmdList->ResourceBarrier(
        1, 
        &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
    // Step4:����ID3D12CommandList::CopySubresourceRegion�������ϴ����ڵ����ݸ��Ƶ�mBuffer�У�ʵ�ʳ�����ʵ�֣�

    // ע�⣺�������ߵ�֪������ɵ���Ϣ�󣬲����ͷ�uploadBuffer
    return defaultBuffer;
}

// ������ɫ��
ComPtr<ID3DBlob> d3dUtil::CompileShader(
    const std::wstring& filename,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
    const std::string& target)
{
    // �����ڵ���ģʽ����ʹ�õ��Ա�־
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = S_OK;

    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(
        filename.c_str(),    // ϣ���������.hlsl��Ϊ��չ����HLSLԴ�����ļ�
        defines,            
        D3D_COMPILE_STANDARD_FILE_INCLUDE,  
        entrypoint.c_str(),  // ��ɫ����ڵ㺯����
        target.c_str(),      // ָ��������ɫ�����ͺͰ汾���ַ���
        compileFlags,        // ָ��Ӧ��α���ı�־
        0, 
        &byteCode,           // �洢����õ���ɫ�������ֽ���
        &errors);            // �洢������ַ���

    // ��������Ϣ��������Դ���
    // ��ʹ��GetBufferPointer()��õ�ָ��ʱ��Ҫ����ת��Ϊ�ʵ�������
    if(errors!=nullptr)
        OutputDebugStringA((char*)errors->GetBufferPointer());  

    ThrowIfFailed(hr);

    return byteCode;
}

// DxException���Ա����ToString�Ķ���
std::wstring DxException::ToString()const
{
    // ��ô��������ַ�������
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}