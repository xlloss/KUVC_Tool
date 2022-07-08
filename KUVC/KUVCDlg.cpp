// KUVCDlg.cpp : implementation file
//
#include "stdafx.h"
#include "KUVC.h"
#include "KUVCDlg.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include "SaveDlg.h"
#include "CMsSQL.h"
#include "XmlProcess.h"
#include "MSAccess.h"

// blur algorithm using opencv
#include "opencv/OpencvLaplacian.h"

// base
#include "xnBase.h"

// video device enumerator
#include "OpenCVDeviceEnumerator/DeviceEnumerator.h"

// USB device change
#include "Dbt.h"
#include "hidsdi.h"

// blur algorithm using opencv
#include "opencv/OpencvLaplacian.h"

CMsSQL g_SqlAdmMain;

// Export to CSV format file
#include "Lib/ExcelControl/CRange.h"
#include "Lib/ExcelControl/CWorkbook.h"
#include "Lib/ExcelControl/CWorkbooks.h"
#include "Lib/ExcelControl/CWorksheet.h"
#include "Lib/ExcelControl/CWorksheets.h"
#include "Lib/ExcelControl/CApplication.h"

/* 
 * Note
 * LPCTSTR pszCharacterString = CA2W(pChar);
 */

using std::string;
using std::exception;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;

#ifdef _DEBUG
#pragma comment(linker, "/subsystem:console /entry:WinMainCRTStartup")
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//#define ENABLE_CON_SQL_SERVER

#define VIDEO_FULL_WND_NAME "VIDEO FULL"
#define VIDEO_SCALE_WND_NAME "VIDEO SCALE"

#define VIDEO_FULL_W 3840
#define VIDEO_FULL_H 2160

#define UVC_VIDEO_ORG_W VIDEO_FULL_W
#define UVC_VIDEO_ORG_H VIDEO_FULL_W

#define UVC_VIDEO_SCALE_W 640
#define UVC_VIDEO_SCALE_H 480

#define TAB_H_GAP 30

#define GUID_DBT_DEV_0 0xA5DCBF10L
#define GUID_DBT_DEV_1 0x6530
#define GUID_DBT_DEV_2 0x11D2
#define GUID_DBT_DEV_3 0x90
#define GUID_DBT_DEV_4 0x1F
#define GUID_DBT_DEV_5 0x00
#define GUID_DBT_DEV_6 0xC0
#define GUID_DBT_DEV_7 0x4F
#define GUID_DBT_DEV_8 0xB9
#define GUID_DBT_DEV_9 0x51
#define GUID_DBT_DEV_10 0xED

#define VIDEO_FULL_W		1920
#define VIDEO_FULL_H		1080
#define VIDEO_SUB_WND_W		640
#define VIDEO_SUB_WND_H		320
#define TAB_H_GAP			30

const char *strVideoSubWndName[SUB_WND_MAX] = { "VIDEO SUB0", "VIDEO SUB1", "VIDEO SUB2", "VIDEO SUB3", "VIDEO SUB4" };
const int g_iVideoFullWndPos[4] = { 0, 0, VIDEO_FULL_W, VIDEO_FULL_H };

int g_iVideoSubWndPos[SUB_WND_MAX][4] = { {0, 0, VIDEO_SUB_WND_W, VIDEO_SUB_WND_H},		// left top
										{1280, 0, VIDEO_SUB_WND_W, VIDEO_SUB_WND_H},	// right top
										{640, 320, VIDEO_SUB_WND_W, VIDEO_SUB_WND_H},	// center
										{0, 640, VIDEO_SUB_WND_W, VIDEO_SUB_WND_H},		// left bottom
										{1280, 640, VIDEO_SUB_WND_W, VIDEO_SUB_WND_H} }; // right bottom

// global variable
const char* strVideoFullWndName = "VIDEO FULL";
cv::Point VertexOne, VertexThree;

// CKUVCDlg dialog
CKUVCDlg::CKUVCDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_KUVC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_iSavePic = AfxGetApp()->GetProfileInt(_T("Setting"), _T("SavePic"), 1);
}

CKUVCDlg::~CKUVCDlg()
{

}

void CKUVCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_VIDEO_SWITCH, m_TabCtrl);
}

BEGIN_MESSAGE_MAP(CKUVCDlg, CDialogEx)
	ON_WM_DEVICECHANGE()
	ON_BN_CLICKED(IDCANCEL, &CKUVCDlg::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO_UVC_DEVICE, &CKUVCDlg::OnCbnSelchangeVideoDevice)
	ON_CBN_DROPDOWN(IDC_COMBO_UVC_DEVICE, &CKUVCDlg::OnCbnDropDownVideoDevice)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_VIDEO_SWITCH, &CKUVCDlg::OnTcnSelchangeTabVideoSwitch)
	ON_WM_SHOWWINDOW()
	ON_WM_TIMER()
	ON_WM_HSCROLL()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_XU, &CKUVCDlg::OnClickedButtonXu)
	ON_BN_CLICKED(IDC_DB_BTN, &CKUVCDlg::OnBnClickedDb)

	/* VK_S */
	ON_MESSAGE(WM_USER_SAVE_PIC, OnSaveData)
END_MESSAGE_MAP()

void CKUVCDlg::Init()
{
	m_bCaptureFrame = false;
	m_bCloseCapture = false;
	m_bFindCam = false;
	m_bStopCpature = false;

	m_hCaptured = NULL;
	m_hCapturing = NULL;
	m_dFullWndMaxLpVal = 0;
	m_dFullWndCurLpVal = 0;

	/* default use image scale mode */
	m_bFullImageMode = FALSE;
}

/* 
 * Register notification,
 * we must add this section for 
 * DBT_DEVICEARRIVAL and DBT_DEVICEREMOVECOMPLETE 
 * Notice Message
 */
void CKUVCDlg::RegisterDevNotification(HANDLE *hWind)
{
	HDEVNOTIFY gNotifyDevHandle;
	GUID InterfaceClassGuid = { GUID_DBT_DEV_0,
			GUID_DBT_DEV_1, GUID_DBT_DEV_2, GUID_DBT_DEV_3, GUID_DBT_DEV_4,
			GUID_DBT_DEV_5, GUID_DBT_DEV_6, GUID_DBT_DEV_7, GUID_DBT_DEV_8,
			GUID_DBT_DEV_9, GUID_DBT_DEV_10};

	// Register to receive notification when a USB device is plugged in.
	DEV_BROADCAST_DEVICEINTERFACE broadcastInterface;
	broadcastInterface.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	broadcastInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	broadcastInterface.dbcc_classguid = InterfaceClassGuid;

	/* Register Device Notification */
	gNotifyDevHandle = RegisterDeviceNotification(*hWind,
		&broadcastInterface, DEVICE_NOTIFY_WINDOW_HANDLE);
	if (!gNotifyDevHandle)
		AfxMessageBox(_T("Register USB Device Notification Fail"));
}

