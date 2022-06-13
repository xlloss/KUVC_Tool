#include "pch.h"
#include "stdafx.h"
#include "afxdb.h"
#include "CMsSQL.h"
#include <windows.h>  
#include <stdio.h> 

#define STR_LEN 128 + 1  
#define REM_LEN 254 + 1

CString DBNAME_TAG = _T("$DBName$");

CMsSQL::CMsSQL()
{
	
}


CMsSQL::~CMsSQL()
{
}

SQLRETURN CMsSQL::SelectDataToSQL(CString cstrSQL, bool sChkHaveData)
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	SQLRETURN retcode, retcode2;

	SQLWCHAR OutConnStr[255];
	SQLSMALLINT OutConnStrLen;
	CString mConnetStr, cstrTemp;
	SQLWCHAR* sSQL;

	SQLSMALLINT textlen = 0;
	SQLINTEGER nativeerror = 0;
	SQLWCHAR sqlstate[32];
	SQLWCHAR message[256];

	cstrTemp = XN_DBName;
	cstrSQL.Replace(DBNAME_TAG, cstrTemp);
	sSQL = (SQLWCHAR*)(LPCTSTR)cstrSQL;

	mConnetStr = ConnetStr();
	cstrTemp = _T("");
	
	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	
	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
			//retcode = SQLAllocConnect(henv, &hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source
				for (int i = 0; i < 3; i++)
				{
					retcode = SQLDriverConnect(
						hdbc,
						NULL,//0,
						//(SQLWCHAR*)L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;",
						//_countof(L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;"),
						(SQLWCHAR*)(LPCWSTR)mConnetStr,
						SQL_NTS,
						NULL,
						0,
						NULL,
						//mConnetStr.GetLength(),
						//OutConnStr,//OutConnStr,
						//255,
						//&OutConnStrLen,
						SQL_DRIVER_NOPROMPT//SQL_DRIVER_COMPLETE    SQL_DRIVER_NOPROMPT
					);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
					{
						break;
					}
				}
				
				// Allocate statement handle
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

					// Process data

					for (int i = 0; i < 3; i++)
					{
						retcode = SQLExecDirect(hstmt, sSQL, SQL_NTS);
						if (retcode == SQL_SUCCESS)
						{
							break;
						}
					}

					if (retcode == SQL_SUCCESS) {
						SQLINTEGER sTestInt, cbTestStr, cbTestInt, cbTestFloat, iCount = 1;
						SQLFLOAT dTestFloat;
						SQLCHAR szTestStr[200];
						retcode = SQLFetch(hstmt);
						retcode2 = retcode;
						while (TRUE) {
							if (retcode2 == SQL_ERROR || retcode2 == SQL_SUCCESS_WITH_INFO) {
								//cout << "An error occurred";
								retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
								//::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
								
							}
							if ((retcode2 == SQL_NO_DATA) && (sChkHaveData)) {
								//cout << "An error occurred";
								break;
							}
							else if (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO) {
								int i=1;
								while (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO)
								{
									retcode2 = SQLGetData(hstmt, i, SQL_C_CHAR, szTestStr, 200, &cbTestStr);
									if (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO)
									{
										if (i == 1 && sChkHaveData)
										{
											cstrTemp.Format(L"%S", szTestStr);
											needWarning = cstrTemp == L"1" ? TRUE:FALSE;
											cstrTemp = L"";
										}
										cstrTemp.Format(L"%s;%S", cstrTemp, szTestStr);
										/* Print the row of data */
										//cout << "Row " << iCount << ":" << endl;
										//cout << szTestStr << endl;
										//cout << sTestInt << endl;
										//cout << dTestFloat << endl;
										//iCount++;
										i++;
									}
								}
								/* Print the row of data */
								//cout << "Row " << iCount << ":" << endl;
								//cout << szTestStr << endl;
								//cout << sTestInt << endl;
								//cout << dTestFloat << endl;
								//iCount++;
							}
							else {
								retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
								::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
								break;
							}
							retcode2 = SQLFetch(hstmt);
						}
					}
					else if (retcode == SQL_NO_DATA)
					{
						
					}
					else {
						//cout << "Query execution error." << endl;
						retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
						::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
						
					}

					SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					SQLDisconnect(hdbc);
				}
				else {
					//cout << "Connection error" << endl;
					retcode2 = SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, 1, sqlstate, &nativeerror, message, 256, &textlen);
					::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
					
				}
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}	
	return retcode;
}

