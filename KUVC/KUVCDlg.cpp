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

#define USE_UVC_AB003 1
//#define USE_UVC_DD002 1

#if USE_UVC_AB003
	#define TEST_CAM_NAME "PC Camera"
	#define ENABLE_DIRECTSHOW 1
#elif USE_UVC_DD002
	#define TEST_CAM_NAME "UVC Camera"
	#define ENABLE_DIRECTSHOW 0
	#define TEST_UVC_XU 1
#else
	#define TEST_CAM_NAME "PC Camera"
	#define ENABLE_DIRECTSHOW 1
#endif


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
	AfxGetApp()->WriteProfileInt(_T("Setting"), _T("SavePic"), m_iSavePic);

	m_pFullWndCtrls.RemoveAll(); //Full tab items release
	m_pStaticWndCtrls.RemoveAll(); //Static tab items release

	m_pVideoDeviceComboCtrls.RemoveAll();
	m_MatFullFrame.release();
	m_MatFullScaleFrame.release();
	ReleaseCaptureEvent();
	CloseCaptureProcedure();

	if (m_hCaptured) {
		CloseHandle(m_hCaptured);
		m_hCaptured = NULL;
	}
}

void CKUVCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_VIDEO_SWITCH, m_TabCtrl);
}

BEGIN_MESSAGE_MAP(CKUVCDlg, CDialogEx)
	ON_WM_DEVICECHANGE()
	//ON_MESSAGE(WM_USER_SAVE_PIC, OnSaveData)
	ON_BN_CLICKED(IDCANCEL, &CKUVCDlg::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO_UVC_DEVICE, &CKUVCDlg::OnCbnSelchangeVideoDevice)
	ON_CBN_DROPDOWN(IDC_COMBO_UVC_DEVICE, &CKUVCDlg::OnCbnDropDownVideoDevice)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_VIDEO_SWITCH, &CKUVCDlg::OnTcnSelchangeTabVideoSwitch)
	ON_WM_SHOWWINDOW()
	ON_WM_TIMER()
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
	CButton *TestModeBox;
	CButton *SavPic_ChkBox;
	CA2T str_tmp(TEST_CAM_NAME);
	int ret;

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
	CreateCaptureProcedure();

	
	
	//ReCheck button
	//CButton *ReChkButton;
	//ReChkButton = new CButton;
	//ReCkBtn_rect.SetRect(1600, 5, 1750, 30);
	//ReChkButton->Create(L"ReCheck",
	//	WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	//	ReCkBtn_rect, this, IDC_PUSHBTN);
	//GetDlgItem(IDC_PUSHBTN)->EnableWindow(false);

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
	CFont *m_font = new CFont();
	
	m_font->CreatePointFont(200, L"IG1600 ID :");

	GetDlgItem(IDC_CHECK_DS)->SetWindowPos(NULL, 1000, 30, 1000 + 10, 30 + 20, SWP_NOZORDER);
	GetDlgItem(IDC_CHECK_DS)->SetFont(m_font);
	
	CStatic *m_Ig1600TextCtl = new CStatic();
	m_Ig1600TextCtl->Create(_T("IG1600 ID :"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(700, 50, 700 + 140, 50 + 40), this);
	m_Ig1600TextCtl->SetFont(m_font);

	CStatic* m_ImageSensorTextCtl = new CStatic();
	m_ImageSensorTextCtl->Create(_T("Sensor ID :"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(700, 50 + 50, 700 + 140, 50 + 40 + 50), this);
	m_ImageSensorTextCtl->SetFont(m_font);

	CStatic* m_LedTextCtl = new CStatic();
	m_LedTextCtl->Create(_T("LED Ctrl :"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(700, 50 + 100, 700 + 140, 50 + 40 + 100), this);
	m_LedTextCtl->SetFont(m_font);


	CStatic* m_LightTextCtl = new CStatic();
	m_LightTextCtl->Create(_T("Light Ctrl :"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(700, 50 + 150, 700 + 140, 50 + 40 + 150), this);
	m_LightTextCtl->SetFont(m_font);

	CStatic* m_AfBtnTextCtl = new CStatic();
	m_AfBtnTextCtl->Create(_T("AF Btn Status :"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(700, 50 + 200, 700 + 200, 50 + 40 + 200), this);
	m_AfBtnTextCtl->SetFont(m_font);


	CStatic* m_LightBtnTextCtl = new CStatic();
	m_LightBtnTextCtl->Create(_T("Light Btn Status :"), WS_CHILD | WS_VISIBLE | SS_LEFT,
		CRect(700, 50 + 250, 700 + 200, 50 + 40 + 250), this);

	m_LightBtnTextCtl->SetFont(m_font);


	
#if defined (TEST_UVC_XU) && defined (USE_UVC_DD002)
	/* Test UVC XU */
	UvcCtl = new Uvc();
	UvcCtl->Set_DevName(TEST_CAM_NAME);
	UvcCtl->Uvc_Init();

	static const GUID UVC_xuGuid =
		{0xE307E649, 0x4618, 0xA3FF, {0x82, 0xFC, 0x2D, 0x8B, 0x5F, 0x21, 0x67, 0x73}};

	#define XU_LED_CMD 0x01
	BYTE read_data[3];
	BYTE write_data[3] = {65, 60, 1};
	unsigned int datalen;

	datalen = sizeof(write_data) / sizeof(write_data[0]);

	UvcCtl->WriteXu(UVC_xuGuid, XU_LED_CMD, write_data, datalen);

	UvcCtl->ReadXu(UVC_xuGuid, XU_LED_CMD, read_data, datalen);
	for (int i = 0; i < datalen; i++)
		printf("read_data[%d] 0x%x\n", i, read_data[i]);

	UvcCtl->Uvc_Close();
	delete UvcCtl;
#endif

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
	 *  sdlg.m_cstrPicSaveName會是PCB S/N,
	 *  否則sdlg.m_cstrPicSaveName會代入時間
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

	printf("save excel \r\n");
	SaveFocusRecordToExcel(sdlg.m_cstrPicSaveName, testResult); //save excel

	if (m_iSavePic) {
		//GetLocalTime(&st); //Get system local time
		strToken.Format(_T("%s\\%s\\%04d%02d%02d"),
			strPath.Left(nFolderLength),
			LOG_DIR_STR, st.wYear, st.wMonth, st.wDay);

		if (GetFileAttributes(strToken) == INVALID_FILE_ATTRIBUTES)
			CreateDirectory(strToken, NULL);

		cstrFileName.Format(L"%s\\%s.jpg", strToken, cstrPicSaveName);
		std::string strFileName = CT2A(cstrFileName);
		cv::imwrite(strFileName, m_UvcFrame);
	}

	return TRUE;
}

BOOL CKUVCDlg::PreTranslateMessage(MSG* pMsg)
{
	#define VK_F 0x46

	switch (pMsg->message) {
		case WM_KEYDOWN: {
			switch (pMsg->wParam) {
			case VK_F:
				m_bFullImageMode = !m_bFullImageMode;
				ShowVideoFullWnd(m_bFullImageMode);
				ShowVideoScaleWnd(!m_bFullImageMode);
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

DWORD WINAPI CKUVCDlg::CaptureVideoThread(LPVOID lpVoid)
{
	bool bShowSubWnd = false;
	Mat inFrame;
	Mat *ScaleFrame;
	char file_name[128];
	int iCount = 0, device_num;
	CvRect full_scale_rect;
	CKUVCDlg* pThis = (CKUVCDlg*)lpVoid;

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

	while (cap.read(inFrame)) {
		if (pThis->m_bStopCpature == TRUE)
			break;

		iCount++;

		if (iCount % 3)
			continue;
		else
			iCount = 0;

//		CWnd *FullModeWnd = NULL;
//		CWnd *ScaleModeWnd = NULL;
//		FullModeWnd = (CWnd*)pThis->m_pFullWndCtrls[0];
//		ScaleModeWnd = (CWnd*)pThis->m_pFullWndCtrls[1];

		if (pThis->m_bFullImageMode) {
			cv::imshow(VIDEO_FULL_WND_NAME, inFrame);
		} else {
			ScaleFrame = &pThis->m_MatFullScaleFrame;
			pThis->VideoScale(&inFrame, ScaleFrame,
				UVC_VIDEO_SCALE_W, UVC_VIDEO_SCALE_H);
			cv::imshow(VIDEO_SCALE_WND_NAME, *ScaleFrame);
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

void CKUVCDlg::SaveFocusRecordToExcel(CString SerialNumber, int TestResult)
{
	// TODO: Add your control notification handler code here
	FILE *pFile;
	bool bFileExists;
	int nFolderLength;
	TCHAR szPath[MAX_PATH] = { 0 };
	CString cstrFolderPath, cstrCSV_Value, strPath, strToken, strSN, strResult, strTime, strError;
	SYSTEMTIME st;

	strSN = L"";
	strSN = SerialNumber;

	//Get system local time
	GetLocalTime(&st);

	//get the execute folder and create "Log" if needed
	GetModuleFileName(NULL, szPath, MAX_PATH);
	strPath = szPath;
	nFolderLength = strPath.ReverseFind('\\');
	strToken.Format(_T("%s\\%s"), strPath.Left(nFolderLength), LOG_DIR_STR);

	if (GetFileAttributes(strToken) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory(strToken, NULL);

	//saveTime.Format(_T("%d/%d/%d %d:%d:%d"), tm.GetYear(), tm.GetMonth(), tm.GetDay(), tm.GetHour(), tm.GetMinute(), tm.GetSecond());
	//strTime.Format(_T("%04d/%02d/%02d %02d:%02d:%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	//printf("%s \r\n", strTime);

	bFileExists = PathFileExists(cstrFolderPath);

	if (TestResult == IDOK)
		strResult = L"PASS";
	else
		strResult = L"FAIL";

	pFile = _wfopen(cstrFolderPath, L"a+, ccs= UTF-16LE");

	//Write data to server
	//m_bTestMode為0是一般模式,會檢查server資料
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


void CKUVCDlg::OnStnClickedStaticVideoSub5()
{
	// TODO: Add your control notification handler code here
}
