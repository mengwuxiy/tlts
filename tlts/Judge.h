#ifndef JUDGE_H
#define JUDGE_H

#include "SolveData.h"


//B超通道定义
#define CH_A1	0
#define CH_A2	1
#define CH_a1	2
#define CH_a2	3
#define CH_B1	4
#define CH_B2	5
#define CH_b1	6
#define CH_b2	7
#define CH_C	8
#define CH_c	9
#define CH_D	10
#define CH_d	11
#define CH_E	12
#define CH_e	13
#define CH_F	14
#define CH_G	15

//A中通道定义
#define ACH_A	0
#define ACH_a	1
#define ACH_B	2
#define ACH_b	3
#define ACH_C	4
#define ACH_D	5
#define ACH_d	6
#define ACH_F	7
#define ACH_c	8
#define ACH_e	9
#define ACH_E	10
#define ACH_G	11


#define N_BLOCKREAD	25

//-----------------------------------------------
// 计算二次波的三角函数
//-----------------------------------------------
typedef struct _TRIANGLE
{
	float sin;
	float cos;
	float tan;
}TRIANGLE;


//-----------------------------------------------
// 波形中探头覆盖范围的数据结构
//-----------------------------------------------
struct PROBE_COVERAGE_AREA
{
	int     Direction;                      // 探头朝向：-1 - 向前；1 - 向后；0 - 垂直向下
	int     Range1;                    // 第一段范围（步进）
	int     Range2;                    // 第二段范围（步进）
};


//B超每步进数据
typedef struct _B_Step2
{
	int			Step;		//总的步进数	
	uint16_t	Draw1[66];	//行数据
}B_Step2;


//B超出失波记录
typedef struct _WaveData
{
	int			block;	//米块索引
	int			step;	//步进索引
	uint8_t		row;	//行	
	uint8_t		find;	// BIT0：0:失波，1：出波，BIT7：0：未处理，1：已处理
}WaveData;

//连通域
typedef struct _Connected_Region
{
	uint32_t			Block;	//米块索引，测试使用
	int32_t				Step;	//米块内步进
	uint8_t				Row1;	//最小行
	int32_t				Step1;	//最小步进
	uint8_t				Row2;
	int32_t				Step2;
	uint8_t				Channel;
	uint8_t				Flag;	//是否判断过了
	vector<WaveData>	Region;
	vector<A_Step>		vASteps;//对应的A超数据
	uint16_t			Index;	//连通域标识
}Connected_Region, CR;

typedef  vector<Connected_Region> VCR;

typedef struct _CR_INFO
{
	uint16_t	MinH;//出波的最小横坐标，A超512
	uint16_t	MaxH;//出波的最大横坐标，A超512
	uint16_t	MaxV;//最小赋值 或 （最大幅值，F,G检测失波时才使用该意义）
	int16_t	Shift;//位移
}CR_INFO;


struct BLOCK_SQL
{
	uint64_t	id;
	BLOCK		Head;
	uint64_t	FileID;
	uint32_t	Index;
	char		Gain[128];
	uint16_t	speed;
	char		date[128];
	double		walk;
	char		Place[128];
	uint16_t	rail_height;
	int			Step;
	int			Step2;
	int			rail_type;
	double		gps_long;
	double		gps_lat;
	int			ZX;//股道站线编号
	uint32_t	StartB;
	uint32_t	StartA;
};


//导出伤损数据
class Wound_Judged
{
public:
	Wound_Judged();
	char						Result[30];	// 伤损
	char						WoundNo[64];

	double						Walk;		// 里程
	char						FoundTime[20]; //分析员校验完成时间
	uint16_t					Place;	//伤损在截面位置
	uint16_t					Type;	//种类
	uint16_t					Degree;//程度
	uint8_t						SizeX;
	uint8_t						SizeY;
	char						ana_time[20];


	uint8_t						Checked;	//复核状态	
	char						Analyser[128];
	char						CheckTime[20]; //分析员校验完成时间
	double						Walk2;		// 里程
	uint16_t					Place2;	//伤损在截面位置
	uint16_t					Type2;	//种类
	uint16_t					Degree2;//程度
	uint8_t						SizeX2;
	uint8_t						SizeY2;
	char						Desc[128];

