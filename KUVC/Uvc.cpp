#include "stdafx.h"
#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <vector>
#include <string>
#include <ks.h>
#include <ksproxy.h>
#include <vidcap.h>
#include <ksmedia.h>
#include "Uvc.h"

Uvc::Uvc()
{
	m_pVideoSource = NULL;
	m_pVideoConfig = NULL;
	m_ppVideoDevices = NULL;
	m_pVideoReader = NULL;
	memset(m_UvcName, 0, sizeof(char) * 10);
	m_noOfVideoDevices = 0;
	m_szFriendlyName = NULL;
}

Uvc::~Uvc()
{
}

HRESULT Uvc::GetVideoDevices()
{
	HRESULT hr;

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	MFStartup(MF_VERSION);

	// Create an attribute store to specify the enumeration parameters.
	hr = MFCreateAttributes(&m_pVideoConfig, 1);
	CHECK_HR_RESULT(hr, "Create attribute store");

	// Source type: video capture devices
	hr = m_pVideoConfig->SetGUID(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
	);
	CHECK_HR_RESULT(hr, "Video capture device SetGUID");

	// Enumerate devices.
	hr = MFEnumDeviceSources(m_pVideoConfig, &m_ppVideoDevices, &m_noOfVideoDevices);
	CHECK_HR_RESULT(hr, "Device enumeration");

done:
	return hr;
}

HRESULT Uvc::GetVideoDeviceFriendlyNames(int deviceIndex)
{
	// Get the the device friendly name.
	UINT32 cchName;
	HRESULT hr;

	 hr = m_ppVideoDevices[deviceIndex]->GetAllocatedString(
		MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
		&m_szFriendlyName, &cchName);

	CHECK_HR_RESULT(hr, "Get video device friendly name");

done:
	return hr;
}

HRESULT Uvc::InitVideoDevice(int deviceIndex)
{
	HRESULT hr;

	hr = m_ppVideoDevices[deviceIndex]->ActivateObject(IID_PPV_ARGS(&m_pVideoSource));
	CHECK_HR_RESULT(hr, "Activating video device");

	// Create a source reader.
	hr = MFCreateSourceReaderFromMediaSource(m_pVideoSource, m_pVideoConfig, &m_pVideoReader);
	CHECK_HR_RESULT(hr, "Creating video source reader");

done:
	return hr;
}

HRESULT Uvc::SetGetExtensionUnit(GUID xuGuid, DWORD dwExtensionNode,
	ULONG xuPropertyId, ULONG flags, void *data, int len, ULONG *readCount)
{
	GUID pNodeType;
	IUnknown *unKnown;
	IKsControl * ks_control = NULL;
	IKsTopologyInfo * pKsTopologyInfo = NULL;
	KSP_NODE kspNode;

	HRESULT hr = m_pVideoSource->QueryInterface(__uuidof(IKsTopologyInfo), (void **)&pKsTopologyInfo);
	CHECK_HR_RESULT(hr, "IMFMediaSource::QueryInterface(IKsTopologyInfo)");

	hr = pKsTopologyInfo->get_NodeType(dwExtensionNode, &pNodeType);
	CHECK_HR_RESULT(hr, "IKsTopologyInfo->get_NodeType(...)");

	hr = pKsTopologyInfo->CreateNodeInstance(dwExtensionNode, IID_IUnknown, (LPVOID *)&unKnown);
	CHECK_HR_RESULT(hr, "ks_topology_info->CreateNodeInstance(...)");

	hr = unKnown->QueryInterface(__uuidof(IKsControl), (void **)&ks_control);
	CHECK_HR_RESULT(hr, "ks_topology_info->QueryInterface(...)");

	kspNode.Property.Set = xuGuid;					// XU GUID
	kspNode.NodeId = (ULONG)dwExtensionNode;		// XU Node ID
	kspNode.Property.Id = xuPropertyId;				// XU control ID
	kspNode.Property.Flags = flags;					// Set/Get request

	hr = ks_control->KsProperty((PKSPROPERTY)&kspNode, sizeof(kspNode),
		(PVOID)data, len, readCount);
	CHECK_HR_RESULT(hr, "ks_control->KsProperty(...)");

done:
	SafeRelease(&ks_control);
	return hr;
}