CString CMsSQL::ConnetStr()
{
	CFile cFile;
	DWORD dwLen;
	int iFolderLength = 0, iStart, iEnd, iLen;
	char *szFile;
	TCHAR szPath[MAX_PATH] = { 0 };
	CString strPath, strToken, strFile, strFile2, strIP, strUserID, strPWD;

	//Read Server Config.txt setting
	GetModuleFileName(NULL, szPath, MAX_PATH);
	strPath = szPath;
	iFolderLength = strPath.ReverseFind('\\');
	strToken.Format(_T("%s\\%s"), strPath.Left(iFolderLength), L"Server Config.txt");
	if (!cFile.Open(strToken, CFile::modeRead))
	{
		AfxMessageBox(L"\"Server Config.txt\" file not exist");
		return L"";
	}
	dwLen = cFile.GetLength();
	szFile = new char[dwLen + 1];
	cFile.Read(szFile, dwLen);
	cFile.Close();

	strFile.Format(L"%S", szFile);
	delete[] szFile;

	//Get server IP
	iStart = strFile.Find(L"IP=");
	if (iStart == -1) //Not find
	{
		AfxMessageBox(L"\"Server Config.txt\" setting error");
		return L"";
	}
	else
	{
		iEnd = strFile.Find(L"\r\n");
		if (iEnd == -1)
		{
			AfxMessageBox(L"\"Server Config.txt\" setting error");
			return L"";
		}
		strIP = strFile.Mid(iStart + 3, iEnd - iStart - 3);
//AfxMessageBox(strIP);

		//Get User ID
		strFile2 = strFile.Mid(iEnd + 2, dwLen - iEnd - 2);
		iLen = strFile2.GetLength();
		iStart = strFile2.Find(L"UserID=");
		iEnd = strFile2.Find(L"\r\n");
		if (iStart == -1 || iEnd == -1)
		{
			AfxMessageBox(L"\"Server Config.txt\" setting error");
			return L"";
		}
		strUserID = strFile2.Mid(iStart + 7, iEnd - iStart - 7);
//AfxMessageBox(strUserID);

		//Get Password
		strFile2 = strFile2.Mid(iEnd + 2, iLen - iEnd - 2);
		iLen = strFile2.GetLength();
		iStart = strFile2.Find(L"Password=");
		iEnd = strFile2.Find(L"\r\n");
		if (iStart == -1 || iEnd == -1)
		{
			AfxMessageBox(L"\"Server Config.txt\" setting error");
			return L"";
		}
		strPWD = strFile2.Mid(iStart + 9, iEnd - iStart - 9);

//str.Format(L"strPWD=%s, iStart=%d, iEnd=%d, iLen=%d", strPWD, iStart, iEnd, iLen);
//AfxMessageBox(str);
//AfxMessageBox(strPWD);
	}

	CString mConnetStr;
	mConnetStr.Format(L"Driver={SQL Server};SERVER=%s;UID=%s;PWD=%s;Connection Timeout=%s;", strIP, strUserID, strPWD, XN_TIMEOUT);
	//mConnetStr.Format(L"Driver={SQL Server};SERVER=%s;UID=%s;PWD=%s;Connection Timeout=%s;"/*;Server=%s;DB=%s;Uid=%s;Pwd=%s;"*/, XN_ServerIP/*, XN_DBName*/, XN_LoginID, XN_PWD, XN_TIMEOUT);
	//mConnetStr.Format(L"Data Source={local};SERVER=db_Feeder;UID=sa2;PWD=1qaz@WSX;Connection Timeout=%s;", XN_TIMEOUT);
	return mConnetStr;
}

