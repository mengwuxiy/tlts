#include "stdafx.h"

#include "MySqlHPE.h"
#include "SolveData.h"
//#pragma comment(lib, "libmysqld.lib")

char szFileName[250] = { 0 };
char szLineWay[][2] = { "D", "S", "X" };
char szLeftRight[][2] = { "R", "L" };//左右股
char szWD[9] = { 0 };//里程

FILE* pLogFileMysql = NULL;

CMySqlHPE::CMySqlHPE()
{
	m_bConnected = false;
}


CMySqlHPE::~CMySqlHPE()
{
	
}


bool CMySqlHPE::ConnectDatabase(CString host, CString user, CString pwd, CString database)
{
	pLogFileMysql = fopen(g_strModuleFolder + "\\mysql_log.txt", "w+");
	fseek(pLogFileMysql, 0, SEEK_SET);
	MYSQL* pConn = mysql_init(&m_conn);
	//if (!(mysql_real_connect(&m_conn, "localhost", "root", "", "test", 0, NULL, 0))) 
	if (!(mysql_real_connect(&m_conn, (char*)(LPCTSTR)host, (char*)(LPCTSTR)user, (char*)(LPCTSTR)pwd, (char*)(LPCTSTR)database, 3306, NULL, MYSQL_OPTION_MULTI_STATEMENTS_ON))) //中间分别是主机，用户名，密码，数据库名，端口号（可以写默认0或者3306等），可以先写成参数再传进去
	{
		CString str;
		str.Format("Error connecting to database:%s\n", mysql_error(&m_conn));
		AfxMessageBox(str);
		return false;
	}

	mysql_query(&m_conn, "set names gbk;");
	return true;
}

bool CMySqlHPE::CloseConn()
{
	mysql_close(&m_conn);
	m_bConnected = false;
	fclose(pLogFileMysql);
	return true;
}

bool CMySqlHPE::Query(CString strSql, MYSQL_RES** rs)
{
	//返回0 查询成功，返回1查询失败
	if (mysql_query(&m_conn, strSql))        //执行SQL语句
	{
		fprintf(pLogFileMysql, "\t%s\n\t%s\r\n\r\n", (char*)(LPCTSTR)strSql, GetErrorMsg());
		fflush(pLogFileMysql);
		return false;
	}
	else
	{
		*rs = mysql_store_result(&m_conn);
		return *rs != NULL;
	}
}

bool CMySqlHPE::Execute(CString strSql)
{
	//返回0 查询成功，返回1查询失败
	bool bOK = mysql_query(&m_conn, strSql) == 0;
	if (bOK == false)
	{
		fprintf(pLogFileMysql, "\t%s\n\t%s\r\n\r\n", (char*)(LPCTSTR)strSql, GetErrorMsg());
		fflush(pLogFileMysql);
		//		AfxMessageBox(GetErrorMsg(), MB_OK | MB_ICONWARNING);
	}
	return bOK;
}

const char* CMySqlHPE::GetErrorMsg()
{
	return mysql_error(&m_conn);
}

uint64_t CMySqlHPE::GetLastInsertID()
{
	CString strSql = "SELECT LAST_INSERT_ID();";
	MYSQL_RES* rs;
	if (!Query(strSql, &rs))
	{
		return 0;
	}
	MYSQL_ROW row = mysql_fetch_row(rs);
	return strtoull(row[0], NULL, 10);
}

CString	 CMySqlHPE::ToHexCString(int data, int length)
{
	CString str;
	str.Format("%X", data);
	while (str.GetLength() < length)
	{
		str = "0" + str;
	}
	return str;
}

CString	CMySqlHPE::ToCString(int data, int length)
{
	CString str;
	str.Format("%d", data);
	while (str.GetLength() < length)
	{
		str = "0" + str;
	}
	return str;
}

CString		GetTimeString(uint32_t date, uint32_t time = 0)
{
	uint8_t year = date >> 16;
	uint8_t	month = ((date & 0xFF00) >> 8);
	uint8_t day = date & 0xFF;

	uint8_t hour = time >> 16;
	uint8_t	minute = ((time & 0xFF00) >> 8);
	uint8_t second = time & 0xFF;
	CString ret;
	ret.Format(_T("%04d-%02d-%02d %02d:%02d:%02d"), year, month, day, hour, minute, second);
	return ret;
}

