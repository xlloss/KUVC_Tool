#pragma once

#define XN_DBName               _T("TESTDB1")
////#define XN_DBName               _T("LC001")
//#define XN_LoginID              _T("adm_testdb1")
//#define XN_PWD                  _T("2wsx%TGB")
////#define XN_ServerIP             _T("SZSVR06") //資料庫的網域名稱,IP應該是192.168.188.86
//#define XN_ServerIP             _T("192.168.188.86")
#define XN_TIMEOUT              _T("15000")

//#include "stdafx.h"
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

class CMsSQL
{
public:
	CMsSQL();
	virtual ~CMsSQL();
	CString cstrManagementFields = L"";
	CString cstrAP = L"", cstrUserID = L"";
	BOOL HaveData,needWarning;
	CString ConnetStr();
	SQLRETURN GetTables(CString* sTables);
	SQLRETURN WriteDataToSQL(CString cstrSQL);
	SQLRETURN SelectDataToSQL(CString cstrSQL, bool sChkHaveData = FALSE);
	SQLRETURN SelectDataToSGrid(bool bAllField, CString cstrField, CString cstrTable, CString cstrWhere, int* sRowCount, int* sColCount, CString* sValues);
	SQLRETURN GetFieldName(CString cstrTable, CString* sFieldNames);
	void SetManagementField(CString cstrTable);
	SQLRETURN GetManagementField(CString cstrTable, CString* sManagementFields);
	CString GetManagementSetting(bool sDoUpdate, CString* sInsField = nullptr);

	SQLRETURN ConnectToSQLServer(SQLHENV shenv,SQLHDBC shdbc,SQLHSTMT shstmt, SQLWCHAR* sSQL, bool sChkHaveData=false);
};