SQLRETURN CMsSQL::GetTables(CString* sTables)
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	SQLRETURN retcode, retcode2;

	SQLWCHAR OutConnStr[255];
	SQLSMALLINT OutConnStrLen;
	CString mConnetStr, cstrSQL,cstrTemp;
	SQLWCHAR* sSQL;

	SQLSMALLINT textlen = 0;
	SQLINTEGER nativeerror = 0;
	SQLWCHAR sqlstate[32];
	SQLWCHAR message[256];

	cstrTemp = XN_DBName;
	cstrSQL = _T("select name from \"$DBName$\".sys.tables where type_desc = 'USER_TABLE'");
	cstrSQL.Replace(DBNAME_TAG, cstrTemp);
	sSQL = (SQLWCHAR*)(LPCTSTR)cstrSQL;
	cstrTemp = _T("");
	
	mConnetStr = ConnetStr();

	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
			//retcode = SQLAllocConnect(henv, &hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source
				for (int i = 0; i < 3; i++)
				{
					retcode = SQLDriverConnect(
						hdbc,
						NULL,//0,
						//(SQLWCHAR*)L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;",
						//_countof(L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;"),
						(SQLWCHAR*)(LPCWSTR)mConnetStr,
						SQL_NTS,
						NULL,
						0,
						NULL,
						//mConnetStr.GetLength(),
						//OutConnStr,//OutConnStr,
						//255,
						//&OutConnStrLen,
						SQL_DRIVER_NOPROMPT//SQL_DRIVER_COMPLETE    SQL_DRIVER_NOPROMPT
					);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
					{
						break;
					}
				}

				// Allocate statement handle
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

					// Process data

					for (int i = 0; i < 3; i++)
					{
						retcode = SQLExecDirect(hstmt, sSQL, SQL_NTS);
						if (retcode == SQL_SUCCESS)
						{
							break;
						}
					}

					if (retcode == SQL_SUCCESS) {
						SQLINTEGER sTestInt, cbTestStr, cbTestInt, cbTestFloat, iCount = 1;
						SQLFLOAT dTestFloat;
						SQLCHAR szTestStr[200];
						retcode = SQLFetch(hstmt);
						retcode2 = retcode;
						while (TRUE) {
							if (retcode2 == SQL_ERROR || retcode2 == SQL_SUCCESS_WITH_INFO) {
								retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
								//::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
								//cout << "An error occurred";
							}
							if (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO) {

								retcode2 = SQLGetData(hstmt, 1, SQL_C_CHAR, szTestStr, 200, &cbTestStr);

								cstrTemp.Format(L"%s;%S", cstrTemp, szTestStr);
							}
							else {
								break;
							}
							retcode2 = SQLFetch(hstmt);
						}
					}
					else {
						//cout << "Query execution error." << endl;
						retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
						::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
						
					}

					SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					SQLDisconnect(hdbc);
				}
				else {
					//cout << "Connection error" << endl;
					retcode2 = SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, 1, sqlstate, &nativeerror, message, 256, &textlen);
					::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
					
					//if (retcode != SQL_ERROR)
					//	printf("%s=%s\n", (CHAR *)sqlstate, (CHAR *)message);
				}
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	*sTables = cstrTemp;
	return retcode;
}