CString CMySqlHPE::GetFileName(CString strFile, F_HEAD& head, uint16_t& gwd, double& gps_log, double& gps_lat)
{
	int year, month, day;
	double dW1, dW2;
	W_D w1, w2;
	size_t nReadretnum;
	uint8_t WalkWay, leftRight, lineWay;
	uint16_t lineNum;
	uint32_t ft, section;

	char sz[10];
	char cShunNi = 0;

	FILE *pFile = fopen(strFile, _T("rb"));
	if (pFile == 0)
	{
		//	fclose(pFile);
		return _T("");
	}

	fseek(pFile, 0L, SEEK_END);
	uint32_t iSizeB = ftell(pFile);
	fseek(pFile, 0L, SEEK_SET);
	nReadretnum = fread(&head, sizeof(F_HEAD), 1, pFile);

	bool bFindGPS = false;
	uint32_t use_Size_B = 0, read_Size_B = 0;
	int iBeginBlock = 0, iBeginStep = 0;
	vector<Read_Info> vInfo;
	do
	{
		vector<BlockData_B> dataB;
		uint32_t iRead = Analyse_Bchao(pFile, dataB, use_Size_B, read_Size_B, iSizeB, iBeginBlock, iBeginStep, vInfo, 100);
		for (int i = 0; i < iRead; ++i)
		{
			if (ParseGPS(dataB[i].BlockHead.gpsInfor, gps_log, gps_lat))
			{
				bFindGPS = true;
				break;
			}
		}
		iBeginBlock += 100;

	} while (bFindGPS == false && vInfo[vInfo.size() - 1].UsedL < iSizeB);

	fclose(pFile);

	ft = head.startD;//开始作业时间

	section = head.deviceP2.TrackSet.sectionNum;//单位编号5个BCD，工务段

	lineNum = head.deviceP2.TrackSet.lineNum;	// 线编号4个BCD 

	w1 = head.startKm;
	w2 = head.endKm;

	WalkWay = head.deviceP2.TrackSet.WalkWay;// 行走方向0: 逆里程, 1: 顺里程

	leftRight = head.deviceP2.TrackSet.leftRight;// 左右股0: 右1：左

	lineWay = head.deviceP2.TrackSet.lineWay;		// 行别0：单线、1：上，2：下行

	//memset(szFileName, 0, 250);

	dW1 = w1.Km * 1000 + w1.m + w1.mm;
	dW2 = w2.Km * 1000 + w2.m + w2.mm;
	if (dW2 > dW1)
	{
		cShunNi = 'S';
	}
	else
	{
		cShunNi = 'N';
	}

	year = (ft & 0xFFFF0000) >> 16;
	month = (ft & 0x00FF00) >> 8;
	day = ft && 0xFF;

	section = INT32ChangeToBCD(section);
	lineNum = INT16ChangeToBCD(lineNum);
	leftRight = INT8ChangeToBCD(leftRight);
	lineWay = INT8ChangeToBCD(lineWay);

	sprintf(szWD, "%04d%04d", w1.Km, w2.Km);
	szWD[8] = 0;

	sprintf(szFileName, "%02d%02d%02d11%05d%04d", year, month, day, section, lineNum);
	gwd = section;

	strcat(szFileName, szLineWay[lineWay & 0x03]);

	strcat(szFileName, szWD);

	sprintf(sz, "%s%c", szLeftRight[leftRight & 0x01], cShunNi);
	strcat(szFileName, sz);
	return CString(szFileName);
}

CString	CMySqlHPE::GetGwdNo(CString railNo, uint8_t xingbie, double startwd)
{
	CString strSql;
	strSql.Format("select gwd_no from rail_gwd where rail_no = '%s' and xingbie = %d and s_mil <= %lf and e_mil >= %lf;",
		railNo, xingbie, startwd, startwd);
	MYSQL_RES* rs;
	if (!Query(strSql, &rs))
	{
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rs);
	if (row == NULL)
	{
		return "";
	}
	mysql_free_result(rs);
	return row[0];
}

