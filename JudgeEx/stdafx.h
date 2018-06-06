
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions


#include <afxdisp.h>        // MFC Automation classes



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars

#define _EXPORT_TO_MYSQL

#define WM_WEBINFO_IN (WM_USER + 10)
#define WM_WEBINFO_RECREATE (WM_USER + 11)

#define WM_STEP (WM_USER+100)

#define WM_BEGIN  (WM_USER+110)
#define WM_FINISH (WM_USER+111)


#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


#include <time.h>
#include <vector>
#include <map>
using namespace std;

typedef signed char  int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

extern int year;
extern int month;
extern int day;

extern int CurrentYear;
extern int CurrentMonth;
extern int CurrentDay;

extern CString				g_strNewFileName;

extern CString				g_strtpBFolder;
extern CString				g_strtpBFileName;
extern CString				g_strModuleFolder;

extern uint64_t				g_FileID;

extern CString				g_strRailNo;
extern CString				g_strRailName;
extern unsigned char		g_xingbie;
extern unsigned char		g_gubie;
extern CString				g_strXingbie;
extern CString				g_strGubie;
extern CString				g_juNo;
extern CString				g_strGwdNo;
extern double				g_startPos;
extern double				g_endPos;
extern bool					g_direction;

extern char ChannelNames[13];
extern CString ChannelNamesB[16];

extern map<uint8_t, char*> g_strGuBieDefines;

extern map<uint8_t, char*> g_strXingBieDefines;

extern map<uint16_t, char*> g_strTypeDefines;

extern map<uint16_t, char*> g_strDegreeDefines;

extern map<uint16_t, char*> g_strWoundPlaceDefines;

extern map<uint8_t, char*> g_strCheckStateDefines;

bool ParseGPS(unsigned char* strGPS, double& log, double&lat);

void PrintTime(tm& time, char* msg);

unsigned char	INT8ChangeToBCD(unsigned char srcNum);

unsigned short	INT16ChangeToBCD(unsigned short srcNum);

unsigned int	INT32ChangeToBCD(unsigned int srcNum);

unsigned char	BCDToINT8(unsigned char bcd);

unsigned short	BCDToINT16(unsigned short bcd);

unsigned int	BCDToINT32(unsigned int bcd);

void			CStringToArray(CString str, char splitter, uint16_t* data, int count);

void			ArrayToCString(uint16_t* data, int count, CString str, char splitter);

static int isendtime = 0;