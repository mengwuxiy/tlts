#ifndef JUDGE_H
#define JUDGE_H

#include "SolveData.h"


//B��ͨ������
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

//A��ͨ������
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
// ������β������Ǻ���
//-----------------------------------------------
typedef struct _TRIANGLE
{
	float sin;
	float cos;
	float tan;
}TRIANGLE;


//-----------------------------------------------
// ������̽ͷ���Ƿ�Χ�����ݽṹ
//-----------------------------------------------
struct PROBE_COVERAGE_AREA
{
	int     Direction;                      // ̽ͷ����-1 - ��ǰ��1 - ���0 - ��ֱ����
	int     Range1;                    // ��һ�η�Χ��������
	int     Range2;                    // �ڶ��η�Χ��������
};


//B��ÿ��������
typedef struct _B_Step2
{
	int			Step;		//�ܵĲ�����	
	uint16_t	Draw1[66];	//������
}B_Step2;


//B����ʧ����¼
typedef struct _WaveData
{
	int			block;	//�׿�����
	int			step;	//��������
	uint8_t		row;	//��	
	uint8_t		find;	// BIT0��0:ʧ����1��������BIT7��0��δ����1���Ѵ���
}WaveData;

//��ͨ��
typedef struct _Connected_Region
{
	uint32_t			Block;	//�׿�����������ʹ��
	int32_t				Step;	//�׿��ڲ���
	uint8_t				Row1;	//��С��
	int32_t				Step1;	//��С����
	uint8_t				Row2;
	int32_t				Step2;
	uint8_t				Channel;
	uint8_t				Flag;	//�Ƿ��жϹ���
	vector<WaveData>	Region;
	vector<A_Step>		vASteps;//��Ӧ��A������
	uint16_t			Index;	//��ͨ���ʶ
}Connected_Region, CR;

typedef  vector<Connected_Region> VCR;

typedef struct _CR_INFO
{
	uint16_t	MinH;//��������С�����꣬A��512
	uint16_t	MaxH;//�������������꣬A��512
	uint16_t	MaxV;//��С��ֵ �� ������ֵ��F,G���ʧ��ʱ��ʹ�ø����壩
	int16_t	Shift;//λ��
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
	int			ZX;//�ɵ�վ�߱��
	uint32_t	StartB;
	uint32_t	StartA;
};


//������������
class Wound_Judged
{
public:
	Wound_Judged();
	char						Result[30];	// ����
	char						WoundNo[64];

	double						Walk;		// ���
	char						FoundTime[20]; //����ԱУ�����ʱ��
	uint16_t					Place;	//�����ڽ���λ��
	uint16_t					Type;	//����
	uint16_t					Degree;//�̶�
	uint8_t						SizeX;
	uint8_t						SizeY;
	char						ana_time[20];


	uint8_t						Checked;	//����״̬	
	char						Analyser[128];
	char						CheckTime[20]; //����ԱУ�����ʱ��
	double						Walk2;		// ���
	uint16_t					Place2;	//�����ڽ���λ��
	uint16_t					Type2;	//����
	uint16_t					Degree2;//�̶�
	uint8_t						SizeX2;
	uint8_t						SizeY2;
	char						Desc[128];

	char						RailNo[128];// �߱��4��BCD
	uint8_t						LeftRight;	// ���ҹ�
	uint8_t						XingType;	// �б𣺵��ߡ����С����У�����Ԥ��
	uint8_t						Xianbie;	// �߱�
	uint8_t						railType;	// BIT0~1: ����(0Ϊ43�죬1-50��2-60��3-75)
	uint8_t						railH;		// ��ǰ���mm

	double						gps_log;
	double						gps_lat;

	uint64_t					BridgeID;
	uint64_t					TunnelID;	//���ID
	uint64_t					CurveID;	//����ID
	uint8_t						IsJoint;	//��ͷID
	uint8_t						IsSew;		//����ID
	uint8_t						IsScrewHole;//�ݿ�ID
	uint8_t						IsGuideHole;//����ID

