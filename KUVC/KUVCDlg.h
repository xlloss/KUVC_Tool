// KUVCDlg.h : header file
//

#pragma once

// CDatabase include
#include <afxdb.h>

// opencv include
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/videoio/legacy/constants_c.h>
#include <Uvc.h>

//#include "ComPortCombo.h"
//#include "serial/serial.h"

#define LOG_DIR_STR			_T("Log")
#define LOG_FAIL_DIR_STR	_T("Fail")

#define XN_MyDBName         _T("TESTDB1")

using namespace cv;
using namespace std;

struct HID_INFO
{
	CString strSymbolinkName;
	CString	strPortName;
};

// CKUVCDlg dialog
class CKUVCDlg : public CDialogEx
{
// Construction
public:
	CKUVCDlg(CWnd* pParent = NULL);	// standard constructor
	~CKUVCDlg();

// Dialog Data
	enum { IDD = IDD_KUVC_DIALOG };

protected:
	HICON m_hIcon;
	HANDLE m_hCaptured;
	HANDLE m_hCapturing;
	int m_iSavePic;
	int m_CamId = 0;
	double m_dFullWndMaxLpVal;
	double m_dFullWndCurLpVal;
	bool m_bCaptureFrame;
	bool m_bCloseCapture;
	bool m_bFindCam;
	bool m_bStopCpature;
	bool m_bFullImageMode;
	cv::Mat m_UvcFrame;
	cv::Mat m_MatFullFrame;
	cv::Mat m_MatFullScaleFrame;

	CPtrArray m_pStaticWndCtrls;
	CPtrArray m_pVideoDeviceComboCtrls;
	CPtrArray m_pFullWndCtrls;
	CTabCtrl m_TabCtrl;

	void VideoScale(Mat *SrcFrame, Mat *DstFrame, int width, int high);
	void RegisterDevNotification(HANDLE *hWind);
	void ShowVideoFullWnd(BOOL Enable);
	void ShowVideoScaleWnd(BOOL Enable);

	static CKUVCDlg* s_pThis;
	static DWORD WINAPI CaptureVideoThread(LPVOID lpVoid);

	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()

public:
	class Uvc *UvcCtl;
	CString m_strCaption;
	static CKUVCDlg* GetInstance();

	bool m_bTestMode;
	CDatabase m_DataBase;

	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
	afx_msg void OnBnClickedCancel();
	//afx_msg void OnBnClickedStart();
	afx_msg void OnSetVideoFullWnd();
	afx_msg void OnSetVideoScaleWnd();
	afx_msg void OnCbnSelchangeVideoDevice();
	afx_msg void OnCbnDropDownVideoDevice();
	afx_msg void OnTcnSelchangeTabVideoSwitch(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnSaveData(WPARAM wParam, LPARAM lParam);

	void Init();
	bool IsCapturing(void) const;
	void CreateCaptureProcedure(void);
	bool CloseCaptureProcedure(void);
	bool WaitCloseCapture(DWORD Timeout);
	void ReleaseCaptureEvent(void);
	void LockUIs(BOOL bLock);
	void LoadConfigSetting(void);
	bool ConnectToServer();
	int GetSubWndMinValue();
	void SaveFocusRecordToExcel(CString SerialNumbe, int TestResult);
	void DebugLog(char *pData, int nLen, bool bHex);

	friend void onMouseFullWnd(int Event, int x, int y, int flags, void* param);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
