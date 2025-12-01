// Algo SDK Sample.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Algo SDK Sample.h"
#include "NSK_Algo.h"
#include "thinkgear.h"
#include <stdio.h>
#include <string>
#include <WinUser.h>
#include <windowsx.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <time.h>

extern "C" {
	void *myMalloc(size_t size) {
		return malloc(size);
	}
};

void * operator new (size_t size) {
	return myMalloc(size);
}

#define MAX_LOADSTRING 100

#define NSK_ALGO_CDECL(ret, func, args)		typedef ret (__cdecl *func##_Dll) args; func##_Dll func##Addr = NULL; char func##Str[] = #func;

NSK_ALGO_CDECL(eNSK_ALGO_RET,	NSK_ALGO_Init,				(eNSK_ALGO_TYPE type, const NS_STR dataPath));
NSK_ALGO_CDECL(eNSK_ALGO_RET,	NSK_ALGO_Uninit,			(NS_VOID));
NSK_ALGO_CDECL(eNSK_ALGO_RET,	NSK_ALGO_RegisterCallback,	(NskAlgo_Callback cbFunc, NS_VOID *userData));
NSK_ALGO_CDECL(NS_STR,			NSK_ALGO_SdkVersion,		(NS_VOID));
NSK_ALGO_CDECL(NS_STR,			NSK_ALGO_AlgoVersion,		(eNSK_ALGO_TYPE type));
NSK_ALGO_CDECL(eNSK_ALGO_RET,	NSK_ALGO_Start,				(NS_BOOL bBaseline));
NSK_ALGO_CDECL(eNSK_ALGO_RET,	NSK_ALGO_Pause,				(NS_VOID));
NSK_ALGO_CDECL(eNSK_ALGO_RET,	NSK_ALGO_Stop,				(NS_VOID));
NSK_ALGO_CDECL(eNSK_ALGO_RET,	NSK_ALGO_DataStream,		(eNSK_ALGO_DATA_TYPE type, NS_INT16 *data, NS_INT dataLenght));

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HWND hInitBtn, hStartBtn, hStopBtn, hBulkDataBtn, hBlinkCheck, hAPCheck, hMECheck, hME2Check, hFCheck, hF2Check, hAttCheck, hMedCheck, hBPCheck, hStateText, hSignalQualityText, hVersionBtn, hOutputText;
HWND hWnd;

HWND hIntervalBtn, hSettingAPCheck, hSettingMECheck, hSettingME2Check, hSettingFCheck, hSettingF2Check, hIntervalSlider, hIntervalText;

int lSelectedAlgos = 0;  // 선택된 알고리즘 비트 플래그

long raw_data_count = 0;  // 오프라인 데이터 개수
short *raw_data = NULL;   // 오프라인 뇌파 데이터 버퍼
bool bRunning = false;    // 분석 실행 중 여부
bool bInited = false;     // SDK 초기화 완료 여부

#define MARGIN					10

#define STATE_TEXT_WIDTH		300
#define STATE_TEXT_HEIGHT		20

#define SIGNAL_TEXT_WIDTH		200
#define SIGNAL_TEXT_HEIGHT		20

#define BUTTON_WIDTH			100
#define BUTTON_HEIGHT			20

#define WM_USER					0x1000
#define WM_USER_SETTEXT			WM_USER

#define MWM_COM					"COM7"      // 헤드셋 연결 COM 포트

#define EEG_RAW_DATA			"ME_Easy_RawData.txt"  // 오프라인 테스트용 뇌파 데이터 파일

#ifdef _WIN64
#define ALGO_SDK_DLL			L"AlgoSdkDll64.dll"  // 64bit 환경용 DLL
#else
#define ALGO_SDK_DLL			L"AlgoSdkDll.dll"    // 32bit 환경용 DLL
#endif

char *comPortName = NULL;  // COM 포트 이름
int   dllVersion = 0;       // DLL 버전
int   connectionId = -1;    // 헤드셋 연결 ID
int   packetsRead = 0;      // 수신한 패킷 개수
int   errCode = 0;          // 에러 코드
DWORD dwThreadId = -1;      // 데이터 읽기 스레드 ID
HANDLE threadHandle = NULL; // 데이터 읽기 스레드 핸들
bool bConnectedHeadset = false;  // 헤드셋 연결 여부

static DWORD ThreadReadPacket(LPVOID lpdwThreadParam);
static void AlgoSdkCallback(sNSK_ALGO_CB_PARAM param);
static bool getFuncAddrs(HINSTANCE hinstLib, HWND hWnd);
static void *getFuncAddr(HINSTANCE hinstLib, HWND hwnd, char *funcName, bool *bError);
static void OutputLog(LPCWSTR log);
static void UIComponentsPositionUpdate(HWND hWnd);
static wchar_t *GetLocalTimeStr();
static void OutputToEditBox(LPCWSTR szBuf);