BOOL CKUVCDlg::OnInitDialog()
{
	CRect ComBox_rect;
	CRect TestBtn_rect;
	CRect SaveBtn_rect;
	CRect ReCkBtn_rect;
	CComboBox* pComboBox;
	CA2T str_tmp(TEST_CAM_NAME);
	int ret;
	#define TEXT_DEF "N/A"

	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	freopen("log.txt", "w", stdout);
	printf("%s\n", __func__);

	SetWindowText(m_strCaption);
	Init();
	HKL defaultLanguageLayout = LoadKeyboardLayout(L"0x00000409", KLF_ACTIVATE);

	m_bTestMode = 0;

	CButton* m_ctlCheck = (CButton *)GetDlgItem(IDC_CHECK_DS);
	/* Enable DirectShow */
#if ENABLE_DIRECTSHOW
	m_ctlCheck->SetCheck(TRUE);
#endif
	m_ctlCheck->EnableWindow(FALSE);

	//load Region file
	ShowWindow(SW_MAXIMIZE);

	//setup video wnd layout
	OnSetVideoFullWnd();
	OnSetVideoScaleWnd();

	ShowVideoFullWnd(FALSE);
	ShowVideoScaleWnd(TRUE);

	//xu test button
	GetDlgItem(IDC_BUTTON_XU)->SetWindowPos(NULL, 0, 600, 100, 30, SWP_NOZORDER);
	GetDlgItem(IDC_DB_BTN)->SetWindowPos(NULL, 0, 650, 100, 30, SWP_NOZORDER);
	
	CFont* m_font = new CFont();

	m_font->CreatePointFont(200, L" ");
	GetDlgItem(IDC_CHECK_DS)->SetWindowPos(NULL, DIRECTSHOW_CKB_X, DIRECTSHOW_CKB_Y, 
	DIRECTSHOW_CKB_W, DIRECTSHOW_CKB_H, SWP_NOZORDER);

	/* IG ID */
	CStatic* m_Ig1600TextCtl = new CStatic();
	m_Ig1600TextCtl->Create(_T("IG1600 ID :"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(TEX_IG1600_X, TEX_IG1600_Y, TEX_IG1600_W, TEX_IG1600_H), this);
	m_Ig1600TextCtl->SetFont(m_font);

	m_Ig1600ID = new CStatic();
	m_Ig1600ID->Create(_T(TEXT_DEF), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(TEX_IG1600_X + 130, TEX_IG1600_Y, TEX_IG1600_W + 200, TEX_IG1600_H), this);
	m_Ig1600ID->SetFont(m_font);


	/* Sensor ID */
	CStatic* m_ImageSensorTextCtl = new CStatic();
	m_ImageSensorTextCtl->Create(_T("Sensor ID :"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(TEX_IMGSEN_X, TEX_IMGSEN_Y, TEX_IMGSEN_W, TEX_IMGSEN_H), this);
	m_ImageSensorTextCtl->SetFont(m_font);

	m_ImageSensorID = new CStatic();
	m_ImageSensorID->Create(_T(TEXT_DEF), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(TEX_IMGSEN_X + 130, TEX_IMGSEN_Y, TEX_IMGSEN_W + 200, TEX_IMGSEN_H), this);
	m_ImageSensorID->SetFont(m_font);

	/* Light LED */
	CStatic* m_LedTextCtl = new CStatic();
	m_LedTextCtl->Create(_T("LLED :"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(TEX_LED_X, TEX_LED_Y, TEX_LED_W, TEX_LED_H), this);
	m_LedTextCtl->SetFont(m_font);

	m_LightLedCtl = new CStatic();
	m_LightLedCtl->Create(_T(TEXT_DEF), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(TEX_LED_X + 130, TEX_LED_Y, TEX_LED_W + 200, TEX_LED_H), this);
	m_LightLedCtl->SetFont(m_font);

	/* PreView LED */
	CStatic* m_LightTextCtl = new CStatic();
	m_LightTextCtl->Create(_T("PLED :"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(TEX_LIGHT_X, TEX_LIGHT_Y, TEX_LIGHT_W, TEX_LIGHT_H), this);
	m_LightTextCtl->SetFont(m_font);

	m_PreLedCtl = new CStatic();
	m_PreLedCtl->Create(_T(TEXT_DEF), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(TEX_LIGHT_X + 130, TEX_LIGHT_Y, TEX_LIGHT_W + 200, TEX_LIGHT_H), this);
	m_PreLedCtl->SetFont(m_font);

	/* AF Btn */
	CStatic* m_AfBtnTextCtl = new CStatic();
	m_AfBtnTextCtl->Create(_T("AF Btn :"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(BTN_AF_X, BTN_AF_Y, BTN_AF_W, BTN_AF_H), this);
	m_AfBtnTextCtl->SetFont(m_font);
	m_AfBtn = new CStatic();
	m_AfBtn->Create(_T(TEXT_DEF), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(BTN_AF_X + 150, BTN_AF_Y, BTN_AF_W + 200, BTN_AF_H), this);
	m_AfBtn->SetFont(m_font);


	CStatic* m_LightBtnTextCtl = new CStatic();
	m_LightBtnTextCtl->Create(_T("Light Btn :"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(BTN_LIGHT_X, BTN_LIGHT_Y, BTN_LIGHT_W, BTN_LIGHT_H), this);
	m_LightBtnTextCtl->SetFont(m_font);
	m_LedBtn = new CStatic();
	m_LedBtn->Create(_T(TEXT_DEF), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(BTN_LIGHT_X + 150, BTN_LIGHT_Y, BTN_LIGHT_W + 200, BTN_LIGHT_H), this);
	m_LedBtn->SetFont(m_font);

	//video device list
	DeviceEnumerator dec_emu;
	std::map<int, Device> usbcam_devices = dec_emu.getVideoDevicesMap();

	// Combobox control create, setup video device to combobox
	pComboBox = new CComboBox;
	ComBox_rect.SetRect(800, 0, 1060, 200);
	pComboBox->Create(WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
		ComBox_rect, this, IDC_COMBO_UVC_DEVICE);

	for (auto const &device : usbcam_devices) {
		CA2T str(device.second.deviceName.c_str());
		pComboBox->InsertString(device.first, str);
	}

	ret = pComboBox->SetCurSel(pComboBox->FindStringExact(-1, str_tmp));
	if (ret == CB_ERR) {
		AfxMessageBox(_T("Camera Find Fail"));
		return FALSE;
	}
	m_pVideoDeviceComboCtrls.Add(pComboBox);
	m_CamId = pComboBox->GetCurSel();
	
	// tab ctrl
	m_TabCtrl.MoveWindow(0, 0, 1920, 1080);
	m_TabCtrl.SetMinTabWidth(50);
	m_TabCtrl.SetPadding(CSize(2, 0));

	// Tab control create
	TC_ITEM TabCtrlItem;
	TabCtrlItem.mask = TCIF_TEXT;
	TabCtrlItem.pszText = L"Normal Mode";
	m_TabCtrl.InsertItem(0, &TabCtrlItem); // append to tab
	m_TabCtrl.SetCurSel(0);

#if MF_TODO
	TabCtrlItem.mask = TCIF_TEXT;
	TabCtrlItem.pszText = L"Static"; // make tab title
	m_TabCtrl.InsertItem(2, &TabCtrlItem); // append to tab
#endif

	//switch to video full tab
	NMHDR nmhdr;
	nmhdr.code = TCN_SELCHANGE;
	nmhdr.hwndFrom = m_TabCtrl.GetSafeHwnd();
	nmhdr.idFrom = m_TabCtrl.GetDlgCtrlID();
	SendMessage(WM_NOTIFY, MAKELONG(TCN_SELCHANGE, 1), (LPARAM)(&nmhdr));

	//Register Device Notice
	RegisterDevNotification((HANDLE *)&this->m_hWnd);
	ShowWindow(SW_SHOW);

#ifdef ENABLE_CON_SQL_SERVER
	if (ConnectToServer() == FALSE)
		return FALSE;
#endif

	CreateCaptureProcedure();

	/* return TRUE, unless you set the focus to a control */
	return TRUE;
}

BOOL CKUVCDlg::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	BOOL bResult = CDialog::OnDeviceChange(nEventType, dwData);
	CComboBox* pCblCtl;
	CString cstr_index_name;
	CA2T str_tmp(TEST_CAM_NAME);
	int retry = 20;
	int ret;

	if (m_bFindCam == TRUE)
		return TRUE;

	DeviceEnumerator de;
	pCblCtl = (CComboBox*)m_pVideoDeviceComboCtrls[0];

	switch (nEventType) {
		case DBT_DEVICEARRIVAL:
			//usb device insent
			pCblCtl->ResetContent();
			while (retry > 0) {
				std::map<int, Device> devices = de.getVideoDevicesMap();
				for (auto const& device : devices) {
					CA2T str(device.second.deviceName.c_str());
					pCblCtl->InsertString(device.first, str);
					if (!strcmp(device.second.deviceName.c_str(), TEST_CAM_NAME)) {
						m_bFindCam = TRUE;
						goto START_CAM;
					}
				}
				Sleep(50);
				retry--;
			};
			goto EXIT;

		case DBT_DEVICEQUERYREMOVEFAILED:
		case DBT_DEVICEREMOVECOMPLETE:
		case DBT_DEVICEQUERYREMOVE:
			AfxMessageBox(_T(TEST_CAM_NAME" Remove Camera"));
			if (IsCapturing() == FALSE)
				goto EXIT;

			m_bStopCpature = TRUE;
			do {
				retry--;
				if (retry < 0) {
					AfxMessageBox(_T("Close Camera Timeout!"));
					break;
				}
				Sleep(10);
				ret = CloseCaptureProcedure();
			} while (ret == FALSE);

			m_hCapturing = NULL;
			m_bFindCam = FALSE;
			CloseHandle(m_hCaptured);
			m_hCaptured = NULL;
			goto EXIT;
	}

START_CAM:
	ret = pCblCtl->SetCurSel(pCblCtl->FindStringExact(-1, str_tmp));
	if (ret == CB_ERR) {
		AfxMessageBox(_T("Camera Find Fail"));
		return FALSE;
	}

	m_CamId = pCblCtl->GetCurSel();
	CreateCaptureProcedure();
	return TRUE;

EXIT:
	return TRUE;
}

void CKUVCDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
}

void CKUVCDlg::OnSetVideoFullWnd()
{
	cvNamedWindow(VIDEO_FULL_WND_NAME, CV_WINDOW_AUTOSIZE);
	HWND hWnd = (HWND)cvGetWindowHandle(VIDEO_FULL_WND_NAME);
	HWND hParent = ::GetParent(hWnd);
	::SetParent(hWnd, GetDlgItem(IDC_STATIC_VIDEO_FULL)->m_hWnd);
	::ShowWindow(hParent, SW_HIDE);

	CWnd *pWnd = this->GetDlgItem(IDC_STATIC_VIDEO_FULL);
	pWnd->MoveWindow(0, 0, 3840, 2160);
	m_pFullWndCtrls.Add(pWnd);
}

void CKUVCDlg::ShowVideoFullWnd(BOOL Enable)
{
	CWnd *pWnd = this->GetDlgItem(IDC_STATIC_VIDEO_FULL);

	if (Enable)
		pWnd->ShowWindow(SW_SHOW);
	else
		pWnd->ShowWindow(SW_HIDE);
}

void CKUVCDlg::OnSetVideoScaleWnd()
{
	cvNamedWindow(VIDEO_SCALE_WND_NAME, CV_WINDOW_AUTOSIZE);
	HWND hWnd = (HWND)cvGetWindowHandle(VIDEO_SCALE_WND_NAME);
	HWND hParent = ::GetParent(hWnd);
	::SetParent(hWnd, GetDlgItem(IDC_STATIC_VIDEO_SCALE)->m_hWnd);
	::ShowWindow(hParent, SW_HIDE);

	CWnd *pWnd = this->GetDlgItem(IDC_STATIC_VIDEO_SCALE);
	pWnd->MoveWindow(0, TAB_H_GAP, 640, 480);
	m_pFullWndCtrls.Add(pWnd);
}

void CKUVCDlg::ShowVideoScaleWnd(BOOL Enable)
{
	CWnd *pWnd = this->GetDlgItem(IDC_STATIC_VIDEO_SCALE);

	if (Enable)
		pWnd->ShowWindow(SW_SHOW);
	else
		pWnd->ShowWindow(SW_HIDE);
}

void CKUVCDlg::OnCbnDropDownVideoDevice()
{
	if (m_pVideoDeviceComboCtrls.IsEmpty())
		return;

	DeviceEnumerator de;
	std::map<int, Device> devices = de.getVideoDevicesMap();
	CComboBox* pCblCtl;
	pCblCtl = (CComboBox*)m_pVideoDeviceComboCtrls[0];
	pCblCtl->ResetContent();

	for (auto const &device : devices) {
		CA2T str(device.second.deviceName.c_str());
		pCblCtl->InsertString(device.first, str);
	}
}

void CKUVCDlg::OnCbnSelchangeVideoDevice()
{
	m_CamId = ((CComboBox *)GetDlgItem(IDC_COMBO_UVC_DEVICE))->GetCurSel();

	CreateCaptureProcedure();
}

void ClearAllSpace(string &str)
{
	int index = 0;

	if (!str.empty()) {
		while ((index = str.find(' ', index)) != string::npos)
			str.erase(index, 1);
	}
}

void CKUVCDlg::CreateCaptureProcedure(void)
{
	DWORD dwThreadId;

	if (m_hCaptured)
		return;

	//freopen("log.txt", "w", stdout);
	//cout << __func__ << __LINE__ << endl;
	Init();

	// callback register
	/*
	cvSetMouseCallback(strVideoFullWndName, onMouseFullWnd, this);
	for (int i = 0; i < SUB_WND_MAX; i++) {
		cvSetMouseCallback(strVideoSubWndName[i], onMouseSubWnd[i], this);
	}
	*/

	Invalidate();
	m_hCaptured = CreateEvent(NULL, TRUE, FALSE, _T("VideoCaptured"));
	m_hCapturing = CreateThread(NULL, 0, CaptureVideoThread, this, 0, &dwThreadId);
}

bool CKUVCDlg::IsCapturing(void) const
{
	if (m_hCapturing)
		return true;

	return false;
}

void CKUVCDlg::ReleaseCaptureEvent(void)
{
	::SetEvent(m_hCaptured);
}

bool CKUVCDlg::CloseCaptureProcedure(void)
{
	if (false == WaitCloseCapture(2000))
		return false;

	return true;
}

void CKUVCDlg::LockUIs(BOOL bLock)
{
	GetDlgItem(IDC_COMBO_UVC_DEVICE)->EnableWindow(~bLock);
}

bool CKUVCDlg::WaitCloseCapture(DWORD Timeout)
{
	bool ret;

	ret = (::WaitForSingleObject(m_hCaptured, Timeout) == WAIT_TIMEOUT) ? false : true;
	return ret;
}

void CKUVCDlg::OnTcnSelchangeTabVideoSwitch(NMHDR *pNMHDR, LRESULT *pResult)
{
	CWnd *NorVideoWnd = NULL;
	int tab_idx;
	int i = 0;
	int ctl;
	#define NORMAL_VIDEO_MODE 0

	tab_idx = m_TabCtrl.GetCurSel();
	ctl = m_pFullWndCtrls.GetSize();

	NorVideoWnd = (CWnd*)m_pFullWndCtrls[1];

	if (tab_idx == NORMAL_VIDEO_MODE)
		NorVideoWnd->ShowWindow(true);
	else
		NorVideoWnd->ShowWindow(false);
}




LRESULT CKUVCDlg::OnSaveData(WPARAM wParam, LPARAM lParam)
{
	int nFolderLength, testResult;
	TCHAR szPath[MAX_PATH] = { 0 };
	CString cstrFileName, cstrPicSaveName, strPath, strToken;
	SYSTEMTIME st;

	CSaveDlg sdlg;
	sdlg.m_iEnableButton = true;
	testResult = sdlg.DoModal();
	if (testResult != IDOK && testResult != IDCANCEL)
		return FALSE;

	GetLocalTime(&st); //Get system local time

	/*
	 * 假如 SaveDlg 有抓到 PCB S/N,
	 * sdlg.m_cstrPicSaveName會是PCB S/N,
	 * 否則sdlg.m_cstrPicSaveName會代入時間
	 */
	cstrPicSaveName = sdlg.m_cstrPicSaveName;
	if (cstrPicSaveName.IsEmpty() != 0)
		cstrPicSaveName.Format(_T("%04d%02d%02d%02d%02d%02d"),
			st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond);

	// get the execute folder and create LOG_DIR_STR if needed. -begin
	GetModuleFileName(NULL, szPath, MAX_PATH);
	strPath = szPath;
	nFolderLength = strPath.ReverseFind('\\');

	return TRUE;
}

BOOL CKUVCDlg::PreTranslateMessage(MSG* pMsg)
{
	#define VK_F 0x46
	#define VK_S 0x53

	switch (pMsg->message) {
		case WM_KEYDOWN: {
			switch (pMsg->wParam) {
			case VK_F:
				m_bFullImageMode = !m_bFullImageMode;
				ShowVideoFullWnd(m_bFullImageMode);
				ShowVideoScaleWnd(!m_bFullImageMode);
				break;
			case VK_S:
				PostMessage(WM_USER_SAVE_PIC, NULL, NULL);
				break;
			default:
				break;
			}
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CKUVCDlg::VideoScale(Mat *SrcFrame, Mat *DstFrame, int width, int high)
{
	CvRect video_scale_rect;
	Size dsize;

	video_scale_rect.x = 0;
	video_scale_rect.y = 0;
	video_scale_rect.width = UVC_VIDEO_ORG_W;
	video_scale_rect.height = UVC_VIDEO_ORG_H;

	dsize = Size(width, high);
	cv::resize(*SrcFrame, *DstFrame, dsize, CV_INTER_LINEAR);
}

vector<Point2f> center(5);


void sort_area(int area[][4])
{
	int i, j;
	int temp1, temp2;
	for (i = 0; i < 5; i++)
	{
		for (j = i + 1; j < 5; j++)
		{
			if (area[i][1] > area[j][1])
			{
				temp1 = area[j][0];
				temp2 = area[j][1];
				area[j][0] = area[i][0];
				area[j][1] = area[i][1];
				area[i][0] = temp1;
				area[i][1] = temp2;
			}
		}
	}

	for (i = 1; i < 4; i++)
	{
		for (j = i + 1; j < 4; j++)
		{
			if (area[i][0] > area[j][0])
			{
				temp1 = area[j][0];
				temp2 = area[j][1];
				area[j][0] = area[i][0];
				area[j][1] = area[i][1];
				area[i][0] = temp1;
				area[i][1] = temp2;
			}
		}
	}
}

int CKUVCDlg::GetSubWndMaxValue()
{
	int i, iMax, iNum[4] = { m_dSubWndCurLpVal[0], m_dSubWndCurLpVal[1], m_dSubWndCurLpVal[3], m_dSubWndCurLpVal[4] };

	iMax = iNum[0];
	for (i = 1; i < 4; i++)
	{
		if (iNum[i] > iMax)
			iMax = iNum[i];
	}

	return iMax;
}

int CKUVCDlg::GetSubWndMinValue()
{
	int i, iMin, iNum[4] = { m_dSubWndCurLpVal[0], m_dSubWndCurLpVal[1], m_dSubWndCurLpVal[3], m_dSubWndCurLpVal[4] };

	iMin = iNum[0];
	for (i = 1; i < 4; i++)
	{
		if (iNum[i] < iMin)
			iMin = iNum[i];
	}

	return iMin;
}


DWORD WINAPI CKUVCDlg::CaptureVideoThread(LPVOID lpVoid)
{
	char max_val_str[16], cur_val_str[16];
	int iCount = 0, width = 1920, high = 1080, x1, y1, x2, y2;
	bool bShowSubWnd = false;
	Mat inFrame;
	Mat *ScaleFrame;
	char file_name[128];
	int device_num;
	CvRect full_scale_rect;
	COpencvLaplacian lp;
	CKUVCDlg* pThis = (CKUVCDlg*)lpVoid;
	#define CHECK_COUNTER 1

	CButton *m_ctlCheck = (CButton*)pThis->GetDlgItem(IDC_CHECK_DS);
	if (m_ctlCheck->GetCheck() == BST_CHECKED) {
		device_num = cv::CAP_DSHOW + pThis->m_CamId;
	} else {
		device_num = pThis->m_CamId;
	}

	cv::VideoCapture cap(0 + device_num);
	//cv::VideoCapture cap(1);
	if (!cap.isOpened()) {
		AfxMessageBox(_T("Camera Open Fail"));
		return 0;
	}

	double w = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	double h = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

	pThis->LockUIs(TRUE);
	memset(file_name, 0x00, sizeof(file_name));
	//cap.set(cv::CAP_PROP_EXPOSURE, 0);
	//cap.set(cv::CAP_PROP_BRIGHTNESS, -56);
	cap.set(cv::CAP_PROP_SETTINGS, 1);
	cap.set(cv::CAP_OPENCV_MJPEG, 1);
	
	while (cap.read(inFrame)) {
		if (pThis->m_bStopCpature == TRUE)
			break;

		iCount++;

		if (iCount % 3)
			continue;
		else
			iCount = 0;

		/* 中心校正 */
		if (pThis->lens_shitf_calibration == 1) {
			lp.shitfcalibration(inFrame, &(pThis->lens_center_x), &(pThis->lens_center_y), &(pThis->lens_center_counter));
		}


		//Assist_lens_installation
		if (pThis->Assist_lens_installation == 1)
		{
			pThis->Assist_lens_drawing = true;
		}
		else if (pThis->check_lens_shift > 0 && pThis->check_lens_shift_pass == false) //Lens shift
		{
			if (pThis->m_dFullWndCurLpVal >= pThis->auto_roi_lower_limit)
			{
				if (lp.findContractAndBrightness(inFrame, pThis->check_lens_shift, width, high, pThis->check_lens_shift_HSVlimit))
				{
					pThis->check_lens_counter++;
					printf("check shift counter = %d \r\n", pThis->check_lens_counter);
				}
				else
				{
					pThis->check_lens_counter = 0;
				}
				if (pThis->check_lens_counter == CHECK_COUNTER)
				{
					pThis->check_lens_counter = 0;
					pThis->check_lens_shift_pass = true;
				}
			}
		}



//		CWnd *FullModeWnd = NULL;
//		CWnd *ScaleModeWnd = NULL;
//		FullModeWnd = (CWnd*)pThis->m_pFullWndCtrls[0];
//		ScaleModeWnd = (CWnd*)pThis->m_pFullWndCtrls[1];

#if 0
		if (pThis->m_bFullImageMode) {
			cv::imshow(VIDEO_FULL_WND_NAME, inFrame);
		} else {
			ScaleFrame = &pThis->m_MatFullScaleFrame;
			pThis->VideoScale(&inFrame, ScaleFrame,
				UVC_VIDEO_SCALE_W, UVC_VIDEO_SCALE_H);
			cv::imshow(VIDEO_SCALE_WND_NAME, *ScaleFrame);
		}
#endif


		//Assist_lens_installation
		if (pThis->Assist_lens_installation == 1)
		{
			pThis->Assist_lens_drawing = true;
		}
		else if (pThis->check_lens_shift > 0 && pThis->check_lens_shift_pass == false) //Lens shift
		{
			if (pThis->m_dFullWndCurLpVal >= pThis->auto_roi_lower_limit)
			{
				if (lp.findContractAndBrightness(inFrame, pThis->check_lens_shift, width, high, pThis->check_lens_shift_HSVlimit))
				{
					pThis->check_lens_counter++;
					printf("check shift counter = %d \r\n", pThis->check_lens_counter);
				}
				else
				{
					pThis->check_lens_counter = 0;
				}
				if (pThis->check_lens_counter == CHECK_COUNTER)
				{
					pThis->check_lens_counter = 0;
					pThis->check_lens_shift_pass = true;
				}
			}
		}

		//find char, 調焦
		if (pThis->auto_roi == 1 && pThis->auto_roi_getArea == 0 && (pThis->check_lens_shift_pass))
		{
			//inFrame = cv::imread("1.jpg", IMREAD_COLOR);
			//if(lp.LosdfindContour(inFrame, center))

			pThis->GetDlgItem(IDC_PUSHBTN)->EnableWindow(false);

			//char buf[256];
			//sprintf(buf, "m_dFullWndCurLpVal=%d, auto_roi_lower_limit=%d\r\n", pThis->m_dFullWndCurLpVal, pThis->auto_roi_lower_limit);
			//pThis->DebugLog(buf, strlen(buf), false);

			if (pThis->m_dFullWndCurLpVal >= pThis->auto_roi_lower_limit)
			{
				if (lp.findchart(inFrame, center))
				{
					pThis->auto_roi_counter++;
					if (pThis->auto_roi_counter == CHECK_COUNTER)
					{
						bShowSubWnd = TRUE;
						pThis->auto_roi_getArea = 1;
						TC_ITEM TabCtrlItem;

						TabCtrlItem.mask = TCIF_TEXT;
						TabCtrlItem.pszText = L"Sub"; // make tab title
						pThis->m_TabCtrl.InsertItem(1, &TabCtrlItem); // append to tab
						//pThis->GetDlgItem(IDC_PUSHBTN)->EnableWindow(true);

						for (int i = 0; i < SUB_WND_MAX; i++)
						{
							if (center[0].x - pThis->sub_region_width / 2 >= 0 && center[0].y - pThis->sub_region_high / 2 >= 0)
							{
								g_iVideoSubWndPos[i][0] = center[i].x - pThis->sub_region_width / 2;
								g_iVideoSubWndPos[i][1] = center[i].y - pThis->sub_region_high / 2;
							}

							//char buf[256];
							//sprintf(buf, "g_iVideoSubWndPosX=%d, g_iVideoSubWndPos=%d\r\n", g_iVideoSubWndPos[i][0], g_iVideoSubWndPos[i][1]);
							//pThis->DebugLog(buf, strlen(buf), false);

														//if (g_iVideoSubWndPos[i][0] < 0 || g_iVideoSubWndPos[i][1] < 0 || g_iVideoSubWndPos[i][2] < 0 || g_iVideoSubWndPos[i][3] < 0)
														//{
														//	pThis->m_bCloseCapture = true;
														//	AfxMessageBox(L"Find chart not is 5, lens focus fail");
														//	break;
														//}

							if (g_iVideoSubWndPos[i][0] < 0)
							{
								//AfxMessageBox(L"Find chart not is 5, lens focus fail");
								g_iVideoSubWndPos[i][0] = 10;
							}

							if (g_iVideoSubWndPos[i][1] < 0)
							{
								//AfxMessageBox(L"Find chart not is 5, lens focus fail");
								g_iVideoSubWndPos[i][1] = 10;
							}

							if (g_iVideoSubWndPos[i][0] > 1920)
							{
								//pThis->m_bCloseCapture = true;
								//AfxMessageBox(L"Find chart not is 5, lens focus fail");
								//break;
								g_iVideoSubWndPos[i][0] = 900;
							}

							if (g_iVideoSubWndPos[i][1] > 1080)
							{
								//pThis->m_bCloseCapture = true;
								//AfxMessageBox(L"Find chart not is 5, lens focus fail");
								//break;
								g_iVideoSubWndPos[i][1] = 500;
							}

							int iSubWndPosSumX, iSubWndPosSumY;
							iSubWndPosSumX = g_iVideoSubWndPos[i][0] + pThis->sub_region_width;
							iSubWndPosSumY = g_iVideoSubWndPos[i][1] + pThis->sub_region_high;

							if (iSubWndPosSumX > 1920)
							{
								//pThis->m_bCloseCapture = true;
								//AfxMessageBox(L"Find chart not is 5, lens focus fail");
								//break;
								g_iVideoSubWndPos[i][0] = 900;
							}

							if (iSubWndPosSumY > 1080)
							{
								//pThis->m_bCloseCapture = true;
								//AfxMessageBox(L"Find chart not is 5, lens focus fail");
								//break;
								g_iVideoSubWndPos[i][1] = 500;
							}
						}

						//if (true == pThis->m_bCloseCapture) { // exit 
						//	pThis->m_bCloseCapture = false;
						//	break;
						//}

						sort_area(g_iVideoSubWndPos); //排序區

						/* TODO */
						//pThis->OnSetVideoSubWnd();
						//pThis->m_TabCtrl.SetCurSel(1);

						NMHDR nmhdr;
						nmhdr.code = TCN_SELCHANGE;
						nmhdr.hwndFrom = pThis->m_TabCtrl.GetSafeHwnd();
						nmhdr.idFrom = pThis->m_TabCtrl.GetDlgCtrlID();
						pThis->SendMessage(WM_NOTIFY, MAKELONG(TCN_SELCHANGE, 1), (LPARAM)(&nmhdr));
					}
				}
				else
				{
					pThis->auto_roi_counter = 0;
				}
			}
		}

		//bool bShowSubWnd = pThis->m_TabCtrl.GetCurSel() ? TRUE : FALSE;

		if (false == bShowSubWnd || true == pThis->m_bCaptureFrame) { // full wnd

			// blur detection
			pThis->m_dFullWndCurLpVal = lp.Laplacian(inFrame);
			if (pThis->m_dFullWndCurLpVal >= pThis->m_dFullWndMaxLpVal) {
				pThis->m_dFullWndMaxLpVal = pThis->m_dFullWndCurLpVal;
			}

			// crop from video frame
			CvRect full_scale_rect;

			full_scale_rect.x = g_iVideoFullWndPos[0] + pThis->m_rectUserCrop_full.x;
			full_scale_rect.y = g_iVideoFullWndPos[1] + pThis->m_rectUserCrop_full.y;
			full_scale_rect.width = pThis->m_rectUserCrop_full.width;
			full_scale_rect.height = pThis->m_rectUserCrop_full.height;

			//CString str;
			//str.Format(L"x=%d, y=%d, width=%d, height=%d", full_scale_rect.x, full_scale_rect.y, full_scale_rect.width, full_scale_rect.height);
			//AfxMessageBox(str);

			pThis->m_MatFullFrame = inFrame(Rect(full_scale_rect));

			Size dsize = Size(VIDEO_FULL_W, VIDEO_FULL_H);

			// inter linear
			cv::resize(pThis->m_MatFullFrame, pThis->m_MatFullScaleFrame, dsize, CV_INTER_LINEAR);

			// put value and show
			sprintf_s(max_val_str, "%.2f(Max)", pThis->m_dFullWndMaxLpVal);
			sprintf_s(cur_val_str, "%.2f(Cur)", pThis->m_dFullWndCurLpVal);

			if (pThis->Assist_lens_drawing == true)
			{
				x1 = 419;
				y1 = pThis->check_lens_shift - 1;
				x2 = 1499;
				y2 = pThis->check_lens_shift - 1;
				cv::line(pThis->m_MatFullScaleFrame, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 255), 2);

				x1 = 419;
				y1 = 1079 - pThis->check_lens_shift;
				x2 = 1499;
				y2 = 1079 - pThis->check_lens_shift;
				cv::line(pThis->m_MatFullScaleFrame, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 255), 2);

				x1 = 419;
				y1 = pThis->check_lens_shift - 1;
				x2 = 419;
				y2 = 1079 - pThis->check_lens_shift;
				cv::line(pThis->m_MatFullScaleFrame, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 255), 2);

				x1 = 1499;
				y1 = pThis->check_lens_shift - 1;
				x2 = 1499;
				y2 = 1079 - pThis->check_lens_shift;
				cv::line(pThis->m_MatFullScaleFrame, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 255), 2);
			}
			else if (pThis->check_lens_shift > 0 && pThis->check_lens_shift_pass == false)
			{
				x1 = width / 2 - 150;
				y1 = pThis->check_lens_shift;
				x2 = width / 2 + 150;
				y2 = pThis->check_lens_shift;
				cv::line(pThis->m_MatFullScaleFrame, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 255), 2);

				y1 = high - pThis->check_lens_shift;
				y2 = high - pThis->check_lens_shift;
				cv::line(pThis->m_MatFullScaleFrame, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 255), 2);
			}
			else if (pThis->lens_shitf_calibration == 1)
			{
				x1 = width / 2 - 30;
				y1 = high / 2;
				x2 = width / 2 + 30;
				y2 = high / 2;
				cv::line(pThis->m_MatFullScaleFrame, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 0), 2);

				x1 = width / 2;
				y1 = high / 2 - 30;
				x2 = width / 2;
				y2 = high / 2 + 30;
				cv::line(pThis->m_MatFullScaleFrame, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 0), 2);

				if (pThis->lens_center_x != 0 && pThis->lens_center_y != 0)
				{
					sprintf_s(cur_val_str, "(%d,%d)", pThis->lens_center_x, pThis->lens_center_y);

					cv::putText(pThis->m_MatFullScaleFrame, cur_val_str, Point(800, 120), 0, 2, Scalar(0, 255, 255), 2);
				}
			}

			if (pThis->Assist_lens_installation == 0 && pThis->lens_shitf_calibration == 0)
				cv::putText(pThis->m_MatFullScaleFrame, cur_val_str, Point(800, 120), 0, 2, Scalar(0, 255, 255), 2);
			cv::imshow(VIDEO_FULL_WND_NAME, pThis->m_MatFullScaleFrame);

			pThis->m_UvcFrame = pThis->m_MatFullScaleFrame;

			// save
			if (TRUE == pThis->m_bCaptureFrame) {
				pThis->m_bCaptureFrame = false;
				SYSTEMTIME curTime;
				GetLocalTime(&curTime);
				sprintf_s(file_name, "%04d%02d%02d%02d%02d%02d.jpg", curTime.wYear, curTime.wMonth, curTime.wDay, curTime.wHour, curTime.wMinute, curTime.wSecond);
				cv::imwrite(file_name, inFrame);
			}
		}
		else //sub wnd
		{
			//crop from video frame
			CvRect full_scale_rect;
			full_scale_rect.x = g_iVideoFullWndPos[0] + pThis->m_rectUserCrop_full.x;
			full_scale_rect.y = g_iVideoFullWndPos[1] + pThis->m_rectUserCrop_full.y;
			full_scale_rect.width = pThis->m_rectUserCrop_full.width;
			full_scale_rect.height = pThis->m_rectUserCrop_full.height;
			pThis->m_MatFullFrame = inFrame(Rect(full_scale_rect));

			Size dsize = Size(VIDEO_FULL_W, VIDEO_FULL_H);

			//inter linear
			cv::resize(pThis->m_MatFullFrame, pThis->m_MatFullScaleFrame, dsize, CV_INTER_LINEAR);
			cv::imshow(VIDEO_FULL_WND_NAME, pThis->m_MatFullScaleFrame);

			pThis->m_UvcFrame = pThis->m_MatFullScaleFrame;

			for (int i = 0; i < SUB_WND_MAX; i++)
			{
				//crop from video frame
				CvRect sub_wnd_rect, scale_rect;
				sub_wnd_rect.x = g_iVideoSubWndPos[i][0];
				sub_wnd_rect.y = g_iVideoSubWndPos[i][1];
				sub_wnd_rect.width = g_iVideoSubWndPos[i][2];
				sub_wnd_rect.height = g_iVideoSubWndPos[i][3];

				//blur detection
				pThis->m_dSubWndCurLpVal[i] = lp.Laplacian(inFrame(Rect(sub_wnd_rect))); //為了讓GetSubWndMaxValue能取得當前的Laplacian值,移到這邊
			}

			int iSubWndLpMax, iSubWndLpMin;
			iSubWndLpMax = pThis->GetSubWndMaxValue();
			iSubWndLpMin = pThis->GetSubWndMinValue();

			for (int i = 0; i < SUB_WND_MAX; i++)
			{

				CvRect scale_rect;

				//crop ROI by user specific
				scale_rect.x = g_iVideoSubWndPos[i][0] + pThis->m_rectUserCrop_sub[i].x;
				scale_rect.y = g_iVideoSubWndPos[i][1] + pThis->m_rectUserCrop_sub[i].y;
				scale_rect.width = pThis->m_rectUserCrop_sub[i].width;
				scale_rect.height = pThis->m_rectUserCrop_sub[i].height;
				pThis->m_MatSubFrame[i] = inFrame(Rect(scale_rect));

				//fit size to (VIDEO_SUB_WND_W, VIDEO_SUB_WND_H)
				Size dsize;
				if (pThis->sub_region_width > 0)
					dsize = Size(pThis->sub_region_width, pThis->sub_region_high);
				else
					dsize = Size(VIDEO_SUB_WND_W, VIDEO_SUB_WND_H);

				//inter linear
				cv::resize(pThis->m_MatSubFrame[i], pThis->m_MatSubScaleFrame[i], dsize, CV_INTER_LINEAR);

				//put value and show
				if (pThis->m_dSubWndCurLpVal[i] >= pThis->m_dSubWndMaxLpVal[i])
				{
					pThis->m_dSubWndMaxLpVal[i] = pThis->m_dSubWndCurLpVal[i];
				}
				else if (pThis->m_dSubWndMaxLpVal[i] - pThis->m_dSubWndCurLpVal[i] > 5)
				{
					pThis->checkLpGetMaxValue[i] = TRUE;
				}

				sprintf_s(max_val_str, "%.2f(Max)", pThis->m_dSubWndMaxLpVal[i]);
				sprintf_s(cur_val_str, "%.2f(Cur)", pThis->m_dSubWndCurLpVal[i]);
				//cv::putText(pThis->m_MatSubScaleFrame[i], max_val_str, Point(10, 30), 0, 1, Scalar(0, 255, 255), 2);
				//cv::putText(pThis->m_MatSubScaleFrame[i], cur_val_str, Point(10, 30), 0, 1, Scalar(0, 255, 255), 2);

				if (pThis->region_threshold[i] > 0 && pThis->checkLpGetMaxValue[SUB_WND_1] && pThis->checkLpGetMaxValue[SUB_WND_2] && pThis->checkLpGetMaxValue[SUB_WND_3] && pThis->checkLpGetMaxValue[SUB_WND_4] && pThis->checkLpGetMaxValue[SUB_WND_5])
				{
					if ((/*(fabs(pThis->m_dSubWndMaxLpVal[i] - pThis->m_dSubWndCurLpVal[i])*/ (iSubWndLpMax - iSubWndLpMin) <= pThis->region_threshold[i]) && (pThis->m_dSubWndCurLpVal[i] >= pThis->region_lower_limit[i]))
					{
						pThis->sub_region_result[i] = true;
						cv::putText(pThis->m_MatSubScaleFrame[i], cur_val_str, Point(10, 30), 0, 1, Scalar(0, 255, 0), 2); //Succeed, show green text
					}
					else
					{
						pThis->sub_region_result[i] = false;
						cv::putText(pThis->m_MatSubScaleFrame[i], cur_val_str, Point(10, 30), 0, 1, Scalar(0, 0, 255), 2); //Fail, show red text
					}
				}
				else
				{
					pThis->sub_region_result[i] = false;
					cv::putText(pThis->m_MatSubScaleFrame[i], cur_val_str, Point(10, 30), 0, 1, Scalar(0, 0, 255), 2);
					//cv::rectangle(pThis->m_MatSubScaleFrame[i], Rect(pThis->sub_region_width / 2 - pThis->check_rect_length / 2, pThis->sub_region_high / 2 - pThis->check_rect_length / 2, pThis->check_rect_length, pThis->check_rect_length), Scalar(0, 0, 255), 3, 1, 0);
				}
				cv::imshow(strVideoSubWndName[i], pThis->m_MatSubScaleFrame[i]);
			}
		}



	}

	cap.release();
	pThis->ReleaseCaptureEvent();
	pThis->LockUIs(FALSE);
