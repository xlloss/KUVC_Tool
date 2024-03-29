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
	// serial port
	BOOL OnSPOpen(int port_num);
	BOOL IsReceiving(void) const;
	BOOL SendSPCmd(const std::wstring &cmd);

	// thread 
	BOOL WaitSPCmdSync(DWORD dwTimeout);
	BOOL CloseReceiverThreadProcedure(void);
	BOOL WaitCloseReceiverThread(DWORD dwTimeout);
	static DWORD WINAPI ReceiverThread(LPVOID lpVoid);
	static DWORD WINAPI TransferThread(LPVOID lpVoid);

	// sync
	void ResetSPCmdSync(void);
	void ReleaseSPCmdSync(void);
	void ResetReceiverEvent(void);

	// serial port
	BOOL ResetSerialPort(void);

	// message event
	afx_msg void OnCbnSelchangeSP();
	afx_msg void OnBnClickedBtnNg();
	afx_msg void OnBnClickedBtnPass();
	afx_msg LRESULT OnReceiveStr(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
	afx_msg LRESULT OnCreateReciverThread(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCreateTransferThread(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCloseDlg(WPARAM wParam, LPARAM lParam);

protected:
	// ICON
	HICON m_hIconOK;
	HICON m_hIconFail;
	HICON m_hIconTBC;

	// serial port
	int	m_iSPNum;
	BOOL m_bSPInitd;
	serial::Serial* m_pSP;
	CComPortCombo m_comboSP;

	// UIs
	CMFCButton m_btnPass;
	CMFCButton m_btnNg;

	// thread 
	BOOL m_bExitReceiverThread;
	HANDLE m_hTransferThread;
	HANDLE m_hRecevierThread;

	// sync
	HANDLE m_hSPCmdSync;
	HANDLE m_hReceived;

public:
	int m_iFocus_mode;
	bool m_iEnableButton;
	CString m_cstrPicSaveName, m_cstrSaveDate; //m_cstrPicSaveName = PCB S/N
	afx_msg void OnEnChangeEditPicName();

	void SaveFocusRecordToExcel();
};
