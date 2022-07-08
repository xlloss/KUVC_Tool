#include "odbcinst.h"
#include "afxdb.h"

#pragma once
class MSAccess
{
	CDatabase database;
	CString SqlString;
	CString strID, strDbName;
	CString sDriver;
	CString sDsn;
	CString sFile;

public:

	void Init(CString database_file_name, CString database_name);
	void Connect();
	void DisConnect();
	void Write(CString db_name, CString field_name, CString field_value);
	void Read();
	int  GetDataNum();
};

