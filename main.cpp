#pragma comment(lib, "detours.lib")
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <wchar.h>
#include <detours.h>
#include <shlwapi.h>
#include "mssign32.h"

#pragma comment(lib, "shlwapi.lib")

HMODULE hModCrypt32 = NULL, hModMssign32 = NULL, hModKernel32 = NULL;
using fntCertVerifyTimeValidity = decltype(CertVerifyTimeValidity);
using fntSignerSign = decltype(SignerSign);
using fntSignerTimeStamp = decltype(SignerTimeStamp);
using fntSignerTimeStampEx2 = decltype(SignerTimeStampEx2);
using fntSignerTimeStampEx3 = decltype(SignerTimeStampEx3);
using fntGetLocalTime = decltype(GetLocalTime);
fntCertVerifyTimeValidity* pOldCertVerifyTimeValidity = NULL;
fntSignerSign* pOldSignerSign = NULL;
fntSignerTimeStamp* pOldSignerTimeStamp = NULL;
fntSignerTimeStampEx2* pOldSignerTimeStampEx2 = NULL;
fntSignerTimeStampEx3* pOldSignerTimeStampEx3 = NULL;
fntGetLocalTime* pOldGetLocalTime = NULL;

int year = -1, month = -1, day = -1, hour = -1, minute = -1, second = -1;
WCHAR lpTimestamp[20];
wchar_t swzServerUrl[260];

LPCWSTR ReplaceTimeStamp(LPCWSTR lpOriginalTS) {
    if (!lpOriginalTS)
        return NULL;
    LPWSTR buf = new WCHAR[65];
    memset(buf, 0, sizeof(WCHAR) * 65);
    if (!_wcsicmp(lpOriginalTS, L"{CustomTimestampMarker-SHA1}")) {
        //wcscat(buf, L"http://timestamp.pki.jemmylovejenny.tk/SHA1/");
        wcscat(buf, swzServerUrl);
        wcscat(buf, lpTimestamp);
        return buf;
    }
    else if (!_wcsicmp(lpOriginalTS, L"{CustomTimestampMarker-SHA256}")) {
        //wcscat(buf, L"http://timestamp.pki.jemmylovejenny.tk/SHA256/");
        wcscat(buf,swzServerUrl);
        wcscat(buf, lpTimestamp);
        return buf;
    }
    else {
        return lpOriginalTS;
    }
}
LONG WINAPI NewCertVerifyTimeValidity(
    LPFILETIME pTimeToVerify,
    PCERT_INFO pCertInfo
)
{
    return 0;
}
HRESULT WINAPI NewSignerSign(
    _In_     SIGNER_SUBJECT_INFO* pSubjectInfo,
    _In_     SIGNER_CERT* pSignerCert,
    _In_     SIGNER_SIGNATURE_INFO* pSignatureInfo,
    _In_opt_ SIGNER_PROVIDER_INFO* pProviderInfo,
    _In_opt_ LPCWSTR               pwszHttpTimeStamp,
    _In_opt_ PCRYPT_ATTRIBUTES     psRequest,
    _In_opt_ LPVOID                pSipData
)
{
    return (*pOldSignerSign)(pSubjectInfo, pSignerCert, pSignatureInfo, pProviderInfo, ReplaceTimeStamp(pwszHttpTimeStamp), psRequest, pSipData);
}
HRESULT WINAPI NewSignerTimeStamp(
    _In_     SIGNER_SUBJECT_INFO* pSubjectInfo,
    _In_     LPCWSTR             pwszHttpTimeStamp,
    _In_opt_ PCRYPT_ATTRIBUTES   psRequest,
    _In_opt_ LPVOID              pSipData
)
{
    return (*pOldSignerTimeStamp)(pSubjectInfo, ReplaceTimeStamp(pwszHttpTimeStamp), psRequest, pSipData);
}
HRESULT WINAPI NewSignerTimeStampEx2(
    _Reserved_ DWORD               dwFlags,
    _In_       SIGNER_SUBJECT_INFO* pSubjectInfo,
    _In_       LPCWSTR             pwszHttpTimeStamp,
    _In_       ALG_ID              dwAlgId,
    _In_       PCRYPT_ATTRIBUTES   psRequest,
    _In_       LPVOID              pSipData,
    _Out_      SIGNER_CONTEXT** ppSignerContext
)
{
    return (*pOldSignerTimeStampEx2)(dwFlags, pSubjectInfo, ReplaceTimeStamp(pwszHttpTimeStamp), dwAlgId, psRequest, pSipData, ppSignerContext);
}
HRESULT WINAPI NewSignerTimeStampEx3(
    _In_       DWORD                  dwFlags,
    _In_       DWORD                  dwIndex,
    _In_       SIGNER_SUBJECT_INFO* pSubjectInfo,
    _In_       PCWSTR                 pwszHttpTimeStamp,
    _In_       PCWSTR                 pszAlgorithmOid,
    _In_opt_   PCRYPT_ATTRIBUTES      psRequest,
    _In_opt_   PVOID                  pSipData,
    _Out_      SIGNER_CONTEXT** ppSignerContext,
    _In_opt_   PCERT_STRONG_SIGN_PARA pCryptoPolicy,
    _Reserved_ PVOID                  pReserved
)
{
    return (*pOldSignerTimeStampEx3)(dwFlags, dwIndex, pSubjectInfo, ReplaceTimeStamp(pwszHttpTimeStamp), pszAlgorithmOid, psRequest, pSipData, ppSignerContext, pCryptoPolicy, pReserved);
}
void WINAPI NewGetLocalTime(
    LPSYSTEMTIME lpSystemTime
)
{
    (*pOldGetLocalTime)(lpSystemTime);
    if (year >= 0)
        lpSystemTime->wYear = year;
    if (month >= 0)
        lpSystemTime->wMonth = month;
    if (day >= 0)
        lpSystemTime->wDay = day;
    if (hour >= 0)
        lpSystemTime->wHour = hour;
    if (minute >= 0)
        lpSystemTime->wMinute = minute;
    if (second >= 0)
        lpSystemTime->wSecond = second;
}

