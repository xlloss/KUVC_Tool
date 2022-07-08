#pragma once
#include "stdafx.h"
#include <afxdb.h>

class XmlProcess
{
	void ProcessNode(CComPtr<IXMLDOMNode>& spNode);

	CComPtr<IXMLDOMDocument> spDoc; //DOM
	CComPtr<IXMLDOMElement> spRootEle;
	CComPtr<IXMLDOMNode> spNewNode;
	CComPtr<IXMLDOMNode> spNode;

	public:
		void Init();
		void Read();
		void Write();
};