bool CMySqlHPE::Addwork(CString strFile, F_HEAD& head, uint64_t& fileID, CString& strGwdNo, double& gps_log, double& gps_lat)
{
	int idx = strFile.ReverseFind('/');
	CString strOldFile = strFile.Mid(idx + 1);

	fprintf(pLogFileMysql, "[%s]\n", (char*)(LPCTSTR)strFile);
	fflush(pLogFileMysql);

	uint16_t gwd;
	g_strNewFileName = GetFileName(strFile, head, gwd, gps_log, gps_lat);
	if (g_strNewFileName.GetLength() == 0)
	{
		return false;
	}
	CString strSql;
	strSql.Format("select id from f_head where data_path = '%s'", g_strNewFileName);
	MYSQL_RES* rs;
	if (!Query(strSql, &rs))
	{
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rs);
	if (row != NULL)
	{
	//	fprintf(pLogFileMysql, "\t 数据已存在\r\n");
	//	g_FileID = strtoull(row[0], NULL, 10);
	//	AfxMessageBox("该数据已存在！", MB_OK | MB_ICONWARNING);
	//	return false;
	}
	mysql_free_result(rs);

	CString strBigGate, strSmallGate, strAngle, strPlace, strZero, strYZZ, strYZ;
	CString strTemp;

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < GA_NUM; ++j)
		{
			strTemp.Format("%d,%d,%d ", head.deviceP2.Gate[i][j].start, head.deviceP2.Gate[i][j].end, head.deviceP2.Gate[i][j].isOn);
			strBigGate += strTemp;
		}
		strBigGate += " ";
	}
	strBigGate = strBigGate.Left(strBigGate.GetLength() - 1);

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < DOR_NUM; ++j)
		{
			strTemp.Format("%d,%d,%d ", head.deviceP.Doors[i][j].start, head.deviceP.Doors[i][j].end, head.deviceP.Doors[i][j].isOn);
			strSmallGate += strTemp;
		}
		strSmallGate += " ";
	}
	strSmallGate = strSmallGate.Left(strSmallGate.GetLength() - 1);

	for (int i = 0; i < CH_N - 1; ++i)
	{
		strTemp.Format("%d,%d ", head.deviceP2.Angle[i].Refrac, head.deviceP2.Angle[i].Angle);
		strAngle += strTemp;
	}
	strTemp.Format("%d,%d", head.deviceP2.Angle[CH_N - 1].Refrac, head.deviceP2.Angle[CH_N - 1].Angle);
	strAngle += strTemp;

	for (int i = 0; i < CH_N - 1; ++i)
	{
		strTemp.Format("%d,", head.deviceP2.Place[i]);
		strPlace += strTemp;
	}
	strTemp.Format("%d", head.deviceP2.Place[CH_N - 1]);
	strPlace += strTemp;

	for (int i = 0; i < CH_N - 1; ++i)
	{
		strTemp.Format("%d,", head.deviceP2.Zero[i]);
		strZero += strTemp;
	}
	strTemp.Format("%d", head.deviceP2.Zero[CH_N - 1]);
	strZero += strTemp;

	for (int i = 0; i < CH_N - 1; ++i)
	{
		strTemp.Format("%d,", head.deviceP2.Restr[i]);
		strYZZ += strTemp;
	}
	strTemp.Format("%d", head.deviceP2.Restr[CH_N - 1]);
	strYZZ += strTemp;

	for (int i = 0; i < CH_N - 1; ++i)
	{
		strTemp.Format("%d,", head.deviceP2.Trig[i]);
		strYZ += strTemp;
	}
	strTemp.Format("%d", head.deviceP2.Trig[CH_N - 1]);
	strYZ += strTemp;

	g_startPos = GetWalk(head.startKm);
	g_endPos = GetWalk(head.endKm);
	if (g_endPos < 0.001)
	{
		if (head.deviceP2.TrackSet.WalkWay == 1)//顺里程
		{
			g_endPos = g_startPos + 0.000001 * head.step * head.distance;
		}
		else//逆里程
		{
			g_endPos = g_startPos - 0.000001 * head.step * head.distance;
		}
	}

	g_strRailNo = ToHexCString(head.deviceP2.TrackSet.lineNum, 4);
	strSql.Format("select rail_name from rail_info where rail_no = '%s'", g_strRailNo);
	if (!Query(strSql, &rs))
	{
		return false;
	}
	row = mysql_fetch_row(rs);
	if (row == NULL)
	{
		fprintf(pLogFileMysql, "\t 无法找到对应的线名：%s \r\n", g_strRailNo);
		return false;
	}
	char name[100] = { 0 };
	strncpy(name, row[0], strlen(row[0]));
	g_strRailName.Format("%s", name);
	mysql_free_result(rs);
	if (gwd != 0)
	{
		strGwdNo.Format("%05d", gwd);
	}
	else
	{
		strGwdNo = GetGwdNo(g_strRailNo, head.deviceP2.TrackSet.lineWay, g_startPos);
		if (strGwdNo.IsEmpty())
		{
			fprintf(pLogFileMysql, "\t 无法找到对应的工务段：%s %f\r\n", g_strRailNo);
			return false;
		}
	}


	strSql.Format("Insert into f_head(data_path, old_path, zuoyebanzu, rail_no, xingbie, gubie, length, s_mil, e_mil, ins_no, damen_set, xiaomen_set, tt_angle, tt_place, tt_zero, all_step, sd_type, station_no, gwd_no, gongqu_no) values('%s','%s','%s', '%s', %d, %d, %.5lf, %.5lf, %.5lf, '%s', '%s', '%s', '%s', '%s', '%s', %d, %d, %d, '%s', '%s');",
		g_strNewFileName, strOldFile, ToHexCString(head.deviceP2.TrackSet.teamNum, 4), g_strRailNo, head.deviceP2.TrackSet.lineWay, head.deviceP2.TrackSet.leftRight, abs(g_endPos - g_startPos),
		g_startPos, g_endPos, head.DeviceNum, strBigGate, strSmallGate, strAngle, strPlace, strZero,
		head.distance, head.deviceP2.TrackSet.lineType, BCDToINT32(head.deviceP2.TrackSet.stationNum), strGwdNo, ToHexCString(head.deviceP2.TrackSet.regionNum, 4));
	if (!Execute(strSql))
	{
		return false;
	}

	g_xingbie = head.deviceP2.TrackSet.lineWay;
	g_strXingbie = g_strXingBieDefines[g_xingbie];
	g_gubie = head.deviceP2.TrackSet.leftRight;
	g_strGubie = g_strGuBieDefines[g_gubie];
	fileID = GetLastInsertID();
	return true;
}

