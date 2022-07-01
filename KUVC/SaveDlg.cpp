// CSaveDlg.cpp : implementation file
//
#include "stdafx.h"
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <cstdio>
#include "KUVC.h"
#include "KUVCDlg.h"
#include "SaveDlg.h"
#include "afxdialogex.h"
#include "xnBase.h"

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
	m_hIconOK = AfxGetApp()->LoadIcon(IDI_OK);
	m_hIconFail = AfxGetApp()->LoadIcon(IDI_FAIL);
	m_hIconTBC = AfxGetApp()->LoadIcon(IDI_TBC);
}

CSaveDlg::~CSaveDlg()
{

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

}

BEGIN_MESSAGE_MAP(CSaveDlg, CDialogEx)
	ON_WM_DEVICECHANGE()
	ON_BN_CLICKED(IDC_BTN_NG, &CSaveDlg::OnBnClickedBtnNg)
	ON_BN_CLICKED(IDC_BTN_PASS, &CSaveDlg::OnBnClickedBtnPass)
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

void CSaveDlg::OnBnClickedBtnPass()
{
	// TODO: Add your control notification handler code here

	SendMessage(WM_USER_EXIT_DLG, NULL, NULL);

}

void CSaveDlg::OnBnClickedBtnNg()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnCancel();
	/* SendMessage(WM_USER_EXIT_DLG, NULL, NULL); */
}

BOOL CSaveDlg::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	BOOL bResult = CDialog::OnDeviceChange(nEventType, dwData);
	return bResult;
}

void CSaveDlg::OnEnChangeEditPicName()
{
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