bool HookFunctions()
{
    if ((hModCrypt32 = LoadLibraryW(L"crypt32.dll")) == NULL
        || (hModMssign32 = LoadLibraryW(L"mssign32.dll")) == NULL
        || (hModKernel32 = LoadLibraryW(L"kernel32.dll")) == NULL)
        return false;

    if ((pOldCertVerifyTimeValidity = (fntCertVerifyTimeValidity*)GetProcAddress(hModCrypt32, "CertVerifyTimeValidity")) == NULL
        || (pOldSignerSign = (fntSignerSign*)GetProcAddress(hModMssign32, "SignerSign")) == NULL
        || (pOldSignerTimeStamp = (fntSignerTimeStamp*)GetProcAddress(hModMssign32, "SignerTimeStamp")) == NULL
        || (pOldSignerTimeStampEx2 = (fntSignerTimeStampEx2*)GetProcAddress(hModMssign32, "SignerTimeStampEx2")) == NULL
        || ((pOldSignerTimeStampEx3 = (fntSignerTimeStampEx3*)GetProcAddress(hModMssign32, "SignerTimeStampEx3")) == NULL && FALSE)
        /* SignerTimeStampEx3 does not exist in Windows 7 */
        || (pOldGetLocalTime = (fntGetLocalTime*)GetProcAddress(hModKernel32, "GetLocalTime")) == NULL)
        return false;

    if (DetourTransactionBegin() != NO_ERROR
        || DetourAttach(&(PVOID&)pOldCertVerifyTimeValidity, NewCertVerifyTimeValidity) != NO_ERROR
        || DetourAttach(&(PVOID&)pOldSignerSign, NewSignerSign) != NO_ERROR
        || DetourAttach(&(PVOID&)pOldSignerTimeStamp, NewSignerTimeStamp) != NO_ERROR
        || DetourAttach(&(PVOID&)pOldSignerTimeStampEx2, NewSignerTimeStampEx2) != NO_ERROR
        || (pOldSignerTimeStampEx3 != NULL ? DetourAttach(&(PVOID&)pOldSignerTimeStampEx3, NewSignerTimeStampEx3) != NO_ERROR : FALSE)
        /* SignerTimeStampEx3 does not exist in Windows 7 */
        || DetourAttach(&(PVOID&)pOldGetLocalTime, NewGetLocalTime) != NO_ERROR
        || DetourTransactionCommit() != NO_ERROR)
        return false;

    return true;
}
//bool ParseConfig(LPWSTR lpCommandLineConfig, LPWSTR lpCommandLineTimestamp)
//{
//    LPWSTR buf = new WCHAR[260];
//    memset(buf, 0, sizeof(WCHAR) * 260);
//
//    if (_wgetcwd(buf, 260) == NULL)
//        return false;
//    wcscat(buf, L"\\");
//
//    if (lpCommandLineConfig) {
//        if ((wcschr(lpCommandLineConfig, L':') - lpCommandLineConfig) == 1) {
//            memset(buf, 0, sizeof(WCHAR) * 260);
//            wsprintfW(buf, lpCommandLineConfig);
//        }
//        else {
//            wcscat(buf, lpCommandLineConfig);
//        }
//    }
//    else {
//        wcscat(buf, L"hook.ini");
//    }
//
//    year = GetPrivateProfileIntW(L"Time", L"Year", -1, buf);
//    month = GetPrivateProfileIntW(L"Time", L"Month", -1, buf);
//    day = GetPrivateProfileIntW(L"Time", L"Day", -1, buf);
//    hour = GetPrivateProfileIntW(L"Time", L"Hour", -1, buf);
//    minute = GetPrivateProfileIntW(L"Time", L"Minute", -1, buf);
//    second = GetPrivateProfileIntW(L"Time", L"Second", -1, buf);
//
//    if (lpCommandLineTimestamp)
//        wsprintfW(lpTimestamp, lpCommandLineTimestamp);
//    else
//        GetPrivateProfileStringW(L"Timestamp", L"Timestamp", NULL, lpTimestamp, 20, buf);
//    
//    return true;
//}

