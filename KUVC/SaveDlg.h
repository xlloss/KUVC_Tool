#pragma once

// CSaveDlg dialog

#include "ComPortCombo.h"
#include "serial/serial.h"

class CSaveDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSaveDlg)

public:
	CSaveDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSaveDlg();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SAVE_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
	DECLARE_MESSAGE_MAP()

protected:

	// message event
	afx_msg void OnBnClickedBtnNg();
	afx_msg void OnBnClickedBtnPass();

	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
	afx_msg LRESULT OnCloseDlg(WPARAM wParam, LPARAM lParam);

protected:
	// ICON
	HICON m_hIconOK;
	HICON m_hIconFail;
	HICON m_hIconTBC;

	// UIs
	CMFCButton m_btnPass;
	CMFCButton m_btnNg;

public:
	int m_iFocus_mode;
	bool m_iEnableButton;

	/* PCB S/N */
	CString m_cstrPicSaveName, m_cstrSaveDate;
	afx_msg void OnEnChangeEditPicName();

	void SaveFocusRecordToExcel();
};