SQLRETURN CMsSQL::GetManagementField(CString cstrTable, CString* sManagementFields)
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	SQLRETURN retcode, retcode2;

	SQLWCHAR OutConnStr[255];
	SQLSMALLINT OutConnStrLen;
	CString mConnetStr, cstrTemp;
	CString cstrMSQL = L"";
	SQLWCHAR* sSQL;

	SQLSMALLINT textlen = 0;
	SQLINTEGER nativeerror = 0;
	SQLWCHAR sqlstate[32];
	SQLWCHAR message[256];

	cstrTemp = XN_DBName;
	cstrMSQL = L" SELECT c.name from \"$DBName$\".sys.all_columns c "
		       L" LEFT JOIN \"$DBName$\".sys.extended_properties p on major_id = column_id "
		       L" WHERE object_id = object_id('\"$DBName$\"..\""+ cstrTable +"\"') "
		         L" AND value = N'Management' ";
	cstrMSQL.Replace(DBNAME_TAG, cstrTemp);
	//cstrMSQL.Format(cstrMSQL, cstrTemp, cstrTable);
	sSQL = (SQLWCHAR*)(LPCTSTR)cstrMSQL;

	mConnetStr = ConnetStr();
	cstrTemp = _T("");

	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
			//retcode = SQLAllocConnect(henv, &hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source
				for (int i = 0; i < 3; i++)
				{
					retcode = SQLDriverConnect(
						hdbc,
						NULL,//0,
						//(SQLWCHAR*)L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;",
						//_countof(L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;"),
						(SQLWCHAR*)(LPCWSTR)mConnetStr,
						SQL_NTS,
						NULL,
						0,
						NULL,
						//mConnetStr.GetLength(),
						//OutConnStr,//OutConnStr,
						//255,
						//&OutConnStrLen,
						SQL_DRIVER_NOPROMPT//SQL_DRIVER_COMPLETE    SQL_DRIVER_NOPROMPT
					);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
					{
						break;
					}
				}

				// Allocate statement handle
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

					// Process data

					for (int i = 0; i < 3; i++)
					{
						retcode = SQLExecDirect(hstmt, sSQL, SQL_NTS);
						if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
						{
							break;
						}
					}

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLINTEGER sTestInt, cbTestStr, cbTestInt, cbTestFloat, iCount = 1;
						SQLFLOAT dTestFloat;
						SQLCHAR szTestStr[200];
						retcode = SQLFetch(hstmt);
						retcode2 = retcode;
						while (TRUE) {
							retcode = SQLFetch(hstmt);
							if (retcode2 == SQL_ERROR || retcode2 == SQL_SUCCESS_WITH_INFO) {
								//retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
								//::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
								//cout << "An error occurred";
							}
							if (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO) {

								retcode2 = SQLGetData(hstmt, 1, SQL_C_CHAR, szTestStr, 200, &cbTestStr);

								cstrTemp.Format(L"%s;%S", cstrTemp, szTestStr);
							}
							else {
								
								break;
							}
							retcode2 = SQLFetch(hstmt);
						}
					}
					else if (retcode == SQL_NO_DATA)
					{
						
					}
					else {
						//cout << "Query execution error." << endl;
						retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
						::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
						
					}

					SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					SQLDisconnect(hdbc);
				}
				else {
					//cout << "Connection error" << endl;
					retcode2 = SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, 1, sqlstate, &nativeerror, message, 256, &textlen);
					::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
					
				}
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	*sManagementFields = cstrTemp;
	return retcode;
}

SQLRETURN CMsSQL::WriteDataToSQL(CString cstrSQL)
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	SQLRETURN retcode, retcode2;

	SQLWCHAR OutConnStr[255];
	SQLSMALLINT OutConnStrLen;
	CString mConnetStr, cstrTemp;
	SQLWCHAR* sSQL;

	SQLSMALLINT textlen = 0;
	SQLINTEGER nativeerror = 0;
	SQLWCHAR sqlstate[32];
	SQLWCHAR message[256];

	cstrTemp = XN_DBName;
	cstrSQL.Replace(DBNAME_TAG, cstrTemp);
	sSQL = (SQLWCHAR*)(LPCTSTR)cstrSQL;

	mConnetStr = ConnetStr();
	cstrTemp = _T("");

	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
			//retcode = SQLAllocConnect(henv, &hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source
				for (int i = 0; i < 3; i++)
				{
					retcode = SQLDriverConnect(
						hdbc,
						NULL,//0,
						//(SQLWCHAR*)L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;",
						//_countof(L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;"),
						(SQLWCHAR*)(LPCWSTR)mConnetStr,
						SQL_NTS,
						NULL,
						0,
						NULL,
						//mConnetStr.GetLength(),
						//OutConnStr,//OutConnStr,
						//255,
						//&OutConnStrLen,
						SQL_DRIVER_NOPROMPT//SQL_DRIVER_COMPLETE    SQL_DRIVER_NOPROMPT
					);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
					{
						break;
					}
				}

				// Allocate statement handle
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

					// Process data

					for (int i = 0; i < 3; i++)
					{
						retcode = SQLExecDirect(hstmt, sSQL, SQL_NTS);
						if (retcode == SQL_SUCCESS)
						{
							break;
						}
					}

					if (retcode == SQL_SUCCESS) {
						SQLINTEGER sTestInt, cbTestStr, cbTestInt, cbTestFloat, iCount = 1;
						SQLFLOAT dTestFloat;
						SQLCHAR szTestStr[200];
						retcode = SQLFetch(hstmt);
						retcode2 = retcode;
						while (TRUE) {
							if (retcode2 == SQL_ERROR || retcode2 == SQL_SUCCESS_WITH_INFO) {
								retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
								//::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
								//cout << "An error occurred";
							}
							if (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO) {

								//SQLGetData(hstmt, 1, SQL_C_CHAR, szTestStr, 200, &cbTestStr);
								//SQLGetData(hstmt, 2, SQL_C_ULONG, &sTestInt, 0, &cbTestInt);
								//SQLGetData(hstmt, 3, SQL_C_DOUBLE, &dTestFloat, 0, &cbTestFloat);

								/* Print the row of data */
								//cout << "Row " << iCount << ":" << endl;
								//cout << szTestStr << endl;
								//cout << sTestInt << endl;
								//cout << dTestFloat << endl;
								//iCount++;
								break;
							}
							else {
								break;
							}
							retcode2 = SQLFetch(hstmt);
						}
					}
					else {
						//cout << "Query execution error." << endl;
						retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
						::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
						
					}

					SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					SQLDisconnect(hdbc);
				}
				else {
					//cout << "Connection error" << endl;
					retcode2 = SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, 1, sqlstate, &nativeerror, message, 256, &textlen);
					::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
					
				}
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	return retcode;
}