	char						RailNo[128];// 线编号4个BCD
	uint8_t						LeftRight;	// 左右股
	uint8_t						XingType;	// 行别：单线、上行、下行，其他预留
	uint8_t						Xianbie;	// 线别
	uint8_t						railType;	// BIT0~1: 轨型(0为43轨，1-50，2-60，3-75)
	uint8_t						railH;		// 当前轨高mm

	double						gps_log;
	double						gps_lat;

	uint64_t					BridgeID;
	uint64_t					TunnelID;	//隧道ID
	uint64_t					CurveID;	//曲线ID
	uint8_t						IsJoint;	//接头ID
	uint8_t						IsSew;		//焊缝ID
	uint8_t						IsScrewHole;//螺孔ID
	uint8_t						IsGuideHole;//导孔ID

	uint8_t						Cycle;		//当前周期数
	uint64_t					LastCycleID;//上周期伤损ID

	vector<Connected_Region>	vCRs;		//伤损出波/失波数据
	vector<A_Frame>				vFrames;	//A超数据

	uint8_t						Flag;

	uint8_t						Manual;		//是否人工判伤，需要确定伤损类型
	char						According[256];//判据

	int32_t						Block;
	int32_t						Step;		//米块内步进
	int32_t						Step2;		//总步进
	uint64_t					FileID;		//作业ID
	uint64_t					id;			//
	char						gwdNo[10];		//工务段ID
};
//enum WOUND_TYPE
//{
//	W_OTHER = 0,
//
//	W_CHHF = 1,
//	W_LRHF = 2,
//	W_HEAD_HS = 3,
//
//	W_HEAD_HS_IN = W_HEAD_HS,	//轨头内侧核伤
//	W_HEAD_HS_MID = W_HEAD_HS,	//轨头中心核伤
//	W_HEAD_HS_OUT = W_HEAD_HS,	//轨头外侧核伤
//
//	W_YLS = 4,					//鱼鳞伤
//	W_SCREW_CRACK = 5,				//螺孔斜裂纹
//	W_SCREW_CRACK1 = W_SCREW_CRACK,
//	W_SCREW_CRACK2 = W_SCREW_CRACK,
//	W_SCREW_CRACK3 = W_SCREW_CRACK,
//	W_SCREW_CRACK4 = W_SCREW_CRACK,
//	W_SCREW_HORIZON_CRACK = 6,		//螺孔水平裂纹
//
//	W_JAW_HORIZON_CRACK = 7,		//轨鄂水平裂纹
//	W_WAIST_HORIZON_CRACK = W_JAW_HORIZON_CRACK,		//轨腰水平裂纹
//
//	W_SKEW_CRACK = 8,				//斜裂纹
//	W_BOTTOM_TRANSVERSE_CRACK = 9	//轨底横向裂纹（半圆形，竖直截面内）
//};
enum WOUND_TYPE
{
	W_OTHER = 0,				//其他，非伤
	W_HEAD_HS = 1,				//核伤

	W_HEAD_HS_IN = W_HEAD_HS,	//轨头内侧核伤
	W_HEAD_HS_MID = W_HEAD_HS,	//轨头中心核伤
	W_HEAD_HS_OUT = W_HEAD_HS,	//轨头外侧核伤

	W_YLS = 2,					//鱼鳞伤
	W_SCREW_CRACK = 16,				//螺孔斜裂纹
	W_SCREW_CRACK1 = 17,
	W_SCREW_CRACK2 = 18,
	W_SCREW_CRACK3 = 19,
	W_SCREW_CRACK4 = 20,
	W_SCREW_HORIZON_CRACK = 32,		//螺孔水平裂纹

	W_JAW_HORIZON_CRACK = 4,		//轨鄂水平裂纹
	W_WAIST_HORIZON_CRACK = 8,		//轨腰水平裂纹

	W_VERTICAL_CRACK = 64,			//纵向水平裂纹

	W_SKEW_CRACK = 128,				//斜裂纹
	W_BOTTOM_TRANSVERSE_CRACK = 256	//轨底横向裂纹（半圆形，竖直截面内）
};

enum WOUND_DEGREE
{
	WD_OK = 0,
	WD_LESS = 1,
	WD_SMALL = 2,
	WD_MEDIUM = 3,
	WD_SERIOUS = 4,
	WD_BREAK = 5,
	WD_NORMAL = 100
};