	uint8_t						Cycle;		//��ǰ������
	uint64_t					LastCycleID;//����������ID

	vector<Connected_Region>	vCRs;		//�������/ʧ������
	vector<A_Frame>				vFrames;	//A������

	uint8_t						Flag;

	uint8_t						Manual;		//�Ƿ��˹����ˣ���Ҫȷ����������
	char						According[256];//�о�

	int32_t						Block;
	int32_t						Step;		//�׿��ڲ���
	int32_t						Step2;		//�ܲ���
	uint64_t					FileID;		//��ҵID
	uint64_t					id;			//
	char						gwdNo[10];		//�����ID
};
//enum WOUND_TYPE
//{
//	W_OTHER = 0,
//
//	W_CHHF = 1,
//	W_LRHF = 2,
//	W_HEAD_HS = 3,
//
//	W_HEAD_HS_IN = W_HEAD_HS,	//��ͷ�ڲ����
//	W_HEAD_HS_MID = W_HEAD_HS,	//��ͷ���ĺ���
//	W_HEAD_HS_OUT = W_HEAD_HS,	//��ͷ������
//
//	W_YLS = 4,					//������
//	W_SCREW_CRACK = 5,				//�ݿ�б����
//	W_SCREW_CRACK1 = W_SCREW_CRACK,
//	W_SCREW_CRACK2 = W_SCREW_CRACK,
//	W_SCREW_CRACK3 = W_SCREW_CRACK,
//	W_SCREW_CRACK4 = W_SCREW_CRACK,
//	W_SCREW_HORIZON_CRACK = 6,		//�ݿ�ˮƽ����
//
//	W_JAW_HORIZON_CRACK = 7,		//���ˮƽ����
//	W_WAIST_HORIZON_CRACK = W_JAW_HORIZON_CRACK,		//����ˮƽ����
//
//	W_SKEW_CRACK = 8,				//б����
//	W_BOTTOM_TRANSVERSE_CRACK = 9	//��׺������ƣ���Բ�Σ���ֱ�����ڣ�
//};
enum WOUND_TYPE
{
	W_OTHER = 0,				//����������
	W_HEAD_HS = 1,				//����

	W_HEAD_HS_IN = W_HEAD_HS,	//��ͷ�ڲ����
	W_HEAD_HS_MID = W_HEAD_HS,	//��ͷ���ĺ���
	W_HEAD_HS_OUT = W_HEAD_HS,	//��ͷ������

	W_YLS = 2,					//������
	W_SCREW_CRACK = 16,				//�ݿ�б����
	W_SCREW_CRACK1 = 17,
	W_SCREW_CRACK2 = 18,
	W_SCREW_CRACK3 = 19,
	W_SCREW_CRACK4 = 20,
	W_SCREW_HORIZON_CRACK = 32,		//�ݿ�ˮƽ����

	W_JAW_HORIZON_CRACK = 4,		//���ˮƽ����
	W_WAIST_HORIZON_CRACK = 8,		//����ˮƽ����

	W_VERTICAL_CRACK = 64,			//����ˮƽ����

	W_SKEW_CRACK = 128,				//б����
	W_BOTTOM_TRANSVERSE_CRACK = 256	//��׺������ƣ���Բ�Σ���ֱ�����ڣ�
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
	WP_TM = BIT0,		//��ͷ̤����
	WP_HEAD_IN = BIT1,	//��ͷ�ڲ�
	WP_HEAD_MID = BIT2, //��ͷ�в�
	WP_HEAD_OUT = BIT3, //��ͷ���
	WP_ANGLE = BIT4,	//����
	WP_JAW_IN = BIT5,	//����ڲ�
	WP_JAW_OUT = BIT6,	//������
	WP_WAIST = BIT7,	//����
	WP_BOTTOM = BIT8,	//���
	WP_BOTTOM_ANGLE_IN = BIT9,	//���
	WP_BOTTOM_ANGLE_OUT = BIT10	//���
};


