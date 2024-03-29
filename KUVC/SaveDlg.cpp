// CSaveDlg.cpp : implementation file
//
#include "stdafx.h"

#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <cstdio>

#include "KUVC.h"
#include "SaveDlg.h"
#include "afxdialogex.h"
#include "KUVCDlg.h"

#include "xnBase.h"

// serial port
#include "serial/serial.h"

using std::string;
using std::exception;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;

// CSaveDlg dialog

IMPLEMENT_DYNAMIC(CSaveDlg, CDialogEx)

CSaveDlg::CSaveDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SAVE_DLG, pParent)
{
	// serial port
	m_pSP = NULL;
	m_iSPNum = 0;
	m_bSPInitd = FALSE;

	// thread 
	m_hTransferThread = NULL;
	m_hRecevierThread = NULL;

	// sync
	m_hSPCmdSync = NULL;
	m_hReceived = NULL;

	m_hIconOK = AfxGetApp()->LoadIcon(IDI_OK);
	m_hIconFail = AfxGetApp()->LoadIcon(IDI_FAIL);
	m_hIconTBC = AfxGetApp()->LoadIcon(IDI_TBC);
}

CSaveDlg::~CSaveDlg()
{
	if (m_hSPCmdSync) {
		CloseHandle(m_hSPCmdSync);
		m_hSPCmdSync = NULL;
	}

	if (m_hReceived) {
		CloseHandle(m_hReceived);
		m_hReceived = NULL;
	}

	CloseReceiverThreadProcedure();

	if (m_pSP) {
		delete m_pSP;
		m_pSP = NULL;
	}
}

BOOL CSaveDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// focus to edit
	GotoDlgCtrl(GetDlgItem(IDC_EDIT_PIC_NAME));

	// set button color
	m_btnPass.EnableWindowsTheming(FALSE);	// (wichtig)
	m_btnPass.SetFaceColor(RGB(0, 255, 0));	// Change to your desired Background Color
	m_btnNg.EnableWindowsTheming(FALSE);	// (wichtig)
	m_btnNg.SetFaceColor(RGB(255, 0, 0));	// Change to your desired Background Color

	// init comport list
	m_comboSP.InitList();
	m_iSPNum = AfxGetApp()->GetProfileInt(_T("Setting"), _T("JED100PortNum"), 0);
	OnSPOpen(m_iSPNum);

	CKUVCDlg *dlg = CKUVCDlg::GetInstance();

	if (m_iEnableButton)
		GetDlgItem(IDC_BTN_PASS)->EnableWindow(true);
	else
		GetDlgItem(IDC_BTN_PASS)->EnableWindow(false);

	UpdateData(FALSE);

	return TRUE;
}

void CSaveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_NG, m_btnNg);
	DDX_Control(pDX, IDC_BTN_PASS, m_btnPass);
	DDX_Control(pDX, IDC_COMBO_SP, m_comboSP);
}

BEGIN_MESSAGE_MAP(CSaveDlg, CDialogEx)
	ON_WM_DEVICECHANGE()
	ON_BN_CLICKED(IDC_BTN_NG, &CSaveDlg::OnBnClickedBtnNg)
	ON_BN_CLICKED(IDC_BTN_PASS, &CSaveDlg::OnBnClickedBtnPass)
	ON_CBN_SELCHANGE(IDC_COMBO_SP, OnCbnSelchangeSP)
	ON_MESSAGE(WM_USER_CREATE_TRANSFER_THREAD, OnCreateTransferThread)
	ON_MESSAGE(WM_USER_CREATE_RECEIVER_THREAD, OnCreateReciverThread)
	ON_MESSAGE(WM_USER_SP_RX_STRING, OnReceiveStr)
	ON_MESSAGE(WM_USER_EXIT_DLG, OnCloseDlg)
	ON_EN_CHANGE(IDC_EDIT_PIC_NAME, &CSaveDlg::OnEnChangeEditPicName)
END_MESSAGE_MAP()

// CSaveDlg message handlers