enum WOUND_POSITION
{
	WP_TM = BIT0,		//轨头踏面中
	WP_HEAD_IN = BIT1,	//轨头内侧
	WP_HEAD_MID = BIT2, //轨头中侧
	WP_HEAD_OUT = BIT3, //轨头外侧
	WP_ANGLE = BIT4,	//轨距角
	WP_JAW_IN = BIT5,	//轨鄂内侧
	WP_JAW_OUT = BIT6,	//轨鄂外侧
	WP_WAIST = BIT7,	//轨腰
	WP_BOTTOM = BIT8,	//轨底
	WP_BOTTOM_ANGLE_IN = BIT9,	//轨底
	WP_BOTTOM_ANGLE_OUT = BIT10	//轨底
};


class Position_Mark
{
public:
	Position_Mark();
	double					Walk;	//里程
	uint16_t				Mark;	//位置标记类型（算法判定）
	uint16_t				Mark2;	//位置标记类型（人工判定）
	uint16_t				Data;	//数据，可有可无
	uint32_t				Block;	//米块
	uint16_t				Step;	//米块内步进

	uint8_t					ARow;	//平均行
	int32_t					AStep;	//平均步进
	uint8_t					ChannelNum;
	uint16_t				Num[16];
	double					Fangcha;
	uint16_t				Size;

	double					gps_log;
	double					gps_lat;
	//线编号
	char					railNo[128];
	uint8_t					xingbie; //行别
	uint8_t					xianbie; //线别	
	uint8_t					Gubie;	//股别

	uint8_t					Length;
	uint8_t					Height;
	uint8_t					Flag;
	uint16_t				Data2;//左导， 左螺，右螺，右导
	uint64_t				id;
	uint8_t					Manual;//是否人工标记
};

enum POSITION_MARK
{
	PM_SEW_CH = 1,		//厂焊
	PM_SEW_LRH = 2,		//铝热焊缝

	PM_JOINT = 3,		//端面

	PM_SCREWHOLE = 4,	//螺孔
	PM_GUIDEHOLE = 5,	//导孔

	PM_OTHER = 6,		//其他

	PM_FORK = 7,		//道岔Y

	PM_SMART1 = 8,		//尖轨变高
	PM_SMART2 = 9,		//尖轨变低	

	PM_BACK = 10,		//回退

	PM_CHECK_BEGIN = 11,//双机复核起点
	PM_CHECK_END = 12,	//双机复核终点

	PM_START = 13 //上道
};

typedef struct _TPoint
{
	int32_t		Step;
	uint8_t		Row;
	uint8_t		Channel;
}TPoint;


typedef struct _Curve
{
	uint64_t	id;
	char		No[20];
	uint8_t		Direction;//曲线方向
	double		Start;
	double		End;
	double		Length;
	double		R;
	double		OverH;	//超高
	double		HC;		//缓长
	char		railNo[30];
	uint8_t		Xingbie;
	uint64_t	gwdNo;
	uint64_t	bureauNo;//局
}Curve;

typedef struct _Bridge
{
	uint64_t	id;
	char		No[20];
	char		Name[20];
	double		Start;
	double		End;
	char		railNo[30];
	uint8_t		Xingbie;
	uint64_t	gwdNo;
	uint64_t	bureauNo;//局
}Bridge;

typedef struct _Tunnel
{
	uint64_t	id;
	char		No[20];
	char		Name[20];
	double		Start;
	double		End;
	char		railNo[30];
	uint8_t		Xingbie;
	char		SupportLineNo[20]; //辅助线编号
	uint64_t	gwdNo;
	uint64_t	bureauNo;//局
}Tunnel;


bool OpenServer(CString strCfg);

bool CloseServer();

bool AddWork(char* strFile, double& gps_log, double& gps_lat);

bool AddWounds(vector<Wound_Judged>& wounds);

bool AddPMs(vector<Position_Mark>& vPMs);

bool AddBlocks(vector<BLOCK>& blocks, vector<Read_Info>& vA, vector<Read_Info>& vB);

void FinishFile();

//获取F出波的行
int16_t GetFRow(B_Step& step);

void FillCR(Connected_Region& cr);

//计算两个连通域的距离，在融合联通域时使用
uint32_t		GetDistance(Connected_Region& cr1, Connected_Region& cr2);

