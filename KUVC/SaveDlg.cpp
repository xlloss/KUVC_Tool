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
#include "MSAccess.h"

using std::string;
using std::exception;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;


IMPLEMENT_DYNAMIC(CSaveDlg, CDialogEx)

#define RESULT_PASS _T("PASS")
#define RESULT_FAIL _T("FAIL")
#define MS_DATABASE_FILE_NAME _T("D:\\test.mdb")
#define DB_NAME _T("AB003 ")
#define DB_FIELD_0 _T("ID")
#define DB_FIELD_1 _T("DATE_TIME")
#define DB_FIELD_2 _T("SN")
#define DB_FIELD_3 _T("RESULT")


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
	GotoDlgCtrl(GetDlgItem(IDC_EDIT_SN));

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
END_MESSAGE_MAP()

// CSaveDlg message handlers

BOOL CSaveDlg::PreTranslateMessage(MSG* pMsg)
{	
	if (pMsg->message == WM_KEYDOWN) {
		if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE) {
			return TRUE;
		}
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
	SaveToMsDb(TRUE);
	SendMessage(WM_USER_EXIT_DLG, NULL, NULL);

}

void CSaveDlg::OnBnClickedBtnNg()
{
	SaveToMsDb(FALSE);
	CDialogEx::OnCancel();
	/* SendMessage(WM_USER_EXIT_DLG, NULL, NULL); */
}

BOOL CSaveDlg::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	BOOL bResult = CDialog::OnDeviceChange(nEventType, dwData);
	return bResult;
}

void CSaveDlg::SaveToMsDb(BOOL TestResult)
{
	bool bFileExists;
	int nFolderLength;
	TCHAR szPath[MAX_PATH] = { 0 };
	CString cstrFolderPath, cstrCSV_Value, strPath, strToken, strSN, strTime, strError;
	CString saveResult;
	SYSTEMTIME st;
	CKUVCDlg* dlg = CKUVCDlg::GetInstance();


	//get the execute folder and create "Log" if needed
	GetModuleFileName(NULL, szPath, MAX_PATH);
	strPath = szPath;
	nFolderLength = strPath.ReverseFind('\\');
	strToken.Format(_T("%s\\%s"), strPath.Left(nFolderLength), LOG_DIR_STR);

	if (GetFileAttributes(strToken) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory(strToken, NULL);

	GetLocalTime(&st);

	strTime.Format(_T("%04d/%02d/%02d %02d:%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
	
	bFileExists = PathFileExists(cstrFolderPath);

	if (TestResult == IDOK)
		saveResult.Append(RESULT_PASS);
	else
		saveResult.Append(RESULT_FAIL);

	GetDlgItem(IDC_EDIT_SN)->GetWindowTextW(strSN);

	MSAccess* ms_access = new MSAccess();
	CString db_name, field_name, field_value;
	int data_total;

	ms_access->Init(MS_DATABASE_FILE_NAME, DB_NAME);
	ms_access->Connect();
	data_total = ms_access->GetDataNum();

	db_name.Append(DB_NAME);
	field_name.Format(_T("(%s,%s,%s,%s) "), DB_FIELD_0, DB_FIELD_1, DB_FIELD_2, DB_FIELD_3);
	field_value.Format(_T("VALUES (%d,'%s','%s','%s')"), data_total, strTime, strSN, saveResult);

	ms_access->Write(db_name, field_name, field_value);
	ms_access->DisConnect();
	delete ms_access;
}