#ifndef MACRO_H_INCLUDED
#define MACRO_H_INCLUDED

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

inline int MSGBOX_WARNING(const char* pszFormat, ...)
{
    char szText[1024];

    if (pszFormat == NULL)
        return 0;

    va_list aParam;
    va_start(aParam, pszFormat);
    vsprintf(szText, pszFormat, aParam);
    va_end(aParam);

    return MessageBox(NULL, szText, "WARNING", MB_OK | MB_ICONWARNING);
}

inline int MSGBOX_ERROR(const char* pszFormat, ...)
{
    if (pszFormat == NULL)
        return 0;

    char szText[1024];

    va_list aParam;
    va_start(aParam, pszFormat);
    vsprintf(szText, pszFormat, aParam);
    va_end(aParam);
    return MessageBox(NULL, szText, "ERROR", MB_OK | MB_ICONERROR);
}

inline void* MALLOC(size_t nSize)
{
    void* pMemory = malloc(nSize);
    if (pMemory == NULL)
        MSGBOX_ERROR("Memory allocation of %d bytes failed", nSize);

    return pMemory;
}

inline int LOG(const char* pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    return vprintf(pszFormat, args);
    va_end(args);
}

#endif // MACRO_H_INCLUDED
