#include "stdafx.h"
#include "Judge.h"
#include "tlts.h"

#include <process.h>
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib") 

//char ChannelNames[13] = "AaBbCDdFceEG";
//CString ChannelNamesB[16] = { "A1", "A2", "a1", "a2", "B1", "B2", "b1", "b2", "C", "c", "D", "d", "E", "e", "F", "G" };

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

TRACK_P		traceParam;

const uint32_t bits[32] =
{ BIT0, BIT1, BIT2, BIT3, BIT4, BIT5, BIT6, BIT7, BIT8, BIT9,
BIT10, BIT11, BIT12, BIT13, BIT14, BIT15, BIT16, BIT17, BIT18, BIT19,
BIT20, BIT21, BIT22, BIT23, BIT24, BIT25, BIT26, BIT27, BIT28, BIT29,
BIT30, BIT31
};

//const double dB_Std[12] = { 38, 38, 38, 38, 0 };
const uint8_t iGuiERow[] = { 13, 13, 13, 13 };

const uint16_t iLuokong_D_Row1_L[4] = { 0, 18, 22, 22 };//螺孔出波D的行
const uint16_t iLuokong_D_Row1_H[4] = { 0, 28, 29, 30 };

const float fC_L[4] = { 0, 25, 45, 0 };//接头位置C两处出波步进差
const float fC_H[4] = { 0, 35, 55, 0 };

const UINT16 rail_hDC[4] = { 33, 35, 38, 46 };			// 轨头内的内外70度反射点的高度mm）
const UINT16 rail_uDC[4] = { 140, 152, 176, 192 };      // 轨底高度

// A, a, B, b, C, D, d, F, c, e, E, G
const double StandardGain[CH_N] = { 38.0, 38.0, 38.0, 38.0, 38.0,
46.0, 46.0, 46.0, 38.0, 46.0, 46.0, 46.0 };

/*
一二次波对应
A1 <=> a2
A2 <=> a1
B1 <=> b2
B2 <=> b1
*/
const uint8_t rcs[8] = { CH_a1, CH_a1, CH_A1, CH_A1, CH_b1, CH_b1, CH_B1, CH_B1 };

const uint8_t ChannelB2A[] = { ACH_A, ACH_A, ACH_a, ACH_a, ACH_B, ACH_B, ACH_b, ACH_b, ACH_C, ACH_c, ACH_D, ACH_d, ACH_E, ACH_e, ACH_F, ACH_G };

static double AngleToRad = 3.1415926 / 180;

map<int, int> mBlockIndex;


float HOR_POINT_FOR_LENTH = 2.7;
float VER_POINT_FOR_LENTH = 3;

#define WINDOW_STEPS	(50)
int hour = 0;
int minute = 0;
int second = 0;

FILE* pLogFile = NULL;
SOCKET sclient;

vector<Curve> vCurves;
vector<Bridge> vBridges;
vector<Tunnel> vTunnels;

Position_Mark::Position_Mark()
{
	memset(this, 0, sizeof(Position_Mark));
	this->gps_lat = 0;
	this->gps_log = 0;
}

Wound_Judged::Wound_Judged()
{
	memset(this, 0, sizeof(Wound_Judged));
	this->Cycle = 1;
}

bool AddWork(char* strFile, double& gps_log, double& gps_lat)
{
#ifdef _EXPORT_TO_MYSQL
	F_HEAD head;
	vTunnels.clear();
	vCurves.clear();
	vBridges.clear();
	bool bOK = theApp.sql.Addwork(strFile, head, g_FileID, g_strGwdNo, gps_log, gps_lat);
	if (bOK == false)
	{
		return false;
	}

	theApp.sql.GetBridges(vBridges, g_strRailNo, g_startPos - 1, g_endPos);
	theApp.sql.GetCurves(vCurves, g_strRailNo, g_startPos - 1, g_endPos);
	theApp.sql.GetTunnels(vTunnels, g_strRailNo, g_gubie, g_startPos - 1, g_endPos);

	traceParam = head.deviceP2.TrackSet;
	return true;
#else
	return true;
#endif
}


bool AddWounds(vector<Wound_Judged>& wounds)
{
#ifdef _EXPORT_TO_MYSQL
	return theApp.sql.AddWounds(wounds);
#else
	return true;
#endif
}

bool AddPMs(vector<Position_Mark>& vPMs)
{
#ifdef _EXPORT_TO_MYSQL
	return theApp.sql.AddPositionMarks(vPMs);
#else
	return true;
#endif
}

bool AddBlocks(vector<BLOCK>& blocks, vector<Read_Info>& vA, vector<Read_Info>& vB)
{
#ifdef _EXPORT_TO_MYSQL
	return theApp.sql.AddBlocks(blocks, vA, vB);
#else
	return true;
#endif
}

void WriteLog(char* log)
{
	fprintf(pLogFile, log);
	fflush(pLogFile);
}



uint32_t Abs(int x)
{
	return x >= 0 ? x : -x;
}


uint8_t	CanCombine(Connected_Region cr1, Connected_Region cr2, uint8_t channel)
{
	Connected_Region cr = cr1;
	if (cr1.Region.size() == 0 || cr2.Region.size() == 0)
	{
		return 0;
	}
	if (cr2.Step1 - cr.Step2 > 5)
	{
		return false;
	}
	Combine(cr, cr2);
	if (channel == CH_D)
	{
		Combine(cr, cr2);
		double k = -9999;
		if (cr.Step1 != cr.Step2)
		{
			k = 1.0 * (cr.Row1 - cr.Row2) / (cr.Step2 - cr.Step1);
		}
		return k >= -1 && k <= -0.5;
	}
	if (channel == CH_E)
	{
		Combine(cr, cr2);
		double k = -9999;
		if (cr.Step1 != cr.Step2)
		{
			k = 1.0 * (cr.Row2 - cr.Row1) / (cr.Step2 - cr.Step1);
		}
		return k >= 0.5 && k <= 1;
	}
	return false;
}

void Combine(Connected_Region& cr1, Connected_Region& cr2)
{
	for (int i = 0; i < cr2.Region.size(); ++i)
	{
		cr1.Region.push_back(cr2.Region[i]);
	}
	FillCR(cr1);
}

void CombineD(vector<Connected_Region> &vCR)
{
	for (int idx = 0; idx < vCR.size(); ++idx)
	{
		if (vCR[idx].Flag == 1)
		{
			continue;
		}
		for (int i_LastFind = idx + 1; i_LastFind < vCR.size(); ++i_LastFind)
		{

			if (vCR[i_LastFind].Step1 - vCR[idx].Step2 > 10)
			{
				break;
			}

			if (CanCombine(vCR[idx], vCR[i_LastFind], CH_D))
			{
				Combine(vCR[idx], vCR[i_LastFind]);
				vCR[i_LastFind].Region.clear();
				vCR[i_LastFind].Flag = 1;
			}
		}
	}
}

void CombineE(vector<Connected_Region> &vCR)
{
	for (int idx = 0; idx < vCR.size(); ++idx)
	{
		if (vCR[idx].Flag == 1)
		{
			continue;
		}
		for (int i_LastFind = idx + 1; i_LastFind < vCR.size(); ++i_LastFind)
		{
			if (vCR[i_LastFind].Step1 - vCR[idx].Step2 > 10)
			{
				break;
			}

			if (CanCombine(vCR[idx], vCR[i_LastFind], CH_D))
			{
				Combine(vCR[idx], vCR[i_LastFind]);
				vCR[i_LastFind].Region.clear();
				vCR[i_LastFind].Flag = 1;
			}
		}
	}
}

void CombineFG(vector<Connected_Region>& vCR)
{
	for (int idx = 0; idx < vCR.size(); ++idx)
	{
		if (vCR[idx].Flag == 1)
		{
			continue;
		}
		int iFlag = (vCR[idx].Region[0].find & BIT0);
		for (int i_LastFind = idx + 1; i_LastFind < vCR.size(); ++i_LastFind)
		{
			if ((vCR[i_LastFind].Region.size() == 0 || vCR[i_LastFind].Region[0].find & BIT0) != iFlag)
			{
				continue;
			}
			if (vCR[i_LastFind].Step2 < vCR[idx].Step1 - 1)
			{
				continue;
			}
			if (vCR[i_LastFind].Step1 > vCR[idx].Step2 + 2)
			{
				break;
			}

			if (GetDistance(vCR[i_LastFind], vCR[idx]) <= 2)
			{
				Combine(vCR[idx], vCR[i_LastFind]);
				vCR[i_LastFind].Region.clear();
				vCR[i_LastFind].Flag = 1;
			}
		}
	}
}

uint32_t GetDistance(Connected_Region& cr1, Connected_Region& cr2)
{
	uint32_t dist = 100;
	for (int i = 0; i < cr1.Region.size(); ++i)
	{
		for (int j = 0; j < cr2.Region.size(); ++j)
		{
			if (Abs(cr2.Region[j].block - cr1.Region[i].block) > 1)
			{
				continue;
			}

			int r = Abs(cr2.Region[j].row - cr1.Region[i].row);
			int s = Abs(cr2.Region[j].step - cr1.Region[i].step);
			uint32_t t = s + r;
			if (t < dist)
			{
				dist = t;
				if (dist == 1)
				{
					break;
				}
			}
		}
	}
	return dist;
}

void FillCR(Connected_Region& cr)
{
	if (cr.Region.size() == 0)
	{
		return;
	}
	int nsize = cr.Region.size();
	cr.Block = cr.Region[0].block;
	cr.Step1 = cr.Region[0].step;
	cr.Row1 = cr.Region[0].row;
	cr.Step2 = cr.Region[nsize - 1].step;
	cr.Row2 = cr.Region[nsize - 1].row;

	for (int j = 0; j < nsize; ++j)
	{
		if (cr.Region[j].step > cr.Step2)
		{
			cr.Step2 = cr.Region[j].step;
		}

		if (cr.Region[j].step < cr.Step1)
		{
			cr.Step1 = cr.Region[j].step;
		}

		if (cr.Region[j].row < cr.Row1)
		{
			cr.Row1 = cr.Region[j].row;
		}

		if (cr.Region[j].row > cr.Row2)
		{
			cr.Row2 = cr.Region[j].row;
		}
	}
}

void CreateCR(vector<BlockData_B>& datas, vector<Connected_Region>* vCRs)
{
	for (int i = 0; i < 16; ++i)
	{
		vCRs[i].clear();
	}

	map<int, int> mBlockIndex;
	for (int i = 0; i < datas.size(); ++i)
	{
		mBlockIndex.insert(make_pair(datas[i].Index, i));
	}

	//C, c, D, d, E, e, F, G
	vector<WaveData> wdata[16];	//16个通道的连通域

	//建立A1, A2, a1, a2, B1, B2, b1, b2
	WaveData wd;
	for (int j = 0; j < datas.size(); ++j)
	{
		for (int step = 0; step < datas[j].vBStepDatas.size(); ++step)
		{
			int railType = datas[j].BlockHead.railType & 0x03;
			for (int row = 0; row < datas[j].vBStepDatas[step].vRowDatas.size() && datas[j].vBStepDatas[step].vRowDatas[row].Row < 14; ++row)
			{
				for (int m = 0; m < 8; ++m)
				{
					if (datas[j].vBStepDatas[step].vRowDatas[row].Point.Draw1 & bits[m])
					{
						wd.block = datas[j].Index;
						wd.step = datas[j].vBStepDatas[step].Step;
						wd.find = 1;

						if (m % 2 == 0)
						{
							wd.row = datas[j].vBStepDatas[step].vRowDatas[row].Row;
							wdata[m].push_back(wd);
						}
						else//二次波
						{
							wd.row = (iGuiERow[railType] << 1) - datas[j].vBStepDatas[step].vRowDatas[row].Row;
							wdata[m - 1].push_back(wd);
						}
					}
				}
			}
		}
	}

	// C, c, D, d, E, e
	for (int j = 0; j < datas.size(); ++j)
	{
		for (int step = 0; step < datas[j].vBStepDatas.size(); ++step)
		{
			for (int row = 0; row < datas[j].vBStepDatas[step].vRowDatas.size(); ++row)
			{
				for (int m = CH_C; m < CH_F; ++m)
				{
					if (datas[j].vBStepDatas[step].vRowDatas[row].Point.Draw1 & bits[m])
					{
						wd.block = datas[j].Index;
						wd.row = datas[j].vBStepDatas[step].vRowDatas[row].Row;
						wd.step = datas[j].vBStepDatas[step].Step;
						wd.find = 1;
						wdata[m].push_back(wd);
					}
				}
			}
		}
	}

	vector<WaveData> wdata2[16];
	for (int i = 0; i < 16; ++i)
	{
		wdata2[i] = wdata[i];
	}

	//F, G
	for (int j = 0; j < datas.size(); ++j)
	{
		uint8_t iFRow1 = datas[j].BlockHead.railH / 3 - 3;
		uint8_t iFRow2 = datas[j].BlockHead.railH / 3 + 3;
		uint8_t iFrow = datas[j].BlockHead.railH / 3;
		for (int step = 0; step < datas[j].vBStepDatas.size(); ++step)
		{
			B_Step& b_step = datas[j].vBStepDatas[step];
			uint8_t iFind[2] = { 0 };
			for (int row = 0; row < datas[j].vBStepDatas[step].vRowDatas.size(); ++row)
			{
				//F,G通道在非轨底位置的出波
				for (int m = 14; m < 16; ++m)
				{
					B_RowData& b_row = datas[j].vBStepDatas[step].vRowDatas[row];
					if (b_row.Point.Draw1 & bits[m] && b_row.Row < iFRow1)
					{
						wd.block = datas[j].Index;
						wd.row = b_row.Row;
						wd.step = b_step.Step;
						wd.find = 1;
						wdata[m].push_back(wd);
						wdata2[m].push_back(wd);
					}
				}

				//F, G通道在轨底的失波
				if (datas[j].vBStepDatas[step].vRowDatas[row].Row >= iFRow1 && datas[j].vBStepDatas[step].vRowDatas[row].Row <= iFRow2)
				{
					B_RowData& b_row = datas[j].vBStepDatas[step].vRowDatas[row];
					for (int m = 14; m < 16; ++m)
					{
						if (b_row.Point.Draw1 & bits[m])
						{
							wd.block = datas[j].Index;
							wd.row = b_row.Row;
							wd.step = b_step.Step;
							wd.find = 1;
							wdata2[m].push_back(wd);
							++iFind[m - 14];
						}
					}
				}
			}

			for (int i = 0; i < 2; ++i)
			{
				if (iFind[i] == 0)
				{
					wd.block = datas[j].Index;
					wd.row = iFrow;
					wd.step = datas[j].vBStepDatas[step].Step;
					wd.find = 0;
					wdata[i + 14].push_back(wd);
				}
			}
		}
	}

#ifdef _EXPORT_TO_MYSQL
	//theApp.sql.AddSteps_B(wdata2);
#endif

	time_t timec;
	tm* p;
	time(&timec);
	p = localtime(&timec);
	PrintTime(*p, "提取图形点完毕！");


	//S3.1 获取连通域
	for (int m = 0; m < 16; ++m)
	{
		int iCount = wdata[m].size();
		int idx = 0;//寻找以第idx个出波位置为起点的连通域
		int ct = 0;
		while (idx < iCount)
		{
			Connected_Region cr;
			cr.Flag = 0;
			cr.Row1 = cr.Row2 = 0;
			cr.Step1 = cr.Step2 = 0;
			cr.Channel = m;
			int iFind = wdata[m][idx].find;
			int t_tt = iFind & BIT7;

			//还未处理
			if ((wdata[m][idx].find & BIT7) == 0)
			{
				wdata[m][idx].find |= BIT7;
				cr.Flag = 0;
				cr.Region.push_back(wdata[m][idx]);
				cr.Block = wdata[m][idx].block;
				++ct;
			}

			int i_LastFind = idx;
			for (int i = i_LastFind + 1; i < wdata[m].size(); ++i)
			{
				WaveData &wd = wdata[m][i];
				if (wdata[m][i].find & BIT7)//当前点已存在于某个连通域
				{
					continue;
				}
				else if (
					abs(wdata[m][i].step - wdata[m][i_LastFind].step) <= 1 &&
					abs(wdata[m][i].row - wdata[m][i_LastFind].row) <= 1 &&
					abs(wdata[m][i].block - wdata[m][i_LastFind].block) <= 1 &&
					(wdata[m][i].find & BIT0) == (wdata[m][i_LastFind].find & BIT0)
					)
				{
					wdata[m][i].find |= BIT7;
					cr.Region.push_back(wdata[m][i]);
					i_LastFind = i;
				}
				else if (wdata[m][i].block - wdata[m][i_LastFind].block > 1)
				{
					break;
				}
				else if (wdata[m][i].step - wdata[m][i_LastFind].step > 1 && wdata[m][i].row - wdata[m][i_LastFind].row > 1)
				{
					break;
				}
			}

			if (cr.Region.size() > 0)
			{
				vCRs[m].push_back(cr);
			}
			++idx;
		}
	}

	for (int m = 0; m < 16; ++m)
	{
		for (int i = 0; i < vCRs[m].size(); ++i)
		{
			FillCR(vCRs[m][i]);
			vCRs[m][i].Step = vCRs[m][i].Step1 - datas[mBlockIndex[vCRs[m][i].Block]].BlockHead.indexL;
			vCRs[m][i].Flag = 0;
		}
	}

	CombineD(vCRs[CH_D]);
	CombineE(vCRs[CH_E]);

	//3.2 初步融合相邻的连通域
	for (int m = 0; m < CH_e; ++m)
	{
		if (m == CH_d)
		{
			continue;;
		}
		int idx = 0;
		int nCount = vCRs[m].size() - 1;
		while (idx < nCount)
		{
			for (int i_Last = idx + 1; i_Last < nCount; ++i_Last)
			{
				if (GetDistance(vCRs[m][idx], vCRs[m][i_Last]) == 1)
				{
					for (int j = 0; j < vCRs[m][i_Last].Region.size(); ++j)
					{
						vCRs[m][idx].Region.push_back(vCRs[m][i_Last].Region[j]);
					}
					vCRs[m][i_Last].Region.clear();
				}

				if (vCRs[m][i_Last].Step1 > vCRs[m][idx].Step2 + 2)
				{
					break;
				}
			}

			++idx;
		}
	}

	CombineFG(vCRs[CH_F]);
	CombineFG(vCRs[CH_G]);

	for (int m = 0; m < 16; ++m)
	{
		for (vector<Connected_Region>::iterator itr = vCRs[m].begin(); itr < vCRs[m].end(); ++itr)
		{
			if (itr->Region.size() < 1)
			{
				itr = vCRs[m].erase(itr);
				--itr;
			}
		}
	}


	time(&timec);
	p = localtime(&timec);
	PrintTime(*p, "连通域建立完毕！");

	map<int, int> blockIndex;
	for (int i = 0; i < datas.size(); ++i)
	{
		blockIndex.insert(make_pair(datas[i].Index, i));
	}

	//3.3 连通域内部排序和参数计算
	for (int m = 0; m < 16; ++m)
	{
		for (int i = 0; i < vCRs[m].size(); ++i)
		{
			int nsize = vCRs[m][i].Region.size();
			vCRs[m][i].Block = vCRs[m][i].Region[0].block;
			vCRs[m][i].Step = vCRs[m][i].Region[0].step - datas[blockIndex[vCRs[m][i].Block]].BlockHead.indexL;
			vCRs[m][i].Step1 = vCRs[m][i].Region[0].step;
			vCRs[m][i].Row1 = vCRs[m][i].Region[0].row;
			vCRs[m][i].Step2 = vCRs[m][i].Region[nsize - 1].step;
			vCRs[m][i].Row2 = vCRs[m][i].Region[nsize - 1].row;


			for (int j = 0; j < nsize; ++j)
			{
				if (vCRs[m][i].Region[j].step > vCRs[m][i].Step2)
				{
					vCRs[m][i].Step2 = vCRs[m][i].Region[j].step;
				}

				if (vCRs[m][i].Region[j].step < vCRs[m][i].Step1)
				{
					vCRs[m][i].Step1 = vCRs[m][i].Region[j].step;
				}

				if (vCRs[m][i].Region[j].row < vCRs[m][i].Row1)
				{
					vCRs[m][i].Row1 = vCRs[m][i].Region[j].row;
				}

				if (vCRs[m][i].Region[j].row > vCRs[m][i].Row2)
				{
					vCRs[m][i].Row2 = vCRs[m][i].Region[j].row;
				}
			}

		}
	}

	time(&timec);
	p = localtime(&timec);
	PrintTime(*p, "连通域参数建立完毕！");
}