BOOL CSaveDlg::PreTranslateMessage(MSG* pMsg)
{	
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
		{
			return TRUE;                // Do not process further
		}
	}

	if ((pMsg->wParam == VK_LBUTTON) && (GetDlgItem(IDC_EDIT_PIC_NAME)->m_hWnd == pMsg->hwnd) && !(GetDlgItem(IDC_EDIT_PIC_NAME)->GetStyle() & ES_READONLY)) {
		GetDlgItem(IDC_EDIT_PIC_NAME)->SetWindowText(L"");
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

LRESULT CSaveDlg::OnCloseDlg(WPARAM wParam, LPARAM lParam)
{
	if (m_cstrPicSaveName.IsEmpty())
	{
		m_cstrPicSaveName = m_cstrSaveDate;
	}

	CDialogEx::OnOK();
	return TRUE;
}

void CSaveDlg::ResetSPCmdSync(void)
{
	ResetEvent(m_hSPCmdSync);
}

BOOL CSaveDlg::SendSPCmd(const std::wstring &cmd)
{
	// send cmd
	if (m_pSP) {
		// reset sync
		ResetSPCmdSync();

		// send cmd
		std::string str(cmd.begin(), cmd.end());

		try {
			m_pSP->write(str);
		}catch (serial::IOException& e) {
			ResetSerialPort();
			cerr << e.what();
		}
		
		Sleep(XN_SP_COMM_SLEEP_MS);

		// wait sync 
		if (FALSE == WaitSPCmdSync(XN_SP_SYNC_TIMEOUT_MS)) {
			::MessageBox(GetSafeHwnd(), _T("Communication error, please check comport"), XN_SP_ERR_STR, MB_ICONSTOP);
			return FALSE;
		}
	}else {
		::MessageBox(GetSafeHwnd(), _T("Communication error, please check comport"), XN_SP_ERR_STR, MB_ICONSTOP);
		return FALSE;
	}

	return TRUE;
}

void CSaveDlg::OnBnClickedBtnPass()
{
	// TODO: Add your control notification handler code here
	if (TRUE == m_bSPInitd) {
		SendMessage(WM_USER_CREATE_TRANSFER_THREAD, NULL, NULL);
	}else {
		SendMessage(WM_USER_EXIT_DLG, NULL, NULL);
	}
}

void CSaveDlg::OnBnClickedBtnNg()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnCancel();
	/*
	if (TRUE == m_bSPInitd) {
		SendMessage(WM_USER_CREATE_TRANSFER_THREAD, NULL, NULL);
	}
	else {
		SendMessage(WM_USER_EXIT_DLG, NULL, NULL);
	}
	*/
}

void CSaveDlg::OnCbnSelchangeSP()
{
	// TODO: Add your control notification handler code here
	int nPortNum = m_comboSP.GetPortNum();
	OnSPOpen(nPortNum);
}

BOOL CSaveDlg::WaitCloseReceiverThread(DWORD dwTimeout)
{
	return (WaitForSingleObject(m_hReceived, dwTimeout) == WAIT_TIMEOUT) ? FALSE : TRUE;
}

BOOL CSaveDlg::WaitSPCmdSync(DWORD dwTimeout)
{
	return (WaitForSingleObject(m_hSPCmdSync, dwTimeout) == WAIT_TIMEOUT) ? FALSE : TRUE;
}

BOOL CSaveDlg::CloseReceiverThreadProcedure(void)
{
	if (IsReceiving()) {
		m_bExitReceiverThread = TRUE; // flag for receiver thread
		Sleep(200); // for thread do-event?
		if (FALSE == WaitCloseReceiverThread(3000)) {
			return FALSE; // timeout
		}
	}
	return TRUE;
}

BOOL CSaveDlg::IsReceiving(void) const
{
	if (m_hRecevierThread) {
		return TRUE;
	}
	return FALSE;
}