bool CMySqlHPE::GetBlocks(uint64_t fileID, vector<BLOCK_SQL>& blocks)
{
	CString strSql;
	strSql.Format(" select * from m_head where head_id = %I64d;", fileID);
	MYSQL_RES *rs;
	if (!Query(strSql, &rs))
	{
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rs);
	while (row != NULL)
	{
		BLOCK_SQL block;
		block.FileID = strtoull(row[1], NULL, 10);
		block.Index = strtoul(row[2], NULL, 10);
		memcpy(block.Gain, row[3], strlen(row[3]));
		block.speed = strtoul(row[4], NULL, 10);
		memcpy(block.date, row[5], strlen(row[5]));
		block.walk = strtod(row[6], NULL);
		memcpy(block.Place, row[7], strlen(row[7]));
		block.rail_height = strtoul(row[8], NULL, 10);
		block.Step = strtoul(row[9], NULL, 10);
		block.Step2 = strtoul(row[10], NULL, 10);
		block.rail_type = strtoul(row[11], NULL, 10);
		block.gps_long = strtod(row[12], NULL);
		block.gps_lat = strtod(row[13], NULL);
		block.ZX = strtoul(row[14], NULL, 10);

		block.StartA = strtoull(row[15], NULL, 10);
		block.StartB = strtoull(row[16], NULL, 10);

		row = mysql_fetch_row(rs);
		blocks.push_back(block);
	}
	mysql_free_result(rs);
	return true;
}

bool CMySqlHPE::AddBlocks(vector<BLOCK>& blocks, vector<Read_Info>& vA, vector<Read_Info>& vB)
{
	if (blocks.size() == 0)
	{
		return true;
	}
	CString strTemp;
	CString strSql = _T("insert into m_head(head_id, m_count, zyz, speed, date, now_mil, tt_place, rail_type, rail_height, s_step, rs_step, gps_long, gps_lat, zhanxian_no, as_byte, bs_byte) values");
	int nIndex = CH_N - 1;
	char szTime[20];
	for (int i = 0; i < blocks.size(); ++i)
	{
		CString strZY;
		for (int j = 0; j < nIndex; ++j)
		{
			strTemp.Format("%d,", blocks[i].gain[j]);
			strZY += strTemp;
		}
		strTemp.Format("%d", blocks[i].gain[nIndex]);
		strZY += strTemp;

		CString strPlace;
		for (int j = 0; j < nIndex; ++j)
		{
			strTemp.Format("%d,", blocks[i].probOff[j]);
			strPlace += strTemp;
		}
		strTemp.Format("%d", blocks[i].probOff[nIndex]);
		strPlace += strTemp;
		sprintf_s(szTime, "%4d-%02d-%02d %02d:%02d:%02d", year, month, day, (blocks[i].time & 0xFF0000) >> 16, (blocks[i].time & 0x00FF00) >> 8, (blocks[i].time & 0x0000FF));

		double lat = 0, log = 0;
		ParseGPS(blocks[i].gpsInfor, log, lat);

		strTemp.Format("(%I64d, %d,'%s', %d, '%s', %lf, '%s',%d, %d, %d, %d, %lf, %lf, %d, %d, %d),",
			g_FileID, i, strZY, blocks[i].speed, szTime, GetWalk(blocks[i].walk), strPlace,
			blocks[i].railType, blocks[i].railH, blocks[i].indexL, 0, log, lat, blocks[i].rev3 * 10 + blocks[i].rev2, vA[i].UsedL, vB[i].UsedL);
		strSql += strTemp;
	}
	strSql = strSql.Left(strSql.GetLength() - 1) + ";";
	return Execute(strSql);
}