//根据B中的通道，寻找A中通道
uint8_t	GetAChannelByBChannel(uint8_t bChannel);

void	FillMarks(vector<BlockData_B>& blocks, vector<Position_Mark>& vPMs, F_HEAD& head);

bool	IsSew(F_HEAD& head, vector<BlockData_B>& blocks, vector<Connected_Region>* vCRs, Connected_Region& cr, int crIndex, vector<Wound_Judged>& vWounds, vector<Position_Mark>& vPMs);

void	AnalyseCR(vector<Connected_Region>& crs, vector<int>& vCRIndex, uint16_t& sum1, uint16_t &sum2, int32_t& iTotalStep1, int32_t& iTotalRow1, int32_t& iTotalStep2, int32_t& iTotalRow2);

bool	GetCRInfo(Connected_Region& cr, CR_INFO& info, BlockData_A& DataA, vector<BlockData_B>& DataB, double angle, int offset, double stepDistance, int16_t	restr, int16_t trig, uint16_t gain);

uint32_t	ParsePosition(F_HEAD& head, vector<BlockData_B>& blocks, VCR* vCRs, CR& cr, int index, uint8_t iFRow, uint8_t railType, vector<int>* t_cr, Position_Mark& pm, int& iAStepBig, int &iAStepSmall);

void	ParseSewCH(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, CR& cr, int iCrIndex, int16_t iFRow, bool carType, uint8_t railType, vector<int>* t_cr, int& iAStepBig, int &iAStepSmall, vector<Wound_Judged>& vWounds);

void	ParseSewLRH(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, CR& cr, int iCrIndex, int16_t iFRow, bool carType, uint8_t railType, vector<int>* t_cr, int& iAStepBig, int &iAStepSmall, vector<Wound_Judged>& vWounds);

bool	ParseScrewHoleSew1(F_HEAD& head, BLOCK& blockHead, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, CR& crF, uint8_t railType, Wound_Judged& w);

bool	ParseScrewHoleSew2(F_HEAD& head, BLOCK& blockHead, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, CR& crF, uint8_t railType, Wound_Judged& w);

bool	ParseScrewHoleSew3(F_HEAD& head, BLOCK& blockHead, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, CR& crF, uint8_t railType, Wound_Judged& w);

bool	ParseScrewHoleSew4(F_HEAD& head, BLOCK& blockHead, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, CR& crF, uint8_t railType, Wound_Judged& w);

bool	ParseScrewHoleCrackLeft(F_HEAD& head, int& iBlock, BLOCK& blockHead, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, int16_t& iFDesiredRow, CR& crFG, Wound_Judged& w);

bool	ParseScrewHoleCrackRight(F_HEAD& head, int& iBlock, BLOCK& blockHead, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, int16_t& iFDesiredRow, CR& crFG, Wound_Judged& w);

//判断与接头一起的螺孔,按6个螺孔的位置读取
bool	ParseScrewHole(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, int step1, int step2, int16_t iFRow, uint8_t railType, int iIndex, vector<Wound_Judged>& vWounds);

void	ParseJoint(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, int stepF1, int stepF2, uint8_t iFRow, bool carType, uint8_t railType, vector<Wound_Judged>& vWounds);

bool	ParseGuideHole(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, CR&cr, int i, int16_t iFRow, uint8_t railType, vector<Wound_Judged>& vWounds);

void	Analyse(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, vector<Wound_Judged>& vWounds, vector<Position_Mark>& vPMs);

//建立连通域
void CombineD(vector<Connected_Region> &vCR);

void CombineE(vector<Connected_Region> &vCR);

void CombineFG(vector<Connected_Region>& vCR);

void	CreateCR(vector<BlockData_B>& datas, vector<Connected_Region>* vCRs);

/************************************************************************/
/*
iblock			当前位置米块
iStep			当前米块中的步进下标
offset			步进偏移量
destBlock		目标位置的米块数
destStep		目标位置的步进下标                                                */
/************************************************************************/
bool	GetStepOffset(int iblock, int iStep, int offset, vector<BlockData_B>& blocks, int* destBlock, int * destStep);