class Position_Mark
{
public:
	Position_Mark();
	double					Walk;	//���
	uint16_t				Mark;	//λ�ñ�����ͣ��㷨�ж���
	uint16_t				Mark2;	//λ�ñ�����ͣ��˹��ж���
	uint16_t				Data;	//���ݣ����п���
	uint32_t				Block;	//�׿�
	uint16_t				Step;	//�׿��ڲ���

	uint8_t					ARow;	//ƽ����
	int32_t					AStep;	//ƽ������
	uint8_t					ChannelNum;
	uint16_t				Num[16];
	double					Fangcha;
	uint16_t				Size;

	double					gps_log;
	double					gps_lat;
	//�߱��
	char					railNo[128];
	uint8_t					xingbie; //�б�
	uint8_t					xianbie; //�߱�	
	uint8_t					Gubie;	//�ɱ�

	uint8_t					Length;
	uint8_t					Height;
	uint8_t					Flag;
	uint16_t				Data2;//�󵼣� ���ݣ����ݣ��ҵ�
	uint64_t				id;
	uint8_t					Manual;//�Ƿ��˹����
};

enum POSITION_MARK
{
	PM_SEW_CH = 1,		//����
	PM_SEW_LRH = 2,		//���Ⱥ���

	PM_JOINT = 3,		//����

	PM_SCREWHOLE = 4,	//�ݿ�
	PM_GUIDEHOLE = 5,	//����

	PM_OTHER = 6,		//����

	PM_FORK = 7,		//����Y

	PM_SMART1 = 8,		//�����
	PM_SMART2 = 9,		//�����	

	PM_BACK = 10,		//����

	PM_CHECK_BEGIN = 11,//˫���������
	PM_CHECK_END = 12,	//˫�������յ�

	PM_START = 13 //�ϵ�
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
	uint8_t		Direction;//���߷���
	double		Start;
	double		End;
	double		Length;
	double		R;
	double		OverH;	//����
	double		HC;		//����
	char		railNo[30];
	uint8_t		Xingbie;
	uint64_t	gwdNo;
	uint64_t	bureauNo;//��
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
	uint64_t	bureauNo;//��
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
	char		SupportLineNo[20]; //�����߱��
	uint64_t	gwdNo;
	uint64_t	bureauNo;//��
}Tunnel;


bool OpenServer(CString strCfg);

bool CloseServer();

bool AddWork(char* strFile, double& gps_log, double& gps_lat);

bool AddWounds(vector<Wound_Judged>& wounds);

bool AddPMs(vector<Position_Mark>& vPMs);

bool AddBlocks(vector<BLOCK>& blocks, vector<Read_Info>& vA, vector<Read_Info>& vB);

void FinishFile();

//��ȡF��������
int16_t GetFRow(B_Step& step);

void FillCR(Connected_Region& cr);

//����������ͨ��ľ��룬���ں���ͨ��ʱʹ��
uint32_t		GetDistance(Connected_Region& cr1, Connected_Region& cr2);

//����B�е�ͨ����Ѱ��A��ͨ��
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

//�ж����ͷһ����ݿ�,��6���ݿ׵�λ�ö�ȡ
bool	ParseScrewHole(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, int step1, int step2, int16_t iFRow, uint8_t railType, int iIndex, vector<Wound_Judged>& vWounds);

void	ParseJoint(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, int stepF1, int stepF2, uint8_t iFRow, bool carType, uint8_t railType, vector<Wound_Judged>& vWounds);

bool	ParseGuideHole(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, VCR* vCRs, CR&cr, int i, int16_t iFRow, uint8_t railType, vector<Wound_Judged>& vWounds);

void	Analyse(F_HEAD& head, BlockData_A& DataA, vector<BlockData_B>& blocks, vector<Wound_Judged>& vWounds, vector<Position_Mark>& vPMs);

//������ͨ��
void CombineD(vector<Connected_Region> &vCR);

void CombineE(vector<Connected_Region> &vCR);

void CombineFG(vector<Connected_Region>& vCR);

void	CreateCR(vector<BlockData_B>& datas, vector<Connected_Region>* vCRs);

