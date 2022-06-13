#pragma once

#define XN_STR_HELPER(x) #x
#define XN_STR(x) XN_STR_HELPER(x)

#define XN_SP_READ_NUM			128

#define XN_LOG_DIR_STR			_T("Log")

#define XN_RESULT_STR			_T("Result")
#define XN_CFG_ERR_STR			_T("Config Error")
#define XN_VER_ERR_STR			_T("Version Error")
#define XN_OQC_ERR_STR			_T("OQC read Error")

#define XN_SP_ERR_STR			_T("Serial port Error")
#define XN_LOG_ERR_STR			_T("Log save Error")
#define XN_THD_ERR_STR			_T("Thread Error")

#define XN_SP_COMM_SLEEP_MS		100

#define XN_SP_SYNC_TIMEOUT_MS	3000

#define XN_SP_BR				115200

#define XN_VER_MAJ_MAX			9
#define XN_VER_MIN_MAX			999
#define XN_VER_PCH_MAX			999

#define XN_OQC_PASS_CRITERIA	0x1FF

#if _DEBUG
// To print Error and Warning messages on screen via printf
#pragma comment(linker, "/subsystem:console /entry:WinMainCRTStartup")
#endif

typedef enum
{
	WM_USER_CREATE_RECEIVER_THREAD = WM_USER + 100,
	WM_USER_CREATE_TRANSFER_THREAD,
	WM_USER_SP_RX_STRING,
	WM_USER_SAVE_PIC,
	WM_USER_EXIT_DLG,
} tag_user_message_t;