// 별도 스레드에서 헤드셋의 뇌파 데이터를 계속 수신하고 Algo SDK로 전송하는 함수
static DWORD ThreadReadPacket(LPVOID lpdwThreadParam) {
	int rawCount = 0;
	wchar_t buffer[100];
	short rawData[512] = { 0 };
	int lastRawCount = 0;

	while (true) {
		/* 1개 패킷 수신 */
		packetsRead = TG_ReadPackets(connectionId, 1);

		/* TG_ReadPackets() 호출이 성공하여 1개의 패킷을 수신한 경우... */
		
		if (packetsRead == 1) {
			/* 수신한 패킷에 새로운 뇌파(Raw Wave) 데이터가 포함된 경우... */
			if (TG_GetValueStatus(connectionId, TG_DATA_RAW) != 0) {
				/* 뇌파 데이터 값 추출 및 저장 */
				rawData[rawCount++] = (short)TG_GetValue(connectionId, TG_DATA_RAW);
				lastRawCount = rawCount;
				// 512개 샘플이 모이면 Algo SDK로 전송
				if (rawCount == 512) {
					(NSK_ALGO_DataStreamAddr)(NSK_ALGO_DATA_TYPE_EEG, rawData, rawCount);
					rawCount = 0;
				}
			}
			// 신호 품질(PQ) 데이터가 업데이트된 경우
			if (TG_GetValueStatus(connectionId, TG_DATA_POOR_SIGNAL) != 0) {
				short pq = (short)TG_GetValue(connectionId, TG_DATA_POOR_SIGNAL);
				SYSTEMTIME st;
				GetSystemTime(&st);
				swprintf(buffer, 100, L"%6d, PQ[%3d], [%d]", st.wSecond*1000 + st.wMilliseconds, pq, lastRawCount);
				rawCount = 0;
				OutputLog(buffer);
				// 신호 품질 정보를 Algo SDK로 전송
				(NSK_ALGO_DataStreamAddr)(NSK_ALGO_DATA_TYPE_PQ, &pq, 1);
			}
			// 주의도(Attention) 데이터가 업데이트된 경우
			if (TG_GetValueStatus(connectionId, TG_DATA_ATTENTION) != 0) {
				short att = (short)TG_GetValue(connectionId, TG_DATA_ATTENTION);
				// 주의도 정보를 Algo SDK로 전송
				(NSK_ALGO_DataStreamAddr)(NSK_ALGO_DATA_TYPE_ATT, &att, 1);
			}
			// 명상도(Meditation) 데이터가 업데이트된 경우
			if (TG_GetValueStatus(connectionId, TG_DATA_MEDITATION) != 0) {
				short med = (short)TG_GetValue(connectionId, TG_DATA_MEDITATION);
				// 명상도 정보를 Algo SDK로 전송
				(NSK_ALGO_DataStreamAddr)(NSK_ALGO_DATA_TYPE_MED, &med, 1);
			}
		}
		//Sleep(1);
	}
}

static void OutputLog(LPCWSTR log) {
	OutputDebugStringW(log);
	OutputDebugStringW(L"\r\n");
}

static void *getFuncAddr(HINSTANCE hinstLib, HWND hwnd, char *funcName, bool *bError) {
	void *funcPtr = (void*)GetProcAddress(hinstLib, funcName);
	*bError = true;
	if (NULL == funcPtr) {
		wchar_t szBuff[100] = { 0 };
		swprintf(szBuff, 100, L"Failed ot get %s function address", (wchar_t*)funcName);
		MessageBox(hwnd, szBuff, L"Error", MB_OK);
		*bError = false;
	}
	return funcPtr;
}

static bool getFuncAddrs(HINSTANCE hinstLib, HWND hWnd) {
	bool bError;

	NSK_ALGO_InitAddr = (NSK_ALGO_Init_Dll)getFuncAddr(hinstLib, hWnd, NSK_ALGO_InitStr, &bError);
	NSK_ALGO_UninitAddr = (NSK_ALGO_Uninit_Dll)getFuncAddr(hinstLib, hWnd, NSK_ALGO_UninitStr, &bError);
	NSK_ALGO_RegisterCallbackAddr = (NSK_ALGO_RegisterCallback_Dll)getFuncAddr(hinstLib, hWnd, NSK_ALGO_RegisterCallbackStr, &bError);
	NSK_ALGO_SdkVersionAddr = (NSK_ALGO_SdkVersion_Dll)getFuncAddr(hinstLib, hWnd, NSK_ALGO_SdkVersionStr, &bError);
	NSK_ALGO_AlgoVersionAddr = (NSK_ALGO_AlgoVersion_Dll)getFuncAddr(hinstLib, hWnd, NSK_ALGO_AlgoVersionStr, &bError);
	NSK_ALGO_StartAddr = (NSK_ALGO_Start_Dll)getFuncAddr(hinstLib, hWnd, NSK_ALGO_StartStr, &bError);
	NSK_ALGO_PauseAddr = (NSK_ALGO_Pause_Dll)getFuncAddr(hinstLib, hWnd, NSK_ALGO_PauseStr, &bError);
	NSK_ALGO_StopAddr = (NSK_ALGO_Stop_Dll)getFuncAddr(hinstLib, hWnd, NSK_ALGO_StopStr, &bError);
	NSK_ALGO_DataStreamAddr = (NSK_ALGO_DataStream_Dll)getFuncAddr(hinstLib, hWnd, NSK_ALGO_DataStreamStr, &bError);

	return bError;
}