//	pThis->PostMessage(WM_USER_SAVE_PIC, NULL, NULL);
//	pThis->GetDlgItem(IDC_PUSHBTN)->EnableWindow(true);
	return 0;
}

void onMouseFullWnd(int Event, int x, int y, int flags, void* param) {
	CKUVCDlg *pThis = (CKUVCDlg*)param;

	if (false == pThis->IsCapturing()) {
		return;
	}

	if (Event == CV_EVENT_LBUTTONDOWN || Event == CV_EVENT_RBUTTONDOWN) {
		VertexOne = cvPoint(x, y);
	}

	if (Event == CV_EVENT_LBUTTONUP || Event == CV_EVENT_RBUTTONUP) {
		VertexThree = cvPoint(x, y);
	}

	if (flags == CV_EVENT_FLAG_LBUTTON || flags == CV_EVENT_FLAG_RBUTTON) {
		VertexThree = cvPoint(x, y);
		CvRect rect(VertexOne.x, VertexOne.y, VertexThree.x - VertexOne.x, VertexThree.y - VertexOne.y);
		rectangle(pThis->m_MatFullScaleFrame, rect, Scalar(255, 0, 0));
		cv::imshow(strVideoFullWndName, pThis->m_MatFullScaleFrame);
	}

	if (Event == CV_EVENT_LBUTTONUP) {
		CvRect rect(VertexOne.x, VertexOne.y, VertexThree.x - VertexOne.x, VertexThree.y - VertexOne.y);
		if (rect.width <= 0 || rect.height <= 0)
			return;
	}
}