void	AnalyseCR(vector<Connected_Region>& crs, vector<int>& vCRIndex, uint16_t& sum1, uint16_t &sum2, int32_t& iTotalStep1, int32_t& iTotalRow1, int32_t& iTotalStep2, int32_t& iTotalRow2)
{
	for (int i = 0; i < vCRIndex.size(); ++i)
	{
		Connected_Region& cr = crs[vCRIndex[i]];
	}
}

bool	GetStepOffset(int iblock, int iStep, int offset, vector<BlockData_B>& blocks, int* destBlock, int * destStep)
{
	int i_block = iblock;
	int i_step = iStep - blocks[iblock].BlockHead.indexL;
	if (offset > 0)//向后
	{
		int k = 0;
		while (offset > blocks[i_block].vBStepDatas.size() - i_step && i_block < blocks.size())
		{
			offset -= (blocks[i_block].vBStepDatas.size() - i_step);
			++i_block;
			i_step = 0;
		}

		if (i_block < blocks.size())// k > nsize
		{
			*destBlock = i_block;
			*destStep = offset;
			return true;
		}
		else
		{
			return false;
		}
	}
	else//向前
	{
		offset = -offset;
		while (offset > i_step && i_block >= 0)
		{
			offset -= i_step;
			if (i_block == 0)
			{
				return false;
			}
			--i_block;
			i_step = blocks[i_block].vBStepDatas.size() - 1;
		}

		if (i_block >= 0)// 
		{
			*destBlock = i_block;
			*destStep = i_step - offset;
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool	GetCRInfo(Connected_Region& cr, CR_INFO& info, BlockData_A& DataA, vector<BlockData_B>& DataB, double angle, int offset, double stepDistance, int16_t restr, int16_t trig, uint16_t gain)
{
	cr.vASteps.clear();
	if (!GetStepAStepFrames(cr.Channel, cr, cr.vASteps, DataA, DataB, angle, offset, stepDistance))
	{
		return false;
	}
	uint8_t iChA = GetAChannelByBChannel(cr.Channel);
	info.MinH = 512;
	info.MaxH = info.MaxV = info.Shift = 0;
	for (int i = 0; i < cr.vASteps.size(); ++i)
	{
		for (int j = 0; j < cr.vASteps[i].Frames.size(); ++j)
		{
			if (cr.vASteps[i].Frames[j].F[iChA] > 0)//此处的0改为出波抑制值
			{
				A_Frame& frame = cr.vASteps[i].Frames[j];
				uint16_t data = frame.F[iChA];
				uint16_t data1 = pow(10, 0.05 * (0.5 * gain - StandardGain[iChA])) * data;
				frame.F[iChA] = data1;
				info.MinH = info.MinH < frame.Horizon ? info.MinH : frame.Horizon;
				info.MaxH = info.MaxH > frame.Horizon ? info.MaxH : frame.Horizon;
				info.MaxV = info.MaxV > frame.F[iChA] ? info.MaxV : frame.F[iChA];
			}
		}
	}
	info.Shift = (info.MaxH - info.MinH);
	if (cr.Channel < CH_D)
	{
		info.Shift = (info.MaxH - info.MinH) + (0.5 * gain - StandardGain[iChA]) * 10;
		if (info.Shift < 0) info.Shift = 0;
	}
	return cr.vASteps.size() > 0;
}

int16_t GetFRow(B_Step& step)
{
	for (int i = 0; i < step.vRowDatas.size(); ++i)
	{
		if (step.vRowDatas[i].Row < 40)
		{
			continue;
		}
		if (step.vRowDatas[i].Point.Draw1 & BIT14)
		{
			return step.vRowDatas[i].Row;
		}
	}
	return -1;
}

void	FillMarks(vector<BlockData_B>& blocks, vector<Position_Mark>& vPMs, F_HEAD& head)
{
	mBlockIndex.clear();
	Position_Mark pm;
	pm.Data = 0;
	for (size_t i = 0; i < blocks.size(); i++)
	{
		mBlockIndex.insert(make_pair(blocks[i].Index, i));
		int railType = blocks[i].BlockHead.railType & BIT4;
		for (size_t j = 0; j < blocks[i].vBStepDatas.size(); j++)
		{
			if (blocks[i].vBStepDatas[j].Mark.Mark == 0)
			{
				continue;
			}

			if (blocks[i].vBStepDatas[j].Mark.Mark & SEW)// 轨缝#
			{
				memset(&pm, 0, sizeof(pm));
				pm.Walk = GetWD(blocks[i].BlockHead.walk, j, head.step, railType);
				pm.Mark = PM_JOINT;
				pm.Block = blocks[i].Index;
				pm.Step = j;
				ParseGPS(blocks[i].BlockHead.gpsInfor, pm.gps_log, pm.gps_lat);
				vPMs.push_back(pm);
			}
			else if (blocks[i].vBStepDatas[j].Mark.Mark & SEW2)// 手动焊缝轨缝*
			{
				uint16_t data = blocks[i].vBStepDatas[j].Mark.Mark >> 20;
				memset(&pm, 0, sizeof(pm));
				if (data == 1)
				{
					pm.Mark = PM_SEW_CH;
					pm.Manual = 1;
					ParseGPS(blocks[i].BlockHead.gpsInfor, pm.gps_log, pm.gps_lat);
				}
				else if (data == 2 || data == 3)
				{
					pm.Mark = PM_SEW_LRH;
					pm.Manual = 1;
					ParseGPS(blocks[i].BlockHead.gpsInfor, pm.gps_log, pm.gps_lat);
				}
				else
				{
					continue;
				}

				pm.Walk = GetWD(blocks[i].BlockHead.walk, j, head.step, railType);
				pm.Block = blocks[i].Index;
				pm.Step = j;
				pm.Data = blocks[i].vBStepDatas[j].Mark.Mark >> 20;
				vPMs.push_back(pm);
			}
			else if (blocks[i].vBStepDatas[j].Mark.Mark & FORK) // 道岔Y
			{
				memset(&pm, 0, sizeof(pm));
				pm.Walk = GetWD(blocks[i].BlockHead.walk, j, head.step, railType);
				pm.Mark = PM_FORK;
				ParseGPS(blocks[i].BlockHead.gpsInfor, pm.gps_log, pm.gps_lat);
				pm.Block = blocks[i].Index;
				pm.Step = j;
				pm.Manual = 1;
			}
			else if (blocks[i].vBStepDatas[j].Mark.Mark & START)//回退
			{
				memset(&pm, 0, sizeof(pm));
				pm.Walk = GetWD(blocks[i].BlockHead.walk, j, head.step, railType);
				pm.Mark = PM_BACK;
				pm.Block = blocks[i].Index;
				pm.Step = j;
				vPMs.push_back(pm);
				pm.Manual = 1;
			}
			else if (blocks[i].vBStepDatas[j].Mark.Mark & BACK_P)//上道
			{
				memset(&pm, 0, sizeof(pm));
				pm.Walk = GetWD(blocks[i].BlockHead.walk, j, head.step, railType);
				pm.Mark = PM_START;
				pm.Block = blocks[i].Index;
				pm.Step = j;
				vPMs.push_back(pm);
				pm.Manual = 1;
			}
		}
	}

	pm.Data = 0;
	for (int i = 1; i < blocks.size(); ++i)
	{
		if (blocks[i].BlockHead.railH - blocks[i - 1].BlockHead.railH >= 10)
		{
			memset(&pm, 0, sizeof(pm));
			pm.Mark = PM_SMART1;
			vector<int16_t>		vFFindRow;
			for (int j = 0; j < blocks[i - 1].vBStepDatas.size(); ++j)
			{
				vFFindRow.push_back(GetFRow(blocks[i - 1].vBStepDatas[j]));
			}
			for (int j = 0; j < blocks[i].vBStepDatas.size(); ++j)
			{
				vFFindRow.push_back(GetFRow(blocks[i].vBStepDatas[j]));
			}
			for (int j = 7; j < vFFindRow.size() - 10; ++j)
			{
				if (vFFindRow[j] > vFFindRow[j - 7] && vFFindRow[j] < vFFindRow[j + 7] && vFFindRow[j + 7] - vFFindRow[j - 7] > 1 && vFFindRow[j + 7] - vFFindRow[j - 7] < 10)
				{
					if (j < blocks[i - 1].BlockHead.row)
					{
						pm.Walk = GetWD(blocks[i - 1].BlockHead.walk, j, head.step, blocks[i - 1].BlockHead.railType & BIT4);
						pm.Block = blocks[i - 1].Index;
						pm.Step = j;
					}
					else
					{
						pm.Walk = GetWD(blocks[i].BlockHead.walk, j - blocks[i - 1].BlockHead.row, head.step, blocks[i].BlockHead.railType & BIT4);
						pm.Block = blocks[i].Index;
						pm.Step = j;
					}
					vPMs.push_back(pm);
					break;
				}
			}
		}
		else if (blocks[i].BlockHead.railH - blocks[i - 1].BlockHead.railH <= -10)
		{
			memset(&pm, 0, sizeof(pm));
			pm.Mark = PM_SMART2;
			vector<int16_t>		vFFindRow;
			for (int j = 0; j < blocks[i - 1].vBStepDatas.size(); ++j)
			{
				vFFindRow.push_back(GetFRow(blocks[i - 1].vBStepDatas[j]));
			}
			for (int j = 0; j < blocks[i].vBStepDatas.size(); ++j)
			{
				vFFindRow.push_back(GetFRow(blocks[i].vBStepDatas[j]));
			}
			for (int j = 7; j < vFFindRow.size() - 10; ++j)
			{
				if (vFFindRow[j] < vFFindRow[j - 7] && vFFindRow[j] > vFFindRow[j + 7] && vFFindRow[j - 7] - vFFindRow[j + 7] > 1 && vFFindRow[j - 7] - vFFindRow[j + 7] < 10)
				{
					if (j < blocks[i - 1].BlockHead.row)
					{
						pm.Walk = GetWD(blocks[i - 1].BlockHead.walk, j, head.step, blocks[i - 1].BlockHead.railType & BIT4);
						pm.Block = blocks[i - 1].Index;
						pm.Step = j;
					}
					else
					{
						pm.Walk = GetWD(blocks[i].BlockHead.walk, j - blocks[i - 1].BlockHead.row, head.step, blocks[i].BlockHead.railType & BIT4);
						pm.Block = blocks[i].Index;
						pm.Step = j;
					}
					vPMs.push_back(pm);
					break;
				}
			}
		}
	}
}

void	GetChannelInfo(VCR& vCRs, vector<int> indexs, CH_INFO& ci, CH_INFO& ci2)
{
	memset(&ci, 0, sizeof(ci)); memset(&ci2, 0, sizeof(ci));
	uint32_t row = 0;
	if (vCRs[0].Channel >= CH_C)//C,c
	{
		for (int i = 0; i < indexs.size(); ++i)
		{
			ci.count += vCRs[indexs[i]].Region.size();
			for (int j = 0; j < vCRs[indexs[i]].Region.size(); ++j)
			{
				ci.iSumRow += vCRs[indexs[i]].Region[j].step;
				ci.iSumStep += vCRs[indexs[i]].Region[j].row;
			}
		}
		ci.AStep = 1.0 * ci.iSumStep / ci.count;
		ci.ARow = 1.0 * ci.iSumRow / ci.count;
	}
	else//A, a, B, b
	{
		for (int i = 0; i < indexs.size(); ++i)
		{
			ci.count += vCRs[indexs[i]].Region.size();
			for (int j = 0; j < vCRs[indexs[i]].Region.size(); ++j)
			{
				if (vCRs[indexs[i]].Region[j].row >= 13)
				{
					ci2.iSumRow += vCRs[indexs[i]].Region[j].step;
					ci2.iSumStep += vCRs[indexs[i]].Region[j].row;
				}
				else
				{
					ci.iSumRow += vCRs[indexs[i]].Region[j].step;
					ci.iSumStep += vCRs[indexs[i]].Region[j].row;
				}
			}
		}
		if (ci.count > 0)
		{
			ci.AStep = 1.0 * ci.iSumStep / ci.count;
			ci.ARow = 1.0 * ci.iSumRow / ci.count;
		}
		if (ci2.count > 0)
		{
			ci2.AStep = 1.0 * ci2.iSumStep / ci.count;
			ci2.ARow = 1.0 * ci2.iSumRow / ci.count;
		}
	}
}

uint32_t	ParsePosition(F_HEAD& head, vector<BlockData_B>& blocks, VCR* vCRs, CR& cr, int index, uint8_t iFRow, uint8_t railType, vector<int>* t_cr, Position_Mark& pm, int& iAStepBig, int &iAStepSmall)
{
	iAStepBig = iAStepSmall = 0;
	vector<int> t_crF, t_crG;
	uint8_t iLoseF = GetCR(CH_F, cr.Step1 - 50, iFRow - 3, cr.Step2 + 50, iFRow + 3, blocks, vCRs[CH_F], t_crF, -1, 2);
	uint8_t iLoseG = GetCR(CH_G, cr.Step1 - 50, iFRow - 3, cr.Step2 + 50, iFRow + 3, blocks, vCRs[CH_G], t_crG, -1, 2);
	int iLose_Left = -1;
	int iLose_Right = -1;
	if (iLoseF)
	{
		iLose_Right = vCRs[CH_F][t_crF[0]].Step1;
		iLose_Left = vCRs[CH_F][t_crF[0]].Step2;
	}
	else if (iLoseG)
	{
		iLose_Right = vCRs[CH_G][t_crG[0]].Step1;
		iLose_Left = vCRs[CH_G][t_crG[0]].Step2;
	}
	uint8_t iFind[10] = { 0 };
	uint16_t sum = 0;
	int iRowH = iGuiERow[railType] << 1;

	uint16_t sum1 = 0, sum2 = 0;

	uint8_t	 findEveyChannel[16] = { 0 };
	uint16_t sumEveyChannel[16] = { 0 };
	int32_t iSumStep = 0, iSumRow = 0;
	uint16_t maxSize = 0;
	vector<TPoint> points;
	int ir1 = iRowH, ir2 = 0, iS2 = cr.Step1 - 100, iS1 = cr.Step1 + 100;
	for (int j = 0; j < 8; ++j)
	{
		iFind[j] = GetCR(j, cr.Step1 - 50, 0, cr.Step2 + 50, iRowH, blocks, vCRs[j], t_cr[j]);
		uint8_t iL = 0, iH = 0;
		for (int k = 0; k < t_cr[j].size(); ++k)
		{
			Connected_Region& crTemp = vCRs[j][t_cr[j][k]];
			for (int ii = 0; ii < crTemp.Region.size(); ++ii)
			{
				WaveData& wd = crTemp.Region[ii];
				TPoint point;
				point.Step = wd.step;
				point.Row = wd.row;

				if (wd.row < iGuiERow[railType])
				{
					findEveyChannel[j] = 1;
					sumEveyChannel[j] += 1;
				}
				else
				{
					findEveyChannel[j + 1] = 1;
					sumEveyChannel[j + 1] += 1;
				}
				point.Channel = j;
				points.push_back(point);

				if (wd.row < ir1)
				{
					ir1 = wd.row;
				}
				if (wd.row > ir2)
				{
					ir2 = wd.row;
				}

				if (wd.step < iS1)
				{
					iS1 = wd.step;
				}
				if (wd.step > iS2)
				{
					iS2 = wd.step;
				}

				iSumRow += wd.row;
				iSumStep += wd.step;
			}
			if (crTemp.Row1 <= iGuiERow[railType])
			{
				iL = 1;
			}
			if (crTemp.Row2 >= iGuiERow[railType])
			{
				iH = 1;
			}
			sum1 += crTemp.Region.size();
			maxSize = maxSize > crTemp.Region.size() ? maxSize : crTemp.Region.size();
		}
		sum += (iL + iH);
	}

	for (int j = 8; j < 10; ++j)
	{
		iFind[j] = GetCR(j, cr.Step1 - 50, 0, cr.Step2 + 50, iRowH, blocks, vCRs[j], t_cr[j]);
		sum += iFind[j];
		for (int k = 0; k < t_cr[j].size(); ++k)
		{
			Connected_Region& crTemp = vCRs[j][t_cr[j][k]];
			for (int ii = 0; ii < crTemp.Region.size(); ++ii)
			{
				WaveData& wd = crTemp.Region[ii];
				TPoint point;
				point.Step = wd.step;
				point.Row = wd.row;
				point.Channel = j;
				points.push_back(point);

				findEveyChannel[j] = 1;
				sumEveyChannel[j] += 1;

				if (wd.row < ir1)
				{
					ir1 = wd.row;
				}
				if (wd.row > ir2)
				{
					ir2 = wd.row;
				}

				if (wd.step < iS1)
				{
					iS1 = wd.step;
				}
				if (wd.step > iS2)
				{
					iS2 = wd.step;
				}

				iSumRow += wd.row;
				iSumStep += wd.step;
			}
			sum2 += crTemp.Region.size();
		}
	}

	memset(&pm, 0, sizeof(pm));
	pm.Walk = GetWD(blocks[mBlockIndex[cr.Block]].BlockHead.walk, cr.Step, g_direction);
	pm.Block = cr.Block;
	pm.AStep = 1.0 * iSumStep / points.size() - cr.Step1;
	pm.ARow = 1.0 * iSumRow / points.size();
	for (int k = 0; k < 16; ++k)
	{
		pm.ChannelNum += findEveyChannel[k];
		pm.Num[k] = sumEveyChannel[k];
	}
	pm.Size = sum1 + sum2;
	pm.Length = iS2 - iS1;
	pm.Height = ir2 - ir1;
	pm.ChannelNum = sum;
	pm.Step = pm.AStep + cr.Step1;
	double s = 0;
	uint32_t iSumBigChannel = findEveyChannel[CH_A1] + findEveyChannel[CH_A2] + findEveyChannel[CH_B1] + findEveyChannel[CH_B2] + findEveyChannel[CH_C];
	uint32_t iSumSmallChannel = findEveyChannel[CH_a1] + findEveyChannel[CH_a2] + findEveyChannel[CH_b1] + findEveyChannel[CH_b2] + findEveyChannel[CH_c];
	uint32_t iBigCount = 0, iSmallCount = 0;
	for (int k = 0; k < points.size(); ++k)
	{
		s += pow((points[k].Step - pm.Step), 2) + pow((points[k].Row - pm.ARow), 2);
		if (points[k].Channel == CH_A1 || points[k].Channel == CH_B1 || points[k].Channel == CH_C)
		{
			iBigCount++;
			iAStepBig += points[k].Step;
		}
		else if (points[k].Channel == CH_a1 || points[k].Channel == CH_b1 || points[k].Channel == CH_c)
		{
			iSmallCount++;
			iAStepSmall += points[k].Step;
		}
	}
	iAStepBig = 1.0 * iAStepBig / iBigCount;
	iAStepSmall = 1.0 * iAStepSmall / iSmallCount;
	if (pm.Size > 0)
	{
		if (s < 0)	pm.Fangcha = 0;
		else		pm.Fangcha = sqrt(s / pm.Size);
	}
	pm.Step = pm.AStep + cr.Step;
	pm.Length = iS2 - iS1;
	pm.Height = ir2 - ir1;

	if (pm.ChannelNum >= 7 && pm.Size >= 150 && sum2 >= 35 && (iLoseF && iLoseG))
	{
		pm.Mark = PM_JOINT;
		for (int j = 0; j < 10; ++j)
			SetJudgedFlag(vCRs[j], t_cr[j], 1);

		vector<int> crD, crE;
		uint8_t iFindD = GetCR(CH_D, iLose_Right, iLose_Left, blocks, vCRs[CH_D], crD);
		uint8_t iFindE = GetCR(CH_E, iLose_Right, iLose_Left, blocks, vCRs[CH_E], crE);
		SetJudgedFlag(vCRs[CH_D], crD, 1); SetJudgedFlag(vCRs[CH_E], crE, 1);
	}
	else if (pm.ChannelNum >= 5 && pm.Size >= 50 && sum2 < 15)
	{
		if (iAStepSmall <= iAStepBig - 3)
		{
			pm.Mark = PM_SEW_LRH;
			for (int j = 0; j < CH_C; ++j)
			{
				SetJudgedFlag(vCRs[j], t_cr[j], 1);
			}
			SetJudgedFlag(vCRs[CH_F], t_crF, 1);
			SetJudgedFlag(vCRs[CH_G], t_crG, 1);
		}
	}
	else if (iSumBigChannel > 0 && iSumSmallChannel > 0 && iAStepSmall <= iAStepBig - 3)
	{
		pm.Mark = PM_SEW_CH;
		for (int j = 0; j < CH_C; ++j)
		{
			SetJudgedFlag(vCRs[j], t_cr[j], 1);
		}
		SetJudgedFlag(vCRs[CH_F], t_crF, 1);
		SetJudgedFlag(vCRs[CH_G], t_crG, 1);
	}
	return pm.Mark;
}

void	ParseSewCH(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, CR& cr, int index, uint8_t iFRow, bool carType, uint8_t railType, vector<int>* t_cr, int& iAStepBig, int &iAStepSmall, vector<Wound_Judged>& vWounds)
{
	int iStepSew = (iAStepBig + iAStepSmall) >> 1;
	BLOCK blockHead = blocks[mBlockIndex[cr.Block]].BlockHead;
	CR_INFO t_info;
	for (int j = 0; j < 10; ++j)
	{
		int t_A = GetAChannelByBChannel(j);
		double t_angle = 0.1 * head.deviceP2.Angle[t_A].Refrac;
		int t_offset = head.deviceP2.Place[t_A] + blockHead.probOff[t_A];
		for (int k = 0; k < t_cr[j].size(); ++k)
		{
			if (vCRs[j][t_cr[j][k]].Step1 < iStepSew)
			{
				if (GetCRInfo(vCRs[j][t_cr[j][k]], t_info, DataA, blocks, t_angle, t_offset, head.step, head.deviceP2.Restr[t_A], head.deviceP2.Trig[t_A], blockHead.gain[t_A]))
				{
					if (t_info.MaxV >= 150 && t_info.MaxH - t_info.MinH >= 25)
					{
						if (j < CH_C)
						{
							if (t_info.MaxH >= 250)
							{
								Wound_Judged wound;
								wound.Block = vCRs[j][t_cr[j][k]].Block;
								wound.Step = vCRs[j][t_cr[j][k]].Step;
								wound.Walk = GetWD(blocks[mBlockIndex[wound.Block]].BlockHead.walk, wound.Step, head.step);
								wound.IsSew = 1;
								FillWound(wound, blockHead, head);
								if ((carType == true && j == CH_A1) || (carType == false && j == CH_B1))
								{
									wound.Type = W_HEAD_HS_IN;
									wound.Place = WP_HEAD_IN;
								}
								else
								{
									wound.Type = W_HEAD_HS_OUT;
									wound.Place = WP_HEAD_OUT;
								}
								wound.Degree = 4;
								wound.SizeX = 3.0 * (vCRs[j][t_cr[j][k]].Row2 - vCRs[j][t_cr[j][k]].Row1);
								wound.SizeY = head.step * (vCRs[j][t_cr[j][k]].Step2 - vCRs[j][t_cr[j][k]].Step1);
								sprintf_s(wound.According, "%s出波，位移：%.1f大格，幅值%d，大于5大格出波", ChannelNamesB[j], 0.02f * t_info.Shift, t_info.MaxV);
								vWounds.push_back(wound);
								break;
							}
						}
						else
						{
							if (vCRs[j][t_cr[j][k]].Row1 <= 8)
							{
								Wound_Judged wound;
								wound.Block = vCRs[j][t_cr[j][k]].Block;
								wound.Step = vCRs[j][t_cr[j][k]].Step;
								wound.Walk = GetWD(blocks[mBlockIndex[wound.Block]].BlockHead.walk, wound.Step, head.step);
								wound.IsSew = 1;
								wound.Type = W_HEAD_HS_MID;
								wound.Place = WP_HEAD_MID;
								wound.Degree = 4;
								wound.SizeX = 3.0 * (vCRs[j][t_cr[j][k]].Row2 - vCRs[j][t_cr[j][k]].Row1);
								wound.SizeY = head.step * (vCRs[j][t_cr[j][k]].Step2 - vCRs[j][t_cr[j][k]].Step1);
								FillWound(wound, blockHead, head);
								sprintf_s(wound.According, "%s出波，位移：%.1f大格，幅值%d，大于5大格出波", ChannelNamesB[j], 0.02f * t_info.Shift, t_info.MaxV);
								vWounds.push_back(wound);
								break;
							}
						}
					}
				}
			}
		}
	}
}

void	ParseSewLRH(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, CR& cr, int index, uint8_t iFRow, bool carType, uint8_t railType, vector<int>* t_cr, int& iAStepBig, int &iAStepSmall, vector<Wound_Judged>& vWounds)
{
	for (int j = 0; j < 10; ++j)
	{
		int t_A = GetAChannelByBChannel(j);
		double t_angle = 0.1 * head.deviceP2.Angle[t_A].Refrac;
		BLOCK blockHead = blocks[mBlockIndex[cr.Block]].BlockHead;
		int t_offset = head.deviceP2.Place[t_A] + blockHead.probOff[t_A];
		int iStepSew = (iAStepBig + iAStepSmall) >> 1;
		if (j % 4 == 0)//A, B, C 0 , 4, 8，大通道
		{
			for (int k = 0; k < t_cr[j].size(); ++k)
			{
				if (vCRs[j][t_cr[j][k]].Step2 <= iStepSew)
				{
					CR_INFO t_info;
					if (GetCRInfo(vCRs[j][t_cr[j][k]], t_info, DataA, blocks, t_angle, t_offset, head.step, head.deviceP2.Restr[t_A], head.deviceP2.Trig[t_A], blockHead.gain[t_A]))
					{
						if (t_info.MaxV >= 150 && t_info.Shift >= 25)
						{
							Wound_Judged wound;
							wound.Block = vCRs[j][t_cr[j][k]].Block;
							wound.Step = vCRs[j][t_cr[j][k]].Step;
							wound.Walk = GetWD(blocks[mBlockIndex[wound.Block]].BlockHead.walk, wound.Step, head.step);
							wound.IsSew = 1;
							if ((carType == true && j == CH_A1) || (carType == false && j == CH_B1))
							{
								wound.Type = W_HEAD_HS_IN;
								wound.Place = WP_HEAD_IN;
							}
							else if (j == CH_C)
							{
								wound.Type = W_HEAD_HS_MID;
								wound.Place = WP_HEAD_MID;
							}
							else
							{
								wound.Type = W_HEAD_HS_OUT;
								wound.Place = WP_HEAD_OUT;
							}

							wound.Degree = 4;
							wound.SizeX = 3.0 * (vCRs[j][t_cr[j][k]].Row2 - vCRs[j][t_cr[j][k]].Row1);
							wound.SizeY = head.step * (vCRs[j][t_cr[j][k]].Step2 - vCRs[j][t_cr[j][k]].Step1);
							FillWound(wound, blockHead, head);
							sprintf_s(wound.According, "%s出波，位移：%.1f大格，幅值%d", ChannelNamesB[j], 0.02f * t_info.Shift, t_info.MaxV);
							vWounds.push_back(wound);
							break;
						}
					}
				}
			}
		}
		else //小通道
		{
			for (int k = 0; k < t_cr[j].size(); ++k)
			{
				if (vCRs[j][t_cr[j][k]].Step1 >= iStepSew)
				{
					CR_INFO t_info;
					if (GetCRInfo(vCRs[j][t_cr[j][k]], t_info, DataA, blocks, t_angle, t_offset, head.step, head.deviceP2.Restr[t_A], head.deviceP2.Trig[t_A], blockHead.gain[t_A]))
					{
						if (t_info.MaxV >= 150 && t_info.MaxH - t_info.MinH >= 25)
						{
							Wound_Judged wound;
							wound.Block = vCRs[j][t_cr[j][k]].Block;
							wound.Step = vCRs[j][t_cr[j][k]].Step;
							wound.Walk = GetWD(blocks[mBlockIndex[wound.Block]].BlockHead.walk, wound.Step, head.step);
							wound.IsSew = 1;
							if ((carType == true && j == CH_A1) || (carType == false && j == CH_B1))
							{
								wound.Type = W_HEAD_HS_IN;
								wound.Place = WP_HEAD_IN;
							}
							else if (j == CH_c)
							{
								wound.Type = W_HEAD_HS_MID;
								wound.Place = WP_HEAD_MID;
							}
							else
							{
								wound.Type = W_HEAD_HS_OUT;
								wound.Place = WP_HEAD_OUT;
							}

							wound.Degree = 4;
							wound.SizeX = 3.0 * (vCRs[j][t_cr[j][k]].Row2 - vCRs[j][t_cr[j][k]].Row1);
							wound.SizeY = head.step * (vCRs[j][t_cr[j][k]].Step2 - vCRs[j][t_cr[j][k]].Step1);
							FillWound(wound, blockHead, head);
							sprintf_s(wound.According, "%s出波，位移：%.1f大格，幅值%d", ChannelNamesB[j], 0.02f * t_info.Shift, t_info.MaxV);
							vWounds.push_back(wound);
							break;
						}
					}
				}
			}
		}
	}
}

void	ParseScrewHole(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, CR& cr, int iCrIndex, uint8_t iFRow, uint8_t railType, vector<Wound_Judged>& vWounds)
{

}

void	ParseSingleScrewHoleByD(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, CR& cr, int i, uint8_t iFRow, uint8_t railType,
	vector<int>&crF, vector<int>& crG, vector<int>& crD, vector<int>& crE, vector<int>& crF2, vector<int>& crG2,
	vector<Wound_Judged>& vWounds)
{
	bool bFind = crD.size() > 0;
	bool bFindE = crE.size() > 0;
	bool bFindF = crF.size() > 0;
	bool bFindG = crG.size() > 0;
	int iFStep2 = 0;
	if (bFindF)//F螺孔出波
	{
		iFStep2 = (vCRs[CH_F][crF2[0]].Step2 + vCRs[CH_F][crF2[0]].Step1) >> 1;
	}
	else if (crF.size() > 0)//底部失波
	{
		iFStep2 = (vCRs[CH_F][crF[0]].Step2 + vCRs[CH_F][crF[0]].Step1) >> 1;
	}
	else
	{
		iFStep2 = -999;
	}
	double angle = 0.1 * head.deviceP2.Angle[ACH_D].Refrac;
	int iBlock = mBlockIndex[cr.Block];
	BLOCK blockHead = blocks[iBlock].BlockHead;
	double offset = head.deviceP2.Place[ACH_D] + blockHead.probOff[ACH_D];
	int step1 = 0, step2 = 0;
	GetStepSteps(CH_D, cr, step1, step2, blocks, angle, offset, head.step);
	uint8_t m = CH_D;
	uint8_t iChA = GetAChannelByBChannel(ACH_D);
	//检测四象限斜裂纹
	vector<int> crDt;
	uint8_t iFindD_4 = GetCR(CH_D, cr.Step1, cr.Row2, cr.Step2, cr.Row2 + 10, blocks, vCRs[CH_D], crDt, i);
	if (iFindD_4)//D通道检测到伤损
	{
		for (int k = 0; k < crDt.size(); ++k)
		{
			if (vCRs[CH_D][crDt[k]].Region.size() < 3)
			{
				continue;
			}

			int step11 = 0, step12 = 0;
			GetStepSteps(m, vCRs[CH_D][crDt[k]], step11, step12, blocks, angle, offset, head.step);
			if (step11 >= step1 && step12 <= step2)
			{
				vCRs[CH_D][crDt[k]].Flag = 1;
			}
			else
			{
				CR_INFO info_4;
				if (!GetCRInfo(vCRs[CH_D][crDt[k]], info_4, DataA, blocks, angle, offset, head.step, head.deviceP2.Restr[iChA], head.deviceP2.Trig[iChA], blockHead.gain[iChA]))
				{
					continue;
				}
				else if (info_4.MaxV >= 150)
				{
					Wound_Judged w;
					w.Block = vCRs[CH_D][crDt[k]].Block;
					w.Step = vCRs[CH_D][crDt[k]].Step;
					w.Walk = GetWD(blocks[mBlockIndex[vCRs[CH_D][crDt[k]].Block]].BlockHead.walk, vCRs[CH_D][crDt[k]].Step1 - blocks[mBlockIndex[vCRs[CH_D][crDt[k]].Block]].BlockHead.indexL, head.step);
					FillWound(w, blockHead, head);
					w.Type = WOUND_TYPE::W_SCREW_CRACK4;
					w.Place = WP_WAIST;
					w.Degree = 4;
					w.SizeX = (vCRs[CH_D][crDt[k]].Row2 - vCRs[CH_D][crDt[k]].Row1) / 0.6;
					w.SizeY = 1;
					sprintf_s(w.According, "D出波，位移：%.1f大格，幅值:%d", 0.02f * info_4.Shift, info_4.MaxV);
					memcpy(w.Result, "螺孔四象限斜裂纹", 30);
					w.vCRs.push_back(cr);
					AddWoundData(w, vCRs[CH_F], crF);
					AddWoundData(w, vCRs[CH_F], crF2);
					AddWoundData(w, vCRs[CH_G], crG);
					AddWoundData(w, vCRs[CH_G], crG2);
					AddWoundData(w, vCRs[CH_E], crE);
					AddWoundData(w, vCRs[CH_D], crDt);
					vWounds.push_back(w);
					SetJudgedFlag(vCRs[CH_D], crDt, 1);
					break;
				}
			}
		}
	}

	//检测二象限斜裂纹
	crDt.clear();
	uint8_t iFindD_2 = GetCR(CH_D, cr.Step1 + 10, cr.Row1, cr.Step2 + 10, cr.Row2, blocks, vCRs[CH_D], crDt, i);
	for (int k = 0; k < crDt.size(); ++k)
	{
		CR_INFO info_2;
		if (GetCRInfo(vCRs[CH_D][crDt[k]], info_2, DataA, blocks, angle, offset, head.step, head.deviceP2.Restr[iChA], head.deviceP2.Trig[iChA], blockHead.gain[iChA]) && info_2.MaxV >= 150)
		{
			Wound_Judged w;
			w.Block = cr.Block;
			w.Step = cr.Step;
			int idx_t = vCRs[CH_D][crDt[k]].Step1 - blocks[mBlockIndex[vCRs[CH_D][crDt[k]].Block]].BlockHead.indexL;
			w.Walk = GetWD(blocks[iBlock].BlockHead.walk, idx_t, head.step);
			w.Type = WOUND_TYPE::W_SCREW_CRACK2;
			w.Place = WP_WAIST;
			w.Degree = 4;
			FillWound(w, blockHead, head);
			w.SizeX = (vCRs[CH_D][crDt[k]].Row2 - vCRs[CH_D][crDt[k]].Row1) / 0.6;
			w.SizeY = 1;
			sprintf_s(w.According, "D出波，位移：%.1f大格，幅值:%d", 0.02f * info_2.Shift, info_2.MaxV);
			memcpy(w.Result, "螺孔二象限斜裂纹", 30);
			w.vCRs.push_back(cr);
			SetJudgedFlag(vCRs[CH_D], crDt, 1);
			AddWoundData(w, vCRs[CH_D], crDt);
			AddWoundData(w, vCRs[CH_F], crF);
			AddWoundData(w, vCRs[CH_F], crF2);
			AddWoundData(w, vCRs[CH_G], crG2);
			AddWoundData(w, vCRs[CH_E], crE);
			vWounds.push_back(w);
			break;
		}
	}

	vector<int> crEt;
	uint8_t iFindE_1 = GetCR(CH_E, cr.Step1, cr.Row1, cr.Step2 + 3, cr.Row2, blocks, vCRs[CH_E], crEt);
	Exclude(crEt, crE);
	for (int k = 0; k < crEt.size(); ++k)
	{
		if (vCRs[CH_E][crEt[k]].Region.size() > 3)
		{
			int iEStep2 = (vCRs[CH_E][crEt[k]].Step2 + vCRs[CH_E][crEt[k]].Step1) >> 1;
			if (iEStep2 < iFStep2 - 3)//E早于F结束，说明E是伤损
			{
				Wound_Judged w;
				w.Block = cr.Block;
				w.Step = cr.Step;
				w.Walk = GetWD(blocks[iBlock].BlockHead.walk, vCRs[CH_E][crEt[0]].Step1 - blocks[iBlock].BlockHead.indexL, head.step);
				FillWound(w, blockHead, head);
				w.Degree = 4;
				memcpy(w.Result, "螺孔一象限斜裂纹", 30);
				w.Place = WP_WAIST;
				w.Type = WOUND_TYPE::W_SCREW_CRACK1;
				w.vCRs.push_back(cr);
				AddWoundData(w, vCRs[CH_E], crEt);
				AddWoundData(w, vCRs[CH_F], crF);
				AddWoundData(w, vCRs[CH_F], crF2);
				AddWoundData(w, vCRs[CH_G], crG);
				AddWoundData(w, vCRs[CH_G], crG2);
				AddWoundData(w, vCRs[CH_E], crE);
				w.SizeX = (vCRs[CH_E][crEt[k]].Row2 - vCRs[CH_E][crEt[k]].Row1) / 0.6;
				w.SizeY = 1;
				sprintf_s(w.According, "E出波，且早于F结束");
				vWounds.push_back(w);
			}
			SetJudgedFlag(vCRs[CH_E], crEt, 1);
		}
	}

	if (bFindE)
	{
		int iStepE1 = 0, iStepE2 = 0;
		GetStepSteps(CH_E, vCRs[CH_E][crE[0]], iStepE1, iStepE2, blocks, 0.1 * head.deviceP2.Angle[ACH_E].Refrac, offset, head.step);

		crEt.clear();
		uint8_t iFind_3 = GetCR(CH_E, cr.Step1 + 10, cr.Row2 + 1, cr.Step2 + 10, cr.Row2 + 10, blocks, vCRs[CH_E], crEt);
		Exclude(crEt, crE);
		iFind_3 = crEt.size() > 0;
		if (iFind_3)
		{
			for (int k = 0; k < crEt.size(); ++k)
			{
				if (vCRs[CH_E][crEt[k]].Region.size() < 3)
				{
					continue;
				}

				int stepE1 = 0, stepE2 = 0;
				GetStepSteps(CH_E, vCRs[CH_E][crE[0]], iStepE1, iStepE2, blocks, 0.1 * head.deviceP2.Angle[ACH_E].Refrac, offset, head.step);
				if (stepE1 >= iStepE1 && iStepE2 <= step2)
				{

				}
				else
				{
					CR_INFO info_3;
					if (GetCRInfo(vCRs[CH_E][crEt[k]], info_3, DataA, blocks, angle, offset, head.step, head.deviceP2.Restr[ACH_E], head.deviceP2.Trig[ACH_E], blockHead.gain[ACH_E]) && info_3.MaxV >= 150)
					{
						Wound_Judged w;
						w.Block = vCRs[CH_E][crEt[k]].Block;
						w.Step = vCRs[CH_E][crEt[k]].Step;
						w.Walk = GetWD(blocks[mBlockIndex[w.Block]].BlockHead.walk, w.Step, head.step);
						FillWound(w, blockHead, head);
						w.Degree = 4;
						memcpy(w.Result, "螺孔三象限斜裂纹", 30);
						w.SizeX = (vCRs[CH_E][crEt[k]].Row2 - vCRs[CH_E][crEt[k]].Row1) / 0.6;
						w.SizeY = 1;
						sprintf_s(w.According, "E出波，位移：%.1f大格，幅值:%d", 0.02f * info_3.Shift, info_3.MaxV);
						w.Place = WP_WAIST;
						w.Type = WOUND_TYPE::W_SCREW_CRACK3;
						SetJudgedFlag(vCRs[CH_D], crDt, 1);
						w.vCRs.push_back(cr);
						AddWoundData(w, vCRs[CH_D], crDt);
						AddWoundData(w, vCRs[CH_E], crEt);
						AddWoundData(w, vCRs[CH_F], crF);
						AddWoundData(w, vCRs[CH_F], crF2);
						AddWoundData(w, vCRs[CH_G], crG2);
						vWounds.push_back(w);
						break;
					}
				}
			}
		}
	}

	//水平裂纹
	vector<int> crF3, crF4, crG3, crG4;
	if (bFindF || bFindG)
	{
		if (bFindF)
		{
			uint8_t iF3 = 0;
			uint8_t iF4 = 0;
			Connected_Region& cr_F_M = vCRs[CH_F][crF2[0]];
			iF3 = GetCR(CH_F, cr_F_M.Step1 - 5, cr_F_M.Row2 + 1, cr_F_M.Step1 + 5, iFRow - 4, blocks, vCRs[CH_F], crF3, crF2[0]);
			if (iF3)
			{
				for (int z = 0; z < crF3.size(); ++z)
				{
					if (vCRs[CH_F][crF3[z]].Step1 >= cr_F_M.Step1 && vCRs[CH_F][crF3[z]].Step2 <= cr_F_M.Step2)
					{
						continue;
					}
					else
					{
						Wound_Judged w;
						w.Block = vCRs[CH_F][crF3[z]].Block;
						w.Step = vCRs[CH_F][crF3[z]].Step;
						w.Type = W_SCREW_HORIZON_CRACK;
						w.Place = WP_WAIST;
						w.SizeX = head.step * (vCRs[CH_F][crF3[z]].Step2 - vCRs[CH_F][crF3[z]].Step1);
						w.SizeY = 1;
						sprintf_s(w.According, "F出波");
						FillWound(w, blockHead, head);
						Connected_Region& cr_F_des = vCRs[CH_F][crF3[0]];
						int j_block = mBlockIndex[cr_F_des.Block];
						w.Walk = GetWD(blocks[j_block].BlockHead.walk, cr_F_des.Step1 - blocks[j_block].BlockHead.indexL, head.step);
						memcpy(w.Result, "螺孔右侧水平裂纹", 30);
						w.Degree = 4;

						SetJudgedFlag(vCRs[CH_F], crF3, 1);
						w.vCRs.push_back(cr);
						AddWoundData(w, vCRs[CH_F], crF3);
						AddWoundData(w, vCRs[CH_E], crEt);
						AddWoundData(w, vCRs[CH_F], crF);
						AddWoundData(w, vCRs[CH_F], crF2);
						AddWoundData(w, vCRs[CH_G], crG2);
						vWounds.push_back(w);
					}
				}
			}

			iF4 = GetCR(CH_F, cr_F_M.Step2 - 5, cr_F_M.Row2 + 1, cr_F_M.Step2 + 5, iFRow - 4, blocks, vCRs[CH_F], crF4, crF2[0]);
			if (iF4)
			{
				for (int z = 0; z < crF4.size(); ++z)
				{
					if (vCRs[CH_F][crF4[z]].Step1 >= cr_F_M.Step1 && vCRs[CH_F][crF4[z]].Step2 <= cr_F_M.Step2)
					{
						continue;
					}
					else
					{
						Wound_Judged w;
						w.Block = vCRs[CH_F][crF4[z]].Block;
						w.Step = vCRs[CH_F][crF4[z]].Step;
						w.Place = WP_WAIST;
						w.Type = W_SCREW_HORIZON_CRACK;
						FillWound(w, blockHead, head);
						w.SizeX = head.step * (vCRs[CH_F][crF4[z]].Step2 - vCRs[CH_F][crF4[z]].Step1);
						w.SizeY = 1;
						sprintf_s(w.According, "F出波");
						Connected_Region& cr_F_des = vCRs[CH_F][crF4[0]];
						int j_block = mBlockIndex[w.Block];
						w.Walk = GetWD(blocks[j_block].BlockHead.walk, w.Step, head.step);
						memcpy(w.Result, "螺孔左侧水平裂纹", 30);
						w.Degree = 4;

						SetJudgedFlag(vCRs[CH_F], crF4, 1);
						w.vCRs.push_back(cr);
						AddWoundData(w, vCRs[CH_F], crF4);
						AddWoundData(w, vCRs[CH_E], crEt);
						AddWoundData(w, vCRs[CH_F], crF);
						AddWoundData(w, vCRs[CH_F], crF2);
						AddWoundData(w, vCRs[CH_G], crG2);
						vWounds.push_back(w);
					}
				}
			}
		}
		else
		{
			uint8_t iG3 = 0;
			uint8_t iG4 = 0;
			Connected_Region& cr_G_M = vCRs[CH_G][crG2[0]];
			iG3 = GetCR(CH_G, cr_G_M.Step1 - 5, cr_G_M.Row2 + 1, cr_G_M.Step2 + 5, iFRow - 4, blocks, vCRs[CH_G], crG3, crG2[0]);
			if (iG3)
			{
				for (int z = 0; z < crG3.size(); ++z)
				{
					if (vCRs[CH_G][crG3[z]].Step1 >= cr_G_M.Step1 && vCRs[CH_G][crG3[z]].Step2 <= cr_G_M.Step2)
					{
						continue;
					}
					else
					{
						Wound_Judged w;
						w.Block = vCRs[CH_G][crG3[z]].Block;
						w.Step = vCRs[CH_G][crG3[z]].Step;
						w.Type = W_SCREW_HORIZON_CRACK;
						w.Place = WP_WAIST;

						w.SizeX = head.step * (vCRs[CH_G][crG3[z]].Step2 - vCRs[CH_G][crG3[z]].Step1);
						w.SizeY = 1;
						sprintf_s(w.According, "G出波");
						FillWound(w, blockHead, head);

						Connected_Region& cr_G_des = vCRs[CH_G][crG3[0]];
						int j_block = mBlockIndex[w.Block];
						w.Walk = GetWD(blocks[j_block].BlockHead.walk, w.Step, head.step);
						memcpy(w.Result, "螺孔右侧水平裂纹", 30);
						w.Degree = 4;

						SetJudgedFlag(vCRs[CH_G], crG3, 1);
						w.vCRs.push_back(cr);
						AddWoundData(w, vCRs[CH_G], crG3);
						AddWoundData(w, vCRs[CH_E], crEt);
						AddWoundData(w, vCRs[CH_G], crG);
						AddWoundData(w, vCRs[CH_G], crG2);
						AddWoundData(w, vCRs[CH_G], crG2);
						vWounds.push_back(w);
					}
				}
			}

			iG4 = GetCR(CH_G, cr_G_M.Step2 - 5, cr_G_M.Row2 + 1, cr_G_M.Step2 + 5, iFRow - 4, blocks, vCRs[CH_G], crG4, crG2[0]);
			if (iG4)
			{
				for (int z = 0; z < crG4.size(); ++z)
				{
					if (vCRs[CH_G][crG4[z]].Step1 >= cr_G_M.Step1 && vCRs[CH_G][crG4[z]].Step2 <= cr_G_M.Step2)
					{
						continue;
					}
					else
					{
						Wound_Judged w;
						w.Block = vCRs[CH_G][crG4[z]].Block;
						w.Step = vCRs[CH_G][crG4[z]].Step;
						w.Place = WP_WAIST;
						w.Type = W_SCREW_HORIZON_CRACK;
						FillWound(w, blockHead, head);
						Connected_Region& cr_G_des = vCRs[CH_G][crG4[0]];
						int j_block = mBlockIndex[w.Block];
						w.Walk = GetWD(blocks[j_block].BlockHead.walk, w.Step, head.step);
						memcpy(w.Result, "螺孔左侧水平裂纹", 30);
						w.Degree = 4;

						SetJudgedFlag(vCRs[CH_G], crG4, 1);
						w.vCRs.push_back(cr);
						AddWoundData(w, vCRs[CH_G], crG4);
						AddWoundData(w, vCRs[CH_E], crEt);
						AddWoundData(w, vCRs[CH_G], crG);
						AddWoundData(w, vCRs[CH_G], crG2);
						AddWoundData(w, vCRs[CH_G], crG2);

						w.SizeX = head.step * (vCRs[CH_G][crG4[z]].Step2 - vCRs[CH_G][crG4[z]].Step1);
						w.SizeY = 1;
						sprintf_s(w.According, "G出波");

						vWounds.push_back(w);
						break;
					}
				}
			}
		}
	}
}

void Analyse(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, vector<Wound_Judged>& vWounds, vector<Position_Mark>& vPMs)
{
	vector<Wound_Judged> hDatas;	//获取历史数据
	vector<Position_Mark> hPMs;		//位置标记，如接头，桥梁，曲线等
	uint32_t nCount = 1000;
#ifdef _EXPORT_TO_MYSQL
	//theApp.sql.AddSteps_A(DataA.vAStepDatas);
#endif;
	//S1 增益标准化


	//S2 本期数据判伤，求连通域
	//A a B b C c D d E e F G	
	//需记录 A,a, B, b,C,c, D, E, F, G的出波
	time_t timec;
	tm* p;
	time(&timec);
	p = localtime(&timec);
	PrintTime(*p, "建立CR");
	vector<Connected_Region> vCRs[16];
	CreateCR(blocks, vCRs);

	year = head.startD >> 16;
	month = (head.startD & 0xFF00) >> 8;
	day = head.startD & 0xFF;

	//S3 获取尖轨的位置标记
	FillMarks(blocks, vPMs, head);
	Position_Mark pm;

	time(&timec);
	p = localtime(&timec);
	PrintTime(*p, "判伤");


	//58行对应7大格
	float pixel = 58.0 / 7;

	//各通道抑制值
	float restr[CH_N] = { 0 };
	for (int i = 0; i < CH_N; ++i)
	{
		restr[i] = 0.005 * head.deviceP2.Restr[i];
	}

	//先寻找二次波，再去找一次波对照
	const int chOrder[10] = { CH_c, CH_a1, CH_A1, CH_b1, CH_B1, CH_C, CH_D, CH_E, CH_F, CH_G };
	for (int order = 0; order < 10; ++order)
	{
		int m = chOrder[order];
		vector<Connected_Region>& vcr = vCRs[m];
		for (int i = 0; i < vCRs[m].size(); ++i)
		{
			Connected_Region &cr = vcr[i];
			if (cr.Region.size() == 1 || cr.Flag > 0 || mBlockIndex.find(cr.Region[0].block) == mBlockIndex.end())
			{
				continue;
			}

			//TRACE("%Channel = 3d, Block = %4d, Step1 = %6d, Row1 = %3d, Step2 = %6d Row2 = %3d\n", cr.Channel, cr.Block, cr.Step1, cr.Row1, cr.Step2, cr.Row2);

			int iBlock = mBlockIndex[cr.Region[0].block];
			uint8_t iChA = GetAChannelByBChannel(m);//该通道对应的A通道
			BLOCK& blockHead = blocks[iBlock].BlockHead;
			hour = (blockHead.time & 0x00FF0000) >> 16;
			minute = (blockHead.time & 0x0000FF00) >> 8;
			second = (blockHead.time & 0x000000FF);

			int offset = head.deviceP2.Place[iChA] + blockHead.probOff[iChA];
			double angle = 0.1 * head.deviceP2.Angle[iChA].Refrac;

			CR_INFO crInfo;
			bool bFindFrames = GetCRInfo(cr, crInfo, DataA, blocks, angle, offset, head.step, head.deviceP2.Restr[iChA], head.deviceP2.Trig[iChA], blockHead.gain[iChA]);
			vector<A_Step>& vASteps = cr.vASteps;

			// BIT0~1: 轨型(0为43轨，1-50，2-60，3-75), BIT4:0逆里程、1顺里程，BIT5:0右股、1左股，BIT6~7：单线、上行、下行，其他预留
			int railType = blockHead.railType & 0x03;
			int iFRow = blockHead.railH / 3;

			bool direction = railType & BIT4;//direction: true:顺里程，false:逆里程
			bool carType = blockHead.detectSet.Identify & BIT2;// 车型，1-右手车，0-左手车
			double wd = GetWD(blockHead.walk, cr.Step, head.step, direction);

			vector<TPoint> points;
			if (m < 14)
			{
				if (m >= CH_A1 && m < CH_D)
				{
					vector<int> t_cr[10];
					int iAStepSmall, iAStepBig;
					if (ParsePosition(head, blocks, vCRs, cr, i, iFRow, railType, t_cr, pm, iAStepBig, iAStepSmall))
					{
						if (pm.Mark == PM_SEW_CH && iAStepBig - iAStepSmall> 10)
						{
							pm.Mark = PM_SEW_LRH;
						}
						vPMs.push_back(pm);
						if (pm.Mark == PM_SEW_CH)
						{
							ParseSewCH(head, DataA, blocks, vCRs, cr, i, iFRow, carType, railType, t_cr, iAStepBig, iAStepSmall, vWounds);
						}
						else if (pm.Mark == PM_SEW_LRH)
						{
							ParseSewLRH(head, DataA, blocks, vCRs, cr, i, iFRow, carType, railType, t_cr, iAStepBig, iAStepSmall, vWounds);
						}

						if (pm.Mark == PM_SEW_CH || pm.Mark == PM_SEW_LRH)
						{
							SetJudgedFlag(vCRs[CH_C], t_cr[8], 1);
							SetJudgedFlag(vCRs[CH_c], t_cr[9], 1);
						}
						continue;
					}
				}

				if (m == CH_A1 || m == CH_a1 || m == CH_B1 || m == CH_b1)
				{
					Wound_Judged wound;
					wound.Walk = wd;
					wound.Block = cr.Block;
					wound.Step = cr.Step;
					FillWound(wound, blockHead, head);
					wound.vCRs.push_back(cr);

					if (cr.Row1 <= 13 && cr.Row2 >= 13)//轨鄂连贯伤
					{
						if (crInfo.MaxV >= 150 && crInfo.Shift >= 30)
						{
							if ((carType == true && (m == CH_A1 || m == CH_a1)) || (carType == false && (m == CH_B1 || m == CH_b1)))
							{
								wound.Type = W_HEAD_HS_IN;
								wound.Place = WP_HEAD_IN;
							}
							else
							{
								wound.Type = W_HEAD_HS_OUT;
								wound.Place = WP_HEAD_OUT;
							}
							wound.SizeX = (cr.Row2 - cr.Row1) * 3;
							wound.SizeY = (cr.Step2 - cr.Step1) * 2.67;
							wound.Degree = 4;
							sprintf_s(wound.According, "%s出波，位移：%.1f大格，幅值%d", ChannelNamesB[m], 0.02f * crInfo.Shift, crInfo.MaxV);
							vWounds.push_back(wound);
						}
					}
					else if (cr.Row1 >= 13)//纯二次波
					{
						if (bFindFrames)//在A超中能找到对应的帧数据
						{
							//二次回波位移2大格以上判定重伤，1.5大格是轻伤
							if (crInfo.Shift >= 75 && crInfo.MaxV >= 150)
							{
								uint8_t iCh_R = rcs[m];
								vector<int> t_cr;
								uint8_t bFind = GetCR(iCh_R, cr.Step1, 13, cr.Step2, 26, blocks, vCRs[iCh_R], t_cr);		//对照 对向二次波 A2 <=> a2

								memcpy(wound.Result, "轨头内侧轻伤", 15);
								if ((carType == true && (m == CH_A1 || m == CH_a1)) || (carType == false && (m == CH_B1 || m == CH_b1)))
								{
									wound.Type = W_HEAD_HS_IN; wound.Place = WP_HEAD_IN;
								}
								else
								{
									wound.Type = W_HEAD_HS_OUT; wound.Place = WP_HEAD_OUT;
								}
								wound.Degree = 2;
								if (!bFind)
								{
									strcat(wound.Result, ",一次顺向斜核伤");
								}
								else
								{
									strcat(wound.Result, ",竖直横核伤");
								}

								if (crInfo.Shift >= 100)
								{
									wound.Degree = 4;
									memcpy(wound.Result, "轨头内侧重伤", 15);

									if (!bFind)
									{
										strcat(wound.Result, ",一次顺向斜核伤");
									}
									else
									{
										strcat(wound.Result, ",竖直横核伤");
									}
								}
								wound.SizeX = 1.5 * (cr.Row2 - cr.Row1);
								wound.SizeY = 1.33 * (cr.Step2 - cr.Step1);
								sprintf_s(wound.According, "%s出波，位移：%.1f大格，幅值%d", ChannelNamesB[m], 0.02f * crInfo.Shift, crInfo.MaxV);
								AddWoundData(wound, vCRs[iCh_R], t_cr);
								vWounds.push_back(wound);
								SetJudgedFlag(vCRs[iCh_R], t_cr, 1);
							}
							else//二次波回波不到1.5大格
							{
								int iCh_R = rcs[m];//对应的对向通道

								//一次波的话任意位移1大格加上对向（例如A对应a）二次波验证判重伤, 找对向一次波
								vector<int> t_cr;
								uint8_t iFindFirst = GetCR(iCh_R, cr.Step1, 0, cr.Step2, 13, blocks, vCRs[iCh_R], t_cr);
								if (iFindFirst)
								{
									CR_INFO info;
									int iChA_R = GetAChannelByBChannel(iCh_R);
									if (GetCRInfo(vCRs[iCh_R][t_cr[0]], info, DataA, blocks, 0.1 * head.deviceP2.Angle[iChA_R].Refrac, head.deviceP2.Place[iChA_R] + blocks[iBlock].BlockHead.probOff[iChA_R], head.step,
										head.deviceP2.Restr[iChA_R], head.deviceP2.Trig[iChA_R], blockHead.gain[iChA_R]))
									{
										if (info.MaxH - info.MinH >= 40 && info.MaxV >= 150)
										{
											if ((carType == true && (m == CH_A1 || m == CH_a1)) || (carType == false && (m == CH_B1 || m == CH_b1)))
											{
												wound.Type = W_HEAD_HS_IN;
												wound.Place = WP_HEAD_IN;
											}
											else
											{
												wound.Type = W_HEAD_HS_OUT;
												wound.Place = WP_HEAD_OUT;
											}
											wound.Degree = 4;
											memcpy(wound.Result, "轨头内侧重伤", 15);
											wound.SizeX = (cr.Row2 - cr.Row1) * 3;
											wound.SizeY = (cr.Step2 - cr.Step1) * 2.67;
											AddWoundData(wound, vCRs[iCh_R], t_cr);
											sprintf_s(wound.According, "%s出波，位移：%.1f大格，幅值%d", ChannelNamesB[m], 0.02f * crInfo.Shift, crInfo.MaxV);
											vWounds.push_back(wound);
											SetJudgedFlag(vCRs[iCh_R], t_cr, 1);
										}
									}
								}
							}
						}
						//设置判断标志
						cr.Flag = 1;
					}
					else if (cr.Row2 <= 13)////纯一次波
					{
						if (crInfo.Shift >= 50 && crInfo.MaxV >= 150)
						{
							vector<int> t_crR;
							int iCh_R = rcs[m];
							bool bFind = GetCR(rcs[iCh_R], cr.Step1, 13, cr.Step2, 26, blocks, vCRs[iCh_R], t_crR);//寻找对应通道二次波 A1 <=> a2							
							if (bFind)
							{
								if ((carType == true && (m == CH_A1 || m == CH_a1)) || (carType == false && (m == CH_B1 || m == CH_b1)))
								{
									memcpy(wound.Result, "轨头内侧重伤", 15);
									wound.Type = W_HEAD_HS_IN;
									wound.Place = WP_HEAD_IN;
								}
								else
								{
									memcpy(wound.Result, "轨头外侧重伤", 15);
									wound.Type = W_HEAD_HS_OUT;
									wound.Place = WP_HEAD_OUT;
								}
								wound.SizeX = (cr.Row2 - cr.Row1) * 3;
								wound.SizeY = (cr.Step2 - cr.Step1) * 2.67;
								wound.Degree = 4;
								wound.vCRs.push_back(cr);
								AddWoundData(wound, vCRs[iCh_R], t_crR);
								sprintf_s(wound.According, "%s出波，位移：%.1f大格，幅值%d", ChannelNamesB[m], 0.02f * crInfo.Shift, crInfo.MaxV);
								vWounds.push_back(wound);
							}

							SetJudgedFlag(vCRs[iCh_R], t_crR, 1);
						}
						cr.Flag = 1;
					}

					cr.Flag = 1;
				}
				else if (m == CH_C || m == CH_c)//C, c 通道
				{
					Wound_Judged wound;
					wound.Walk = wd;
					wound.Block = cr.Block;
					wound.Step = cr.Step;
					FillWound(wound, blockHead, head);
					wound.vCRs.push_back(cr);
					wound.Type = W_HEAD_HS_MID;
					wound.Place = WP_HEAD_MID;
					wound.Degree = 4;
					wound.SizeX = (cr.Row2 - cr.Row1) * 3;
					wound.SizeY = (cr.Step2 - cr.Step1) * 2.67;
					memcpy(wound.Result, "轨头中部横向裂纹", 22);
					if (bFindFrames)
					{
						if (crInfo.Shift >= 25 && crInfo.MaxV >= 150 && cr.Row1 < 26)
						{
							sprintf_s(wound.According, "%s出波，位移：%.1f大格，幅值%d", ChannelNamesB[m], 0.02f * crInfo.Shift, crInfo.MaxV);
							vWounds.push_back(wound);
						}
					}
					//设置判断标志
					cr.Flag = 1;
				}
				else if (m == CH_D)//D通道
				{
					Wound_Judged wound;
					wound.Place = WP_WAIST;
					wound.Walk = wd;
					wound.Block = cr.Block;
					wound.Step = cr.Step;
					FillWound(wound, blockHead, head);
					wound.vCRs.push_back(cr);

					int iBottomRow = GetFRow(blocks[iBlock].vBStepDatas[cr.Step]);
					int iDesiredFRow = blocks[iBlock].BlockHead.railH / 3;//期望的F出波行
					if (iDesiredFRow - cr.Row1 <= 5 && crInfo.MaxV >= 150)//轨底出波
					{
						vector<int> crE;
						uint8_t bFindE = GetCR(CH_E, cr.Step1, cr.Row1, cr.Step2 + 5, cr.Row2, blocks, vCRs[CH_E], crE);
						if (bFindE)
						{
							int iD = cr.Step1 + cr.Step2;
							for (int p = 0; p < crE.size(); ++p)
							{
								int iE = vCRs[CH_E][crE[p]].Step1 + vCRs[CH_E][crE[p]].Step2;
								if (iD < iE)
								{
									CR_INFO info;
									GetCRInfo(vCRs[CH_E][crE[p]], info, DataA, blocks, 0.1 * head.deviceP2.Angle[ACH_E].Refrac, head.deviceP2.Place[ACH_E] + blockHead.probOff[ACH_E], head.step, head.deviceP2.Restr[ACH_E], head.deviceP2.Trig[ACH_E], blockHead.gain[ACH_E]);
									if (info.MaxV >= 150)
									{
										wound.Type = WOUND_TYPE::W_BOTTOM_TRANSVERSE_CRACK;
										wound.Place = WP_BOTTOM;
										wound.Degree = 4;
										wound.SizeX = (cr.Row2 - cr.Row1) * 3;
										wound.SizeY = (cr.Step2 - cr.Step1) * 2.67;
										memcpy(wound.Result, "轨底横向裂纹", 20);
										AddWoundData(wound, vCRs[CH_E], crE);
										sprintf_s(wound.According, "D出波，位移：%.1f大格，幅值:%d;E出波，位移：%.1f大格，幅值:%d", 0.02f * crInfo.Shift, crInfo.MaxV, 0.02f * info.Shift, info.MaxV);
										vWounds.push_back(wound);
										break;
									}
								}
							}
						}

						cr.Flag = 1;
						SetJudgedFlag(vCRs[CH_E], crE, 1);
					}
					else
					{
						bool bGuideHole = false;//导孔
						bool bScrewHole = false;//螺孔

						bool bWhole = crInfo.MaxV >= 150; //D是否满峰	
						vector<int> crF, crG, crE, crF2, crG2, crD;
						uint8_t bFindD = GetCR(CH_D, cr.Step2 - 1, cr.Row1, cr.Step2 + 15, cr.Row2, blocks, vCRs[CH_D], crD, i, 3);
						uint8_t bFindE = GetCR(CH_E, cr.Step2 - 1, cr.Row1, cr.Step2 + 15, cr.Row2, blocks, vCRs[CH_E], crE, -1, 3);
						uint8_t bFindF = GetCR(CH_F, cr.Step1, cr.Row1 - 1, cr.Step2 + 15, cr.Row2, blocks, vCRs[CH_F], crF2, -1, 2);//F螺孔高度出波
						uint8_t bFindG = GetCR(CH_G, cr.Step1, cr.Row1 - 1, cr.Step2 + 15, cr.Row2, blocks, vCRs[CH_G], crG2, -1, 2);//G螺孔高度出波
						uint8_t bLoseF = GetCR(CH_F, cr.Step2 - 1, iDesiredFRow - 3, cr.Step2 + 4, iDesiredFRow + 3, blocks, vCRs[CH_F], crF);
						uint8_t bLoseG = GetCR(CH_G, cr.Step2 - 1, iDesiredFRow - 3, cr.Step2 + 4, iDesiredFRow + 3, blocks, vCRs[CH_G], crG);

						int iFStep2 = 0;
						if (bFindF)//F螺孔出波
						{
							iFStep2 = (vCRs[CH_F][crF2[0]].Step2 + vCRs[CH_F][crF2[0]].Step1) >> 1;
						}
						else if (bLoseF)
						{
							iFStep2 = (vCRs[CH_F][crF[0]].Step2 + vCRs[CH_F][crF[0]].Step1) >> 1;
						}
						else
						{
							iFStep2 = -999;
						}

						if (cr.Row1 <= iLuokong_D_Row1_H[railType] && cr.Row1 >= iLuokong_D_Row1_L[railType])//螺孔高度出D波
						{
							bool bLose = bLoseF || bLoseG;
							if ((crInfo.Shift >= 10 && bWhole) && (bFindE || bFindF || bFindG) && (bLoseF && bLoseG))
							{
								//F失波长度 >= 8个步进，为螺孔而不是导孔
								int iF1 = vCRs[CH_F][crF[0]].Step1;
								int iF2 = vCRs[CH_F][crF[0]].Step2;
								int ir1 = vCRs[CH_F][crF[0]].Row1;
								int ir2 = vCRs[CH_F][crF[0]].Row2;

								if (iF2 - iF1 >= 3)//F底部失波步进数
								{
									Position_Mark pm;
									memset(&pm, 0, sizeof(pm));
									pm.Walk = wd;
									pm.Block = cr.Block;
									pm.Step = cr.Step;
									pm.Mark = PM_SCREWHOLE;
									vPMs.push_back(pm);
									bScrewHole = true;
								}
								else
								{
									Position_Mark pm;
									memset(&pm, 0, sizeof(pm));
									pm.Walk = wd;
									pm.Mark = PM_GUIDEHOLE;
									pm.Block = cr.Block;
									pm.Step = cr.Step;
									vPMs.push_back(pm);
									bGuideHole = true;
								}
							}
							else if ((crInfo.Shift >= 6 && bWhole) && (bFindE || bFindF || bFindG))
							{
								Position_Mark pm;
								memset(&pm, 0, sizeof(pm));
								pm.Walk = wd;
								pm.Mark = PM_GUIDEHOLE;
								pm.Block = cr.Block;
								pm.Step = cr.Step;
								vPMs.push_back(pm);
								bGuideHole = true;
							}
						}
						else if (cr.Row1 <= blocks[iBlock].BlockHead.railH / 3 - 4)//导孔、杂波、斜裂纹
						{
							bool bWhole = crInfo.MaxV >= 150; //D是否满峰
							vector<int> crF, crG, crE, crF2, crG2;
							uint8_t bFindF = GetCR(CH_F, cr.Step1, cr.Row1 - 1, cr.Step2 + 4, cr.Row2, blocks, vCRs[CH_F], crF2, -1, 2);//F螺孔高度出波
							uint8_t bFindG = GetCR(CH_G, cr.Step1, cr.Row1 - 1, cr.Step2 + 4, cr.Row2, blocks, vCRs[CH_G], crG2, -1, 2);//G螺孔高度出波
							uint8_t bFindE = GetCR(CH_E, cr.Step2 - 1, cr.Row1, cr.Step2 + 15, cr.Row2, blocks, vCRs[CH_E], crE, -1, 3);
							uint8_t bLoseF = GetCR(CH_F, cr.Step2 - 1, iDesiredFRow - 3, cr.Step2 + 4, iDesiredFRow + 3, blocks, vCRs[CH_F], crF);
							uint8_t bLoseG = GetCR(CH_G, cr.Step2 - 1, iDesiredFRow - 3, cr.Step2 + 4, iDesiredFRow + 3, blocks, vCRs[CH_G], crG);
							bool bLose = bLoseF || bLoseG;

							if ((crInfo.Shift >= 6 && bWhole) && bFindE)
							{
								Position_Mark pm;
								memset(&pm, 0, sizeof(pm));
								pm.Walk = wd;
								pm.Mark = PM_GUIDEHOLE;
								pm.Block = cr.Block;
								pm.Step = cr.Step;
								vPMs.push_back(pm);
								bGuideHole = true;
							}
						}

						int step1 = 0, step2 = 0;//A超中cr对应的步进
						GetStepSteps(CH_D, cr, step1, step2, blocks, angle, offset, head.step);

						if (bScrewHole) //螺孔
						{
							//ParseScrewHole(head, DataA, blocks, vCRs, cr, i, iFRow, railType, vWounds);
							//检测四象限斜裂纹
							vector<int> crDt;
							uint8_t iFindD_4 = GetCR(CH_D, cr.Step1, cr.Row2, cr.Step2, cr.Row2 + 10, blocks, vCRs[CH_D], crDt, i);
							if (iFindD_4)//D通道检测到伤损
							{
								for (int k = 0; k < crDt.size(); ++k)
								{
									if (vCRs[CH_D][crDt[k]].Region.size() < 3)
									{
										continue;
									}

									int step11 = 0, step12 = 0;
									GetStepSteps(m, vCRs[CH_D][crDt[k]], step11, step12, blocks, angle, offset, head.step);
									if (step11 >= step1 && step12 <= step2)
									{
										vCRs[CH_D][crDt[k]].Flag = 1;
									}
									else
									{
										CR_INFO info_4;
										if (!GetCRInfo(vCRs[CH_D][crDt[k]], info_4, DataA, blocks, angle, offset, head.step, head.deviceP2.Restr[iChA], head.deviceP2.Trig[iChA], blockHead.gain[iChA]))
										{
											continue;
										}
										else if (info_4.MaxV >= 150)
										{
											Wound_Judged w;
											w.Block = vCRs[CH_D][crDt[k]].Block;
											w.Step = vCRs[CH_D][crDt[k]].Step;
											w.Walk = GetWD(blocks[mBlockIndex[vCRs[CH_D][crDt[k]].Block]].BlockHead.walk, vCRs[CH_D][crDt[k]].Step1 - blocks[mBlockIndex[vCRs[CH_D][crDt[k]].Block]].BlockHead.indexL, head.step);
											FillWound(w, blockHead, head);
											w.Type = WOUND_TYPE::W_SCREW_CRACK4;
											w.Place = WP_WAIST;
											w.Degree = 4;
											w.SizeX = (vCRs[CH_D][crDt[k]].Row2 - vCRs[CH_D][crDt[k]].Row1) / 0.6;
											w.SizeY = 1;
											sprintf_s(w.According, "D出波，位移：%.1f大格，幅值:%d", 0.02f * info_4.Shift, info_4.MaxV);
											memcpy(w.Result, "螺孔四象限斜裂纹", 30);
											w.vCRs.push_back(cr);
											AddWoundData(w, vCRs[CH_F], crF);
											AddWoundData(w, vCRs[CH_F], crF2);
											AddWoundData(w, vCRs[CH_G], crG);
											AddWoundData(w, vCRs[CH_G], crG2);
											AddWoundData(w, vCRs[CH_E], crE);
											AddWoundData(w, vCRs[CH_D], crDt);
											vWounds.push_back(w);
											SetJudgedFlag(vCRs[CH_D], crDt, 1);
											break;
										}
									}
								}
							}

							//检测二象限斜裂纹
							crDt.clear();
							uint8_t iFindD_2 = GetCR(CH_D, cr.Step1 + 10, cr.Row1, cr.Step2 + 10, cr.Row2, blocks, vCRs[CH_D], crDt, i);
							for (int k = 0; k < crDt.size(); ++k)
							{
								CR_INFO info_2;
								if (GetCRInfo(vCRs[CH_D][crDt[k]], info_2, DataA, blocks, angle, offset, head.step, head.deviceP2.Restr[iChA], head.deviceP2.Trig[iChA], blockHead.gain[iChA]) && info_2.MaxV >= 150)
								{
									Wound_Judged w;
									w.Block = cr.Block;
									w.Step = cr.Step;
									int idx_t = vCRs[CH_D][crDt[k]].Step1 - blocks[mBlockIndex[vCRs[CH_D][crDt[k]].Block]].BlockHead.indexL;
									w.Walk = GetWD(blocks[iBlock].BlockHead.walk, idx_t, head.step);
									w.Type = WOUND_TYPE::W_SCREW_CRACK2;
									w.Place = WP_WAIST;
									w.Degree = 4;
									FillWound(w, blockHead, head);
									w.SizeX = (vCRs[CH_D][crDt[k]].Row2 - vCRs[CH_D][crDt[k]].Row1) / 0.6;
									w.SizeY = 1;
									sprintf_s(w.According, "D出波，位移：%.1f大格，幅值:%d", 0.02f * info_2.Shift, info_2.MaxV);
									memcpy(w.Result, "螺孔二象限斜裂纹", 30);
									w.vCRs.push_back(cr);
									SetJudgedFlag(vCRs[CH_D], crDt, 1);
									AddWoundData(w, vCRs[CH_D], crDt);
									AddWoundData(w, vCRs[CH_F], crF);
									AddWoundData(w, vCRs[CH_F], crF2);
									AddWoundData(w, vCRs[CH_G], crG2);
									AddWoundData(w, vCRs[CH_E], crE);
									vWounds.push_back(w);
									break;
								}
							}

							vector<int> crEt;
							uint8_t iFindE_1 = GetCR(CH_E, cr.Step1, cr.Row1, cr.Step2 + 3, cr.Row2, blocks, vCRs[CH_E], crEt);
							Exclude(crEt, crE);
							for (int k = 0; k < crEt.size(); ++k)
							{
								if (vCRs[CH_E][crEt[k]].Region.size() > 3)
								{
									int iEStep2 = (vCRs[CH_E][crEt[k]].Step2 + vCRs[CH_E][crEt[k]].Step1) >> 1;
									if (iEStep2 < iFStep2 - 3)//E早于F结束，说明E是伤损
									{
										Wound_Judged w;
										w.Block = cr.Block;
										w.Step = cr.Step;
										w.Walk = GetWD(blocks[iBlock].BlockHead.walk, vCRs[CH_E][crEt[0]].Step1 - blocks[iBlock].BlockHead.indexL, head.step);
										FillWound(w, blockHead, head);
										w.Degree = 4;
										memcpy(w.Result, "螺孔一象限斜裂纹", 30);
										w.Place = WP_WAIST;
										w.Type = WOUND_TYPE::W_SCREW_CRACK1;
										w.vCRs.push_back(cr);
										AddWoundData(w, vCRs[CH_E], crEt);
										AddWoundData(w, vCRs[CH_F], crF);
										AddWoundData(w, vCRs[CH_F], crF2);
										AddWoundData(w, vCRs[CH_G], crG);
										AddWoundData(w, vCRs[CH_G], crG2);
										AddWoundData(w, vCRs[CH_E], crE);
										w.SizeX = (vCRs[CH_E][crEt[k]].Row2 - vCRs[CH_E][crEt[k]].Row1) / 0.6;
										w.SizeY = 1;
										sprintf_s(w.According, "E出波，且早于F结束");
										vWounds.push_back(w);
									}
									SetJudgedFlag(vCRs[CH_E], crEt, 1);
								}
							}

							if (bFindE)
							{
								int iStepE1 = 0, iStepE2 = 0;
								GetStepSteps(CH_E, vCRs[CH_E][crE[0]], iStepE1, iStepE2, blocks, 0.1 * head.deviceP2.Angle[ACH_E].Refrac, offset, head.step);

								crEt.clear();
								uint8_t iFind_3 = GetCR(CH_E, cr.Step1 + 10, cr.Row2 + 1, cr.Step2 + 10, cr.Row2 + 10, blocks, vCRs[CH_E], crEt);
								Exclude(crEt, crE);
								iFind_3 = crEt.size() > 0;
								if (iFind_3)
								{
									for (int k = 0; k < crEt.size(); ++k)
									{
										if (vCRs[CH_E][crEt[k]].Region.size() < 3)
										{
											continue;
										}

										int stepE1 = 0, stepE2 = 0;
										GetStepSteps(CH_E, vCRs[CH_E][crE[0]], iStepE1, iStepE2, blocks, 0.1 * head.deviceP2.Angle[ACH_E].Refrac, offset, head.step);
										if (stepE1 >= iStepE1 && iStepE2 <= step2)
										{

										}
										else
										{
											CR_INFO info_3;
											if (GetCRInfo(vCRs[CH_E][crEt[k]], info_3, DataA, blocks, angle, offset, head.step, head.deviceP2.Restr[ACH_E], head.deviceP2.Trig[ACH_E], blockHead.gain[ACH_E]) && info_3.MaxV >= 150)
											{
												Wound_Judged w;
												w.Block = vCRs[CH_E][crEt[k]].Block;
												w.Step = vCRs[CH_E][crEt[k]].Step;
												w.Walk = GetWD(blocks[mBlockIndex[w.Block]].BlockHead.walk, w.Step, head.step);
												FillWound(w, blockHead, head);
												w.Degree = 4;
												memcpy(w.Result, "螺孔三象限斜裂纹", 30);
												w.SizeX = (vCRs[CH_E][crEt[k]].Row2 - vCRs[CH_E][crEt[k]].Row1) / 0.6;
												w.SizeY = 1;
												sprintf_s(w.According, "E出波，位移：%.1f大格，幅值:%d", 0.02f * info_3.Shift, info_3.MaxV);
												w.Place = WP_WAIST;
												w.Type = WOUND_TYPE::W_SCREW_CRACK3;
												SetJudgedFlag(vCRs[CH_D], crDt, 1);
												w.vCRs.push_back(cr);
												AddWoundData(w, vCRs[CH_D], crDt);
												AddWoundData(w, vCRs[CH_E], crEt);
												AddWoundData(w, vCRs[CH_F], crF);
												AddWoundData(w, vCRs[CH_F], crF2);
												AddWoundData(w, vCRs[CH_G], crG2);
												vWounds.push_back(w);
												break;
											}
										}
									}
								}
							}

							//水平裂纹
							vector<int> crF3, crF4, crG3, crG4;
							if (bFindF || bFindG)
							{
								if (bFindF)
								{
									uint8_t iF3 = 0;
									uint8_t iF4 = 0;
									Connected_Region& cr_F_M = vCRs[CH_F][crF2[0]];
									iF3 = GetCR(CH_F, cr_F_M.Step1 - 5, cr_F_M.Row2 + 1, cr_F_M.Step1 + 5, iDesiredFRow - 4, blocks, vCRs[CH_F], crF3, crF2[0]);
									if (iF3)
									{
										for (int z = 0; z < crF3.size(); ++z)
										{
											if (vCRs[CH_F][crF3[z]].Step1 >= cr_F_M.Step1 && vCRs[CH_F][crF3[z]].Step2 <= cr_F_M.Step2)
											{
												continue;
											}
											else
											{
												Wound_Judged w;
												w.Block = vCRs[CH_F][crF3[z]].Block;
												w.Step = vCRs[CH_F][crF3[z]].Step;
												w.Type = W_SCREW_HORIZON_CRACK;
												w.Place = WP_WAIST;
												w.SizeX = head.step * (vCRs[CH_F][crF3[z]].Step2 - vCRs[CH_F][crF3[z]].Step1);
												w.SizeY = 1;
												sprintf_s(w.According, "F出波");
												FillWound(w, blockHead, head);
												Connected_Region& cr_F_des = vCRs[CH_F][crF3[0]];
												int j_block = mBlockIndex[cr_F_des.Block];
												w.Walk = GetWD(blocks[j_block].BlockHead.walk, cr_F_des.Step1 - blocks[j_block].BlockHead.indexL, head.step);
												memcpy(w.Result, "螺孔右侧水平裂纹", 30);
												w.Degree = 4;

												SetJudgedFlag(vCRs[CH_F], crF3, 1);
												w.vCRs.push_back(cr);
												AddWoundData(w, vCRs[CH_F], crF3);
												AddWoundData(w, vCRs[CH_E], crEt);
												AddWoundData(w, vCRs[CH_F], crF);
												AddWoundData(w, vCRs[CH_F], crF2);
												AddWoundData(w, vCRs[CH_G], crG2);
												vWounds.push_back(w);
											}
										}
									}

									iF4 = GetCR(CH_F, cr_F_M.Step2 - 5, cr_F_M.Row2 + 1, cr_F_M.Step2 + 5, iDesiredFRow - 4, blocks, vCRs[CH_F], crF4, crF2[0]);
									if (iF4)
									{
										for (int z = 0; z < crF4.size(); ++z)
										{
											if (vCRs[CH_F][crF4[z]].Step1 >= cr_F_M.Step1 && vCRs[CH_F][crF4[z]].Step2 <= cr_F_M.Step2)
											{
												continue;
											}
											else
											{
												Wound_Judged w;
												w.Block = vCRs[CH_F][crF4[z]].Block;
												w.Step = vCRs[CH_F][crF4[z]].Step;
												w.Place = WP_WAIST;
												w.Type = W_SCREW_HORIZON_CRACK;
												FillWound(w, blockHead, head);
												w.SizeX = head.step * (vCRs[CH_F][crF4[z]].Step2 - vCRs[CH_F][crF4[z]].Step1);
												w.SizeY = 1;
												sprintf_s(w.According, "F出波");
												Connected_Region& cr_F_des = vCRs[CH_F][crF4[0]];
												int j_block = mBlockIndex[w.Block];
												w.Walk = GetWD(blocks[j_block].BlockHead.walk, w.Step, head.step);
												memcpy(w.Result, "螺孔左侧水平裂纹", 30);
												w.Degree = 4;

												SetJudgedFlag(vCRs[CH_F], crF4, 1);
												w.vCRs.push_back(cr);
												AddWoundData(w, vCRs[CH_F], crF4);
												AddWoundData(w, vCRs[CH_E], crEt);
												AddWoundData(w, vCRs[CH_F], crF);
												AddWoundData(w, vCRs[CH_F], crF2);
												AddWoundData(w, vCRs[CH_G], crG2);
												vWounds.push_back(w);
											}
										}
									}
								}
								else
								{
									uint8_t iG3 = 0;
									uint8_t iG4 = 0;
									Connected_Region& cr_G_M = vCRs[CH_G][crG2[0]];
									iG3 = GetCR(CH_G, cr_G_M.Step1 - 5, cr_G_M.Row2 + 1, cr_G_M.Step2 + 5, iDesiredFRow - 4, blocks, vCRs[CH_G], crG3, crG2[0]);
									if (iG3)
									{
										for (int z = 0; z < crG3.size(); ++z)
										{
											if (vCRs[CH_G][crG3[z]].Step1 >= cr_G_M.Step1 && vCRs[CH_G][crG3[z]].Step2 <= cr_G_M.Step2)
											{
												continue;
											}
											else
											{
												Wound_Judged w;
												w.Block = vCRs[CH_G][crG3[z]].Block;
												w.Step = vCRs[CH_G][crG3[z]].Step;
												w.Type = W_SCREW_HORIZON_CRACK;
												w.Place = WP_WAIST;

												w.SizeX = head.step * (vCRs[CH_G][crG3[z]].Step2 - vCRs[CH_G][crG3[z]].Step1);
												w.SizeY = 1;
												sprintf_s(w.According, "G出波");
												FillWound(w, blockHead, head);

												Connected_Region& cr_G_des = vCRs[CH_G][crG3[0]];
												int j_block = mBlockIndex[w.Block];
												w.Walk = GetWD(blocks[j_block].BlockHead.walk, w.Step, head.step);
												memcpy(w.Result, "螺孔右侧水平裂纹", 30);
												w.Degree = 4;

												SetJudgedFlag(vCRs[CH_G], crG3, 1);
												w.vCRs.push_back(cr);
												AddWoundData(w, vCRs[CH_G], crG3);
												AddWoundData(w, vCRs[CH_E], crEt);
												AddWoundData(w, vCRs[CH_G], crG);
												AddWoundData(w, vCRs[CH_G], crG2);
												AddWoundData(w, vCRs[CH_G], crG2);
												vWounds.push_back(w);
											}
										}
									}

									iG4 = GetCR(CH_G, cr_G_M.Step2 - 5, cr_G_M.Row2 + 1, cr_G_M.Step2 + 5, iDesiredFRow - 4, blocks, vCRs[CH_G], crG4, crG2[0]);
									if (iG4)
									{
										for (int z = 0; z < crG4.size(); ++z)
										{
											if (vCRs[CH_G][crG4[z]].Step1 >= cr_G_M.Step1 && vCRs[CH_G][crG4[z]].Step2 <= cr_G_M.Step2)
											{
												continue;
											}
											else
											{
												Wound_Judged w;
												w.Block = vCRs[CH_G][crG4[z]].Block;
												w.Step = vCRs[CH_G][crG4[z]].Step;
												w.Place = WP_WAIST;
												w.Type = W_SCREW_HORIZON_CRACK;
												FillWound(w, blockHead, head);
												Connected_Region& cr_G_des = vCRs[CH_G][crG4[0]];
												int j_block = mBlockIndex[w.Block];
												w.Walk = GetWD(blocks[j_block].BlockHead.walk, w.Step, head.step);
												memcpy(w.Result, "螺孔左侧水平裂纹", 30);
												w.Degree = 4;

												SetJudgedFlag(vCRs[CH_G], crG4, 1);
												w.vCRs.push_back(cr);
												AddWoundData(w, vCRs[CH_G], crG4);
												AddWoundData(w, vCRs[CH_E], crEt);
												AddWoundData(w, vCRs[CH_G], crG);
												AddWoundData(w, vCRs[CH_G], crG2);
												AddWoundData(w, vCRs[CH_G], crG2);

												w.SizeX = head.step * (vCRs[CH_G][crG4[z]].Step2 - vCRs[CH_G][crG4[z]].Step1);
												w.SizeY = 1;
												sprintf_s(w.According, "G出波");

												vWounds.push_back(w);
												break;
											}
										}
									}
								}
							}
						}
						else if (bGuideHole)//导孔
						{
							vector<int> crDt;
							uint8_t iFindD_4 = GetCR(CH_D, cr.Step1, cr.Row2, cr.Step2, cr.Row2 + 10, blocks, vCRs[CH_D], crDt, i);
							if (iFindD_4)//D通道检测到伤损
							{
								for (int k = 0; k < crDt.size(); ++k)
								{
									if (vCRs[CH_D][crDt[k]].Region.size() < 3)
									{
										continue;
									}

									int step11 = 0, step12 = 0;
									GetStepSteps(m, vCRs[CH_D][crDt[k]], step11, step12, blocks, angle, offset, head.step);
									if (step11 >= step1 && step12 <= step2)
									{
										SetJudgedFlag(vCRs[CH_D], crDt, 1); //螺孔双波——4象限
									}
									else
									{
										CR_INFO t_info;
										if (GetCRInfo(vCRs[CH_D][crDt[k]], t_info, DataA, blocks, angle, offset, head.step, head.deviceP2.Restr[ACH_D], head.deviceP2.Trig[ACH_D], blockHead.gain[ACH_D]) && t_info.MaxV >= 150)
										{
											Wound_Judged w;
											w.Block = vCRs[CH_D][crDt[k]].Block;
											w.Step = vCRs[CH_D][crDt[k]].Step;
											w.Walk = GetWD(blocks[mBlockIndex[w.Block]].BlockHead.walk, w.Step, head.step);
											w.Type = WOUND_TYPE::W_SCREW_CRACK4;
											FillWound(w, blockHead, head);
											w.Place = WP_WAIST;
											w.Degree = 4;
											memcpy(w.Result, "螺孔四象限斜裂纹", 30);
											w.SizeX = head.step * (vCRs[CH_D][crDt[k]].Step2 - vCRs[CH_D][crDt[k]].Step1) / 0.8;
											w.SizeY = 1;
											sprintf_s(w.According, "D出波，位移：%.1f大格，幅值：%d", 0.02f * t_info.Shift, t_info.MaxV);

											SetJudgedFlag(vCRs[CH_D], crDt, 1);
											w.vCRs.push_back(cr);
											AddWoundData(w, vCRs[CH_F], crF);
											AddWoundData(w, vCRs[CH_F], crF2);
											AddWoundData(w, vCRs[CH_G], crG);
											AddWoundData(w, vCRs[CH_G], crG2);
											AddWoundData(w, vCRs[CH_E], crE);
											AddWoundData(w, vCRs[CH_D], crDt);
											vWounds.push_back(w);
											break;
										}
									}
								}
							}

							crDt.clear();
							uint8_t iFindD_2 = GetCR(CH_D, cr.Step1 + 10, cr.Row1, cr.Step2 + 10, cr.Row2, blocks, vCRs[CH_D], crDt, i);
							for (int k = 0; k < crDt.size(); ++k)
							{
								CR_INFO t_info;
								if (GetCRInfo(vCRs[CH_D][crDt[k]], t_info, DataA, blocks, angle, offset, head.step, head.deviceP2.Restr[ACH_D], head.deviceP2.Trig[ACH_D], blockHead.gain[ACH_D]) && t_info.MaxV >= 150)
								{
									Wound_Judged w;
									w.Block = vCRs[CH_D][crDt[k]].Block;
									w.Step = vCRs[CH_D][crDt[k]].Step;
									w.Walk = GetWD(blocks[mBlockIndex[w.Block]].BlockHead.walk, w.Step, head.step);
									w.Place = WP_WAIST;
									FillWound(w, blockHead, head);
									w.Type = WOUND_TYPE::W_SCREW_CRACK2;
									w.Degree = 4;
									memcpy(w.Result, "导孔二象限斜裂纹", 30);
									w.vCRs.push_back(cr);
									SetJudgedFlag(vCRs[CH_D], crDt, 1);
									AddWoundData(w, vCRs[CH_F], crF);
									AddWoundData(w, vCRs[CH_F], crF2);
									AddWoundData(w, vCRs[CH_G], crG);
									AddWoundData(w, vCRs[CH_G], crG2);
									AddWoundData(w, vCRs[CH_E], crE);
									AddWoundData(w, vCRs[CH_D], crDt);
									w.SizeX = head.step * (vCRs[CH_D][crDt[k]].Step2 - vCRs[CH_D][crDt[k]].Step1) / 0.8;
									w.SizeY = 1;
									sprintf_s(w.According, "D出波，位移：%.1f大格，幅值：%d", 0.02f * t_info.Shift, t_info.MaxV);
									vWounds.push_back(w);
									break;
								}
							}
						}
						else if (crInfo.Shift >= 10 && bWhole)
						{
							wound.Type = WOUND_TYPE::W_SKEW_CRACK;
							wound.Place = WP_WAIST;
							wound.Degree = 4;
							wound.Block = cr.Block;
							wound.Step = cr.Step;
							memcpy(wound.Result, "斜裂纹,重伤", 30);

							wound.SizeX = head.step * (cr.Step2 - cr.Step1) / 0.8;
							wound.SizeY = 1;
							sprintf_s(wound.According, "D出波，位移：%.1f大格，幅值：%d", 0.02f * crInfo.Shift, crInfo.MaxV);
						}

						if (wound.Type != 0)
						{
							AddWoundData(wound, vCRs[CH_F], crF);
							AddWoundData(wound, vCRs[CH_F], crF2);
							AddWoundData(wound, vCRs[CH_G], crG);
							AddWoundData(wound, vCRs[CH_G], crG2);
							AddWoundData(wound, vCRs[CH_E], crE);
							vWounds.push_back(wound);
						}

						SetJudgedFlag(vCRs[CH_F], crF, 1);
						SetJudgedFlag(vCRs[CH_F], crF2, 1);
						SetJudgedFlag(vCRs[CH_G], crG, 1);
						SetJudgedFlag(vCRs[CH_G], crG2, 1);
						SetJudgedFlag(vCRs[CH_E], crE, 1);
					}
				}
				else if (m == CH_E)//E 通道
				{
					Wound_Judged wound;
					wound.Walk = wd;
					wound.Block = cr.Block;
					wound.Step = cr.Step;
					FillWound(wound, blockHead, head);
					wound.vCRs.push_back(cr);

					int iBottomRow = GetFRow(blocks[iBlock].vBStepDatas[cr.Step]);
					if (iFRow - cr.Row1 <= 5)//轨底出波
					{

					}
					else
					{
						bool bGuideHole = false;//导孔
						bool bScrewHole = false;//螺孔

						bool bWhole = crInfo.MaxV >= 150; //D是否满峰	
						vector<int> crF, crG, crE, crF2, crG2;
						uint8_t bFindF = GetCR(CH_F, cr.Step1 - 15, cr.Row1 - 1, cr.Step1 - 1, cr.Row2, blocks, vCRs[CH_F], crF2, -1, 2);//F螺孔高度出波
						uint8_t bFindG = GetCR(CH_G, cr.Step1 - 15, cr.Row1 - 1, cr.Step1 - 1, cr.Row2, blocks, vCRs[CH_G], crG2, -1, 2);//G螺孔高度出波
						uint8_t bLoseF = GetCR(CH_F, cr.Step1 - 15, iFRow - 3, cr.Step1 - 1, iFRow + 3, blocks, vCRs[CH_F], crF);
						uint8_t bLoseG = GetCR(CH_G, cr.Step1 - 15, iFRow - 3, cr.Step1 - 1, iFRow + 3, blocks, vCRs[CH_G], crG);

						int iFStep2 = 0;
						if (bFindF)//F螺孔出波
						{
							iFStep2 = (vCRs[CH_F][crF2[0]].Step2 + vCRs[CH_F][crF2[0]].Step1) >> 1;
						}
						else if (bLoseF)
						{
							iFStep2 = (vCRs[CH_F][crF[0]].Step2 + vCRs[CH_F][crF[0]].Step1) >> 1;
						}
						else
						{
							iFStep2 = -999;
						}

						if (cr.Row1 <= iLuokong_D_Row1_H[railType] && cr.Row1 >= iLuokong_D_Row1_L[railType])//螺孔高度出E波
						{
							bool bLose = bLoseF || bLoseG;
							if ((crInfo.Shift >= 10 && bWhole) && (bFindF || bFindG) && (bLoseF && bLoseG))
							{
								int step1 = 0, step2 = 0;
								GetStepSteps(m, cr, step1, step2, blocks, angle, offset, head.step);

								//F失波长度 >= 8个步进，为螺孔而不是导孔
								int iF1 = vCRs[CH_F][crF[0]].Step1;
								int iF2 = vCRs[CH_F][crF[0]].Step2;
								int ir1 = vCRs[CH_F][crF[0]].Row1;
								int ir2 = vCRs[CH_F][crF[0]].Row2;

								if (iF2 - iF1 >= 7)//F底部失波步进数
								{
									memcpy(wound.Result, "螺孔", 20);
									Position_Mark pm;
									memset(&pm, 0, sizeof(pm));
									pm.Walk = wd;
									pm.Block = cr.Block;
									pm.Step = cr.Step;
									pm.Mark = PM_SCREWHOLE;
									vPMs.push_back(pm);
									bScrewHole = true;
								}
								else
								{
									memcpy(wound.Result, "导孔", 20);
									Position_Mark pm;
									memset(&pm, 0, sizeof(pm));
									pm.Walk = wd;
									pm.Mark = PM_GUIDEHOLE;
									pm.Block = cr.Block;
									pm.Step = cr.Step;
									vPMs.push_back(pm);
									bGuideHole = true;
								}
							}
							else if ((crInfo.Shift >= 6 && bWhole) && (bFindF || bFindG))
							{
								memcpy(wound.Result, "导孔", 20);
								Position_Mark pm;
								memset(&pm, 0, sizeof(pm));
								pm.Walk = wd;
								pm.Mark = PM_GUIDEHOLE;
								pm.Block = cr.Block;
								pm.Step = cr.Step;
								vPMs.push_back(pm);
								bGuideHole = true;
							}
						}
						else if (cr.Row1 <= blocks[iBlock].BlockHead.railH / 3 - 4)//导孔、杂波、斜裂纹
						{
							bool bWhole = crInfo.MaxV >= 150; //D是否满峰
							vector<int> crF, crG, crF2, crG2;
							uint8_t bFindF = GetCR(CH_F, cr.Step1, cr.Row1 - 1, cr.Step2 + 4, cr.Row2, blocks, vCRs[CH_F], crF2, -1, 2);//F螺孔高度出波
							uint8_t bFindG = GetCR(CH_G, cr.Step1, cr.Row1 - 1, cr.Step2 + 4, cr.Row2, blocks, vCRs[CH_G], crG2, -1, 2);//G螺孔高度出波
							uint8_t bLoseF = GetCR(CH_F, cr.Step2 - 1, iFRow - 3, cr.Step2 + 4, iFRow + 3, blocks, vCRs[CH_F], crF);
							uint8_t bLoseG = GetCR(CH_G, cr.Step2 - 1, iFRow - 3, cr.Step2 + 4, iFRow + 3, blocks, vCRs[CH_G], crG);
							bool bLose = bLoseF || bLoseG;

							if ((crInfo.Shift >= 6 && bWhole))
							{
								memcpy(wound.Result, "导孔", 20);
								Position_Mark pm;
								memset(&pm, 0, sizeof(pm));
								pm.Walk = wd;
								pm.Mark = PM_GUIDEHOLE;
								pm.Block = cr.Block;
								pm.Step = cr.Step;
								vPMs.push_back(pm);
								bGuideHole = true;
							}
						}

						int step1 = 0, step2 = 0;//A超中cr对应的步进
						GetStepSteps(CH_E, cr, step1, step2, blocks, angle, offset, head.step);

						if (bScrewHole) //螺孔
						{
							/*
							//检测四象限斜裂纹
							vector<int> crDt;
							uint8_t iFindD_4 = GetCR(CH_D, iBlock, cr.Step1, cr.Row2, cr.Step2, cr.Row2 + 10, blocks, vCRs[CH_D], crDt, i);
							if (iFindD_4)//D通道检测到伤损
							{
							for (int k = 0; k < crDt.size(); ++k)
							{
							if (vCRs[CH_D][crDt[k]].Region.size() < 3)
							{
							continue;
							}

							int step11 = 0, step12 = 0;
							GetStepSteps(m, vCRs[CH_D][crDt[k]], step11, step12, blocks, angle, angle2, offset, head.step);
							if (step11 >= step1 && step12 <= step2)
							{
							SetJudgedFlag(vCRs[CH_D], crDt, 1); //螺孔双波——4象限
							}
							else
							{
							Wound_Judged w;
							w.Walk = GetWD(blocks[iBlock].BlockHead.walk, vCRs[CH_D][crDt[0]].Step1 - blocks[iBlock].BlockHead.indexL, head.step);
							w.Type = WOUND_TYPE::W_SCREW_CRACK4;
							memcpy(w.Result, "螺孔四象限斜裂纹", 30);
							SetJudgedFlag(vCRs[CH_D], crDt, 1);
							w.vCRs.push_back(cr);
							AddWoundData(w, vCRs[CH_F], crF);
							AddWoundData(w, vCRs[CH_F], crF2);
							AddWoundData(w, vCRs[CH_G], crG);
							AddWoundData(w, vCRs[CH_G], crG2);
							AddWoundData(w, vCRs[CH_E], crE);
							AddWoundData(w, vCRs[CH_D], crDt);
							vWounds.push_back(w);
							}
							}
							}

							crDt.clear();
							uint8_t iFindD_2 = GetCR(CH_D, iBlock, cr.Step1 + 10, cr.Row1, cr.Step2 + 10, cr.Row2, blocks, vCRs[CH_D], crDt, i);
							for (int k = 0; k < crDt.size(); ++k)
							{
							Wound_Judged w;
							int idx_t = vCRs[CH_D][crDt[0]].Step1 - blocks[mBlockIndex[vCRs[CH_D][crDt[0]].Block]].BlockHead.indexL;
							w.Walk = GetWD(blocks[iBlock].BlockHead.walk, idx_t, head.step);
							w.Type = WOUND_TYPE::W_SCREW_CRACK2;
							memcpy(w.Result, "螺孔二象限斜裂纹", 30);
							w.vCRs.push_back(cr);
							SetJudgedFlag(vCRs[CH_D], crDt, 1);
							AddWoundData(w, vCRs[CH_D], crDt);
							AddWoundData(w, vCRs[CH_F], crF);
							AddWoundData(w, vCRs[CH_F], crF2);
							AddWoundData(w, vCRs[CH_G], crG2);
							AddWoundData(w, vCRs[CH_E], crE);
							vWounds.push_back(w);
							break;
							}
							*/

							vector<int> crEt;
							uint8_t iFindE_1 = GetCR(CH_E, cr.Step1 - 15, cr.Row1, cr.Step1, cr.Row2, blocks, vCRs[CH_E], crEt, i);
							Exclude(crEt, crE);
							if (crEt.size() > 0 && vCRs[CH_E][crEt[0]].Region.size() > 3)
							{
								int iEStep2 = (vCRs[CH_E][crEt[0]].Step2 + vCRs[CH_E][crEt[0]].Step1) >> 1;
								if (iEStep2 < iFStep2 - 3)//E早于F结束，说明E是伤损
								{
									Wound_Judged w;
									w.Block = cr.Block;
									w.Step = cr.Step;
									w.Walk = GetWD(blocks[iBlock].BlockHead.walk, vCRs[CH_E][crEt[0]].Step1 - blocks[iBlock].BlockHead.indexL, head.step);
									w.Place = WP_WAIST;
									w.Type = W_SCREW_CRACK1;
									FillWound(w, blockHead, head);
									memcpy(w.Result, "螺孔一象限斜裂纹", 30);
									w.Degree = 4;
									w.vCRs.push_back(cr);
									AddWoundData(w, vCRs[CH_E], crEt);
									AddWoundData(w, vCRs[CH_F], crF);
									AddWoundData(w, vCRs[CH_F], crF2);
									AddWoundData(w, vCRs[CH_G], crG);
									AddWoundData(w, vCRs[CH_G], crG2);

									wound.SizeX = head.step * (vCRs[CH_E][crEt[0]].Step2 - vCRs[CH_E][crEt[0]].Step1) / 0.8;
									wound.SizeY = 1;
									sprintf_s(wound.According, "E出波");

									vWounds.push_back(w);
								}
								SetJudgedFlag(vCRs[CH_E], crEt, 1);
							}

							crEt.clear();
							uint8_t bFindE_3 = GetCR(CH_E, cr.Step1, cr.Row1, cr.Step2, cr.Row2 + 10, blocks, vCRs[CH_E], crEt, i);
							if (bFindE_3)
							{
								for (int k = 0; k < crEt.size(); ++k)
								{
									if (vCRs[CH_E][crEt[k]].Region.size() < 3)
									{
										continue;
									}

									int stepE1 = 0, stepE2 = 0;
									GetStepSteps(CH_E, cr, stepE1, stepE2, blocks, 0.1 * head.deviceP2.Angle[ACH_E].Refrac, offset, head.step);
									if (stepE1 >= step1 && stepE2 <= step2)//螺孔双波——3象限
									{
									}
									else
									{
										Wound_Judged w;
										w.Block = vCRs[CH_E][crEt[k]].Block;
										w.Step = vCRs[CH_E][crEt[k]].Step;
										FillWound(w, blockHead, head);
										w.Walk = GetWD(blocks[mBlockIndex[w.Block]].BlockHead.walk, w.Step, head.step);
										memcpy(w.Result, "螺孔三象限斜裂纹", 30);
										w.Type = WOUND_TYPE::W_SCREW_CRACK3;
										w.Place = WP_WAIST;
										w.Degree = 4;
										SetJudgedFlag(vCRs[CH_E], crEt, 1);
										w.vCRs.push_back(cr);
										AddWoundData(w, vCRs[CH_E], crE);
										AddWoundData(w, vCRs[CH_F], crF);
										AddWoundData(w, vCRs[CH_G], crG);
										AddWoundData(w, vCRs[CH_F], crF2);
										AddWoundData(w, vCRs[CH_G], crG2);
										wound.SizeX = head.step * (vCRs[CH_E][crEt[k]].Step2 - vCRs[CH_E][crEt[k]].Step1) / 0.8;
										wound.SizeY = 1;
										sprintf_s(wound.According, "E出波");
										vWounds.push_back(w);
										break;
									}
								}
							}

							//水平裂纹
							vector<int> crF3, crF4;
							if (bFindF || bFindG)
							{
								Connected_Region& cr_F_M = bFindF ? vCRs[CH_F][crF2[0]] : vCRs[CH_G][crG2[0]];
								uint8_t iF3 = 0;
								uint8_t iF4 = 0;
								if (bFindF)
								{
									iF3 = GetCR(CH_F, cr_F_M.Step1 - 5, cr_F_M.Row2 + 1, cr_F_M.Step1 + 5, cr_F_M.Row2 + 7, blocks, vCRs[CH_F], crF3, crF2);
									iF4 = GetCR(CH_F, cr_F_M.Step2 - 5, cr_F_M.Row2 + 1, cr_F_M.Step2 + 5, cr_F_M.Row2 + 7, blocks, vCRs[CH_F], crF4, crF2);
								}
								else
								{
									iF3 = GetCR(CH_G, cr_F_M.Step1 - 5, cr_F_M.Row2 + 1, cr_F_M.Step1 + 5, cr_F_M.Row2 + 7, blocks, vCRs[CH_G], crF3, crG2);
									iF4 = GetCR(CH_G, cr_F_M.Step2 - 5, cr_F_M.Row2 + 1, cr_F_M.Step2 + 5, cr_F_M.Row2 + 7, blocks, vCRs[CH_G], crF4, crG2);
								}


								if (iF3)
								{
									Wound_Judged w;
									w.Block = cr.Block;
									FillWound(w, blockHead, head);
									w.Place = WP_WAIST;
									w.Degree = 4;
									w.Step = cr.Step;
									Connected_Region& cr_F_des = bFindF ? vCRs[CH_F][crF3[0]] : vCRs[CH_G][crF3[0]];
									int j_block = mBlockIndex[cr_F_des.Region[0].block];
									w.Walk = GetWD(blocks[j_block].BlockHead.walk, cr_F_des.Step1 - blocks[j_block].BlockHead.indexL, head.step);
									w.Type = WOUND_TYPE::W_SCREW_HORIZON_CRACK;
									memcpy(w.Result, "螺孔右侧水平裂纹", 30);

									SetJudgedFlag(vCRs[CH_F], crF3, 1);
									w.vCRs.push_back(cr);
									AddWoundData(w, vCRs[CH_F], crF3);
									AddWoundData(w, vCRs[CH_E], crEt);
									AddWoundData(w, vCRs[CH_F], crF);
									AddWoundData(w, vCRs[CH_F], crF2);
									AddWoundData(w, vCRs[CH_G], crG2);
									if (bFindF)
									{
										wound.SizeX = head.step * (vCRs[CH_F][crF3[0]].Step2 - vCRs[CH_F][crF3[0]].Step1);
										wound.SizeY = 1;
										sprintf_s(wound.According, "F出波");
									}
									else
									{
										wound.SizeX = head.step * (vCRs[CH_G][crF3[0]].Step2 - vCRs[CH_G][crF3[0]].Step1);
										wound.SizeY = 1;
										sprintf_s(wound.According, "G出波");
									}
									vWounds.push_back(w);
								}
								else if (iF4)
								{
									Wound_Judged w;
									w.Block = cr.Block;
									FillWound(w, blockHead, head);
									w.Place = WP_WAIST;
									w.Degree = 4;
									w.Step = cr.Step;
									Connected_Region& cr_F_des = bFindF ? vCRs[CH_F][crF4[0]] : vCRs[CH_G][crF4[0]];
									int j_block = mBlockIndex[cr_F_des.Region[0].block];
									w.Walk = GetWD(blocks[j_block].BlockHead.walk, cr_F_des.Step1 - blocks[j_block].BlockHead.indexL, head.step);
									memcpy(w.Result, "螺孔左侧水平裂纹", 30);
									w.Type = WOUND_TYPE::W_SCREW_HORIZON_CRACK;
									w.vCRs.push_back(cr);
									SetJudgedFlag(vCRs[CH_F], crF4, 1);
									AddWoundData(w, vCRs[CH_F], crF4);
									AddWoundData(w, vCRs[CH_E], crEt);
									AddWoundData(w, vCRs[CH_F], crF);
									AddWoundData(w, vCRs[CH_F], crF2);
									AddWoundData(w, vCRs[CH_G], crG2);
									if (bFindF)
									{
										wound.SizeX = head.step * (vCRs[CH_F][crF4[0]].Step2 - vCRs[CH_F][crF4[0]].Step1);
										wound.SizeY = 1;
										sprintf_s(wound.According, "F出波");
									}
									else
									{
										wound.SizeX = head.step * (vCRs[CH_G][crF4[0]].Step2 - vCRs[CH_G][crF4[0]].Step1);
										wound.SizeY = 1;
										sprintf_s(wound.According, "G出波");
									}
									vWounds.push_back(w);
								}
							}
						}
						else if (bGuideHole)//导孔
						{
							vector<int> crDt;
							uint8_t iFindD_4 = GetCR(CH_D, cr.Step1, cr.Row2, cr.Step2, cr.Row2 + 10, blocks, vCRs[CH_D], crDt, i);
							if (iFindD_4)//D通道检测到伤损
							{
								for (int k = 0; k < crDt.size(); ++k)
								{
									if (vCRs[CH_D][crDt[k]].Region.size() < 3)
									{
										continue;
									}

									int step11 = 0, step12 = 0;
									GetStepSteps(m, vCRs[CH_D][crDt[k]], step11, step12, blocks, angle, offset, head.step);
									if (step11 >= step1 && step12 <= step2)
									{
										SetJudgedFlag(vCRs[CH_D], crDt, 1); //螺孔双波——4象限
									}
									else
									{
										Wound_Judged w;
										w.Block = cr.Block;
										FillWound(w, blockHead, head);
										w.Step = cr.Step;
										w.Walk = GetWD(blocks[iBlock].BlockHead.walk, vCRs[CH_D][crDt[0]].Step1 - blocks[iBlock].BlockHead.indexL, head.step);
										w.Type = WOUND_TYPE::W_SCREW_CRACK4;
										w.Place = WP_WAIST;
										w.Degree = 4;
										memcpy(w.Result, "螺孔四象限斜裂纹", 30);
										SetJudgedFlag(vCRs[CH_D], crDt, 1);
										w.vCRs.push_back(cr);
										AddWoundData(w, vCRs[CH_F], crF);
										AddWoundData(w, vCRs[CH_F], crF2);
										AddWoundData(w, vCRs[CH_G], crG);
										AddWoundData(w, vCRs[CH_G], crG2);
										AddWoundData(w, vCRs[CH_E], crE);
										AddWoundData(w, vCRs[CH_D], crDt);
										vWounds.push_back(w);
									}
								}
							}

							crDt.clear();
							uint8_t iFindD_2 = GetCR(CH_D, cr.Step1 + 10, cr.Row1, cr.Step2 + 10, cr.Row2, blocks, vCRs[CH_D], crDt, i);
							for (int k = 0; k < crDt.size(); ++k)
							{
								Wound_Judged w;
								w.Block = cr.Block;
								FillWound(w, blockHead, head);
								w.Step = cr.Step;
								int idx_t = vCRs[CH_D][crDt[0]].Step1 - blocks[mBlockIndex[vCRs[CH_D][crDt[0]].Block]].BlockHead.indexL;
								w.Walk = GetWD(blocks[iBlock].BlockHead.walk, idx_t, head.step);
								w.Type = WOUND_TYPE::W_SCREW_CRACK2;
								memcpy(w.Result, "导孔二象限斜裂纹", 30);
								w.Place = WP_WAIST;
								w.Degree = 4;
								w.vCRs.push_back(cr);
								SetJudgedFlag(vCRs[CH_D], crDt, 1);
								AddWoundData(w, vCRs[CH_F], crF);
								AddWoundData(w, vCRs[CH_F], crF2);
								AddWoundData(w, vCRs[CH_G], crG);
								AddWoundData(w, vCRs[CH_G], crG2);
								AddWoundData(w, vCRs[CH_E], crE);
								AddWoundData(w, vCRs[CH_D], crDt);
								vWounds.push_back(w);
							}
						}
						else if (crInfo.Shift >= 10 && bWhole)
						{
							wound.Block = cr.Block;
							wound.Step = cr.Step;
							wound.Type = WOUND_TYPE::W_SKEW_CRACK;
							wound.Place = WP_WAIST;
							wound.Degree = 4;
							FillWound(wound, blockHead, head);
							memcpy(wound.Result, "斜裂纹,重伤", 30);
							wound.SizeX = head.step * (cr.Row2 - cr.Row1) / 0.6;
							wound.SizeY = 1;
							sprintf_s(wound.According, "E出波，位移：%.1f大格，幅值：%d", 0.02f * crInfo.Shift, crInfo.MaxV);
						}

						if (wound.Type != 0)
						{
							AddWoundData(wound, vCRs[CH_F], crF);
							AddWoundData(wound, vCRs[CH_F], crF2);
							AddWoundData(wound, vCRs[CH_G], crG);
							AddWoundData(wound, vCRs[CH_G], crG2);
							AddWoundData(wound, vCRs[CH_E], crE);
							vWounds.push_back(wound);
						}

						SetJudgedFlag(vCRs[CH_F], crF, 1);
						SetJudgedFlag(vCRs[CH_F], crF2, 1);
						AddWoundData(wound, vCRs[CH_G], crG);
						SetJudgedFlag(vCRs[CH_G], crG2, 1);
						SetJudgedFlag(vCRs[CH_E], crE, 1);
					}
				}
			}
			else if (m == CH_F || m == CH_G)//F 出波失波，不考虑G，除非F失耦而G不失耦
			{
				Wound_Judged wound;
				wound.Walk = wd;
				FillWound(wound, blockHead, head);
				wound.Block = cr.Block;
				wound.Step = cr.Step;

				WaveData t_wd = cr.Region[0];
				//轨鄂裂纹（最多） 在1.2­~1.5之间有半幅以上回波，底波波幅减弱或消失，在接头处有D或E做辅助校验；判重伤；报告：轨鄂水平裂纹。。mm 
				if (t_wd.row >= 1.2 *pixel && t_wd.row <= 1.5 *pixel)
				{
					bool bHalfWave = false;
					for (int p = 0; p < vASteps.size(); ++p)
					{
						for (int q = 0; q < vASteps[p].Frames.size(); ++q)
						{
							if (vASteps[p].Frames[q].Horizon >= 60 && vASteps[p].Frames[q].Horizon <= 75 && vASteps[p].Frames[q].F[CH_F] >= 75)
							{
								bHalfWave = true;
								break;
							}
						}
					}

					if (bHalfWave)
					{
						memcpy(wound.Result, "轨鄂水平裂纹", 20);
						wound.Type = W_JAW_HORIZON_CRACK;
						wound.Place = WP_JAW_IN | WP_JAW_OUT;
						wound.Degree = 4;
						wound.SizeX = head.step * (cr.Step2 - cr.Step1);
						wound.SizeY = 1;
						sprintf_s(wound.According, "%s出波，位移：%.1f大格，幅值：%d", ChannelNamesB[m], 0.02f * crInfo.Shift, crInfo.MaxV);
						wound.vCRs.push_back(cr);
						cr.Flag = 1;
					}
				}
				else if (t_wd.row < 1.2 * pixel)//轨头水平裂纹（其次）
				{
					bool bHalfWave = false;
					for (int p = 0; p < vASteps.size(); ++p)
					{
						for (int q = 0; q < vASteps[p].Frames.size(); ++q)
						{
							if (vASteps[p].Frames[q].Horizon >= 60 && vASteps[p].Frames[q].Horizon <= 75 && vASteps[p].Frames[q].F[CH_F] >= 75)
							{
								bHalfWave = true;
								break;
							}
						}
					}

					vector<int> cr1, cr2;
					bool bHaveDE = false;
					if (bHalfWave || bHaveDE)
					{
						wound.Type = W_VERTICAL_CRACK;
						wound.vCRs.push_back(cr);
						wound.Place = WP_HEAD_IN | WP_HEAD_OUT;
						wound.Degree = 4;
						wound.SizeX = head.step * (cr.Step2 - cr.Step1);
						wound.SizeY = 1;
						sprintf_s(wound.According, "%s出波，位移：%.1f大格，幅值：%d", ChannelNamesB[m], 0.02f * crInfo.Shift, crInfo.MaxV);
						cr.Flag = 1;
					}
				}
				//螺孔高度出波
				else if (t_wd.row >= 25 && t_wd.row <= 28)
				{
					bool bLose = false;
					bool bWound = false;
					vector<int> crD, crG, crE, crF;
					uint8_t bLoseF = GetCR(CH_F, cr.Step1, iFRow - 3, cr.Step2, iFRow + 3, blocks, vCRs[CH_F], crF);//F失波
					uint8_t bLoseG = GetCR(CH_G, cr.Step1, iFRow - 3, cr.Step2, iFRow + 3, blocks, vCRs[CH_G], crG);
					uint8_t bFindD = GetCR(CH_D, cr.Step1, cr.Row1, cr.Step2, cr.Row2, blocks, vCRs[CH_D], crD);
					uint8_t bFindE = GetCR(CH_E, cr.Step1 + 3, cr.Row1, cr.Step2 + 3, cr.Row2, blocks, vCRs[CH_E], crE);
					if (bLoseF + bLoseG > 0)
					{
						bLose = true;
					}

					if ((bFindD || bFindE) && bLose)
					{
						Position_Mark pm;
						memset(&pm, 0, sizeof(pm));
						pm.Block = cr.Block;
						pm.Step = cr.Step;
						pm.Mark = PM_SCREWHOLE;
					}

					wound.vCRs.push_back(cr);
					AddWoundData(wound, vCRs[CH_F], crF);
					AddWoundData(wound, vCRs[CH_G], crG);
					AddWoundData(wound, vCRs[CH_E], crE);
					AddWoundData(wound, vCRs[CH_D], crD);
					if (bWound)
					{
						vWounds.push_back(wound);
					}
				}
				//轨底失波
				else if (t_wd.find & BIT0 == 0)
				{
					bool bLose = false;
					vector<int> crD, crG, crE, crF;
					uint8_t bLoseF = GetCR(CH_F, cr.Step1, 50, cr.Step2, 60, blocks, vCRs[CH_F], crF);//F失波
					uint8_t bLoseG = GetCR(CH_G, cr.Step1, 50, cr.Step2, 60, blocks, vCRs[CH_G], crG);
					uint8_t bFindD = GetCR(CH_D, cr.Step1, cr.Row1, cr.Step2, cr.Row2, blocks, vCRs[CH_D], crD);
					uint8_t bFindE = GetCR(CH_E, cr.Step1 + 3, cr.Row1, cr.Step2 + 3, cr.Row2, blocks, vCRs[CH_E], crE);
					if (bLoseF + bLoseG > 0)
					{
						bLose = true;
					}

					if (bFindD && bFindE)
					{
						memcpy(wound.Result, "螺孔", 20);
					}
				}
			}
		}
	}
}

uint8_t	GetAChannelByBChannel(uint8_t iCh)
{
	int iChA = -1;
	if (iCh == CH_A1 || iCh == CH_A2)
	{
		iChA = ACH_A;
	}
	else if (iCh == CH_a1 || iCh == CH_a2)
	{
		iChA = ACH_a;
	}

	else if (iCh == CH_B1 || iCh == CH_B2)
	{
		iChA = ACH_B;
	}
	else if (iCh == CH_b1 || iCh == CH_b2)
	{
		iChA = ACH_b;
	}
	else if (iCh == CH_C)
	{
		iChA = ACH_C;
	}
	else if (iCh == CH_c)
	{
		iChA = ACH_c;
	}
	else if (iCh == CH_D)
	{
		iChA = ACH_D;
	}
	else if (iCh == CH_d)
	{
		iChA = ACH_d;
	}
	else if (iCh == CH_E)
	{
		iChA = ACH_E;
	}
	else if (iCh == CH_e)
	{
		iChA = ACH_e;
	}
	else if (iCh == CH_F)
	{
		iChA = ACH_F;
	}
	else if (iCh == CH_G)
	{
		iChA = ACH_G;
	}
	return iChA;
}

int		FindStepInAData(int32_t steps, BlockData_A& vAFrames, uint32_t block)
{
	int iStepIndex = -1;
	int low = 0;
	int mid = 0;
	int high = vAFrames.vAStepDatas.size() - 1;
	if (block > 0)
	{
		while (low <= high)
		{
			mid = (low + high) / 2;

			if (vAFrames.vAStepDatas[mid].Block < block)
				low = mid + 1;
			else if (vAFrames.vAStepDatas[mid].Block > block)
				high = mid - 1;
			else
			{
				if (vAFrames.vAStepDatas[mid].Step < steps)
				{
					while (vAFrames.vAStepDatas[mid].Block <= block + 1 && mid < vAFrames.vAStepDatas.size() - 1)
					{
						if (vAFrames.vAStepDatas[mid].Step == steps)
						{
							return mid;
						}
						if (vAFrames.vAStepDatas[mid].Step > steps)
						{
							return -1;
						}
						++mid;
					}
				}
				else if (vAFrames.vAStepDatas[mid].Step >= steps)
				{
					bool bFind = false;
					while (mid >= 0 && vAFrames.vAStepDatas[mid].Block >= block - 1)
					{
						A_Step& b_step = vAFrames.vAStepDatas[mid];
						if (vAFrames.vAStepDatas[mid].Step == steps)
						{
							return mid;
						}
						if (vAFrames.vAStepDatas[mid].Step < steps)
						{
							return -1;
						}
						--mid;
					}
				}
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < vAFrames.vAStepDatas.size(); ++i)
		{
			if (vAFrames.vAStepDatas[i].Block > block)
			{
				return -1;
			}
			if (vAFrames.vAStepDatas[i].Step == steps)
			{
				return i;
			}
		}
	}
	return -1;
}

bool	GetStepSteps(uint8_t channel, Connected_Region& cr, int32_t& step1, int32_t& step2, vector<BlockData_B>& vBBlocks, double angle, int offset, double stepDistance)
{
	int nChA = GetAChannelByBChannel(channel);
	int step = cr.Region[0].step;

	int32_t iBeginStep = 0x7FFFFFFF, iEndStep = 0 - iBeginStep;
	Connected_Region cr_temp = cr;

	//double rad = acos(cos(angle * AngleToRad) * cos(angle2 * AngleToRad));
	double rad = angle * AngleToRad;
	bool bFind = false;

	for (int i = 0; i < cr_temp.Region.size(); ++i)
	{
		WaveData& wd = cr_temp.Region[i];
		double dStep = (1.0 * offset + 3.0 * wd.row * tan(rad)) / stepDistance;
		int iRealStep = wd.step - dStep;
		if (iRealStep < iBeginStep)
		{
			iBeginStep = iRealStep;
		}
		if (iRealStep > iEndStep)
		{
			iEndStep = iRealStep;
		}
	}

	step1 = iBeginStep;
	step2 = iEndStep;
	return true;
}

bool	GetStepAStepFrames(uint8_t channel, Connected_Region& cr, vector<A_Step>& vSteps, BlockData_A& DataA, vector<BlockData_B>& DataB, double angle, int offset, double stepDistance)
{
	int nChA = GetAChannelByBChannel(channel);
	int step = cr.Region[0].step;
	int32_t iBeginStep = 0x7FFFFFFF, iEndStep = 0 - iBeginStep;
	int iBeginStepIndex = 0x7FFFFFFF, iEndStepIndex = 0 - iBeginStepIndex;

	Connected_Region cr_temp = cr;
	if (channel == CH_A2 || channel == CH_B2 || channel == CH_a2 || channel == CH_b2)
	{
		for (int i = 0; i < cr_temp.Region.size(); ++i)
		{
			cr_temp.Region[i].row = 26 - cr_temp.Region[i].row;
		}
		FillCR(cr_temp);
	}

	double rad = angle * AngleToRad;
	bool bFind = false;

	int iBeginBlock = cr.Region[0].block;
	int iEndBlock = cr.Region[cr.Region.size() - 1].block;

	//A中的步进数应小于B中的步进数
	for (int i = 0; i < cr_temp.Region.size(); ++i)
	{
		WaveData& wd = cr_temp.Region[i];
		double dStep = (1.0 * offset + 3.0 * wd.row * tan(rad)) / stepDistance;
		//if (channel < CH_C && wd.row > 13)
		//{
		//	dStep = (1.0 * offset + 3.0 * 13 * tan(rad) + 1.0 * offset + 3.0 * (wd.row - 13) * 2.0068767) / stepDistance;
		//}
		int iDestBlock, iDestStep;
		if (!GetStepOffset(mBlockIndex[wd.block], wd.step, -dStep, DataB, &iDestBlock, &iDestStep))
		{
			continue;
		}

		bFind = true;
		int iRealStep = DataB[iDestBlock].vBStepDatas[iDestStep].Step;
		if (iRealStep < iBeginStep)
		{
			iBeginStep = iRealStep;
			iBeginBlock = iDestBlock;
		}
		if (iRealStep > iEndStep)
		{
			iEndStep = iRealStep;
			iEndBlock = iDestBlock;
		}
	}

	if (bFind == false)
	{
		return false;
	}

	int iFlag = iBeginStep;

	while (true)
	{
		iBeginStepIndex = FindStepInAData(iBeginStep, DataA, DataB[iBeginBlock].Index);
		if (iBeginStepIndex >= 0)
		{
			break;
		}
		--iBeginStep;
		if (iFlag - iBeginStep > 9)
		{
			return false;
		}
	}

	iFlag = iEndStep;
	while (true)
	{
		iEndStepIndex = FindStepInAData(iEndStep, DataA, DataB[iEndBlock].Index);
		if (iEndStepIndex >= 0)
		{
			break;
		}
		++iEndStep;
		if (iEndStep - iFlag > 10)
		{
			return false;
		}
	}

	int iH1 = (350.0 / 58.0) * cr_temp.Row1 / cos(rad);
	int iH2 = (350.0 / 58.0) * cr_temp.Row2 / cos(rad);

	int nTotalFrameCount = DataA.vAStepDatas.size() - 1;
	int iRecordCount = 0;
	if (channel < CH_C)
	{
		// true :向前，false：向后
		bool tDirection = (cr.Channel % 4) == 0;
		int iH11 = iH1, iH22 = iH2;
		if (cr_temp.Row1 > 13)
		{
			iH11 = (350.0 / 58.0) * 13 / cos(rad) + (350.0 / 58.0) * (cr_temp.Row1 - 13) / cos(rad);
		}
		iH22 = iH2 + 150;

		iBeginStepIndex -= 1;
		if (iBeginStepIndex < 0)
		{
			iBeginStepIndex = 0;
		}
		iEndStepIndex += 1;
		if (iEndStepIndex == DataA.vAStepDatas.size())
		{
			iEndStepIndex = DataA.vAStepDatas.size() - 1;
		}

		for (int i = iBeginStepIndex; i <= iEndStepIndex; ++i)
		{
			A_Step step;
			step.Index = DataA.vAStepDatas[i].Index;
			step.Index2 = DataA.vAStepDatas[i].Index2;
			step.Block = DataA.vAStepDatas[i].Block;
			step.Step = DataA.vAStepDatas[i].Step;
			int nFramesCount = DataA.vAStepDatas[i].Frames.size();
			int t_min = 512, t_max = 0;
			for (int j = 0; j < nFramesCount; ++j)
			{
				A_Frame &frame = DataA.vAStepDatas[i].Frames[j];
				if (!frame.Used && frame.Horizon >= iH11 - 4 && frame.Horizon <= iH2 + 4 && frame.F[nChA] > 0)
				{
					frame.Used = true;
					step.Frames.push_back(frame);
				}
			}
			if (step.Frames.size() > 0)
			{
				vSteps.push_back(step);
			}
		}
		if (vSteps.size() > 0)
		{
			int iHS1 = vSteps[0].Frames[0].Horizon;
			int iHE1 = vSteps[0].Frames[vSteps[0].Frames.size() - 1].Horizon;
			int iFirstFrame = vSteps[0].Index2;
			if (tDirection)
			{
				for (int i = iBeginStepIndex - 1; i >= 0; --i)
				{
					A_Step step;
					step.Index = DataA.vAStepDatas[i].Index;
					step.Index2 = DataA.vAStepDatas[i].Index2;
					step.Block = DataA.vAStepDatas[i].Block;
					step.Step = DataA.vAStepDatas[i].Step;
					int nFramesCount = DataA.vAStepDatas[i].Frames.size();
					for (int j = 0; j < nFramesCount; ++j)
					{
						A_Frame &frame = DataA.vAStepDatas[i].Frames[j];
						if (!frame.Used && frame.Horizon < iHS1 && frame.F[nChA] > 0)
						{
							frame.Used = true;
							step.Frames.push_back(frame);
						}
						else if (frame.Horizon > iHS1)
						{
							continue;
						}
					}
					if (step.Frames.size() > 0 && iFirstFrame - step.Index2 == 1)
					{
						vSteps.push_back(step);
						iFirstFrame = step.Index2;
						iHS1 = step.Frames[0].Horizon;
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				for (int i = iBeginStepIndex - 1; i >= 0; --i)
				{
					A_Step step;
					step.Index = DataA.vAStepDatas[i].Index;
					step.Index2 = DataA.vAStepDatas[i].Index2;
					step.Block = DataA.vAStepDatas[i].Block;
					step.Step = DataA.vAStepDatas[i].Step;
					int nFramesCount = DataA.vAStepDatas[i].Frames.size();
					for (int j = 0; j < nFramesCount; ++j)
					{
						A_Frame &frame = DataA.vAStepDatas[i].Frames[j];
						if (!frame.Used && frame.Horizon > iHE1 && frame.F[nChA] > 0)
						{
							frame.Used = true;
							step.Frames.push_back(frame);
						}
						else if (frame.Horizon < iHS1)
						{
							continue;
						}
					}
					if (step.Frames.size() > 0 && iFirstFrame - step.Index2 == 1)
					{
						vSteps.push_back(step);
						iFirstFrame = step.Index2;
						iHE1 = step.Frames[step.Frames.size() - 1].Horizon;
					}
					else
					{
						break;
					}
				}
			}

			int iHS2 = vSteps[vSteps.size() - 1].Frames[0].Horizon;
			int iHE2 = vSteps[vSteps.size() - 1].Frames[vSteps[vSteps.size() - 1].Frames.size() - 1].Horizon;
			int iLastFrame = vSteps[vSteps.size() - 1].Index2;
			if (tDirection)
			{
				for (int i = iEndStepIndex + 1; i < DataA.vAStepDatas.size(); ++i)
				{
					A_Step step;
					step.Index = DataA.vAStepDatas[i].Index;
					step.Index2 = DataA.vAStepDatas[i].Index2;
					step.Block = DataA.vAStepDatas[i].Block;
					step.Step = DataA.vAStepDatas[i].Step;
					int nFramesCount = DataA.vAStepDatas[i].Frames.size();
					for (int j = 0; j < nFramesCount; ++j)
					{
						A_Frame &frame = DataA.vAStepDatas[i].Frames[j];
						if (!frame.Used && frame.Horizon < iHS2 && frame.F[nChA] > 0)
						{
							frame.Used = true;
							step.Frames.push_back(frame);
						}
						else if (frame.Horizon < iHS2)
						{
							continue;
						}
					}
					if (step.Frames.size() > 0 && step.Index2 - iLastFrame == 1)
					{
						vSteps.push_back(step);
						iLastFrame = step.Index2;
						iHS2 = step.Frames[0].Horizon;
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				for (int i = iEndStepIndex + 1; i < DataA.vAStepDatas.size(); ++i)
				{
					A_Step step;
					step.Index = DataA.vAStepDatas[i].Index;
					step.Index2 = DataA.vAStepDatas[i].Index2;
					step.Block = DataA.vAStepDatas[i].Block;
					step.Step = DataA.vAStepDatas[i].Step;
					int nFramesCount = DataA.vAStepDatas[i].Frames.size();
					for (int j = 0; j < nFramesCount; ++j)
					{
						A_Frame &frame = DataA.vAStepDatas[i].Frames[j];
						if (!frame.Used && frame.Horizon > iHE2 && frame.F[nChA] > 0)
						{
							frame.Used = true;
							step.Frames.push_back(frame);
						}
						else if (frame.Horizon > iHS2)
						{
							continue;
						}
					}
					if (step.Frames.size() > 0 && iLastFrame - step.Index2 == 1)
					{
						vSteps.push_back(step);
						iLastFrame = step.Index2;
						iHE2 = step.Frames[step.Frames.size() - 1].Horizon;
					}
					else
					{
						break;
					}
				}
			}
		}
	}
	else if (channel >= CH_C && channel < CH_F)
	{
		for (int i = iBeginStepIndex; i <= iEndStepIndex; ++i)
		{
			A_Step step;
			step.Index = DataA.vAStepDatas[i].Index;
			step.Index2 = DataA.vAStepDatas[i].Index2;
			step.Block = DataA.vAStepDatas[i].Block;
			step.Step = DataA.vAStepDatas[i].Step;
			int nFramesCount = DataA.vAStepDatas[i].Frames.size();
			for (int j = 0; j < nFramesCount; ++j)
			{
				A_Frame &frame = DataA.vAStepDatas[i].Frames[j];
				if (frame.Horizon >= iH1 - 4 && frame.Horizon <= iH2 + 4 && frame.F[nChA] > 0)
				{
					step.Frames.push_back(frame);
				}
			}
			if (step.Frames.size() > 0)
			{
				vSteps.push_back(step);
			}
		}
	}
	else if (channel == CH_F || channel == CH_G)
	{
		for (int i = iBeginStepIndex; i <= iEndStepIndex; ++i)
		{
			A_Step step;
			step.Index2 = DataA.vAStepDatas[i].Index2;
			step.Block = DataA.vAStepDatas[i].Block;
			step.Step = DataA.vAStepDatas[i].Step;
			int nFramesCount = DataA.vAStepDatas[i].Frames.size();
			for (int j = 0; j < nFramesCount; ++j)
			{
				A_Frame &frame = DataA.vAStepDatas[i].Frames[j];
				if ((cr.Region[0].find & BIT0 == BIT0) && frame.F[nChA] < 150 && frame.Horizon >= iH1 - 7 && frame.Horizon <= iH2 + 7)
				{
					step.Frames.push_back(frame);
					++iRecordCount;
				}
				else if ((cr.Region[0].find & BIT0 != BIT0) && frame.F[nChA] > 0 && frame.Horizon >= iH1 - 7 && frame.Horizon <= iH2 + 7)
				{
					step.Frames.push_back(frame);
					++iRecordCount;
				}
			}
			if (step.Frames.size() > 0)
			{
				vSteps.push_back(step);
			}
		}
	}
	return vSteps.size() > 0;
}

uint8_t GetCR(uint8_t channel, int step1, int step2, vector<BlockData_B>& vBDatas, vector<Connected_Region>& crToFind, vector<int>& vCrFound, int32_t iExcept, int iMinimunSize)
{
	for (int i = 0; i < crToFind.size(); ++i)
	{
		if (crToFind[i].Step1 > step2)
		{
			break;;
		}
		if (crToFind[i].Step2 < step1 || crToFind[i].Region.size() < iMinimunSize)
		{
			continue;
		}
		if (iExcept != i)
		{
			vCrFound.push_back(i);
		}
	}
	return vCrFound.size() > 0 ? 1 : 0;
}

uint8_t GetCR(uint8_t channel, int step1, uint8_t row1, int step2, uint8_t row2, vector<BlockData_B>& vBDatas, vector<Connected_Region>& crToFind, vector<int>& vCrFound, int32_t iExcept, int iMinimunSize)
{
	for (int i = 0; i < crToFind.size(); ++i)
	{
		Connected_Region& cr = crToFind[i];
		if (cr.Step1 > step2)
		{
			break;;
		}

		int mi = min(cr.Row1, cr.Row2);
		int ma = max(cr.Row1, cr.Row2);
		if (cr.Step2 < step1 || row1 > ma || row2 <= mi)
		{
			continue;
		}
		if (cr.Step2 < step1 || cr.Region.size() < iMinimunSize)
		{
			continue;
		}
		if (iExcept != i)
		{
			vCrFound.push_back(i);
		}
	}
	return vCrFound.size() > 0 ? 1 : 0;
}

uint8_t GetCR(uint8_t channel, int step1, uint8_t row1, int step2, uint8_t row2, vector<BlockData_B>& vBDatas, vector<Connected_Region>& crToFind, vector<int>& vCrFound, vector<int> vExcept, int iMinimunSize/* = 1*/)
{
	GetCR(channel, step1, row1, step2, row2, vBDatas, crToFind, vCrFound, -1, iMinimunSize);
	Exclude(vCrFound, vExcept);
	return vCrFound.size()  > 0 ? 1 : 0;
}


double GetWD(W_D pos)
{
	return pos.Km + 0.001 * pos.m + 0.000001 * pos.mm;
}

double GetWD(W_D pos, int nStep, float stepDistance, bool direction)
{
	double wd = pos.Km * 1000 + pos.m + 0.001 * pos.mm;
	double r = 0.001 * nStep * stepDistance;
	if (direction)
	{
		return 0.001 * (wd + r);
	}
	else
	{
		return 0.001 * (wd - r);
	}
}

bool FindNextJoint(int block, int step, vector<BlockData_B>& blocks, int* destBlock, int * destStep)
{
	int istep = step;
	bool bFind = false;
	for (int j = 0; j < blocks[block].vBStepDatas.size(); ++j)
	{
		if (blocks[block].vBStepDatas[j].Step >= step && blocks[block].vBStepDatas[j].Mark.Mark & BIT4)
		{
			bFind = true;
			*destBlock = block;
			*destStep = j;
			return true;
		}

		if (j == blocks[block].vBStepDatas.size() - 1)
		{
			++block;
			j = 0;
		}
	}
	return false;
}

bool FindNextWeld(int block, int step, vector<BlockData_B>& blocks, int* destBlock, int * destStep)
{
	int istep = step;
	bool bFind = false;
	for (int j = 0; j < blocks[block].vBStepDatas.size(); ++j)
	{
		if (blocks[block].vBStepDatas[j].Step >= step && blocks[block].vBStepDatas[j].Mark.Mark & BIT4)
		{
			bFind = true;
			*destBlock = block;
			*destStep = j;
			return true;
		}

		if (j == blocks[block].vBStepDatas.size() - 1)
		{
			++block;
			j = 0;
		}
	}
	return false;
}

void  SetJudgedFlag(vector<Connected_Region>& vCR, vector<int>& idx, int Flag)
{
	for (int i = 0; i < idx.size(); ++i)
	{
		vCR[idx[i]].Flag = Flag;
	}
}

void  AddWoundData(Wound_Judged& wound, vector<Connected_Region>& vCR, vector<int>& idx)
{
	for (int i = 0; i < idx.size(); ++i)
	{
		wound.vCRs.push_back(vCR[idx[i]]);
	}
}


void	AnalyseWithHistory(vector<Wound_Judged>& vWounds, vector<Wound_Judged> &vHWounds, vector<Position_Mark> &vMarks)
{

}


void	CombineOutputData(vector<Position_Mark>& vPMs, vector<Wound_Judged>& vWounds, vector<BLOCK>& blockheads)
{
	//bool direction = true;//顺逆里程
	double *dist = new double[vPMs.size()];
	vector<Position_Mark> vBack;
	for (int i = vPMs.size() - 1; i >= 1; --i)
	{
		if (vPMs[i].Mark == PM_SEW_LRH)
		{
			for (int j = i - 1; j >= 0; --j)
			{
				double dw = vPMs[i].Walk - vPMs[j].Walk;
				if (dw >= -0.006 && dw <= 0.006)
				{
					if (vPMs[j].Mark == PM_SEW_LRH)
					{
						vPMs[j].Flag = 1;
					}
				}
				else
				{
					break;
				}
			}
		}
	}

	vector<Position_Mark> vPM2;
	for (int i = 0; i < vPMs.size(); ++i)
	{
		if (vPMs[i].Flag == 0)
		{
			vPM2.push_back(vPMs[i]);
		}
	}
	vPMs = vPM2;

	if (g_direction)//顺里程
	{
		for (int i = 0; i < vWounds.size(); ++i)
		{
			for (int j = 0; j < vTunnels.size(); ++j)
			{
				if (vWounds[i].Walk >= vTunnels[j].Start && vWounds[i].Walk >= vTunnels[j].End)
				{
					vWounds[i].TunnelID = vTunnels[i].id;
				}
				if (vTunnels[j].Start > vWounds[i].Walk)
				{
					break;
				}
			}

			for (int j = 0; j < vBridges.size(); ++j)
			{
				if (vWounds[i].Walk >= vBridges[j].Start && vWounds[i].Walk >= vBridges[j].End)
				{
					vWounds[i].TunnelID = vBridges[i].id;
				}
				if (vBridges[j].Start > vWounds[i].Walk)
				{
					break;
				}
			}

			for (int j = 0; j < vCurves.size(); ++j)
			{
				if (vWounds[i].Walk >= vCurves[j].Start && vWounds[i].Walk >= vCurves[j].End)
				{
					vWounds[i].TunnelID = vCurves[i].id;
				}
				if (vCurves[j].Start > vWounds[i].Walk)
				{
					break;
				}
			}
		}
	}
	else//逆里程
	{

	}
}


void	Correlate(int* current, int* index, int count, int* ret)
{
	for (int i = 0; i < count; ++i)
	{
		ret[i] = 0;
		for (int j = 0; j < count - 1; j++)
		{
			if (i - j >= 0)
			{
				ret[i] += current[j] * index[i - j];
			}
			else
			{
				ret[i] += current[j] * index[count - 1 - i + j];
			}
		}
	}
}

void	Exclude(vector<int>& cr1, vector<int>& cr2)
{
	uint8_t* bEexist = new uint8_t[cr1.size()];
	for (int i = 0; i < cr1.size(); ++i)
	{
		bEexist[i] = 0;
		for (int j = 0; j < cr2.size(); ++j)
		{
			if (cr1[i] == cr2[j])
			{
				bEexist[i] = 1;
				break;
			}
		}
	}

	vector<int> ret;
	for (int i = 0; i < cr1.size(); ++i)
	{
		if (bEexist[i] == 0)
		{
			ret.push_back(cr1[i]);
		}
	}

	cr1 = ret;
	delete bEexist;
}

void	FillWound(Wound_Judged& wd, BLOCK& blockHead, F_HEAD& fhead)
{
	//memcpy(wd.RailNo, (char*)(LPCTSTR)g_strRailNo, g_strRailNo.GetLength());
	wd.LeftRight = (blockHead.railType >> 5) & 0x01;
	wd.XingType = blockHead.railType >> 6;
	wd.railType = blockHead.railType & 0x03;
	wd.railH = blockHead.railH;
	wd.Xianbie = fhead.deviceP2.TrackSet.lineType;
	ParseGPS(blockHead.gpsInfor, wd.gps_log, wd.gps_lat);

	sprintf_s(wd.FoundTime, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
	time_t timec;
	time(&timec);
	tm* p = localtime(&timec);
	sprintf_s(wd.ana_time, "%04d-%02d-%02d %02d:%02d:%02d", p->tm_year + 1900, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
}
