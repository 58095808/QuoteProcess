#include "stdafx.h"
#include "CreateDir.h"


CCreateDir::CCreateDir()
{
}


CCreateDir::~CCreateDir()
{
}



// �����������ж��ļ����Ƿ����,�����ھʹ���
// example: /home/root/mkdir/1/2/3/4/
// ע��:���һ��������ļ��еĻ�,��Ҫ���� '\' ���� '/'
int32_t CCreateDir::createDirectory(const std::string &directoryPath)
{
	uint32_t dirPathLen = directoryPath.length();
	if (dirPathLen > MAX_PATH_LEN)
	{
		return -1;
	}
	char tmpDirPath[MAX_PATH_LEN] = { 0 };
	for (uint32_t i = 0; i < dirPathLen; ++i)
	{
		tmpDirPath[i] = directoryPath[i];
		if (tmpDirPath[i] == '\\' || tmpDirPath[i] == '/')
		{
			if (ACCESS(tmpDirPath, 0) != 0)
			{
				int32_t ret = MKDIR(tmpDirPath);
				if (ret != 0)
				{
					return ret;
				}
			}
		}
	}
	return 0;
}