/************************************************************************/
/*
iblock			��ǰλ���׿�
iStep			��ǰ�׿��еĲ����±�
offset			����ƫ����
destBlock		Ŀ��λ�õ��׿���
destStep		Ŀ��λ�õĲ����±�                                                */
/************************************************************************/
bool	GetStepOffset(int iblock, int iStep, int offset, vector<BlockData_B>& blocks, int* destBlock, int * destStep);

int32_t	GetStepOffset2(int iBlock1, int iStep1, int iBlock2, int iStep2, vector<BlockData_B>& blocks);
//ƴͼ
bool	PreDeal(F_HEAD& head, vector<BlockData_B>& blocks, vector<Connected_Region>* vCRs, BlockData_A& vAFrames, int * offset);

uint8_t	CanCombine(Connected_Region cr1, Connected_Region cr2, uint8_t channel);

void Combine(Connected_Region& cr1, Connected_Region& cr2);

//��1���ų�2���е�
void	Exclude(vector<int>& cr1, vector<int>& cr2);

/************************************************************************/
/*           ���˿��ܵ��²�����С��������Ҫ�����׿���Ϣ����������λ                                                           */
/************************************************************************/
int		FindStepInAData(int32_t steps, BlockData_A& DataA, uint32_t block);

//��ȡB���в�����Ӧ��A������
/***********************************************************************
channel��	ͨ��,DE,�����ж��ݿס����׵�˫��
cr��		��������
DataB��	B���׿�ͷ
angle��	ͨ��̽ͷ�Ƕ�
offset��	̽ͷƫ����
stepDistance��ÿ�������ȣ�mm��
***********************************************************************/
bool	GetStepSteps(uint8_t channel, Connected_Region& cr, int32_t& step1, int32_t& step2, vector<BlockData_B>& DataB, double angle, int offset, double stepDistance);

//��ȡB���в�����Ӧ��A������
/***********************************************************************
channel��	ͨ��
cr��		��������
frames��	�ҵ���֡
DataA��	A��ȫ������
DataB��	B���׿�ͷ
angle��	ͨ��̽ͷ�Ƕ�
offset��	̽ͷƫ����
stepDistance��ÿ�������ȣ�mm��
***********************************************************************/
bool	GetStepAStepFrames(uint8_t channel, Connected_Region& cr, vector<A_Step>& vSteps, BlockData_A& DataA, vector<BlockData_B>& DataB, double angle, int offset, double stepDistance);

//��ȡָ��ͨ����CR
/************************************************************************/
/*             channel��ͨ��                                                  */

/************************************************************************/
uint8_t GetCR(uint8_t channel, int step1, int step2, vector<BlockData_B>& vBDatas, vector<Connected_Region>& crToFind, vector<int>& vCrFound, int32_t iExcept = -1, int iMinimunSize = 1);

uint8_t GetCR(uint8_t channel, int step1, uint8_t row1, int step2, uint8_t row2, vector<BlockData_B>& vBDatas, vector<Connected_Region>& crToFind, vector<int>& vCrFound, int32_t iExcept = -1, int iMinimunSize = 1);

uint8_t GetCR(uint8_t channel, int step1, uint8_t row1, int step2, uint8_t row2, vector<BlockData_B>& vBDatas, vector<Connected_Region>& crToFind, vector<int>& vCrFound, vector<int> vExcept, int iMinimunSize = 1);


//�������
//direction: true:˳��̣�false:�����
double	GetWD(W_D pos, int nStep, float stepDistance, bool  direction = true);

double	GetWD(W_D walk);

//Ѱ����һ����ͷ
bool	FindNextJoint(int block, int step, vector<BlockData_B>& blocks, int* destBlock, int * destStep);

//Ѱ����һ������
bool	FindNextWeld(int block, int step, vector<BlockData_B>& blocks, int* destBlock, int * destStep);

//�����жϱ�־
void	SetJudgedFlag(vector<Connected_Region>& vCR, vector<int>& idx, int Flag);

//��������/λ���о�
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


