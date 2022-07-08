#include "stdafx.h"
#include "MSAccess.h"

#define MS_DATABASE_DRV _T("MICROSOFT ACCESS DRIVER (*.mdb)")

void MSAccess::Init(CString database_file_name, CString database_name)
{
	sDriver.Append(MS_DATABASE_DRV);
	sFile.Append(database_file_name);
	strDbName.Append(database_name);
}

void MSAccess::Connect()
{
	// Build ODBC connection string
	sDsn.Format(L"ODBC;DRIVER={%s};DSN='';DBQ=%s", sDriver, sFile);

	TRY {
		database.Open(NULL, false, false, sDsn);
	} CATCH(CDBException, e) {
		// If a database exception occured, show error msg
		AfxMessageBox(L"Database error: " + e->m_strError);
	}
	END_CATCH;

}

void MSAccess::DisConnect()
{
	database.Close();
}

void MSAccess::Write(CString db_name, CString field_name, CString field_value)
{
	SqlString.Append(_T("INSERT INTO "));
	SqlString.Append(db_name);
	SqlString.Append(field_name);
	SqlString.Append(field_value);

	/* AfxMessageBox(SqlString.GetBuffer()); */
	database.ExecuteSQL(SqlString);
}

void MSAccess::Read()
{

}

int MSAccess::GetDataNum()
{
	#define DDB_FILE_ID _T("ID")
	#define SQL_CMD1 _T("SELECT")
	#define SQL_CMD2 DDB_FILE_ID
	#define SQL_CMD3 _T("FROM")

	int data_cnt = 0;
	CString SqlString;

	CRecordset recset(&database);
	//SqlString = "SELECT ID " "FROM AB003";
	SqlString.Format(_T("%s %s %s %s"), SQL_CMD1, SQL_CMD2, SQL_CMD3, strDbName);

	recset.Open(CRecordset::forwardOnly, SqlString, CRecordset::readOnly);

	while (!recset.IsEOF()) {
		recset.MoveNext();
		data_cnt++;
	}

	return data_cnt;
}