// ��ȡ�����ļ�·��
bool GetConfigFilePath(LPWSTR lpCommandLineConfig, LPWSTR configFilePath, size_t bufferSize) {
    // ���������д���������ļ�·��
    if (lpCommandLineConfig) {
        if ((wcschr(lpCommandLineConfig, L':') - lpCommandLineConfig) == 1) {
            // ����Ǿ���·����ֱ��ʹ��
            wcscpy_s(configFilePath, bufferSize, lpCommandLineConfig);
        }
        else {
            // ��������·����ƴ�ӵ���ǰ����Ŀ¼
            // ��ȡ��ǰ����Ŀ¼
            if (_wgetcwd(configFilePath, bufferSize) == NULL) {
                return false;
            }
            wcscat(configFilePath, L"\\");
            wcscat_s(configFilePath, bufferSize, lpCommandLineConfig);
        }
    }
    else {
        if (0 == ::GetModuleFileNameW(NULL, configFilePath, bufferSize)) {
            return false;
        }
        PathRemoveFileSpecW(configFilePath);
        // Ĭ�������ļ�
        wcscat_s(configFilePath, bufferSize, L"\\hook.ini");
    }
    // MessageBoxW(NULL, configFilePath, L"CWD", MB_OK);
    return true;
}

// ���������ļ�
bool ParseConfig(LPWSTR lpCommandLineConfig, LPWSTR lpCommandLineTimestamp, LPWSTR lpCommandLineServerUrl) {
    wchar_t configFilePath[MAX_PATH];
    if (!GetConfigFilePath(lpCommandLineConfig, configFilePath, MAX_PATH)) {
        return false;
    }

    // ��ȡʱ������
    year = GetPrivateProfileIntW(L"Time", L"Year", -1, configFilePath);
    month = GetPrivateProfileIntW(L"Time", L"Month", -1, configFilePath);
    day = GetPrivateProfileIntW(L"Time", L"Day", -1, configFilePath);
    hour = GetPrivateProfileIntW(L"Time", L"Hour", -1, configFilePath);
    minute = GetPrivateProfileIntW(L"Time", L"Minute", -1, configFilePath);
    second = GetPrivateProfileIntW(L"Time", L"Second", -1, configFilePath);

    // ��ȡʱ�����ʽ
    if (lpCommandLineTimestamp) {
        wcscpy_s(lpTimestamp, 20, lpCommandLineTimestamp);
    }
    else {
        GetPrivateProfileStringW(L"Timestamp", L"Timestamp", L"", lpTimestamp, 20, configFilePath);
    }

    // ��ȡ��������ַ
    if (lpCommandLineServerUrl) {
        wcscpy_s(swzServerUrl, lpCommandLineServerUrl);
    }
    else {
        GetPrivateProfileStringW(L"Server", L"server_url", L"", swzServerUrl, 260, configFilePath);
    }

    return true;
}
BOOL WINAPI DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD fdwReason,
    _In_ LPVOID lpvReserved
)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        LPWSTR* szArglist = NULL;
        int nArgs = 0;
        szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

        int iconfig = -1, its = -1, iurl = -1;

        for (int i = 0; i <= nArgs - 2; i++) {
            if (!wcscmp(szArglist[i], L"-config"))
                iconfig = i + 1;
            if (!wcscmp(szArglist[i], L"-ts"))
                its = i + 1;
            if (!wcscmp(szArglist[i], L"-server"))
                iurl = i + 1;
        }

        if (!ParseConfig(iconfig >= 0 ? szArglist[iconfig] : NULL, its >= 0 ? szArglist[its] : NULL, iurl >= 0 ? szArglist[iurl] : NULL)) {
            MessageBoxW(NULL, L"���ó�ʼ��ʧ�ܣ�����hook.ini�������в�����", L"��ʼ��ʧ��", MB_ICONERROR);
        }
        
        LocalFree(szArglist);

        if (!HookFunctions())
            MessageBoxW(NULL, L"���ִ����޷�Hookָ���ĺ���\r\n��رճ������ԣ�", L"Hookʧ��", MB_ICONERROR);
#ifdef DEBUG       
        MessageBoxW(NULL, lpTimestamp, L"�Զ���ʱ���Ϊ", MB_OK);
        MessageBoxW(NULL, swzServerUrl, L"ʱ���������Ϊ", MB_OK);
#endif
    }
    return 1;
}

extern "C" __declspec(dllexport) int attach()
{
    return 0;
}