BOOL CSaveDlg::OnSPOpen(int port_num)
{
	BOOL ret = FALSE;

	// close receiver thread.
	if (FALSE == CloseReceiverThreadProcedure()) {
		::MessageBox(this->GetSafeHwnd(), _T("Close Serial port fail..."), _T("ERROR"), MB_ICONSTOP);
	}

	// close serial port handel
	if (m_pSP) {
		delete m_pSP;
		m_pSP = NULL;
	}

	CKUVCDlg *dlg = CKUVCDlg::GetInstance();

	// check exist serial port and try to open serial by port_num
	if ((0 != port_num) && (m_comboSP.GetCount() > 1)) {
		// serial port init
		printf("port_num = %d \r\n", port_num);
		((CEdit*)GetDlgItem(IDC_EDIT_PIC_NAME))->SetReadOnly(true);

		char buf[16];
		sprintf_s(buf, 16, "COM%d", port_num);
		std::stringstream ss;
		ss << buf;
		std::string port = ss.str();

		try {
			m_pSP = new serial::Serial(port, XN_SP_BR, serial::Timeout::simpleTimeout(250));
			m_pSP->setTimeout(serial::Timeout::max(), 250, 0, 250, 0);
		}
		catch (exception &e) {
			cerr << "Unhandled Exception: " << e.what() << endl;
			((CStatic*)GetDlgItem(IDC_ICON_SP))->SetIcon(m_hIconFail);
			return FALSE;
		}

		if ((m_pSP != NULL) && m_pSP->isOpen()) {
			// init success
			printf("m_pSP->isOpen() \r\n");
			m_bSPInitd = TRUE;
			m_iSPNum = port_num;
			m_comboSP.InitList(port_num);
			((CStatic*)GetDlgItem(IDC_ICON_SP))->SetIcon(m_hIconOK);
			AfxGetApp()->WriteProfileInt(_T("Setting"), _T("JED100PortNum"), port_num);
			SendMessage(WM_USER_CREATE_RECEIVER_THREAD, NULL, NULL);
			ret = TRUE;
		}
	}
	else {
		// clear serial port 
		printf("clear serial port \r\n");
		m_iSPNum = 0;
		m_bSPInitd = FALSE;
		m_comboSP.InitList(0);
		((CStatic*)GetDlgItem(IDC_ICON_SP))->SetIcon(m_hIconFail);
		((CEdit*)GetDlgItem(IDC_EDIT_PIC_NAME))->SetReadOnly(false);
		AfxGetApp()->WriteProfileInt(_T("Setting"), _T("JED100PortNum"), 0);
		ret = FALSE;
	}

	return ret;
}

// send serial command threads
LRESULT CSaveDlg::OnCreateTransferThread(WPARAM wParam, LPARAM lParam)
{
	DWORD dwThreadId;
	m_hSPCmdSync = CreateEvent(NULL, TRUE, FALSE, _T("SerialPortCmdSync"));
	m_hTransferThread = ::CreateThread(NULL, 0, TransferThread, (PVOID)this, 0, &dwThreadId);
	if (m_hTransferThread == NULL) {
		::MessageBox(this->GetSafeHwnd(), _T("Please try again !"), XN_THD_ERR_STR, MB_ICONSTOP);
	}
	return TRUE;
}