bool CMySqlHPE::AddSteps_A(vector<A_Step>& vSteps)
{
	if (vSteps.size() == 0)
	{
		return true;
	}
	CString strSql = _T("insert into data_a(head_id, frameIndex, step, frame) values");
	CString strTemp;
	CString strFrame, strCh;
	uint16_t count = 0, ichannelCount = 0;
	for (int i = 0; i < vSteps.size(); ++i)
	{
		strFrame = "";
		ichannelCount = 0;
		A_Step& step = vSteps[i];
		for (int k = 0; k < CH_N; ++k)
		{
			count = 0;
			strCh.Format("{%d:", k);
			for (int j = 0; j < step.Frames.size(); ++j)
			{
				if (step.Frames[j].F[k] > 0)
				{
					++count;
					strTemp.Format("[%d,%d],", step.Frames[j].Horizon, step.Frames[j].F[k]);
					strCh += strTemp;
				}
			}
			if (count > 0)
			{
				++ichannelCount;
				strFrame += strCh.Left(strCh.GetLength() - 1) + "},";
			}
		}
		strFrame = strFrame.Left(strFrame.GetLength() - 1);
		strTemp.Format(_T("(%I64d, %d, %d, '%s'),"), g_FileID, step.Index2, step.Step, strFrame);
		strSql += strTemp;
	}
	if (vSteps.size() > 1)
	{
		strSql = strSql.Left(strSql.GetLength() - 1) + ";";
	}
	return Execute(strSql);
}

bool CMySqlHPE::AddSteps_B(vector<WaveData>* vDatas)
{
	CString strSql = _T("insert into data_b(head_id, channel, step, row) values");
	CString strTemp;
	int nFrame = 0;
	for (int m = 0; m < 16; ++m)
	{
		for (int i = 0; i < vDatas[m].size(); ++i)
		{
			strTemp.Format("(%I64d, %d, %d, %d),", g_FileID, m, vDatas[m][i].step, vDatas[m][i].row);
			strSql += strTemp;
		}
	}
	strSql = strSql.Left(strSql.GetLength() - 1) + ";";
	return Execute(strSql);
}

bool CMySqlHPE::AddCRs(vector<Connected_Region> & vcr)
{
	if (vcr.size() == 0)
	{
		return true;
	}
	CString strSql = _T("insert into lty_info(head_id, channel, block, s_step, flag) values");
	CString strTemp;
	for (int i = 0; i < vcr.size(); ++i)
	{
		strTemp.Format(_T("(%I64d, %d, %d, %d, %d),"), g_FileID, GetAChannelByBChannel(vcr[i].Channel), vcr[i].Block, vcr[i].Step, (vcr[i].Flag & 0x01));
		strSql += strTemp;
	}
	strSql = strSql.Left(strSql.GetLength() - 1) + ";";
	if (!Execute(strSql))
	{
		return false;
	}
	uint64_t crID = GetLastInsertID();
	strSql = _T("insert into tpb_data(s_step, s_row, c_id) values");
	for (int i = 0; i < vcr.size(); ++i)
	{
		//vcr[i].CRID = crID + i;
		//vcr[i].WorkID = g_FileID;
		for (int j = 0; j < vcr[i].Region.size(); ++j)
		{
		//	strTemp.Format("(%d, %d, %I64d),", vcr[i].Region[j].step - vcr[i].Step1, vcr[i].Region[j].row, vcr[i].CRID);
			strSql += strTemp;
		}
	}
	strSql = strSql.Left(strSql.GetLength() - 1) + ";";
	return Execute(strSql);
}