SQLRETURN CMsSQL::SelectDataToSGrid(bool bAllField, CString cstrField, CString cstrTable, CString cstrWhere,int* sRowCount, int* sColCount, CString* sValues)
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	SQLRETURN retcode, retcode2;

	SQLWCHAR OutConnStr[255];
	SQLSMALLINT OutConnStrLen;
	CString mConnetStr, cstrTemp, cstrValue, strField, cstrSQL;
	SQLWCHAR* sSQL;

	SQLSMALLINT textlen = 0;
	SQLINTEGER nativeerror = 0;
	SQLWCHAR sqlstate[32];
	SQLWCHAR message[256];

	SQLWCHAR strColumnName[STR_LEN];
	SQLLEN lenColumnName = STR_LEN +1;

	if (bAllField == 1) //所有欄位
	{
		SetManagementField(cstrTable);
		GetFieldName(cstrTable, &cstrTemp);
		strField = cstrTemp;
		strField.Delete(0, 2);
		strField.Replace(L"\3", L"\",\"");
		cstrSQL.Format(L"SELECT \"%s\" FROM \"%s\"..\"%s\"", strField, XN_DBName, cstrTable);
	}
	else if (bAllField == 0) //指定的欄位
	{
		cstrSQL.Format(L"SELECT %s FROM \"%s\"..\"%s\"", cstrField, XN_DBName, cstrTable);
	}

//AfxMessageBox(cstrSQL);

	cstrWhere.Trim();
	if (cstrWhere != L"")
	{
		cstrSQL.Format(L"%s WHERE %s", cstrSQL, cstrWhere);
	}

//AfxMessageBox(cstrSQL);

	//cstrSQL.Replace(DBNAME_TAG, XN_DBName);
	sSQL = (SQLWCHAR*)(LPCTSTR)cstrSQL;

	cstrTemp.Format(L"%s\4", cstrTemp);