bool CKUVCDlg::ConnectToServer()
{
	CFile cFile;
	DWORD dwLen;
	int iFolderLength = 0, iStart, iEnd, iLen;
	char *szFile;
	TCHAR szPath[MAX_PATH] = { 0 };
	CString strPath, strToken, strFile, strFile2, strIP, strUserID, strPWD, strConnetStr;

	//Read Server Config.txt setting
	GetModuleFileName(NULL, szPath, MAX_PATH);
	strPath = szPath;
	iFolderLength = strPath.ReverseFind('\\');
	strToken.Format(_T("%s\\%s"), strPath.Left(iFolderLength), L"Server Config.txt");

	if (!cFile.Open(strToken, CFile::modeRead)) {
		AfxMessageBox(L"\"Server Config.txt\" file not exist");
		return FALSE;
	}

	dwLen = (DWORD)cFile.GetLength();
	szFile = new char[dwLen + 1];
	cFile.Read(szFile, dwLen);
	cFile.Close();

	strFile.Format(L"%S", szFile);
	delete[] szFile;

	//Get server IP
	iStart = strFile.Find(L"IP=");

	//Not find
	if (iStart == -1) {
		AfxMessageBox(L"\"Server Config.txt\" setting error");
		return FALSE;
	} else {
		iEnd = strFile.Find(L"\r\n");
		if (iEnd == -1) {
			AfxMessageBox(L"\"Server Config.txt\" setting error");
			return FALSE;
		}
		strIP = strFile.Mid(iStart + 3, iEnd - iStart - 3);

		//Get User ID
		strFile2 = strFile.Mid(iEnd + 2, dwLen - iEnd - 2);
		iLen = strFile2.GetLength();
		iStart = strFile2.Find(L"UserID=");
		iEnd = strFile2.Find(L"\r\n");

		if (iStart == -1 || iEnd == -1) {
			AfxMessageBox(L"\"Server Config.txt\" setting error");
			return FALSE;
		}

		strUserID = strFile2.Mid(iStart + 7, iEnd - iStart - 7);

		//Get Password
		strFile2 = strFile2.Mid(iEnd + 2, iLen - iEnd - 2);
		iLen = strFile2.GetLength();
		iStart = strFile2.Find(L"Password=");
		iEnd = strFile2.Find(L"\r\n");

		if (iStart == -1 || iEnd == -1) {
			AfxMessageBox(L"\"Server Config.txt\" setting error");
			return FALSE;
		}
		strPWD = strFile2.Mid(iStart + 9, iEnd - iStart - 9);
	}

	strConnetStr.Format(L"Driver={SQL Server};SERVER=%s;UID=%s;PWD=%s;Connection Timeout=%d;", strIP, strUserID, strPWD, 15000);
	m_DataBase.OpenEx(strConnetStr, CDatabase::noOdbcDialog | CDatabase::useCursorLib);
	if (!m_DataBase.IsOpen()) {
		AfxMessageBox(L"Connect database server fail, please check connect");
		return FALSE;
	}
}