static wchar_t *GetLocalTimeStr() {
	static wchar_t buffer[128];
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	wsprintf(buffer, L"%2d/%2d/%4d %02d:%02d:%02d:%03d", lt.wDay, lt.wMonth, lt.wYear, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
	return buffer;
}

static void OutputToEditBox(LPCWSTR szBuf) {
	if (Edit_GetLineCount(hOutputText) >= 1024) {
		Edit_SetSel(hOutputText, 0, -1);
		Edit_ReplaceSel(hOutputText, L"");
	}
	wchar_t buffer[1024];
	int nLength = GetWindowTextLength(hOutputText);
	Edit_SetSel(hOutputText, nLength, nLength);
	wsprintf(buffer, L"  [%s]: %s\r\n\r\n", GetLocalTimeStr(), szBuf);
	Edit_ReplaceSel(hOutputText, buffer);
}

// Algo SDK에서 분석 결과가 나올 때 자동으로 호출되는 콜백 함수
static void AlgoSdkCallback(sNSK_ALGO_CB_PARAM param) {
	static wchar_t buffer[512];
	static wchar_t sbuffer[512];
	static wchar_t qbuffer[512];
	buffer[0] = sbuffer[0] = qbuffer[0] = 0;
	switch (param.cbType) {
	case NSK_ALGO_CB_TYPE_STATE:  // 상태 변경 콜백
	{
		HWND handle = (HWND)param.userData;
		eNSK_ALGO_STATE state = (eNSK_ALGO_STATE)(param.param.state & NSK_ALGO_STATE_MASK);
		eNSK_ALGO_STATE reason = (eNSK_ALGO_STATE)(param.param.state & NSK_ALGO_REASON_MASK);
		swprintf(sbuffer, 512, L"State: ");
		switch (state) {
		case NSK_ALGO_STATE_INITED:  // SDK 초기화 완료
			swprintf(sbuffer, 512, L"%sInited", sbuffer);
			bRunning = false;
			bInited = true;

			PostMessage(hWnd, WM_ENABLE, (WPARAM)hAPCheck, (LPARAM)false);
			PostMessage(hWnd, WM_ENABLE, (WPARAM)hMECheck, (LPARAM)false);
			PostMessage(hWnd, WM_ENABLE, (WPARAM)hME2Check, (LPARAM)false);
			PostMessage(hWnd, WM_ENABLE, (WPARAM)hFCheck, (LPARAM)false);
			PostMessage(hWnd, WM_ENABLE, (WPARAM)hF2Check, (LPARAM)false);
			PostMessage(hWnd, WM_ENABLE, (WPARAM)hAttCheck, (LPARAM)false);
			PostMessage(hWnd, WM_ENABLE, (WPARAM)hMedCheck, (LPARAM)false);
			PostMessage(hWnd, WM_ENABLE, (WPARAM)hBlinkCheck, (LPARAM)false);
			PostMessage(hWnd, WM_ENABLE, (WPARAM)hBPCheck, (LPARAM)false);
			PostMessageW(hWnd, WM_USER_SETTEXT, (WPARAM)hStartBtn, (LPARAM)L"Start");
			PostMessageW(hWnd, WM_ENABLE, (WPARAM)hStartBtn, (LPARAM)true);
			PostMessageW(hWnd, WM_USER_SETTEXT, (WPARAM)hInitBtn, (LPARAM)L"Uninit");

			break;
		case NSK_ALGO_STATE_ANALYSING_BULK_DATA:  // 오프라인 데이터 분석 중
			swprintf(sbuffer, 512, L"%sAnalysing data", sbuffer);
			bRunning = true;

			PostMessageW(hWnd, WM_USER_SETTEXT, (WPARAM)hStartBtn, (LPARAM)L"Pause");
			PostMessageW(hWnd, WM_ENABLE, (WPARAM)hStartBtn, (LPARAM)false);
			PostMessageW(hWnd, WM_ENABLE, (WPARAM)hStopBtn, (LPARAM)true);

			break;
		case NSK_ALGO_STATE_COLLECTING_BASELINE_DATA:  // 기준점(Baseline) 데이터 수집 중
			swprintf(sbuffer, 512, L"%sCollecting baseline", sbuffer);
			bRunning = true;

			PostMessageW(hWnd, WM_USER_SETTEXT, (WPARAM)hStartBtn, (LPARAM)L"Pause");
			PostMessageW(hWnd, WM_ENABLE, (WPARAM)hStopBtn, (LPARAM)true);

			break;
		case NSK_ALGO_STATE_PAUSE:  // 분석 일시 중지
			swprintf(sbuffer, 512, L"%sPause", sbuffer);
			bRunning = false;

			PostMessageW(hWnd, WM_USER_SETTEXT, (WPARAM)hStartBtn, (LPARAM)L"Start");
			PostMessageW(hWnd, WM_ENABLE, (WPARAM)hStopBtn, (LPARAM)false);

			break;
		case NSK_ALGO_STATE_RUNNING:  // 분석 진행 중 (온라인 모드)
			swprintf(sbuffer, 512, L"%sRunning", sbuffer);
			bRunning = true;

			PostMessageW(hWnd, WM_USER_SETTEXT, (WPARAM)hStartBtn, (LPARAM)L"Pause");
			PostMessageW(hWnd, WM_ENABLE, (WPARAM)hStopBtn, (LPARAM)true);

			break;

		case NSK_ALGO_STATE_RUNNING_DEMO:  // 데모 모드 실행
			swprintf(sbuffer, 512, L"%sRunning Demo", sbuffer);
			bRunning = true;

			PostMessageW(hWnd, WM_USER_SETTEXT, (WPARAM)hStartBtn, (LPARAM)L"Pause");
			PostMessageW(hWnd, WM_ENABLE, (WPARAM)hStopBtn, (LPARAM)true);

			break;

		case NSK_ALGO_STATE_STOP:  // 분석 중지
		{
			swprintf(sbuffer, 512, L"%sStop", sbuffer);
			bRunning = false;

			PostMessageW(hWnd, WM_USER_SETTEXT, (WPARAM)hStartBtn, (LPARAM)L"Start");
			PostMessageW(hWnd, WM_ENABLE, (WPARAM)hStartBtn, (LPARAM)true);
			PostMessageW(hWnd, WM_ENABLE, (WPARAM)hStopBtn, (LPARAM)false);
			if (bConnectedHeadset == false) {
				PostMessageW(hWnd, WM_ENABLE, (WPARAM)hBulkDataBtn, (LPARAM)true);
			}
		}
			break;
		case NSK_ALGO_STATE_UNINTIED:  // SDK 초기화 해제
			swprintf(sbuffer, 512, L"%sUninited", sbuffer);
			bRunning = false;
			bInited = false;

			//PostMessage(hWnd, WM_ENABLE, (WPARAM)hAPCheck, (LPARAM)true);
			//PostMessage(hWnd, WM_ENABLE, (WPARAM)hMECheck, (LPARAM)true);
			//PostMessage(hWnd, WM_ENABLE, (WPARAM)hME2Check, (LPARAM)true);
			//PostMessage(hWnd, WM_ENABLE, (WPARAM)hFCheck, (LPARAM)true);
			//PostMessage(hWnd, WM_ENABLE, (WPARAM)hF2Check, (LPARAM)true);
			PostMessage(hWnd, WM_ENABLE, (WPARAM)hAttCheck, (LPARAM)true);
			PostMessage(hWnd, WM_ENABLE, (WPARAM)hMedCheck, (LPARAM)true);
			PostMessage(hWnd, WM_ENABLE, (WPARAM)hBlinkCheck, (LPARAM)true);
			PostMessage(hWnd, WM_ENABLE, (WPARAM)hBPCheck, (LPARAM)true);
			PostMessageW(hWnd, WM_USER_SETTEXT, (WPARAM)hStartBtn, (LPARAM)L"Start");
			PostMessageW(hWnd, WM_ENABLE, (WPARAM)hStartBtn, (LPARAM)false);
			PostMessageW(hWnd, WM_ENABLE, (WPARAM)hStopBtn, (LPARAM)false);
			PostMessageW(hWnd, WM_USER_SETTEXT, (WPARAM)hInitBtn, (LPARAM)L"Init");

			break;
		default:
			return;
		}
		switch (reason) {
		case NSK_ALGO_REASON_BY_USER:
			swprintf(sbuffer, 512, L"%s | By user", sbuffer);
			break;
		case NSK_ALGO_REASON_CB_CHANGED:
			swprintf(sbuffer, 512, L"%s | CB changed", sbuffer);
			break;
		case NSK_ALGO_REASON_NO_BASELINE:
			swprintf(sbuffer, 512, L"%s | No baseline", sbuffer);
			break;
		case NSK_ALGO_REASON_SIGNAL_QUALITY:
			swprintf(sbuffer, 512, L"%s | Signal quality", sbuffer);
			break;
		default:
			break;
		}
		PostMessageW(hWnd, WM_USER_SETTEXT, (WPARAM)hStateText, (LPARAM)sbuffer);
	}
		break;
	case NSK_ALGO_CB_TYPE_SIGNAL_LEVEL:  // 신호 품질 변경 콜백
	{
		HWND handle = (HWND)param.userData;
		eNSK_ALGO_SIGNAL_QUALITY sq = (eNSK_ALGO_SIGNAL_QUALITY)param.param.sq;
		swprintf(qbuffer, 512, L"Signal Quality: ");
		switch (sq) {
		case NSK_ALGO_SQ_GOOD:
			swprintf(qbuffer, 512, L"%sGood", qbuffer);
			break;
		case NSK_ALGO_SQ_MEDIUM:
			swprintf(qbuffer, 512, L"%sMedium", qbuffer);
			break;
		case NSK_ALGO_SQ_NOT_DETECTED:
			swprintf(qbuffer, 512, L"%sNot detected", qbuffer);
			break;
		case NSK_ALGO_SQ_POOR:
			swprintf(qbuffer, 512, L"%sPoor", qbuffer);
			break;
		}
		PostMessage(hWnd, WM_USER_SETTEXT, (WPARAM)hSignalQualityText, (LPARAM)qbuffer);
	}
		break;
	case NSK_ALGO_CB_TYPE_ALGO:  // 알고리즘 결과 콜백 (집중도, 명상도 등)
	{
		switch (param.param.index.type) {
			case NSK_ALGO_TYPE_ATT:  // 집중도(Attention) 결과
			{
				int att = param.param.index.value.group.att_index;
				swprintf(buffer, 512, L"ATT = %3d", att);
				OutputToEditBox(buffer);
				OutputLog(buffer);
				break;
			}
			case NSK_ALGO_TYPE_MED:  // 명상도(Meditation) 결과
			{
				int med = param.param.index.value.group.med_index;
				swprintf(buffer, 512, L"MED = %3d", med);
				OutputToEditBox(buffer);
				OutputLog(buffer);
				break;
			}
			case NSK_ALGO_TYPE_BLINK:  // 눈깜빡임(Blink) 감지 결과
			{
				int strength = param.param.index.value.group.eye_blink_strength;
				swprintf(buffer, 512, L"Eye blink strength = %4d", strength);
				OutputToEditBox(buffer);
				OutputLog(buffer);
				break;
			}
			case NSK_ALGO_TYPE_BP:  // 대역별 전력(Bandpower) 결과
			{
				swprintf(buffer, 512, L"EEG Bandpower: Delta: %1.4f Theta: %1.4f Alpha: %1.4f Beta: %1.4f Gamma: %1.4f", 
					param.param.index.value.group.bp_index.delta_power,
					param.param.index.value.group.bp_index.theta_power,
					param.param.index.value.group.bp_index.alpha_power,
					param.param.index.value.group.bp_index.beta_power,
					param.param.index.value.group.bp_index.gamma_power);
				OutputToEditBox(buffer);
				OutputLog(buffer);
				break;
			}
		}
	}
		break;
	}
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ALGOSDKSAMPLE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALGOSDKSAMPLE));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALGOSDKSAMPLE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_HAND);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_ALGOSDKSAMPLE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