int32_t	GetStepOffset2(int iBlock1, int iStep1, int iBlock2, int iStep2, vector<BlockData_B>& blocks);
//拼图
bool	PreDeal(F_HEAD& head, vector<BlockData_B>& blocks, vector<Connected_Region>* vCRs, BlockData_A& vAFrames, int * offset);

uint8_t	CanCombine(Connected_Region cr1, Connected_Region cr2, uint8_t channel);

void Combine(Connected_Region& cr1, Connected_Region& cr2);

//从1中排除2中有的
void	Exclude(vector<int>& cr1, vector<int>& cr2);

/************************************************************************/
/*           回退可能导致步进减小，所以需要增加米块信息，来辅助定位                                                           */
/************************************************************************/
int		FindStepInAData(int32_t steps, BlockData_A& DataA, uint32_t block);

//获取B超中步进对应的A超步进
/***********************************************************************
channel：	通道,DE,用于判断螺孔、导孔的双波
cr：		出波区域
DataB：	B超米块头
angle：	通道探头角度
offset：	探头偏移量
stepDistance：每步进长度（mm）
***********************************************************************/
bool	GetStepSteps(uint8_t channel, Connected_Region& cr, int32_t& step1, int32_t& step2, vector<BlockData_B>& DataB, double angle, int offset, double stepDistance);

//获取B超中步进对应的A超数据
/***********************************************************************
channel：	通道
cr：		出波区域
frames：	找到的帧
DataA：	A超全部数据
DataB：	B超米块头
angle：	通道探头角度
offset：	探头偏移量
stepDistance：每步进长度（mm）
***********************************************************************/
bool	GetStepAStepFrames(uint8_t channel, Connected_Region& cr, vector<A_Step>& vSteps, BlockData_A& DataA, vector<BlockData_B>& DataB, double angle, int offset, double stepDistance);

//获取指定通道的CR
/************************************************************************/
/*             channel：通道                                                  */

/************************************************************************/
uint8_t GetCR(uint8_t channel, int step1, int step2, vector<BlockData_B>& vBDatas, vector<Connected_Region>& crToFind, vector<int>& vCrFound, int32_t iExcept = -1, int iMinimunSize = 1);

uint8_t GetCR(uint8_t channel, int step1, uint8_t row1, int step2, uint8_t row2, vector<BlockData_B>& vBDatas, vector<Connected_Region>& crToFind, vector<int>& vCrFound, int32_t iExcept = -1, int iMinimunSize = 1);

uint8_t GetCR(uint8_t channel, int step1, uint8_t row1, int step2, uint8_t row2, vector<BlockData_B>& vBDatas, vector<Connected_Region>& crToFind, vector<int>& vCrFound, vector<int> vExcept, int iMinimunSize = 1);


//计算里程
//direction: true:顺里程，false:逆里程
double	GetWD(W_D pos, int nStep, float stepDistance, bool  direction = true);

double	GetWD(W_D walk);

//寻找下一个接头
bool	FindNextJoint(int block, int step, vector<BlockData_B>& blocks, int* destBlock, int * destStep);

//寻找下一个焊缝
bool	FindNextWeld(int block, int step, vector<BlockData_B>& blocks, int* destBlock, int * destStep);

//设置判断标志
void	SetJudgedFlag(vector<Connected_Region>& vCR, vector<int>& idx, int Flag);

//增加伤损/位置判据
void	AddWoundData(Wound_Judged& wound, vector<Connected_Region>& vCR, vector<int>& idx);

void	AddToWounds(vector<Wound_Judged>& vWounds, Wound_Judged& w);

uint64_t IsInBridge(double walk, vector<Bridge>& vBridges, int& iBeginIndex);

uint64_t	IsInCurve(double walk, vector<Curve>& vCurves, int& iBeginIndex);

uint64_t	IsInTunnel(double walk, vector<Tunnel>& vTunnels, int& iBeginIndex);

void	CombineOutputData(vector<Position_Mark>& vPMs, vector<Wound_Judged>& vWounds, vector<BLOCK>& blockheads);

void	Correlate(int* current, int* index, int count, int* ret);

void	AnalyseWithHistory(vector<Wound_Judged>& vWounds, vector<Wound_Judged> &vHWounds, vector<Position_Mark> &vMarks);

void	FillWound(Wound_Judged& wd, BLOCK& blockHead, F_HEAD& fhead);

#endif