void CKUVCDlg::DebugLog(char *pData, int nLen, bool bHex)
{
	CFile cf;
	int i;
	char szData[42000];
	char szTemp[16];
	memset(szData, 0x0, sizeof(szData));
	memset(szTemp, 0x0, sizeof(szTemp));

	int iFolderLength;
	TCHAR szPath[MAX_PATH] = { 0 };
	CString strPath, strToken;
	GetModuleFileName(NULL, szPath, MAX_PATH);
	strPath = szPath;
	iFolderLength = strPath.ReverseFind('\\');
	strToken.Format(_T("%s\\Log.txt"), strPath.Left(iFolderLength));

	//if (cf.Open(L"C:\\test\\PCBLog.txt", CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite, NULL))
	if (cf.Open(strToken, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite, NULL)) {
		if (bHex) {
			for (i = 0; i < nLen; i++) {
				sprintf(szTemp, "%.2X ", *(pData + i) & 0x000000FF);
				strcat(szData, szTemp);
			}
		} else
			sprintf(szData, "%s", pData);

		cf.SeekToEnd();
		cf.Write(szData, strlen(szData));
		cf.Close();
	}
}

CKUVCDlg* CKUVCDlg::s_pThis = NULL;

CKUVCDlg* CKUVCDlg::GetInstance()
{
	if (s_pThis == NULL)
		s_pThis = new CKUVCDlg;

	return s_pThis;
}