//bool CMySqlHPE::UpdateCR(Connected_Region& cr)
//{	
//	CString strTemp;
//	CString strA = "";
//	for (int j = 0; j < cr.vASteps.size(); ++j)
//	{
//		strTemp.Format("%d:[", cr.vASteps[j].Step);
//		strA += strTemp;
//
//		uint8_t iChA = GetAChannelByBChannel(cr.Channel);
//		for (int k = 0; k < cr.vASteps[j].Frames.size(); ++k)
//		{
//			strTemp.Format("%d,%d ", cr.vASteps[j].Frames[k].Horizon, cr.vASteps[j].Frames[k].F[iChA]);
//			strA += strTemp;
//		}	
//		strA += "]";
//	}	
//	
//	CString strSql;
//	strSql.Format(_T("update lty_info set tpa_data = '%s' where id = %I64d"), strA, cr.CRID);
//	return Execute(strSql);
//}

bool CMySqlHPE::UpdateCR(Connected_Region& cr)
{
	CString strTemp;
	CString strSql = "insert into tpa_data(c_id, step, frame, tpa_data) values";
	for (int j = 0; j < cr.vASteps.size(); ++j)
	{
		CString strA;
		uint8_t iChA = GetAChannelByBChannel(cr.Channel);
		for (int k = 0; k < cr.vASteps[j].Frames.size(); ++k)
		{
			strTemp.Format("%d,%d ", cr.vASteps[j].Frames[k].Horizon, cr.vASteps[j].Frames[k].F[iChA]);
			strA += strTemp;
		}
		if (strA.GetLength() > 0)
		{
			strA = strA.Left(strA.GetLength() - 1);
		//	strTemp.Format("(%I64d, %d, %d, '%s'),", cr.CRID, cr.Step1, cr.vASteps[j].Index2, strA);
			strSql += strTemp;
		}
	}
	if (strSql.GetLength() > 60)
	{
		strSql = strSql.Left(strSql.GetLength() - 1);
		return Execute(strSql);
	}
	return true;
}

bool CMySqlHPE::AddPositionMarks(vector<Position_Mark>& vPMs)
{
	if (vPMs.size() == 0)
	{
		return true;
	}
	CString strTemp;
	CString strSql = "Insert into mark_info(head_id, rail_no, gubie, xingbie, xianbie, gps_long, gps_lat,mark_type, mark_type2, mark_mil, block, step, ARow, AStep, Chnum, num,fangcha,size, length, height) values";

	for (int i = 0; i < vPMs.size(); ++i)
	{
		Position_Mark& pm = vPMs[i];
		CString strNum;
		for (int i = 0; i < 15; ++i)
		{
			strTemp.Format("%d,", pm.Num[i]);
			strNum += strTemp;
		}
		strTemp.Format("%d", pm.Num[15]);
		strNum += strTemp;

		strTemp.Format("(%I64d, '%s', %d, %d, %d, %lf, %lf, %d, %d, %lf, %d, %d, %d, %d, %d, '%s',%lf, %d, %d, %d),",
			g_FileID, g_strRailNo, pm.Gubie, pm.xingbie, pm.xianbie, pm.gps_log, pm.gps_lat, pm.Mark, pm.Mark2,
			pm.Walk, pm.Block, pm.Step, pm.ARow, pm.AStep, pm.ChannelNum, strNum, pm.Fangcha, pm.Size, pm.Length, pm.Height);
		strSql += strTemp;
	}

	strSql = strSql.Left(strSql.GetLength() - 1) + ";";
	if (!Execute(strSql))
	{
		return false;
	}

	uint64_t id = GetLastInsertID();
	for (int i = 0; i < vPMs.size(); ++i)
	{
		vPMs[i].id = id + i;
	}
	return true;
}

bool CMySqlHPE::GetPositionMarks(CString strRailNo, uint8_t xingbie, uint8_t gubie, double start, double end, vector<Position_Mark>& vPMs)
{
	CString strSql;
	strSql.Format("select * from mark_info where rail_no = '%s' and xingbie = %d and gubie = %d and mark_type2 = %d and marl_mil >= %lf and mid <= %lf order by s_mil",
		strRailNo, xingbie, gubie, PM_SEW_LRH, start, end);
	MYSQL_RES* rs;
	if (!Query(strSql, &rs))
	{
		return false;
	}

	Position_Mark pm;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(rs)))
	{
		pm.id = strtoull(row[0], NULL, 10);
		memcpy(pm.railNo, row[2], strlen(row[2]));
		pm.xingbie = xingbie;
		pm.Gubie = gubie;
		pm.gps_log = atof(row[6]);
		pm.gps_lat = atof(row[7]);
		pm.Mark = strtol(row[8], NULL, 10);
		pm.Walk = atof(row[18]);
		vPMs.push_back(pm);
	}
	mysql_free_result(rs);
	return true;
}

