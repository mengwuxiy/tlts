#include "stdafx.h"


char ChannelNames[13] = "AaBbCDdFceEG";
CString ChannelNamesB[16] = { "A1", "A2", "a1", "a2", "B1", "B2", "b1", "b2", "C", "c", "D", "d", "E", "e", "F", "G" };

map<uint8_t, char*> g_strGuBieDefines = { { 0, "�ҹ�" }, { 1, "���" } };

map<uint8_t, char*> g_strXingBieDefines = { { 0, "����" }, { 1, "����" }, { 2, "����" } };

map<uint16_t, char*> g_strTypeDefines = { { 1, "����" }, { 2, "������" }, { 4, "���ˮƽ����" }, { 8, "����ˮƽ����" }, { 16, "�ݿ�б����" }, { 32, "�ݿ�ˮƽ����" }, { 64, "��������" }, { 128, "б����" }, { 256, "��׺�������" } };

map<uint16_t, char*> g_strDegreeDefines = { { 0, "����" }, { 1, "��������" }, { 2, "����" }, { 3, "�ᷢ" }, { 4, "����" }, { 5, "�۶�" } };

map<uint16_t, char*> g_strWoundPlaceDefines =
{
	{ 1, "��ͷ̤����" }, { 2, "��ͷ��" }, { 4, "��ͷ��" }, { 8, "��ͷ��" },
	{ 16, "����" }, { 32, "����ڲ�" }, { 64, "������" }, { 128, "����" },
	{ 256, "���" }, { 512, "��׽���" }, { 1024, "��׽���" }
};

map<uint8_t, char*> g_strCheckStateDefines = { { 0, "δ����" }, { 1, "�Ѹ���" } };


//tpB�ļ�·��
CString				g_strtpBFolder;
CString				g_strtpBFileName;

CString				g_strModuleFolder;

unsigned long long g_FileID = 0;


int					CurrentYear = 0;
int					CurrentMonth = 0;
int					CurrentDay = 0;


CString				g_strNewFileName;

//�߱��
CString				g_strRailNo;
CString				g_strRailName;

/*
�б�
*/
unsigned char		g_xingbie;
CString				g_strXingbie;

/*
�ɱ�
*/
unsigned char		g_gubie;
CString				g_strGubie;

/*
��·�ֱ��
*/
CString				g_juNo = _T("11");

/*
����α��
*/
CString				g_strGwdNo;
double				g_startPos;
double				g_endPos;

int year = 0;
int month = 0;
int day = 0;


/*
true��˳��̣�false�������
*/
bool				g_direction;

//log ������
bool ParseGPS(unsigned char* strGPS, double& log, double&lat)
{
	if (strGPS[0] == 'V')
	{
		log = lat = 0;
		return false;
	}
	char *pstrGPS = (char*)strGPS;
	char sz[20] = { 0 };
	strncpy(sz, pstrGPS + 4, 2);
	lat = atof(sz);
	strncpy(sz, pstrGPS + 7, 7);
	lat += atof(sz) / 60;

	memset(sz, 0, 20);
	strncpy(sz, pstrGPS + 16, 3);
	log = atof(sz);
	strncpy(sz, pstrGPS + 20, 7);
	log += atof(sz) / 60;

	return true;
}

void PrintTime(tm& time, char* msg)
{
	TRACE("%4d-%02d-%02d %02d:%02d:%02d %s\n", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, msg);
}

unsigned char INT8ChangeToBCD(unsigned char srcNum)
{
	unsigned char aimNum = ((srcNum >> 4) & 0x0f) * 10 + (srcNum & 0x0f);
	return aimNum;
}

unsigned short INT16ChangeToBCD(unsigned short srcNum)
{
	unsigned short aimNum = (srcNum >> 12) * 1000 + ((srcNum >> 8) & 0x0f) * 100 + ((srcNum >> 4) & 0x0f) * 10 + (srcNum & 0x0f);
	return aimNum;
}

unsigned int INT32ChangeToBCD(unsigned int srcNum)
{
	unsigned int aimNum = ((srcNum >> 28) & 0x0f) * 10000000
		+ ((srcNum >> 24) & 0x0f) * 1000000
		+ ((srcNum >> 20) & 0x0f) * 100000
		+ ((srcNum >> 16) & 0x0f) * 10000
		+ ((srcNum >> 12) & 0x0f) * 1000
		+ ((srcNum >> 8) & 0x0f) * 100
		+ ((srcNum >> 4) & 0x0f) * 10
		+ (srcNum & 0x0f);
	return aimNum;
}


unsigned char	BCDToINT8(unsigned char bcd)
{
	return  ((0xf0 & bcd) >> 4) * 10 + (0x0f & bcd);
}

unsigned short	BCDToINT16(unsigned short bcd)
{
	return ((0xf000 & bcd) >> 12) * 1000
		+ ((0xf00 & bcd) >> 8) * 100
		+ ((0xf0 & bcd) >> 4) * 10
		+ (0x0f & bcd);
}

unsigned int	BCDToINT32(unsigned int bcd)
{
	unsigned int ret =
		((0xf0000000 & bcd) >> 28) * 10000000
		+ ((0xf000000 & bcd) >> 24) * 1000000
		+ ((0xf00000 & (bcd) >> 20)) * 100000
		+ ((0xf0000 & bcd) >> 16) * 10000
		+ ((0xf000 & bcd) >> 12) * 1000
		+ ((0xf00 & bcd) >> 8) * 100
		+ ((0xf0 & bcd) >> 4) * 10
		+ (0x0f & bcd);
	return ret;
}

void CStringToArray(CString str, char splitter, uint16_t* data, int count)
{
	int idx = str.Find(splitter);
	int index = 0;
	CString strTemp;
	while (idx > 0)
	{
		strTemp = str.Left(idx);
		data[index++] = StrToInt(strTemp);
		str = str.Mid(idx + 1);
		idx = str.Find(splitter);
	}
	data[index] = StrToInt(str);
}

void ArrayToCString(uint16_t* data, int count, CString str, char splitter)
{
	CString strTemp;
	for (int i = 0; i < count; ++i)
	{
		strTemp.Format("%d%c", data[i], splitter);
		str += strTemp;
	}
	str = str.Left(str.GetLength() - 1);
}