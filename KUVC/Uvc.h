#pragma once

#include "KUVC.h"
#include "KUVCDlg.h"

#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <vector>
#include <string>
#include <ks.h>
#include <ksproxy.h>
#include <vidcap.h>


#define CHECK_HR_RESULT(hr, msg, ...) \
if (hr != S_OK) \
{\
printf("info: Function: %s, %s failed, Error code: 0x%.2x \n", \
__FUNCTION__, msg, hr, __VA_ARGS__); \
goto done; \
}

//Templates for the App
template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

class Uvc
{

	public:
		Uvc();
		~Uvc();

		BOOL ReadXu(GUID xuGuid, unsigned int xu_cmd, BYTE *data, unsigned int datalen);
		BOOL WriteXu(GUID xuGuid, unsigned int xu_cmd, BYTE *data, unsigned int datalen);
		BOOL Set_DevName(char *device_name);
		BOOL Uvc_Init();
		void Uvc_Close();

	private:
		HRESULT GetVideoDevices();
		HRESULT GetVideoDeviceFriendlyNames(int deviceIndex);
		HRESULT InitVideoDevice(int deviceIndex);
		HRESULT SetGetExtensionUnit(GUID xuGuid, DWORD dwExtensionNode,
			ULONG xuPropertyId, ULONG flags, void *data, int len, ULONG *readCount);

		HRESULT FindExtensionNode(IKsTopologyInfo* pIksTopologyInfo, DWORD* pNodeId);
		BOOL GetNodeId(int *pNodeId);

		//Media foundation and DSHOW specific structures, class and variables
		IMFMediaSource *m_pVideoSource;
		IMFAttributes *m_pVideoConfig;
		IMFActivate **m_ppVideoDevices;
		IMFSourceReader *m_pVideoReader;
		char m_UvcName[10];

		//Other variables
		UINT32 m_noOfVideoDevices;
		WCHAR *m_szFriendlyName;
		int m_NodeId;
};

