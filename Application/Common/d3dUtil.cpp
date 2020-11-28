/*
*  ʵ�ô��빤����ʵ��
*  d3dUtil.cpp
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

// DxException���Ա����ToString�Ķ���
std::wstring DxException::ToString()const
{
    // ��ô��������ַ�������
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}