void CKUVCDlg::LoadConfigSetting()
{
	std::fstream file;
	file.open("ROI_Config.ini", std::ios::in);

	if (file)
	{
		string line, str, token;
		while (std::getline(file, line))
		{
			ClearAllSpace(line);
			string::size_type position;
			std::istringstream stream(line);
			while (stream >> str)
			{
				position = str.find("=");
				if (position != str.npos)
				{
					token.assign(str, 0, position);
					if (token.compare("model") == 0)
					{
						token.assign(str, position + 1);
						focus_model = token.c_str();
						std::wcout << "focus_model = " << focus_model.GetString() << endl;
					}
					else if (token.compare("focus_mode") == 0)
					{
						token.assign(str, position + 1);
						m_iFocus_mode = atoi(token.c_str());
						cout << "focus_mode = " << m_iFocus_mode << endl;
					}
					else if (token.compare("auto_roi") == 0)
					{
						token.assign(str, position + 1);
						auto_roi = atoi(token.c_str());
						cout << "auto_roi = " << auto_roi << endl;
					}
					else if (token.compare("auto_roi_lower_limit") == 0)
					{
						token.assign(str, position + 1);
						auto_roi_lower_limit = atof(token.c_str());
						cout << "auto_roi_lower_limit = " << auto_roi_lower_limit << endl;
					}
					else if (token.compare("Assist_lens_installation") == 0)
					{
						token.assign(str, position + 1);
						Assist_lens_installation = atoi(token.c_str());
						cout << "Assist_lens_installation = " << Assist_lens_installation << endl;
					}
					else if (token.compare("check_lens_shift") == 0)
					{
						token.assign(str, position + 1);
						check_lens_shift = atof(token.c_str());
						cout << "check_lens_shift = " << check_lens_shift << endl;
					}
					else if (token.compare("check_lens_shift_HSVlimit") == 0)
					{
						token.assign(str, position + 1);
						check_lens_shift_HSVlimit = atof(token.c_str());
						cout << "check_lens_shift_HSVlimit = " << check_lens_shift_HSVlimit << endl;
					}
					else if (token.compare("lens_shitf_calibration") == 0)
					{
						token.assign(str, position + 1);
						lens_shitf_calibration = atoi(token.c_str());
						cout << "lens_shitf_calibration = " << lens_shitf_calibration << endl;
					}
					else if (token.compare("sub_region_width") == 0)
					{
						token.assign(str, position + 1);
						sub_region_width = atoi(token.c_str());
						cout << "sub_region_width = " << sub_region_width << endl;
					}
					else if (token.compare("sub_region_high") == 0)
					{
						token.assign(str, position + 1);
						sub_region_high = atoi(token.c_str());
						cout << "sub_region_high = " << sub_region_high << endl;
					}
					else if (token.compare("region1_x") == 0)
					{
						token.assign(str, position + 1);
						regionX[SUB_WND_1] = atoi(token.c_str());
						cout << "region1_x = " << regionX[SUB_WND_1] << endl;
					}
					else if (token.compare("region1_y") == 0)
					{
						token.assign(str, position + 1);
						regionY[SUB_WND_1] = atoi(token.c_str());
						cout << "region1_y = " << regionY[SUB_WND_1] << endl;
					}
					else if (token.compare("region1_threshold") == 0)
					{
						token.assign(str, position + 1);
						region_threshold[SUB_WND_1] = atof(token.c_str());
						cout << "region1_threshold = " << region_threshold[SUB_WND_1] << endl;
					}
					else if (token.compare("region1_lower_limit") == 0)
					{
						token.assign(str, position + 1);
						region_lower_limit[SUB_WND_1] = atof(token.c_str());
						cout << "region1_lower_limit = " << region_lower_limit[SUB_WND_1] << endl;
					}
					else if (token.compare("region2_x") == 0)
					{
						token.assign(str, position + 1);
						regionX[SUB_WND_2] = atoi(token.c_str());
						cout << "region2_x = " << regionX[SUB_WND_2] << endl;
					}
					else if (token.compare("region2_y") == 0)
					{
						token.assign(str, position + 1);
						regionY[SUB_WND_2] = atoi(token.c_str());
						cout << "region2_y = " << regionY[SUB_WND_2] << endl;
					}
					else if (token.compare("region2_threshold") == 0)
					{
						token.assign(str, position + 1);
						region_threshold[SUB_WND_2] = atof(token.c_str());
						cout << "region2_threshold = " << region_threshold[SUB_WND_2] << endl;
					}
					else if (token.compare("region2_lower_limit") == 0)
					{
						token.assign(str, position + 1);
						region_lower_limit[SUB_WND_2] = atof(token.c_str());
						cout << "region2_lower_limit = " << region_lower_limit[SUB_WND_2] << endl;
					}
					else if (token.compare("region3_x") == 0)
					{
						token.assign(str, position + 1);
						regionX[SUB_WND_3] = atoi(token.c_str());
						cout << "region3_x = " << regionX[SUB_WND_3] << endl;
					}
					else if (token.compare("region3_y") == 0)
					{
						token.assign(str, position + 1);
						regionY[SUB_WND_3] = atoi(token.c_str());
						cout << "region3_y = " << regionY[SUB_WND_3] << endl;
					}
					else if (token.compare("region3_threshold") == 0)
					{
						token.assign(str, position + 1);
						region_threshold[SUB_WND_3] = atof(token.c_str());
						cout << "region3_threshold = " << region_threshold[SUB_WND_3] << endl;
					}
					else if (token.compare("region3_lower_limit") == 0)
					{
						token.assign(str, position + 1);
						region_lower_limit[SUB_WND_3] = atof(token.c_str());
						cout << "region3_lower_limit = " << region_lower_limit[SUB_WND_3] << endl;
					}
					else if (token.compare("region4_x") == 0)
					{
						token.assign(str, position + 1);
						regionX[SUB_WND_4] = atoi(token.c_str());
						cout << "region4_x = " << regionX[SUB_WND_4] << endl;
					}
					else if (token.compare("region4_y") == 0)
					{
						token.assign(str, position + 1);
						regionY[SUB_WND_4] = atoi(token.c_str());
						cout << "region4_y = " << regionY[SUB_WND_4] << endl;
					}
					else if (token.compare("region4_threshold") == 0)
					{
						token.assign(str, position + 1);
						region_threshold[SUB_WND_4] = atof(token.c_str());
						cout << "region4_threshold = " << region_threshold[SUB_WND_4] << endl;
					}
					else if (token.compare("region4_lower_limit") == 0)
					{
						token.assign(str, position + 1);
						region_lower_limit[SUB_WND_4] = atof(token.c_str());
						cout << "region4_lower_limit = " << region_lower_limit[SUB_WND_4] << endl;
					}
					else if (token.compare("region5_x") == 0)
					{
						token.assign(str, position + 1);
						regionX[SUB_WND_5] = atoi(token.c_str());
						cout << "region5_x = " << regionX[SUB_WND_5] << endl;
					}
					else if (token.compare("region5_y") == 0)
					{
						token.assign(str, position + 1);
						regionY[SUB_WND_5] = atoi(token.c_str());
						cout << "region5_y = " << regionY[SUB_WND_5] << endl;
					}
					else if (token.compare("region5_threshold") == 0)
					{
						token.assign(str, position + 1);
						region_threshold[SUB_WND_5] = atof(token.c_str());
						cout << "region5_threshold = " << region_threshold[SUB_WND_5] << endl;
					}
					else if (token.compare("region5_lower_limit") == 0)
					{
						token.assign(str, position + 1);
						region_lower_limit[SUB_WND_5] = atof(token.c_str());
						cout << "region5_lower_limit = " << region_lower_limit[SUB_WND_5] << endl;
					}
				}
			}
		}

		for (int i = 0; i < SUB_WND_MAX; i++)
		{
			g_iVideoSubWndPos[i][0] = regionX[i];
			g_iVideoSubWndPos[i][1] = regionY[i];
			g_iVideoSubWndPos[i][2] = sub_region_width;
			g_iVideoSubWndPos[i][3] = sub_region_high;
		}
		file.close();
	}
	else
	{
		MessageBox(L"ROI_Config.ini读取档案失败");
	}
}