// serial port send command thread
DWORD WINAPI CSaveDlg::TransferThread(LPVOID lpVoid)
{
	bool bExitSaveDlg = false;
	CString strTargetCmd /*, cstrPicName*/;

	CSaveDlg* pThis = (CSaveDlg*)lpVoid;
	CKUVCDlg *dlg = CKUVCDlg::GetInstance();

	//寫入調焦日期或中心校正數值到JED100

	//pThis->GetDlgItem(IDC_EDIT_PIC_NAME)->GetWindowText(cstrPicName);
	SYSTEMTIME st;
	GetLocalTime(&st); //Get system local time
	pThis->m_cstrSaveDate.Format(_T("%04d/%02d/%02d %02d:%02d:%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	//pThis->m_cstrSaveDate = CTime::GetCurrentTime().Format("%Y%m%d%H%M%S");
	strTargetCmd.Format(L"xn Calib fdate_w %s\n", pThis->m_cstrSaveDate);

	TCHAR strArry[64] = { 0 };
	_stprintf_s(strArry, sizeof(strArry) / sizeof(TCHAR), _T("%s"), strTargetCmd.AllocSysString());
	if (FALSE == pThis->SendSPCmd(strArry)) {
		goto _exit;
	}

	//從JED100讀取調焦日期,確認有寫入
	if (FALSE == pThis->SendSPCmd(L"xn Calib fdate_r\n")) {
		((CStatic*)pThis->GetDlgItem(IDC_ICON_FILE_NAME_FLASH))->SetIcon(pThis->m_hIconFail);
		goto _exit;
	}else {
		CString cstrPicNameFlash;
		pThis->GetDlgItem(IDC_EDIT_FILE_NAME_FLASH)->GetWindowText(cstrPicNameFlash);
		if (pThis->m_cstrSaveDate == cstrPicNameFlash) {
			((CStatic*)pThis->GetDlgItem(IDC_ICON_FILE_NAME_FLASH))->SetIcon(pThis->m_hIconOK);
			::MessageBox(pThis->GetSafeHwnd(), _T("Save success!"), XN_RESULT_STR, MB_ICONINFORMATION);
			bExitSaveDlg = true;
		}else {
			((CStatic*)pThis->GetDlgItem(IDC_ICON_FILE_NAME_FLASH))->SetIcon(pThis->m_hIconFail);
			::MessageBox(pThis->GetSafeHwnd(), _T("Save fail, please try again"), XN_RESULT_STR, MB_ICONERROR);
			bExitSaveDlg = false;
		}
	}


_exit:
	// close thread
	pThis->m_hTransferThread = NULL;

	if (pThis->m_hSPCmdSync) {
		::CloseHandle(pThis->m_hSPCmdSync);
		pThis->m_hSPCmdSync = NULL;
	}

	// close this dialog
	if (true == bExitSaveDlg) {
		pThis->SendMessage(WM_USER_EXIT_DLG, NULL, NULL);
	}

	return TRUE;
}

// communication threads
LRESULT CSaveDlg::OnCreateReciverThread(WPARAM wParam, LPARAM lParam)
{
	DWORD dwThreadId;
	m_hReceived = CreateEvent(NULL, TRUE, FALSE, _T("SerialReceived"));
	m_hRecevierThread = ::CreateThread(NULL, 0, ReceiverThread, (PVOID)this, 0, &dwThreadId);
	if (m_hRecevierThread == NULL) {
		::MessageBox(this->GetSafeHwnd(), _T("Please try again !"), XN_THD_ERR_STR, MB_ICONSTOP);
	}
	return TRUE;
}

DWORD WINAPI CSaveDlg::ReceiverThread(LPVOID lpVoid)
{
	CSaveDlg* pThis = (CSaveDlg*)lpVoid;

	while (1)
	{
		if (TRUE == pThis->m_bExitReceiverThread) { // exit 
			pThis->m_bExitReceiverThread = FALSE;
			break;
		}
		
		if ((pThis->m_pSP != NULL) && (pThis->m_pSP->isOpen())) 
		{	
			//prevent exception when remove serial port in receiving
			try {
				string result = pThis->m_pSP->read(XN_SP_READ_NUM);
				
				if (0 != result.length()) {
					LPCSTR str = result.c_str();
					pThis->SendMessage(WM_USER_SP_RX_STRING, (WPARAM)str, NULL);
					cout <<"receive ..." <<str << endl;
				}
			}catch (serial::IOException& e) {
				pThis->ResetSerialPort();
				cerr << e.what();
				break;
			}
		}
		Sleep(XN_SP_COMM_SLEEP_MS);
	}
	pThis->ResetReceiverEvent();
	return TRUE;
}

BOOL CSaveDlg::ResetSerialPort(void)
{
	m_iSPNum = 0;
	m_bSPInitd = FALSE;
	m_comboSP.InitList(0);
	return TRUE;
}

void CSaveDlg::ResetReceiverEvent(void)
{
	m_hRecevierThread = NULL;
	SetEvent(m_hReceived);
}

LRESULT CSaveDlg::OnReceiveStr(WPARAM wParam, LPARAM lParam)
{
	CStringA cstrReceive;
	cstrReceive.Format(("%s"), (char*)wParam);

	// is "Calib " command?
	CStringA str_target = "Calib:";
	int nTokenPos = -1;
	nTokenPos = cstrReceive.Find(str_target);
	if (-1 != nTokenPos) {
		// stract the "Calib "
		cstrReceive.Delete(0, nTokenPos + str_target.GetLength());

		char cmd[16] = { 0 };
		char param1[32] = { 0 };
		// get CMD and parameter
		sscanf(cstrReceive, "%[^\r\n:]:%[^\r\n:]", cmd, param1);

		// CMD execute
		if (0 == strcmp(cmd, "fdate_r")) {
			printf("cmd fdate_r -------------- \r\n");
			CString cstr(param1);
			GetDlgItem(IDC_EDIT_FILE_NAME_FLASH)->SetWindowText(cstr);
			ReleaseSPCmdSync();
		}
		else if (0 == strcmp(cmd, "pcbsn_r")) {
			printf("cmd pcbsn_r -------------- \r\n");
			CString cstr(param1);
			GetDlgItem(IDC_EDIT_PIC_NAME)->SetWindowText(cstr);
			ReleaseSPCmdSync();
		}else if (0 == strcmp(cmd, "date_w")) {
			ReleaseSPCmdSync();
		}
		else if (0 == strcmp(cmd, "center_w")) { //center_w只在lens_shitf_calibration = 1發送
			printf("transfer ceter data success\r\n");
			ReleaseSPCmdSync();
			::MessageBox(this->GetSafeHwnd(), _T("Save success!"), _T("success"), MB_ICONINFORMATION);
			//SaveFocusRecordToExcel(); //只在成功執行
		}
	}

	return TRUE;
}

void CSaveDlg::ReleaseSPCmdSync(void)
{
	SetEvent(m_hSPCmdSync);
}

BOOL CSaveDlg::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	BOOL bResult = CDialog::OnDeviceChange(nEventType, dwData);

	m_comboSP.InitList();

	return bResult;
}

void CSaveDlg::OnEnChangeEditPicName()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	GetDlgItem(IDC_EDIT_PIC_NAME)->GetWindowText(m_cstrPicSaveName);
}