uint64_t CMySqlHPE::GetTimeStamp(char* pStr)
{
	return 0;
}

bool CMySqlHPE::GetWound(uint64_t id, Wound_Judged& wd)
{
	CString strSql;
	strSql.Format("select * from wound where id = %I64d", id);
	MYSQL_RES* rs;
	if (!Query(strSql, &rs))
	{
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(rs);
	if (!row)
	{
		return false;
	}
	
	wd.id = id;

	if (row[1] == NULL)	memset(wd.WoundNo, 0, sizeof(wd.WoundNo));
	else memcpy(wd.WoundNo, row[1], strlen(row[1]));

	if (row[2] == NULL)	memset(wd.WoundNo, 0, sizeof(wd.WoundNo));
	else memcpy(wd.gwdNo, row[2], strlen(row[2]));
	
	wd.FileID = strtoull(row[3], NULL, 10);
	wd.Block = strtoul(row[4], NULL, 10);
	wd.Step = strtoul(row[5], NULL, 10);
	wd.Step2 = strtoul(row[6], NULL, 10);
	memcpy(wd.FoundTime, row[7], strlen(row[7]));
	memcpy(wd.RailNo, row[8], strlen(row[8]));
	wd.LeftRight = strtoul(row[9], NULL, 10);
	wd.XingType = strtoul(row[10], NULL, 10);
	wd.Walk = strtod(row[11], NULL);
	wd.gps_log = strtod(row[12], NULL);
	wd.gps_lat = strtod(row[13], NULL);

	//wd.year 14
	//wd.month 15

	memcpy(wd.ana_time, row[16], strlen(row[16]));
	wd.SizeX = strtol(row[17], NULL, 10);
	wd.SizeY = strtol(row[18], NULL, 10);
	wd.Type = strtol(row[19], NULL, 10);
	wd.Degree = strtol(row[20], NULL, 10);
	wd.Place = strtol(row[21], NULL, 10);

	wd.BridgeID = strtoull(row[22], NULL, 10);
	wd.TunnelID = strtoull(row[23], NULL, 10);
	wd.CurveID = strtoull(row[24], NULL, 10);

	wd.IsJoint = strtol(row[25], NULL, 10);
	wd.IsSew = strtol(row[26], NULL, 10);
	wd.IsScrewHole = strtol(row[27], NULL, 10);
	wd.IsGuideHole = strtol(row[28], NULL, 10);

	wd.Xianbie = strtol(row[29], NULL, 10);
	wd.Cycle = strtol(row[30], NULL, 10);
	wd.LastCycleID = strtoull(row[31], NULL, 10);

	wd.Checked = strtol(row[32], NULL, 10);
	if (row[33] == NULL)  memset(wd.Analyser, 0, sizeof(wd.Analyser));
	else memcpy(wd.Analyser, row[33], strlen(row[33]));

	if (row[34] == NULL)  memset(wd.CheckTime, 0, sizeof(wd.CheckTime));
	else memcpy(wd.CheckTime, row[34], strlen(row[34]));

	/*
	wd.SizeX2 = strtol(row[35], NULL, 10);
	wd.SizeY2 = strtol(row[36], NULL, 10);
	wd.Type2 = strtol(row[37], NULL, 10);
	wd.Degree2 = strtol(row[38], NULL, 10);
	wd.Place2 = strtol(row[39], NULL, 10);
	wd.Walk2 = strtod(row[40], NULL);
	*/
	//memcpy(wd.Desc, row[41], strlen(row[41]));
	if (row[41] == NULL)  memset(wd.Desc, 0, sizeof(wd.CheckTime));
	else memcpy(wd.Desc, row[41], strlen(row[41]));

	if (row[42] != NULL)wd.Manual = strtol(row[42], NULL, 10);
	else wd.Manual = 0;

	//memcpy(wd.According, row[43], strlen(row[43])); 
	if (row[43] == NULL)  memset(wd.According, 0, sizeof(wd.According));
	else memcpy(wd.According, row[43], strlen(row[43]));

	mysql_free_result(rs);
	return true;
}

bool CMySqlHPE::AddWounds(vector<Wound_Judged>& vWounds)
{
	if (vWounds.size() == 0)
	{
		return true;
	}
	CString strTemp;
	CString strSql = "Insert into wound(gwd_no, head_id, mi_num, step, step2, wx_size, wy_size, w_type, w_degree, w_place, w_address, found_time,rail_no, gubie, xingbie, xianbie, gps_long, gps_lat, ana_time, bridge_id, tunnel_id, curve_id, jietou, hanfeng, luokong, daokong, zhouqi, upw_id, year, month,manual,According) values";
	for (int i = 0; i < vWounds.size(); ++i)
	{
		Wound_Judged& wd = vWounds[i];
		strTemp.Format(_T("('%s',%I64d, %d, %d, %d,%d,%d,%d,%d,%d,%lf,'%s','%s',%d,%d,%d,%lf,%lf,'%s',%I64d,%I64d,%I64d,%d,%d,%d,%d,%d,%I64d,%d,%d,%d,'%s'),"),
			g_strGwdNo, g_FileID, wd.Block, wd.Step, wd.Step2, wd.SizeX, wd.SizeY, wd.Type, wd.Degree, wd.Place, wd.Walk, wd.FoundTime,
			g_strRailNo, g_gubie, g_xingbie, wd.Xianbie, wd.gps_log, wd.gps_lat, wd.ana_time,
			wd.BridgeID, wd.TunnelID, wd.CurveID, wd.IsJoint, wd.IsSew, wd.IsScrewHole, wd.IsGuideHole, wd.Cycle, wd.LastCycleID, CurrentYear, CurrentMonth, wd.Manual, wd.According
			);
		strSql += strTemp;
	}
	strSql = strSql.Left(strSql.GetLength() - 1) + ";";
	if (!Execute(strSql))
	{
		return false;
	}

	uint64_t id = GetLastInsertID();
	for (int i = 0; i < vWounds.size(); ++i)
	{
		vWounds[i].id = id + i;
	}
	return true;
}


bool CMySqlHPE::GetCurves(vector<Curve>& curves, CString railNo, double start, double end)
{
	CString strSql;
	strSql.Format("select * from curve_info where rail_no = '%s' and s_mil >= %lf and e_mil <= %lf order by s_mil", railNo, start - 1, end + 1);
	MYSQL_RES* rs;
	if (!Query(strSql, &rs))
	{
		return false;
	}

	Curve curve;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(rs)))
	{
		curve.Start = atof(row[2]);
		curve.End = atof(row[3]);

		if (curve.End < start - 1)
		{
			continue;
		}

		curve.id = strtoull(row[0], NULL, 10);
		curve.Direction = atoi(row[1]);

		curve.Length = atof(row[4]);
		curve.R = atof(row[5]);
		curve.OverH = atof(row[6]);
		curve.HC = atof(row[7]);

		curve.bureauNo = strtoull(row[10], NULL, 10);
		//curve.gwdNo = strtoull(row[11], NULL, 10);
		curves.push_back(curve);
	}
	mysql_free_result(rs);
	return true;
}


