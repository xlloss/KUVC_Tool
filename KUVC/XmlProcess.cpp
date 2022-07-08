#include "stdafx.h"
#include "XmlProcess.h"

void XmlProcess::Init()
{
}

void XmlProcess::Write()
{
	//写入XML
	
	//spRootEle->selectSingleNode(OLESTR("/root/node1"), &spNode);
	//spNode->put_text(OLESTR("newText")); //写入text
	//spRootEle->selectSingleNode(OLESTR("/root/node2/childnode1/@attrib1"), &spNode);
	//spNode->put_nodeValue(CComVariant(OLESTR("newValue"))); //写入value

	//spDoc->createNode(CComVariant(NODE_ELEMENT), OLESTR("childnode3"), OLESTR(""), &spNewNode); //创建新节点
	//spRootEle->selectSingleNode(OLESTR("/root/node2"), &spNode);
	//spNode->appendChild(spNewNode, &spNewNode); //将新节点加为node2的子节点
	//spNewNode->put_text(OLESTR("childtext2")); //写入新节点text
	//CComQIPtr<IXMLDOMElement> spEle = spNewNode; //注意这里使用CComQIPtr
	//spEle->setAttribute(OLESTR("attrib1"), CComVariant(OLESTR("value1")));//给新节点添加属性
	//spDoc->save(CComVariant(OLESTR("stocks.xml")));
}

void XmlProcess::Read()
{
	//读取XML
	spDoc.CoCreateInstance(CLSID_DOMDocument);
	VARIANT_BOOL vb;
	spDoc->load(CComVariant(OLESTR("stocks.xml")), &vb); //加载XML文件

	spDoc->get_documentElement(&spRootEle); //根节点
	CComPtr<IXMLDOMNodeList> spNodeList;
	spRootEle->get_childNodes(&spNodeList); //子节点列表

	long nLen;
	spNodeList->get_length(&nLen); //子节点数

	//遍历子节点
	for (long i = 0; i != nLen; ++i) {
		CComPtr<IXMLDOMNode> spNode;
		spNodeList->get_item(i, &spNode);
		ProcessNode(spNode); //节点处理函数
	}
}


void XmlProcess::ProcessNode(CComPtr<IXMLDOMNode>& spNode)
{
	CComBSTR bsNodeName;
	spNode->get_nodeName(&bsNodeName); //节点名
	AfxMessageBox(COLE2CT(bsNodeName));
	CComVariant varVal;
	spNode->get_nodeValue(&varVal); //节点值
	AfxMessageBox(COLE2CT(varVal.bstrVal));

	DOMNodeType eNodeType;
	spNode->get_nodeType(&eNodeType);

	if (eNodeType == NODE_ELEMENT) //只有NODE_ELEMENT类型才能包含有属性和子节点
	{
		//递归遍历节点属性
		CComPtr<IXMLDOMNamedNodeMap> spNameNodeMap;
		spNode->get_attributes(&spNameNodeMap);
		long nLength;
		spNameNodeMap->get_length(&nLength);

		for (long i = 0; i != nLength; ++i)
		{
			CComPtr<IXMLDOMNode> spNodeAttrib; //注意属性也是一个IXMLDOMNode
			spNameNodeMap->get_item(i, &spNodeAttrib);
			ProcessNode(spNodeAttrib);
		}

		//递归遍历子节点
		CComPtr<IXMLDOMNodeList> spNodeList;
		spNode->get_childNodes(&spNodeList);
		spNodeList->get_length(&nLength);
		for (long i = 0; i != nLength; ++i)
		{
			CComPtr<IXMLDOMNode> spChildNode;
			spNodeList->get_item(i, &spChildNode);
			ProcessNode(spChildNode);
		}
	}
}