//AfxMessageBox(cstrTemp);

	//retcode = ConnectToSQLServer(henv, hdbc, hstmt, sSQL);
	
	int i=1;
	*sRowCount += 1;
	mConnetStr = ConnetStr();

	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
			//retcode = SQLAllocConnect(henv, &hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source
				for (int i = 0; i < 3; i++)
				{
					retcode = SQLDriverConnect(
						hdbc,
						NULL,//0,
						//(SQLWCHAR*)L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;",
						//_countof(L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;"),
						(SQLWCHAR*)(LPCWSTR)mConnetStr,
						SQL_NTS,
						NULL,
						0,
						NULL,
						//mConnetStr.GetLength(),
						//OutConnStr,//OutConnStr,
						//255,
						//&OutConnStrLen,
						SQL_DRIVER_NOPROMPT//SQL_DRIVER_COMPLETE    SQL_DRIVER_NOPROMPT
					);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
					{
						break;
					}
				}

				// Allocate statement handle
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

					// Process data

					for (int i = 0; i < 3; i++)
					{
						retcode = SQLExecDirect(hstmt, sSQL, SQL_NTS);
						if (retcode == SQL_SUCCESS)
						{
							break;
						}
					}

					if (retcode == SQL_SUCCESS) {
						SQLINTEGER sTestInt, cbTestStr, cbTestInt, cbTestFloat, iCount = 1;
						SQLFLOAT dTestFloat;
						SQLCHAR szTestStr[200];
						retcode = SQLFetch(hstmt);
						retcode2 = retcode;
						while (TRUE) {

							if (retcode2 == SQL_ERROR || retcode2 == SQL_SUCCESS_WITH_INFO) {
								//cout << "An error occurred";
								retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
								//::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
								
							}
							if (retcode2 == SQL_NO_DATA) {
								//cout << "An error occurred";
								break;
							}
							else if (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO) {
								while (TRUE)
								{
									retcode2 = SQLGetData(hstmt, i, SQL_C_CHAR, szTestStr, 200, &cbTestStr);

									if (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO)
									{
										if (cbTestStr != -1)
										{
											cstrValue = (LPSTR)szTestStr;
										}
										else
										{
											cstrValue = L"";
										}

										if (cstrValue != "")
										{
											cstrTemp.Format(L"%s\3%s", cstrTemp, cstrValue);
										}
										else
										{
											cstrTemp.Format(L"%s\3%s", cstrTemp, L"\2");
										}
//AfxMessageBox(cstrTemp);
										/* Print the row of data */
										//cout << "Row " << iCount << ":" << endl;
										//cout << szTestStr << endl;
										//cout << sTestInt << endl;
										//cout << dTestFloat << endl;
										//iCount++;
										i++;
									}
									else
									{
										retcode2 = SQLFetchScroll(hstmt, 1, 1);
										if (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO)
										{
											cstrTemp.Format(L"%s\4", cstrTemp);
											*sRowCount += 1;
											i = 1;
//AfxMessageBox(cstrTemp);
										}
										else
										{
											*sColCount = i;
											break;
										}
									}
								}
								/* Print the row of data */
								//cout << "Row " << iCount << ":" << endl;
								//cout << szTestStr << endl;
								//cout << sTestInt << endl;
								//cout << dTestFloat << endl;
								//iCount++;
								
							}
							else {
								retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
								::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
								break;
							}
							retcode2 = SQLFetch(hstmt);
						}
					}
					else if (retcode == SQL_NO_DATA)
					{
						//return 0;
					}
					else {
						//cout << "Query execution error." << endl;
						retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
						::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
						
					}

					SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					SQLDisconnect(hdbc);
				}
				else {
					//cout << "Connection error" << endl;
					retcode2 = SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, 1, sqlstate, &nativeerror, message, 256, &textlen);
					::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
					
				}
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	*sValues = cstrTemp;
	return retcode;
}