bool CMySqlHPE::GetBridges(vector<Bridge>& bridges, CString railNo,  double start, double end)
{
	CString strSql;
	strSql.Format("select * from bridge_info where rail_no = '%s' and s_mil >= %lf and e_mil <= %lf order by s_mil", railNo, start - 1, end + 1);
	MYSQL_RES* rs;
	if (!Query(strSql, &rs))
	{
		return false;
	}

	Bridge bridge;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(rs)))
	{
		bridge.Start = atof(row[5]);
		bridge.End = atof(row[6]);

		if (bridge.End < start - 1)
		{
			continue;
		}

		bridge.id = strtoull(row[0], NULL, 10);
		strcpy(bridge.No, row[1]);
		strcpy(bridge.Name, row[2]);

		bridge.gwdNo = strtoull(row[8], NULL, 10);
		bridge.bureauNo = strtoull(row[9], NULL, 10);

		bridges.push_back(bridge);
	}
	mysql_free_result(rs);
	return true;
}


bool CMySqlHPE::GetTunnels(vector<Tunnel>& tunnels, CString railNo, uint8_t xingbie, double start, double end)
{
	CString strSql;
	strSql.Format("select * from tunnel_info where rail_no = '%s' and xingbie = %d and s_mil >= %lf and e_mil <= %lf order by s_mil", railNo, xingbie, start - 1, end + 1);
	MYSQL_RES* rs;
	if (!Query(strSql, &rs))
	{
		return false;
	}

	Tunnel tunnel;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(rs)))
	{
		tunnel.Start = atof(row[5]);
		tunnel.End = atof(row[6]);

		if (tunnel.End < start - 1)
		{
			continue;
		}

		tunnel.id = strtoull(row[0], NULL, 10);
		strcpy(tunnel.No, row[1]);
		strcpy(tunnel.Name, row[2]);
		tunnels.push_back(tunnel);
	}
	mysql_free_result(rs);
	return true;
}