void CSaveDlg::SaveFocusRecordToExcel() //只儲存中心校正資料
{
	// TODO: Add your control notification handler code here
	FILE *pFile;
	bool bFileExists;
	int nFolderLength;
	TCHAR szPath[MAX_PATH] = { 0 };
	CString cstrFolderPath, cstrCSV_Value, strPath, strToken, strSN, strResult, strTime, strError;
	SYSTEMTIME st;

	CKUVCDlg *dlg = CKUVCDlg::GetInstance();

	//get the execute folder and create "Log" if needed
	GetModuleFileName(NULL, szPath, MAX_PATH);
	strPath = szPath;
	nFolderLength = strPath.ReverseFind('\\');
	strToken.Format(_T("%s\\%s"), strPath.Left(nFolderLength), LOG_DIR_STR);

	if (GetFileAttributes(strToken) == INVALID_FILE_ATTRIBUTES) {
		CreateDirectory(strToken, NULL);
	}

	GetLocalTime(&st); //Get system local time

	//strTime.Format(_T("%d/%d/%d %d:%d:%d"), tm.GetYear(), tm.GetMonth(), tm.GetDay(), tm.GetHour(), tm.GetMinute(), tm.GetSecond());
	strTime.Format(_T("%04d/%02d/%02d %02d:%02d:%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	printf("%S \r\n", strTime);

	bFileExists = PathFileExists(cstrFolderPath);

	//if (TestResult == IDOK)
	//	saveResult = L"PASS";
	//else
	//	saveResult = L"FAIL";

	strResult = L"PASS";

	pFile = _wfopen(cstrFolderPath, L"a+, ccs= UTF-16LE");

	if (pFile)
	{
		if (!bFileExists)
		{
			cstrCSV_Value.Format(_T("PCB SN\tup\tleft\tcenter\tright\tdown\tresult\ttime\tcenter_x\tcenter_y\n"));
			fwrite(cstrCSV_Value, sizeof(wchar_t), wcslen(cstrCSV_Value), pFile);
		}
		CSaveDlg sdlg;
		fwrite(cstrCSV_Value, sizeof(wchar_t), wcslen(cstrCSV_Value), pFile);
		fclose(pFile);
	}
}

