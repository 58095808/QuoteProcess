#pragma once
class InitFile
{
public:
	InitFile();
	~InitFile();

public:

	typedef struct tagIniString
	{
		CString m_strKey;
		CString m_strValue;
	}IniString;


	void LoadIniString();
	void DeleteIniString();
	void SaveIniString();
	CString GetIniString(CString, CString);
	void WriteIniString(CString, CString);
	int GetIniInt(CString, int);
	void WriteIniInt(CString, int);

private:
	CPtrArray m_aIniString;
};