SQLRETURN CMsSQL::GetFieldName(CString cstrTable, CString* sFieldNames)
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	SQLRETURN retcode, retcode2;

	SQLWCHAR OutConnStr[255];
	SQLSMALLINT OutConnStrLen;
	CString mConnetStr, cstrTemp, cstrSQL;
	SQLWCHAR* sSQL;

	SQLSMALLINT textlen = 0;
	SQLINTEGER nativeerror = 0;
	SQLWCHAR sqlstate[32];
	SQLWCHAR message[256];

	cstrTemp = XN_DBName;
	cstrSQL.Format(L"SELECT \"name\" FROM \"$DBName$\".\"sys\".\"all_columns\" WHERE object_id = object_id('\"$DBName$\"..\"%s\"') ORDER BY \"column_id\" ", cstrTable);
	cstrSQL.Replace(DBNAME_TAG, cstrTemp);
	sSQL = (SQLWCHAR*)(LPCTSTR)cstrSQL;
	cstrTemp = L"\4";

	mConnetStr = ConnetStr();

	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
			//retcode = SQLAllocConnect(henv, &hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source
				for (int i = 0; i < 3; i++)
				{
					retcode = SQLDriverConnect(
						hdbc,
						NULL,//0,
						//(SQLWCHAR*)L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;",
						//_countof(L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;"),
						(SQLWCHAR*)(LPCWSTR)mConnetStr,
						SQL_NTS,
						NULL,
						0,
						NULL,
						//mConnetStr.GetLength(),
						//OutConnStr,//OutConnStr,
						//255,
						//&OutConnStrLen,
						SQL_DRIVER_NOPROMPT//SQL_DRIVER_COMPLETE    SQL_DRIVER_NOPROMPT
					);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
					{
						break;
					}
				}

				// Allocate statement handle
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

					// Process data

					for (int i = 0; i < 3; i++)
					{
						retcode = SQLExecDirect(hstmt, sSQL, SQL_NTS);
						if (retcode == SQL_SUCCESS)
						{
							break;
						}
					}

					if (retcode == SQL_SUCCESS) {
						SQLINTEGER sTestInt, cbTestStr, cbTestInt, cbTestFloat, iCount = 1;
						SQLFLOAT dTestFloat;
						SQLCHAR szTestStr[200];
						retcode = SQLFetch(hstmt);
						retcode2 = retcode;
						while (TRUE) {
							if (retcode2 == SQL_ERROR || retcode2 == SQL_SUCCESS_WITH_INFO) {
								retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
								//::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
								//cout << "An error occurred";
								
							}
							if (retcode2 == SQL_NO_DATA) {
								//cout << "An error occurred";
								break;
							}
							else if (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO) {
								int i = 1;
								while (TRUE)
								{
									retcode2 = SQLGetData(hstmt, i, SQL_C_CHAR, szTestStr, 200, &cbTestStr);
									if (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO)
									{
										cstrTemp.Format(L"%s\3%S", cstrTemp, szTestStr);
										/* Print the row of data */
										//cout << "Row " << iCount << ":" << endl;
										//cout << szTestStr << endl;
										//cout << sTestInt << endl;
										//cout << dTestFloat << endl;
										//iCount++;
										i++;
									}
									else
									{
										retcode2 = SQLFetchScroll(hstmt, 1, 1);
										if (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO)
										{
											i = 1;
										}
										else
										{
											break;
										}
									}
								}
								/* Print the row of data */
								//cout << "Row " << iCount << ":" << endl;
								//cout << szTestStr << endl;
								//cout << sTestInt << endl;
								//cout << dTestFloat << endl;
								//iCount++;
								
							}
							else {
								retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
								::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
								break;
							}
							retcode2 = SQLFetch(hstmt);
						}
					}
					else if (retcode == SQL_NO_DATA)
					{
						//return 0;
					}
					else {
						//cout << "Query execution error." << endl;
						retcode2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &nativeerror, message, 256, &textlen);
						::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
						
					}

					SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					SQLDisconnect(hdbc);
				}
				else {
					//cout << "Connection error" << endl;
					retcode2 = SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, 1, sqlstate, &nativeerror, message, 256, &textlen);
					::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
					
				}
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	*sFieldNames = cstrTemp;
	return retcode;
}

void CMsSQL::SetManagementField(CString cstrTable)
{
	 GetManagementField(cstrTable, &cstrManagementFields);
}

CString CMsSQL::GetManagementSetting(bool sDoUpdate, CString* sInsField)
{
	CString cstrReturn = L"",cstrTemp,cstrValue, cstrField, cstrFields=L"";
	if (cstrManagementFields != L"")
	{
		int intTokenPos=0;
		cstrTemp = cstrManagementFields.Tokenize(L";", intTokenPos);
		while (cstrTemp != L"")
		{

			cstrField = L"";

			if (sDoUpdate && cstrTemp.Find(L"Modi") != -1)
			{
				cstrField = cstrTemp;
			}
			else if (!sDoUpdate && cstrTemp.Find(L"Create") != -1)
			{
				cstrField = cstrTemp;
			}
			else if (cstrTemp.Find(L"Modi") == -1 && cstrTemp.Find(L"Create") == -1)
			{
				cstrField = cstrTemp;
			}

			if (cstrField != L"")
			{
				if (cstrField.Find(L" Date") != -1)
				{
					cstrValue = L" DATE_FORMAT(now(),\"%Y%m%d\") ";
				}
				else if (cstrField.Find(L" Time") != -1)
				{
					cstrValue = L" DATE_FORMAT(now(),\"%H:%i:%S\") ";
				}
				else if (cstrField.Find(L" AP") != -1)
				{
					cstrValue.Format(L" '%s' ", cstrAP);
					//cstrValue = L" '" + cstrAP + L"' ";
				}
				else if (cstrField.Find(L" User") != -1)
				{
					cstrValue.Format(L" '%s' ", cstrUserID);
					//cstrValue = L" '" + cstrUserID + L"' ";
				}
				else if (cstrField = L"Flag")
				{
					cstrValue = L" IFNull(Case when (Flag+1)%1000 = 0 then 1 else (Flag)%1000 + 1 end,1) ";
				}

				if (sDoUpdate)
				{
					cstrReturn += (cstrReturn == L"" ? L"" : L" , ") + (L" \"" + cstrField + L"\" = " + cstrValue);
				}
				else
				{
					cstrFields += (cstrFields == L"" ? L"" : L" , ") + (L" \"" + cstrField + L"\" ");
					cstrReturn += (cstrReturn == L"" ? L"" : L" , ") + (L" " + cstrValue + L" ");
				}
			}
			cstrTemp = cstrManagementFields.Tokenize(L";", intTokenPos);
		}
		
	}

	if (sInsField != nullptr)
	{
		*sInsField = cstrFields;
	}
	return cstrReturn;
}