void CKUVCDlg::OnClose()
{
	int retry = 10;
	int ret;
	AfxMessageBox(_T("Close Camera!"));
	AfxGetApp()->WriteProfileInt(_T("Setting"), _T("SavePic"), m_iSavePic);

	m_pFullWndCtrls.RemoveAll(); //Full tab items release
	m_pStaticWndCtrls.RemoveAll(); //Static tab items release

	m_pVideoDeviceComboCtrls.RemoveAll();
	m_MatFullFrame.release();
	m_MatFullScaleFrame.release();
	ReleaseCaptureEvent();

	m_bStopCpature = TRUE;
	do {
		retry--;
		if (retry < 0) {
			AfxMessageBox(_T("Close Camera Timeout!"));
			break;
		}
		Sleep(10);
		ret = CloseCaptureProcedure();
	} while (ret == FALSE);

	Sleep(500);
	m_hCapturing = NULL;
	m_bFindCam = FALSE;

	if (m_hCaptured) {
		CloseHandle(m_hCaptured);
		m_hCaptured = NULL;
	}

	CDialogEx::OnClose();
}

void CKUVCDlg::OnClickedButtonXu()
{
	//AfxMessageBox(_T("XuTest"));
	/* Test UVC XU */
	UvcCtl = new Uvc();
	UvcCtl->Set_DevName(TEST_CAM_NAME);
	UvcCtl->Uvc_Init();
	
	static const GUID UVC_xuGuid =
	{ 0x0FB885C3, 0x68C2, 0x4547, {0x90, 0xF7, 0x8F, 0x47, 0x57, 0x9D, 0x95, 0xFC }};

	#define XU_AF_BTN_CMD 0x01
	#define XU_LED_BTN_CMD 0x02
	#define XU_LIGHT_LED_CMD 0x03
	#define XU_PRE_LED_CMD 0x04
	#define XU_SENSOR_ID_CMD 0x05
	#define XU_IG1600_ID_CMD 0x06

	BYTE read_data[4] = {0x00};
	BYTE write_data[5];
	unsigned int datalen;
	char testme[10];

	/* IG1600 ID */
	datalen = 4;
	UvcCtl->ReadXu(UVC_xuGuid, XU_IG1600_ID_CMD, read_data, datalen);
	sprintf(testme, "0x%x%x%x%x", read_data[0], read_data[1], read_data[2], read_data[3]);
	CA2T str_ig1600_id(testme);
	m_Ig1600ID->SetWindowTextW(str_ig1600_id);

	/* sensor ID */
	datalen = 2;
	UvcCtl->ReadXu(UVC_xuGuid, XU_SENSOR_ID_CMD, read_data, datalen);
	sprintf(testme, "0x%x%x", read_data[0], read_data[1]);
	CA2T str_sensor_id(testme);
	m_ImageSensorID->SetWindowTextW(str_sensor_id);

	/* Light LED Ctrl */
	datalen = 1;
	write_data[0] = 0;
	UvcCtl->WriteXu(UVC_xuGuid, XU_LIGHT_LED_CMD, write_data, datalen);
	m_LightLedCtl->SetWindowTextW(_T("LED ON"));
	Sleep(1000);
	write_data[0] = 1;
	UvcCtl->WriteXu(UVC_xuGuid, XU_LIGHT_LED_CMD, write_data, datalen);
	m_LightLedCtl->SetWindowTextW(_T("LED OFF"));

	/* Preview LED Ctrl */
	datalen = 1;
	write_data[0] = 0x01;
	UvcCtl->WriteXu(UVC_xuGuid, XU_PRE_LED_CMD, write_data, datalen);
	m_PreLedCtl->SetWindowTextW(_T("LED ON"));
	Sleep(1000);
	write_data[0] = 0x00;
	UvcCtl->WriteXu(UVC_xuGuid, XU_PRE_LED_CMD, write_data, datalen);
	m_PreLedCtl->SetWindowTextW(_T("LED OFF"));


	/* AF Btn */
	datalen = 1;
	UvcCtl->ReadXu(UVC_xuGuid, XU_AF_BTN_CMD, read_data, datalen);

	sprintf(testme, "Key OFF");
	if (read_data[0])
		sprintf(testme, "Key ON");

	CA2T str_af_btn(testme);
	m_AfBtn->SetWindowTextW(str_af_btn);

	/* LED Btn */
	datalen = 1;
	UvcCtl->ReadXu(UVC_xuGuid, XU_LED_BTN_CMD, read_data, datalen);

	sprintf(testme, "Key OFF");
	if (read_data[0])
		sprintf(testme, "Key ON");

	CA2T str_led_btn(testme);
	m_LedBtn->SetWindowTextW(str_led_btn);

	UvcCtl->Uvc_Close();
	delete UvcCtl;
}

void CKUVCDlg::OnBnClickedDb()
{

}