static void UIComponentsPositionUpdate(HWND hWnd) {
	RECT rect;
	GetClientRect(hWnd, &rect);
	SetWindowPos(hInitBtn, NULL, MARGIN, MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);
	SetWindowPos(hVersionBtn, NULL, MARGIN + MARGIN + BUTTON_WIDTH, MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);

	SetWindowPos(hStartBtn, NULL, MARGIN, (MARGIN + BUTTON_HEIGHT) + MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);
	SetWindowPos(hBulkDataBtn, NULL, MARGIN + BUTTON_WIDTH + MARGIN, (MARGIN + BUTTON_HEIGHT) + MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);

	SetWindowPos(hStopBtn, NULL, MARGIN, (MARGIN + BUTTON_HEIGHT) * 2 + MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);
	SetWindowPos(hStateText, NULL, MARGIN, rect.bottom - STATE_TEXT_HEIGHT - MARGIN, STATE_TEXT_WIDTH, STATE_TEXT_HEIGHT, SWP_NOZORDER);
	SetWindowPos(hSignalQualityText, NULL, rect.right - SIGNAL_TEXT_WIDTH - MARGIN, rect.bottom - SIGNAL_TEXT_HEIGHT - MARGIN, SIGNAL_TEXT_WIDTH, SIGNAL_TEXT_HEIGHT, SWP_NOZORDER);

	SetWindowPos(hMedCheck, NULL, rect.right - BUTTON_WIDTH - MARGIN, MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);
	SetWindowPos(hAttCheck, NULL, rect.right - (BUTTON_WIDTH + MARGIN)*2, MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);
	SetWindowPos(hBPCheck, NULL, rect.right - (BUTTON_WIDTH + MARGIN) * 3, MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);

	SetWindowPos(hBlinkCheck, NULL, rect.right - (BUTTON_WIDTH + MARGIN) * 2, MARGIN + BUTTON_HEIGHT + MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);
	SetWindowPos(hAPCheck, NULL, rect.right - BUTTON_WIDTH - MARGIN, MARGIN + BUTTON_HEIGHT + MARGIN, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);

	SetWindowPos(hMECheck, NULL, rect.right - BUTTON_WIDTH - MARGIN, MARGIN + (BUTTON_HEIGHT + MARGIN)*2, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);
	SetWindowPos(hME2Check, NULL, rect.right - (BUTTON_WIDTH + MARGIN)*2, MARGIN + (BUTTON_HEIGHT + MARGIN) * 2, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);

	SetWindowPos(hFCheck, NULL, rect.right - BUTTON_WIDTH - MARGIN, MARGIN + (BUTTON_HEIGHT + MARGIN) * 3, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);
	SetWindowPos(hF2Check, NULL, rect.right - (BUTTON_WIDTH + MARGIN) * 2, MARGIN + (BUTTON_HEIGHT + MARGIN) * 3, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);

	SetWindowPos(hOutputText, NULL, MARGIN, MARGIN + (BUTTON_HEIGHT + MARGIN) * 4 + MARGIN, rect.right - (MARGIN * 2), rect.bottom - (MARGIN + (BUTTON_HEIGHT + MARGIN) * 4 + MARGIN) - BUTTON_HEIGHT - (MARGIN * 2), SWP_NOZORDER);
	//SetWindowText(hOutputText, L"1\r\n2\r\n3\r\n");
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:  // 윈도우 생성 시 (GUI 컴포넌트 초기화)
	{
		// Algo SDK DLL 로드
		HINSTANCE hinstLib = LoadLibrary(ALGO_SDK_DLL);
		if (hinstLib == NULL) {
			MessageBox(hWnd, L"Failed to load library AlgoSdkDll.dll", L"Error", MB_OK);
			OutputLog(L"Failed to load library AlgoSdkDll.dll");
		}
		else {
			OutputLog(L"Loaded library AlgoSdkDll.dll");
			// Algo SDK 함수 포인터 로드
			if (getFuncAddrs(hinstLib, hWnd) == false) {
				FreeLibrary(hinstLib);
				return FALSE;
			}
			//MessageBox(hWnd, L"All Algo SDK functions are loaded successfully", L"Information", MB_OK);
		}

		/* ThinkGear 연결 ID 핸들 생성 */
		connectionId = TG_GetNewConnectionId();
		if (connectionId < 0) {
			MessageBox(hWnd, L"Failed to new connection ID", L"Error", MB_OK);
		}
		else {
			/* COM 포트 연결 시도 */
			/* 주의: Windows에서는 COM10 이상의 경우 \\.\를 앞에 붙여야 함
			*       COM9 이하는 필수는 아니지만 붙일 수 있음
			*       Mac OS X에서는 "/dev/tty.MindSet-DevB-1" 같은 형태
			*/
			comPortName = "\\\\.\\" MWM_COM;
			errCode = TG_Connect(connectionId,
				comPortName,
				TG_BAUD_57600,
				TG_STREAM_PACKETS);
			if (errCode < 0) {
				MessageBox(hWnd, L"TG_Connect() failed", L"Error", MB_OK);
			} else {
				wchar_t buffer[100];

				swprintf(buffer, 100, L"Connected to headset with %S", MWM_COM);
				MessageBox(hWnd, buffer, L"Information", MB_OK);
				bConnectedHeadset = true;
				// 뇌파 데이터 읽기 스레드 생성
				if ((threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&ThreadReadPacket, NULL, 0, &dwThreadId)) == NULL) {
					MessageBox(hWnd, L"Failed to create packet reading thread", L"Error", MB_OK);
				}
			}
		}

		/*HWND hWnd1 = CreateWindowEx(0, szWindowClass, L"Child window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 320, 480, hWnd, NULL, hInst, NULL);
		ShowWindow(hWnd1, SW_SHOWNORMAL);
		UpdateWindow(hWnd1);*/

		// GUI 버튼 및 컴포넌트 생성
		hInitBtn = CreateWindow(L"Button", L"Init", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_INIT_BUTTON, hInst, NULL);
		hStartBtn = CreateWindow(L"Button", L"Start", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_START_BUTTON, hInst, NULL);
		hStopBtn = CreateWindow(L"Button", L"Stop", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_STOP_BUTTON, hInst, NULL);
		hBulkDataBtn = CreateWindow(L"Button", L"Data", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_BULK_DATA_BUTTON, hInst, NULL);

		hStateText = CreateWindow(L"Static", L"State: ", WS_VISIBLE | WS_CHILD, 0, 0, STATE_TEXT_WIDTH, STATE_TEXT_HEIGHT, hWnd, (HMENU)IDD_STATE_TEXT, hInst, NULL);
		hSignalQualityText = CreateWindow(L"Static", L"Signal Quality: ", WS_VISIBLE | WS_CHILD, 0, 0, SIGNAL_TEXT_WIDTH, SIGNAL_TEXT_HEIGHT, hWnd, (HMENU)IDD_SIGNAL_TEXT, hInst, NULL);
		hVersionBtn = CreateWindow(L"Button", L"Version", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_INIT_BUTTON, hInst, NULL);
		hOutputText = CreateWindow(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL, 0, 0, 10, 10, hWnd, (HMENU)IDD_OUTPUT_TEXT, hInst, NULL);

		// 알고리즘 선택 체크박스 생성
		hBPCheck = CreateWindow(L"Button", L"BP", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_BP_CHECK, hInst, NULL);
		hBlinkCheck = CreateWindow(L"Button", L"Blink", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_AP_CHECK, hInst, NULL);
		hAPCheck = CreateWindow(L"Button", L"AP", WS_VISIBLE | WS_DISABLED | WS_CHILD | BS_CHECKBOX, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_AP_CHECK, hInst, NULL);
		hAttCheck = CreateWindow(L"Button", L"Att", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_ATT_CHECK, hInst, NULL);
		hMedCheck = CreateWindow(L"Button", L"Med", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_MED_CHECK, hInst, NULL);
		hMECheck = CreateWindow(L"Button", L"ME", WS_VISIBLE | WS_DISABLED | WS_CHILD | BS_CHECKBOX, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_ME_CHECK, hInst, NULL);
		hME2Check = CreateWindow(L"Button", L"ME2", WS_VISIBLE | WS_DISABLED | WS_CHILD | BS_CHECKBOX, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_ME2_CHECK, hInst, NULL);
		hFCheck = CreateWindow(L"Button", L"F", WS_VISIBLE | WS_DISABLED | WS_CHILD | BS_CHECKBOX, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_F_CHECK, hInst, NULL);
		hF2Check = CreateWindow(L"Button", L"F2", WS_VISIBLE | WS_DISABLED | WS_CHILD | BS_CHECKBOX, 10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hWnd, (HMENU)IDD_F2_CHECK, hInst, NULL);

		// 초기 버튼 상태 설정
		Button_Enable(hStartBtn, false);
		Button_Enable(hStopBtn, false);

		// 헤드셋 미연결 시 오프라인 데이터 분석 버튼 활성화
		if (bConnectedHeadset == false) {
			Button_Enable(hBulkDataBtn, true);
		}
		else {
			Button_Enable(hBulkDataBtn, false);
		}

		// GUI 컴포넌트 위치 업데이트
		UIComponentsPositionUpdate(hWnd);
	}
		break;
	case WM_USER_SETTEXT:
		if ((HWND)wParam == hStateText) {
			OutputLog((LPCWSTR)lParam);
		}
		SetWindowText((HWND)wParam, (LPCWSTR)lParam);
		break;
	case WM_ENABLE:
		EnableWindow((HWND)wParam, (BOOL)lParam);
		/*if ((HWND)wParam == hBulkDataBtn) {
			if ((BOOL)lParam == true) {
				(NSK_ALGO_StartAddr)(NS_FALSE);
				eNSK_ALGO_RET ret = (NSK_ALGO_DataStreamAddr)(NSK_ALGO_DATA_TYPE_BULK_EEG, raw_data, (raw_data_count / 512) * 512);
			}
		}*/
		break;
	case WM_CTLCOLOREDIT:
	{
		HDC hdc = (HDC)wParam;
		SetTextColor(hdc, RGB(0, 0, 0));
		SetBkColor(hdc, RGB(200, 200, 200));
	}
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
			case IDD_AP_CHECK:
			case IDD_ATT_CHECK:  // 집중도 선택 체크박스
			case IDD_MED_CHECK:  // 명상도 선택 체크박스
			case IDD_ME_CHECK:
			case IDD_ME2_CHECK:
			case IDD_F_CHECK:
			case IDD_F2_CHECK:
			case IDD_BP_CHECK:  // 대역별 전력 선택 체크박스
			{
				// 체크박스 상태 토글
				if (Button_GetCheck((HWND)lParam) == BST_CHECKED) {
					Button_SetCheck((HWND)lParam, false);
				} else {
					Button_SetCheck((HWND)lParam, true);
				}
			}
				break;
			case IDD_INIT_BUTTON:		// [Init] 버튼: Algo SDK 초기화
			case IDD_START_BUTTON:		// [Start] 버튼: 분석 시작
			case IDD_STOP_BUTTON:		// [Stop] 버튼: 분석 중지
			case IDD_BULK_DATA_BUTTON:	// [Data] 버튼: 파일에서 데이터 로드 및 오프라인 분석
				if (((HWND)lParam) && (HIWORD(wParam) == BN_CLICKED)) {
					if ((HWND)lParam == hInitBtn) {
						wchar_t ReadBuffer[1024] = { 0 };
						char readBuf[1024] = { 0 };

						GetCurrentDirectory(1024, ReadBuffer);
						wcstombs_s(NULL, readBuf, ReadBuffer, 1024);

						// 선택된 알고리즘 확인
						lSelectedAlgos = 0;
						if (Button_GetCheck(hAttCheck)) {
							lSelectedAlgos |= NSK_ALGO_TYPE_ATT;
						}
						if (Button_GetCheck(hMedCheck)) {
							lSelectedAlgos |= NSK_ALGO_TYPE_MED;
						}
						if (Button_GetCheck(hBlinkCheck)) {
							lSelectedAlgos |= NSK_ALGO_TYPE_BLINK;
						}
						if (Button_GetCheck(hBPCheck)) {
							lSelectedAlgos |= NSK_ALGO_TYPE_BP;
						}

						// 최소 하나의 알고리즘 선택 확인
						if (lSelectedAlgos == 0) {
							MessageBox(hWnd, L"Please select at least one algorithm", L"Error", MB_OK);
							OutputLog(L"Please select at least one algorithm");
							return 0;
						}

						if (bInited == false) {
							// Algo SDK 콜백 등록 및 초기화
							eNSK_ALGO_RET ret = (NSK_ALGO_RegisterCallbackAddr)(&AlgoSdkCallback, hWnd);
							ret = (NSK_ALGO_InitAddr)((eNSK_ALGO_TYPE)(lSelectedAlgos), readBuf);
							if (NSK_ALGO_RET_SUCCESS == ret) {
								//MessageBox(hWnd, L"Algo SDK inited successfully", L"Information", MB_OK);
								OutputLog(L"Algo SDK inited successfully");
							}
							else {
								wchar_t buffer[100];
								swprintf(buffer, 100, L"Algo SDK inited failure [%d]", ret);
								MessageBox(hWnd, buffer, L"Error", MB_OK);
								OutputLog(buffer);
							}
						} else {
							// Algo SDK 초기화 해제
							eNSK_ALGO_RET ret = (NSK_ALGO_UninitAddr)();
							if (NSK_ALGO_RET_SUCCESS == ret) {
								//MessageBox(hWnd, L"Algo SDK inited successfully", L"Information", MB_OK);
								OutputLog(L"Algo SDK uninited successfully");
							}
							else {
								wchar_t buffer[100];
								swprintf(buffer, 100, L"Algo SDK uninit failure [%d]", ret);
								MessageBox(hWnd, buffer, L"Error", MB_OK);
								OutputLog(buffer);
							}
						}
					} else if ((HWND)lParam == hStartBtn) {
						eNSK_ALGO_RET ret;
						wchar_t caption[20];
						Button_GetText(hStartBtn, caption, 20);
						if (lstrcmpW(caption, L"Start") == 0) {
							// 분석 시작
							ret = (NSK_ALGO_StartAddr)(NS_FALSE);
							if (NSK_ALGO_RET_SUCCESS == ret) {
								OutputLog(L"Algo SDK started successfully");
							} else {
								wchar_t buffer[100];
								swprintf(buffer, 100, L"Algo SDK start failure [%d]", ret);
								MessageBox(hWnd, buffer, L"Error", MB_OK);
								OutputLog(buffer);
							}
						} else {
							// 분석 일시 중지
							ret = (NSK_ALGO_PauseAddr)();
							if (NSK_ALGO_RET_SUCCESS == ret) {
								OutputLog(L"Algo SDK paused successfully");
							}
							else {
								wchar_t buffer[100];
								swprintf(buffer, 100, L"Algo SDK pause failure [%d]", ret);
								MessageBox(hWnd, buffer, L"Error", MB_OK);
								OutputLog(buffer);
							}
						}
					} else if ((HWND)lParam == hStopBtn) {
						// 분석 중지
						eNSK_ALGO_RET ret = (NSK_ALGO_StopAddr)();
						if (NSK_ALGO_RET_SUCCESS == ret) {
							//MessageBox(hWnd, L"Algo SDK inited successfully", L"Information", MB_OK);
							OutputLog(L"Algo SDK stopped successfully");
						}
						else {
							wchar_t buffer[100];
							swprintf(buffer, 100, L"Algo SDK stop failure [%d]", ret);
							MessageBox(hWnd, buffer, L"Error", MB_OK);
							OutputLog(buffer);
						}
					} else if ((HWND)lParam == hVersionBtn) {
						// 버전 정보 표시
						wchar_t buffer[1024];
						char *verStr = (NSK_ALGO_SdkVersionAddr)();
						if (verStr != NULL) {
							if (verStr[6] == 0x12) {
								MessageBox(hWnd, L"Cracked version", L"Error", MB_OK);
							}
							swprintf(buffer, 1024, L"Comm SDK Ver: %d\r\nAlgo SDK Ver: %S\r\n", TG_GetVersion(), verStr);
							if (lSelectedAlgos & NSK_ALGO_TYPE_ATT) {
								swprintf(buffer, 1024, L"%sATT: %S\r\n", buffer, (NSK_ALGO_AlgoVersionAddr)(NSK_ALGO_TYPE_ATT));
							}
							if (lSelectedAlgos & NSK_ALGO_TYPE_MED) {
								swprintf(buffer, 1024, L"%sMED: %S\r\n", buffer, (NSK_ALGO_AlgoVersionAddr)(NSK_ALGO_TYPE_MED));
							}
							if (lSelectedAlgos & NSK_ALGO_TYPE_BLINK) {
								swprintf(buffer, 1024, L"%sBlink: %S\r\n", buffer, (NSK_ALGO_AlgoVersionAddr)(NSK_ALGO_TYPE_BLINK));
							}
							if (lSelectedAlgos & NSK_ALGO_TYPE_BP) {
								swprintf(buffer, 1024, L"%sEEG Bandpower: %S\r\n", buffer, (NSK_ALGO_AlgoVersionAddr)(NSK_ALGO_TYPE_BP));
							}
							MessageBox(hWnd, buffer, L"Information", MB_OK);
						}
					}
					else if ((HWND)lParam == hBulkDataBtn) {
						// 파일에서 뇌파 데이터 로드 및 오프라인 분석
						HANDLE hFile;
						DWORD  dwBytesRead = 0;
						wchar_t ReadBuffer[1024] = { 0 };
						char readBuf[1024] = { 0 };
						int index = 0;

						Button_Enable(hBulkDataBtn, false);

						// 기존 데이터 버퍼 해제
						if (raw_data) {
							free(raw_data);
							raw_data = NULL;
						}
						raw_data_count = 0;
						GetCurrentDirectory(1024, ReadBuffer);

						wcstombs_s(NULL, readBuf, ReadBuffer, 1024);
						sprintf_s(readBuf, "%s\\%s", readBuf, EEG_RAW_DATA);
						FILE *fp;
						fopen_s(&fp, readBuf, "rb");
						if (fp == NULL) {
							MessageBox(hWnd, L"Failed to open file", L"Error", MB_OK);
							Button_Enable(hBulkDataBtn, true);
							return 0;
						}
						// 파일의 데이터 개수 계산
						while (1) {
							short raw_data_value = 0;
							fgets(readBuf, 1024, fp);
							raw_data_value = atoi(readBuf);
							raw_data_count++;
							if (feof(fp)) {
								break;
							}
						}
						fseek(fp, 0, SEEK_SET);
						
						// 데이터 버퍼 할당
						raw_data = (short*)malloc(sizeof(short)*raw_data_count);
						if (raw_data == NULL) {
							MessageBox(hWnd, L"Failed to allocate buffer for raw data", L"Error", MB_OK);
							Button_Enable(hBulkDataBtn, true);
							return 0;
						}
						// 파일에서 데이터 읽기
						while (1) {
							fgets(readBuf, 1024, fp);
							raw_data[index++] = atoi(readBuf);
							if (feof(fp)) {
								break;
							}
						}
						fclose(fp);

						// Algo SDK로 오프라인 데이터 전송
						eNSK_ALGO_RET ret = (NSK_ALGO_DataStreamAddr)(NSK_ALGO_DATA_TYPE_BULK_EEG, raw_data, (raw_data_count/512) * 512);
						if (NSK_ALGO_RET_SUCCESS == ret) {
						} else {
							Button_Enable(hBulkDataBtn, true);
						}
					}
				}
				break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
				(NSK_ALGO_UninitAddr)();
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
			/*MoveToEx(hdc, 60, 20, NULL);
			LineTo(hdc, 60, 122);
			LineTo(hdc, 264, 122);
			LineTo(hdc, 60, 20);*/
            EndPaint(hWnd, &ps);
        }
        break;
	case WM_SIZING:
		{
			UIComponentsPositionUpdate(hWnd);
		}
		break;
    case WM_DESTROY:  // 윈도우 종료 시 (리소스 정리)
		// 헤드셋 연결 해제
		if (connectionId >= 0) {
			TG_Disconnect(connectionId);
			/* 리소스 정리 */
			TG_FreeConnection(connectionId);

			connectionId = -1;
		}
		// 데이터 읽기 스레드 종료
		if (threadHandle != NULL) {
			TerminateThread(threadHandle, 0);
			threadHandle = NULL;
		}
		// 모든 GUI 컴포넌트 삭제
		DestroyWindow(hInitBtn);
		DestroyWindow(hStartBtn);
		DestroyWindow(hStopBtn);
		DestroyWindow(hAttCheck);
		DestroyWindow(hMedCheck);
		DestroyWindow(hAPCheck);
		DestroyWindow(hBlinkCheck);
		DestroyWindow(hBPCheck);
		DestroyWindow(hMECheck);
		DestroyWindow(hME2Check);
		DestroyWindow(hFCheck);
		DestroyWindow(hF2Check);
		DestroyWindow(hBulkDataBtn);
		DestroyWindow(hStateText);
		DestroyWindow(hSignalQualityText);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