SQLRETURN CMsSQL::ConnectToSQLServer(SQLHENV shenv, SQLHDBC shdbc, SQLHSTMT shstmt, SQLWCHAR* sSQL, bool sChkHaveData)
{
	SQLRETURN retcode, retcode2;
	CString mConnetStr;
	SQLWCHAR OutConnStr[255];
	SQLSMALLINT OutConnStrLen;

	SQLSMALLINT textlen = 0;
	SQLINTEGER nativeerror = 0;
	SQLWCHAR sqlstate[32];
	SQLWCHAR message[256];

	mConnetStr = ConnetStr();
	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &shenv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(shenv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, shenv, &shdbc);
			//retcode = SQLAllocConnect(henv, &hdbc);
			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				retcode = SQLSetConnectAttr(shdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source
				for (int i = 0; i < 3; i++)
				{
					retcode = SQLDriverConnect(
						shdbc,
						NULL,//0,
						//(SQLWCHAR*)L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;",
						//_countof(L"Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;DB=testdb;Uid=root;Pwd=test1234;"),
						(SQLWCHAR*)(LPCWSTR)mConnetStr,
						SQL_NTS,
						NULL,
						0,
						NULL,
						//mConnetStr.GetLength(),
						//OutConnStr,//OutConnStr,
						//255,
						//&OutConnStrLen,
						SQL_DRIVER_NOPROMPT//SQL_DRIVER_COMPLETE    SQL_DRIVER_NOPROMPT
					);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
					{
						break;
					}
				}

				// Allocate statement handle
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, shdbc, &shstmt);

					// Process data

					for (int i = 0; i < 3; i++)
					{
						retcode = SQLExecDirect(shstmt, sSQL, SQL_NTS);
						if (retcode == SQL_SUCCESS)
						{
							break;
						}
					}

					if (retcode == SQL_SUCCESS) {
						SQLINTEGER sTestInt, cbTestStr, cbTestInt, cbTestFloat, iCount = 1;
						SQLFLOAT dTestFloat;
						SQLCHAR szTestStr[200];
						retcode = SQLFetch(shstmt);
						retcode2 = retcode;
						while (TRUE) {
							if (retcode2 == SQL_ERROR || retcode2 == SQL_SUCCESS_WITH_INFO) {
								//cout << "An error occurred";
								retcode2 = SQLGetDiagRec(SQL_HANDLE_DBC, shdbc, 1, sqlstate, &nativeerror, message, 256, &textlen);
								//::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
								
							}
							if ((retcode2 == SQL_NO_DATA) && (sChkHaveData)) {
								//cout << "An error occurred";
								
							}
							else if (retcode2 == SQL_SUCCESS || retcode2 == SQL_SUCCESS_WITH_INFO) {
							}
							else {
								break;
							}
							retcode2 = SQLFetch(shstmt);
						}
					}
					else {
						//cout << "Query execution error." << endl;
						retcode2 = SQLGetDiagRec(SQL_HANDLE_DBC, shdbc, 1, sqlstate, &nativeerror, message, 256, &textlen);
						::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
						
					}

					SQLFreeHandle(SQL_HANDLE_STMT, shstmt);
					SQLDisconnect(shdbc);
				}
				else {
					//cout << "Connection error" << endl;
					retcode2 = SQLGetDiagRec(SQL_HANDLE_DBC, shdbc, 1, sqlstate, &nativeerror, message, 256, &textlen);
					::MessageBox(NULL, message, sqlstate, MB_ICONSTOP | MB_OK);
					
				}
				SQLFreeHandle(SQL_HANDLE_DBC, shdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, shenv);
	}
	return retcode;
}
