// KUVC.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "KUVC.h"
#include "KUVCDlg.h"
#include "MFVersion.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CKUVCApp			

BEGIN_MESSAGE_MAP(CKUVCApp, CWinApp)				
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CKUVCApp construction

CKUVCApp::CKUVCApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CKUVCApp object

CKUVCApp theApp;


// CKUVCApp initialization

BOOL CKUVCApp::InitInstance()
{
	CWinApp::InitInstance();


	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CKUVCDlg *dlg = CKUVCDlg::GetInstance();
	dlg->m_strCaption = "JED100 KUVC ";
#ifdef TestVersion
	dlg->m_strCaption += "T";
#else
	dlg->m_strCaption += "V";
#endif // DEBUG
	dlg->m_strCaption += MF_STR(MF_VERSION_MAJOR);
	dlg->m_strCaption += ".";
	dlg->m_strCaption += MF_STR(MF_VERSION_MINOR);
	dlg->m_strCaption += ".";
	dlg->m_strCaption += MF_STR(MF_VERSION_PATCH);
#ifdef TestVersion
	dlg->m_strCaption += ".";
	dlg->m_strCaption += MF_STR(MF_VERSION_DETAIL);
#endif // DEBUG

	m_pMainWnd = dlg;

#ifdef TestVersion
	MessageBox(m_pMainWnd->GetSafeHwnd(),
		L"軟件測試版不可用於量產\r\n"
		L"Software beta is not available for mass production.",
		L"This PTS is beta",
		MB_ICONWARNING | MB_OK);
#endif
	INT_PTR nResponse = dlg->DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
		::OutputDebugString(L"IDCANCEL");
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

#ifndef _AFXDLL
	ControlBarCleanUp();
#endif

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