HRESULT Uvc::FindExtensionNode(IKsTopologyInfo* pIksTopologyInfo, DWORD* pNodeId)
{
	DWORD numberOfNodes;
	HRESULT hResult = S_FALSE;

	hResult = pIksTopologyInfo->get_NumNodes(&numberOfNodes);
	if (SUCCEEDED(hResult)) {
		DWORD i;
		GUID nodeGuid;

		for (i = 0; i < numberOfNodes; i++) {
			if (SUCCEEDED(pIksTopologyInfo->get_NodeType(i, &nodeGuid))) {
				/* found the extension node */
				if (IsEqualGUID(KSNODETYPE_DEV_SPECIFIC, nodeGuid)) {
					*pNodeId = i;
					return S_OK;
				}
			}
		}

		if (i == numberOfNodes)
			hResult = S_FALSE;
	}

	return hResult;
}


BOOL Uvc::GetNodeId(int* pNodeId)
{
	IKsTopologyInfo *pKsToplogyInfo;
	HRESULT hResult;
	DWORD dwNode;

	hResult = m_pVideoSource->QueryInterface(__uuidof(IKsTopologyInfo),
		(void **)&pKsToplogyInfo);
	if (S_OK == hResult) {
		hResult = FindExtensionNode(pKsToplogyInfo, &dwNode);
		pKsToplogyInfo->Release();
		if (S_OK == hResult) {
			*pNodeId = dwNode;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Uvc::WriteXu(GUID xuGuid, unsigned int xu_cmd, BYTE *data, unsigned int datalen)
{
	ULONG flags, readCount;
	BYTE *xu_data;
	HRESULT hr;
	unsigned int xu_data_len;

	xu_data = data;
	xu_data_len = datalen;
	if (xu_data_len <= 0)
		return FALSE;

	flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
	hr = SetGetExtensionUnit(xuGuid, m_NodeId, xu_cmd,
		flags, (void *)xu_data, xu_data_len, &readCount);
	if (hr != S_OK)
		return FALSE;

	/* printf("Found UVC extension unit\n"); */
	/* printf("\nSend Device LED command\r\n"); */

	return TRUE;
}

BOOL Uvc::ReadXu(GUID xuGuid, unsigned int xu_cmd, BYTE *data, unsigned int datalen)
{
	ULONG flags, readCount;
	//BYTE data[10] = {0}; //Write value
	BYTE *xu_data;
	unsigned int xu_data_len;
	HRESULT hr;

	xu_data = data;
	xu_data_len = datalen;
	if (xu_data_len <= 0)
		return FALSE;

	flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
	hr = SetGetExtensionUnit(xuGuid, m_NodeId, xu_cmd,
		flags, (void *)xu_data, xu_data_len, &readCount);
	if (hr != S_OK) {
		AfxMessageBox(_T("KSPROPERTY_TYPE_GET Fail"));
		return FALSE;
	}

	/* printf("Found UVC extension unit\n"); */
	/* printf("\nSend Device LED command\r\n"); */

	return TRUE;
}

BOOL Uvc::Set_DevName(char *device_name)
{
	if (!device_name)
		return FALSE;

	strcpy(m_UvcName, device_name);
	return TRUE;
}

void Uvc::Uvc_Close()
{
	for (UINT32 i = 0; i < m_noOfVideoDevices; i++)
		SafeRelease(&m_ppVideoDevices[i]);

	CoTaskMemFree(m_ppVideoDevices);
	SafeRelease(&m_pVideoConfig);
	SafeRelease(&m_pVideoSource);

}


BOOL Uvc::Uvc_Init()
{
	HRESULT hr;
	BOOL ret = TRUE;
	CHAR videoDevName[20][MAX_PATH];
	size_t returnValue;
	UINT32 selectedVal = 0xFFFFFFFF;

	hr = GetVideoDevices();
	if (hr != S_OK)
		return FALSE;

	/* printf("Video Devices connected:\n"); */
	for (UINT32 i = 0; i < m_noOfVideoDevices; i++) {
		//Get the device names
		GetVideoDeviceFriendlyNames(i);
		wcstombs_s(&returnValue, videoDevName[i], MAX_PATH, m_szFriendlyName, MAX_PATH);
		/* printf("%d: %s\n", i, videoDevName[i]); */

		//Store the App note firmware (Whose name is *FX3*) device index  
		if (!(strcmp(videoDevName[i], m_UvcName)))
			selectedVal = i;
	}

	if (selectedVal == 0xFFFFFFFF)
		return FALSE;

	/* printf("\nFound device\n"); */
	//Initialize the selected device
	InitVideoDevice(selectedVal);
	GetNodeId(&m_NodeId);

	return TRUE;
}
