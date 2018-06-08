#ifndef SOLVEDATA_H
#define SOLVEDATA_H

#define 	BIT0 		0x00000001
#define 	BIT1 		0x00000002
#define 	BIT2 		0x00000004
#define 	BIT3 		0x00000008
#define 	BIT4 		0x00000010
#define 	BIT5 		0x00000020
#define 	BIT6 		0x00000040
#define 	BIT7 		0x00000080
#define 	BIT8 		0x00000100
#define 	BIT9 		0x00000200
#define 	BIT10 		0x00000400
#define 	BIT11 		0x00000800
#define 	BIT12 		0x00001000
#define 	BIT13 		0x00002000
#define 	BIT14 		0x00004000
#define 	BIT15 		0x00008000
#define 	BIT16 		0x00010000
#define 	BIT17 		0x00020000
#define 	BIT18 		0x00040000
#define 	BIT19 		0x00080000
#define 	BIT20 		0x00100000
#define 	BIT21 		0x00200000
#define 	BIT22 		0x00400000
#define 	BIT23 		0x00800000
#define 	BIT24 		0x01000000
#define 	BIT25 		0x02000000
#define 	BIT26 		0x04000000
#define 	BIT27 		0x08000000
#define 	BIT28 		0x10000000
#define 	BIT29 		0x20000000
#define 	BIT30 		0x40000000
#define 	BIT31 		0x80000000

#define    	DOR_NUM   	7
#define    	CH_N   		12
#define    	GA_NUM   	5	
#define   	R_N   		4	


struct FILE_TIME {
	uint32_t dwLowDateTime;
	uint32_t dwHighDateTime;
};

struct DGC {
	int16_t	h[6];			// ��λ(��)�������ʾʱ��������
	int16_t	gS[6];			// �ڵ���ʼλ�����棬��λ/2db
	int16_t	gE[6];			// �ڵ����λ�����棬��λ/2db
};


struct STATUS {
	FILE_TIME	creatT;		// �����ļ���ʱ��
	FILE_TIME	ExitT;		// �ػ�ʱ��
	int16_t		ExitKm;		// �ػ����
	int16_t		CreatKm;	// �������
};

// ���
struct W_D{
public:
	int	m;
	int16_t	mm;
	int16_t	Km;			// 
	bool operator >= (W_D p);
	bool operator <= (W_D p);
	W_D operator +(W_D p);
	W_D operator -(W_D p);
};



/*
uint8_t gpsInfor[32];
gpsInfor[0]��ʾGPS��λ״̬��GPRMC�� A��ЧV��Ч
gpsInfor[1]~[2]:��Ч����������[1]��λ��[2]ʮλ
gpsInfor[3]~[14]��γ���磺"N30.42.6304";
gpsInfor[15]~[27]�����ȣ���"E104.02.6090"
gpsInfor[28]~[31]��UTCʱ�䣬��ʾ��[28][29]ʱʱ��[30][31]����
*/


//-----------------------------------------------
// ���б��������˵ı�׼
//-----------------------------------------------
typedef struct _STANDARD {
	uint8_t	speed;		// �ٶ�����*km/h
	uint8_t	speed_D;	// ���پ����5λΪ���پ��룬��λm����3λ����
	uint8_t	coup;		// ��϶ȣ���ϲ��������У���Ϊ1~5����5��Ϊ���ϸ�
	uint8_t	coup_D;		// ʧ����ֵ��ʧż�����˾��룬��������λ0.1m
	uint8_t	abrasion;	// ĥ����ֵmm,��������������
	uint8_t	QAB_M;		// 70�ȹ����������ڵĻز��ƶ���������,��λ/15us
	uint8_t	QDE_M;		// 37�ȹ����ʴ���ڵĻز��ƶ���������,��λ/15us
	uint8_t	QDE1_Al;	// 37���ݿ׽��沨��ࣨ��ֱ���룩
	uint8_t	QDE1_D;		// 37���ݿ�˫���������
	uint8_t	QF1_D;		// 0���ݿ�˫���������
	uint8_t	QF1_AL;		// 0���ݿ׽��沨��ࣨ��ֱ��
	uint8_t	wDouble0_D;	// 0���ݿ�˫��������
	uint8_t	wDouble37_D;// 37���ݿ�˫��������
	uint8_t	rev1;		// 
	uint8_t	rev2;		//����
	uint8_t	rev3;		// 
	uint8_t	revS[4];	// 
	uint8_t	rev4;		// 
	uint8_t	lose_Step;	// ʧ���������
	uint16_t	rev5;
}STD;

enum  _ALARM_OC {
	A_OC = BIT1,	// ������Ƶ���أ�������
	A_SPECH = BIT2,	// ������ʾ
	A_0LOS = BIT3,	// 0��ʧ��������ʽ��0������ʧ����1���������(�����ڹ���Ա������)
	A_SEW = BIT4,	// ���ز���0-�أ�1-��,�ر�ʾ��츽��������ʧ����������
	A_QDE = BIT5,	// QDE����
	A_QAB = BIT6,	// QAB����
	A_ALT = BIT7,	// ���沨
	A_SPEED = BIT8,	// ����
	A_COUPLE = BIT9,// ���
	//	A_FG = BIT15,	// 0�ȴ�͸ʧ��ͨ��ѡ��
};

enum  _DETECT_ELSE {
	R_LS = BIT0,		// ���̹�1������
	R_BS = BIT1,		// ���ƴ�С��1-��0-С
	C_LR = BIT2,		// ���ͣ�1-���ֳ���0-���ֳ�
	D_DUB = BIT3,		// �Ƿ�˫��ʶ��
	D_CB = BIT4,		// �Ƿ��������ʶ��
	D_MOV = BIT5,		// �Ƿ��ƶ�����ʶ��
	D_TYP = BIT6,		// �Ƿ����ʶ��
};

typedef struct _DETECT_PARAM {
	uint16_t	Alarm;			// ����_ALARM_OC
	uint16_t	Identify;		// ����_DETECT_ELSE
}DETECT;

//-----------------------------------------------
// �Ƕȼ�ƫ��
//-----------------------------------------------
typedef struct _ANGLE {
	int16_t	Refrac;		// ����ǣ���λ0.5��
	int16_t	Angle;		// б70�ȵ�ƫб�ǣ���λ0.5��
}ANGLE;

//-----------------------------------------------
// �ϵ�����
//-----------------------------------------------
typedef struct _TRACK_PARAM {
	uint8_t	lineType;			// �ϵ�����0: վ��, 1: ����
	uint8_t	lineWay;			// �б�0�����ߡ�1���ϣ�2������
	uint8_t	leftRight;			// ���ҹ�0: ��1����
	uint8_t	WalkWay;			// ���߷���0: �����, 1: ˳���
	uint8_t	railType;			// ����0~3���α�ʾ43, 50, 60, 75
	uint8_t	rev1;
	uint8_t	rev2;
	uint8_t	rev3;

	uint16_t	railNum;		// �ֹ��ţ�4��BCD
	uint16_t	plyNum;			// �ɵ���ţ�վ�ߣ�4��BCD
	uint16_t	KMm[2];			// ��̣� KMm[0]��Km, KMm[1]��m
	uint32_t	stationNum;		// ��վ��ţ�վ�ߣ�4��BCD

	uint32_t	sectionNum;		// ��λ���5��BCD
	uint16_t	regionNum;		// ����4��BCD
	uint16_t	lineNum;		// �߱��4��BCD
	uint16_t	teamNum;		// ����4��BCD
	uint16_t	workerNum;		// ����4��BCD
}TRACK_P;


//-----------------------------------------------
// ��
//-----------------------------------------------
struct DOOR {
	int16_t	start;		// ���ŵ���ʼλ�ã������̱�ʾ����λ/15us
	int16_t	end;		// ���ŵĽ���λ��
	int16_t	isOn;		// ���ſ���
};


//-----------------------------------------------
typedef struct _B_POINT {
	uint16_t	Draw1;			// ������ͼ
	uint16_t	Draw2;			// 3dB��ͼ
	uint16_t	Alarm;
	uint8_t		Weight;			// Ȩֵֻ�õ�λ����������,
	uint8_t		Wound;			// �����ǣ�_B_WOUND_DEF��BIT7�����ã���*****
}B_POINT;



typedef struct _B_MARK {
	uint32_t	Mark;
	uint16_t	Couple;		// BIT0~BIT11: 12��ͨ������������BIT15: �����ж���ϲ���(��ʾ��һ��ͨ��ŷ�ϲ����Ѿ��ﵽһ������)��
	uint16_t	rev;		// ������Bit15�����ã���*****
}B_MARK;

enum _B_MARK_DEF {
	ALARM_M = BIT0,				// ����������
	LOSE_0 = BIT1,				// 0��ʧ��
	LOSE_37 = BIT2,				// 37��ʧ��
	SP_P = BIT3,				// ���ٵ�>
	SEW = BIT4,					// ���#
	BACK_P = BIT5,				// ����@
	START = BIT6,				// �ϵ�S
	CHECK = BIT7,				// Ч��C

	CK_KM = BIT9,				// ���У��

	SEW2 = BIT16,				// �ֶ�������*
	FORK = BIT17,				// ����Y
	CURVE = BIT18,				// ����$
	QIAO = BIT19,				// ����Q
	SEW_N = BIT20 | BIT21 | BIT22 | BIT23,// �ֶ�*��ǵı�ţ�����*1,*2....
};

typedef struct _B_WOUND {
	uint16_t	W_Mark;			// BIT0~BIT7��������ı�ǲ���_B_WOUND_DEF, BIT8~BIT11����̶�(�˹��趨���������˲��趨), BIT12~BIT15��Ϊ������ע��1��37�ȴ�͸�ˣ�2��0��ʧ���ˣ�
	uint16_t	W_Code;			// 4��BCD�룬�ӵ͵������α�ʾ������λ�á�����λ�á�����״̬��ϸ��
	uint32_t	Other;			// ��λ��ʾ���������δ��������ϼа壬�İ��ϼУ�ԭλ����������������Bit31�����ã���*****
}B_WOUND;

enum _B_WOUND_DEF {
	W_MAN = BIT0,			// �ֶ�����
	W_D_SER = BIT1,			// ��ͨ��������ˣ�Ȩֵ����7
	W_D_SLI = BIT2,			// ��ͨ��������ˣ�Ȩֵ<7
	W_SIG = BIT3,			// �Զ���ͨ������
	W_HOL = BIT4,			// �ݿ�˫����\���沨\����¼ΪZ_WOUND������ȼ�Ϊ����
	W_ADD = BIT6			// �ط����
};


//-----------------------------------------------
// �������в���
//-----------------------------------------------
struct ALL_P {
	DOOR	Doors[4][DOR_NUM];	// С��
	uint8_t	Gain[CH_N];			// �ֶ�����
	DETECT	DetectSet;			// ̽������
	STD		Standard;			// ����̽�˱�����׼
	uint16_t	fork;			// ����ţ�4��BCD
	uint16_t	rev16;			// ����
	uint8_t	disMode;			// ��ʾģʽ
	uint8_t	volume;				// ����16��
	uint8_t	autoGain;			// BIT0~1�Ƿ��Զ����棬BIT2~3̽ͷѡ��(��ͨ/���ϲ���)����������
	uint8_t	BitState;			// BIT0��������BIT1����ֱ��
};

//--------------------------------------------------------------
// ALL_P2
// �˲���ֻ�����ļ�ͷ�������׿�ͷ
//--------------------------------------------------------------
struct ALL_P2 {
	ANGLE	Angle[CH_N];	// ̽ͷ�Ƕȼ�ƫ��
	int16_t	Place[CH_N];	// ̽ͷλ��
	int16_t	Zero[CH_N];		// ̽ͷ���, ��λ/15us
	int16_t	Restr[CH_N];	// ���ƣ�С�ڴ�ֵ�Ļز�����ʾ
	int16_t	Trig[CH_N];		// ��ֵ��С�ڴ�ֵ�Ļز�����B�������ֵ��һ�� = ����ֵ
	DOOR	Gate[4][GA_NUM];// ����, ��λ/15us
	DGC		Dgc[4][4];		// ����DGC������
	TRACK_P	TrackSet;		// �ϵ�����
	STATUS	dev_S;			// �豸״̬������
	int16_t	S_factor;		// ���ϵ��
	uint16_t	rev;
};


//-----------------------------------------------
// �ļ�ͷ
//-----------------------------------------------
struct F_HEAD
{
	uint32_t		CheckSum;
	int8_t			Name[32];			// "SZT-800_RailTrack_Bchao"
	int8_t			DataVerS[8];		// ���ݰ汾��DataVerS[8] = {'2', '.', '4', 0, }; V2.2֮ǰ�����ݽṹ��2015��7��֮ǰ��
	int8_t			SoftVerS[8];		// ����汾
	int8_t			FPGAVerS[8];		// FPGA�汾
	int8_t			DeviceNum[8];		// �������
	uint32_t		Reserved[30];		// ����
	W_D				startKm;
	uint32_t		startT;				// ��ʼ��ҵʱ�䣺D23~D16ANGLEAnalysisPrint(srcdeviceP2.Place[i],pFile);	ʱ,D15~D8��,D7~D0��
	uint32_t		startD;				// ������
	uint32_t		endT;				// ������ҵʱ�䣺ʱ����
	uint32_t		endD;				// ������ҵʱ�䣺������
	int32_t			distance;			// �ܹ����߶��پ���, ����
	uint32_t		block;				// �ܹ������׿�,
	uint32_t		alarm;				// �ܹ����ٱ���,����
	uint32_t		wound;				// �ܹ���������,����
	uint32_t		run;				// ���ٴ���
	uint32_t		orderNum;			// ̽�������,�ݶ�8λBCD,����
	float			step;				// ÿ���������ٺ���
	W_D				endKm;				// �������
	ALL_P			deviceP;			// ��������
	ALL_P2			deviceP2;			// ��������2
	DGC				DGC_Sd[R_N][4];		// ����DGC�ڵ�λ��, ����,4�ֹ��ͣ�ÿ�ֹ���4��������
	DGC				DGC_St[R_N][4];		// ǿ������Ŀǰ���ã�������
	uint8_t			zero_AD[2];			// AD���.
	uint8_t			rev8[2];			// �Ӵ˴���ʼ100���ֽڱ������ط�����á�
	uint32_t		rev[395];			// ����

};

//----------------------------------------------------------------------------------------
//tpA�׿�ͷ
//----------------------------------------------------------------------------------------
typedef struct _BLOCK_WAVE {
	uint32_t	symbol;			// �׿��־ 0xFFFFFFFF ����Ҫ��֤���е�����С��0xFFFFFFFF
	int32_t		index;			// �׿���ʼ��������B���еĲ������Ӧ��
	uint32_t	len;			// ���׿�����ѹ��������ֽ�

	uint16_t	checkD;			// �׿�ѹ�������ݵ�У���
	uint16_t	fNum;			// ���׿����֡A��

	uint8_t		log;			// ������־v2.2
	uint8_t		rev;			// ����
	uint16_t	rev1;			// ����
}BLOCK_A;


//----------------------------------------------------------------------------------------
//tpB�׿�ͷ��ĿǰB�����ݲ���λѹ�����׿�ͷsizeӦ�ǵ�����������������
//----------------------------------------------------------------------------------------
typedef struct _BLOCK {
	uint32_t	symbol;			// �׿��־0xFFFFFFFF,����Ҫ��֤���е���������С��0xFFFFFFFF��
	uint16_t	checkSum;		// �׿�ͷЧ���,����, λ�ò��ܸ�
	uint16_t	checkD;			// �׿����ݵ�Ч���
	int32_t		indexL;			// ��ǰ����
	uint32_t	len;			// ���׿�����ѹ�����ֽ���	
	uint32_t	time;			// ʱ�䣺��λ����λÿ8λ����Ϊ���ա�ʱ���֡���
	uint16_t	row;			// ���׿���ٸ�����
	uint16_t	railNum;		// �ֹ���,4��BCD���ʾ
	uint16_t	swNum;			// ����ţ�4��BCD��ʾ��һ��3λ
	uint16_t	user;			// ����,4��BCD
	DETECT		detectSet;		// ̽������
	uint16_t	door[DOR_NUM][2];	// С�����ã�DOR_NUM=7��door[x][0]�����λΪon/off	
	uint16_t	addVal[CH_N];		// �ۼ�ֵ��CH_N=12 deͨ�����ã�����
	int8_t		probOff[CH_N];		// ̽ͷλ��ƫ��
	W_D			walk;			// ��ʱ���
	int16_t		speed;			// �ٶȣ���λ0.1km/h
	uint8_t		gpsInfor[32];	// GPS��γ�ȡ�UTCʱ�䣬�ַ���
	uint8_t		gain[CH_N];		// 12ͨ����0~160��ʾ0~80.0dB����λ0.5dB��
	uint8_t		railType;		// BIT0~1: ����(0Ϊ43�죬1-50��2-60��3-75), BIT4:0����̡�1˳��̣�BIT5:0�ҹɡ�1��ɣ�BIT6~7�����ߡ����С����У�����Ԥ��
	uint8_t		railH;			// ��ǰ���mm
	uint8_t		autoGain;		// BIT0~1:�Ƿ��Զ�����, BIT2~3̽ͷѡ������Ԥ��
	uint8_t		log;			// ������־
	uint8_t		volume;			// BIT0~6������32��������Ԥ��,��λ������ʾģʽ
	uint8_t		BitS;			// BIT0:��������BIT1������ֱ����BIT2:�����ǰ
	uint8_t		rev2;			// ����վ�ߵĹɵ���ŵĸ�λ
	uint8_t		rev3;			// ����վ�ߵĹɵ���ŵ�ʮλ
}BLOCK;

typedef struct _TIME_ANALYSIS
{
	uint8_t		second;
	uint8_t		minute;
	uint16_t	hour;
	uint8_t		day;
	uint8_t		month;
	uint16_t	year;
	struct _TIME_ANALYSIS *p;
}TIME_ANALYSIS;


//��¼�׿����ʼλ��
typedef struct _ReadFile_Info
{
	uint32_t	ReadL;
	uint32_t	UsedL;
}Read_Info;
typedef vector<Read_Info> VRI;

/************************************************************************/
/*                           �������ݸ�ʽ                               */
/************************************************************************/

//B����ÿ������
typedef struct _B_RowData
{
	uint8_t		Row;			//��
	B_POINT		Point;			//��ͼ��Ϣ
}B_RowData;

//B��ÿ��������
typedef struct _B_Step
{
	//�ܵĲ�����
	int			Step;

	//�ܵĲ�����2�����˲���
	//int			Step2;		

	//���
	B_MARK		Mark;

	//����
	B_WOUND		Wound;

	//��������
	vector<B_RowData>	vRowDatas;
}B_Step;


//A��ÿ֡����
typedef struct _A_Frame
{
	uint16_t	Horizon;	//������
	uint8_t		Used;		//�Ƿ���ʹ��
	uint16_t	F[CH_N];	//��������	
}A_Frame;

typedef struct _A_Step
{
	uint32_t		Block;		//�׿�����
	int32_t			Step;		//����ƫ����
	uint16_t		Index;		//�׿���֡ID
	int32_t			Index2;		//֡ID
	vector<A_Frame> Frames;
}A_Step;

typedef struct _BlockData_A
{
	vector<A_Step>		vAStepDatas;//A��֡����
}BlockData_A;

typedef struct _BlockData_B
{
	BLOCK	BlockHead;			//�׿�ͷ
	int		Index;				//�׿����������ڼ����׿�
	int		StepCount;			//�׿��в�������
	vector<B_Step>		vBStepDatas;//B����������
}BlockData_B;


/*******************************************************/
//��������

//A�����ݽ���
uint32_t	Analyse_Achao(FILE* pFileA, BlockData_A& vADatas, vector<BlockData_B>& vBDatas, int32_t& readL, int &useL, int iFileSize, int iBeginBlock, int& iBeginFrame, vector<Read_Info>& vInfo, int iBlockCountToRead = 100);

//B�����ݽ���
uint32_t	Analyse_Bchao(FILE* pFileB, vector<BlockData_B>& vBDatas, uint32_t& use_Size, uint32_t& read_Size, int iFileSize, int iBeginBlock, int& iBeginStep, vector<Read_Info>& vInfo, int iBlockCountToRead = 100);

//У��ͺ���
uint16_t 	Check_Sum(uint16_t *p16Buffer, uint16_t uLength);

//B����ѹ����
uint16_t	RLE64_Decompress_Loop2Loop(uint64_t *pDestBuf, uint32_t width, uint32_t height, uint32_t start, uint32_t L, uint8_t *pSorc, uint8_t *pHead, uint8_t *pTail, uint32_t size);

//A����ѹ����
uint16_t 	RLE16_Decompress_Loop2Buf(uint16_t *pDest, uint16_t *pSrc, uint16_t *pSrcHead, uint16_t *pSrcTail, uint32_t uLength);


uint32_t	Analyse_Achao2(FILE* pFileA, BlockData_A& vADatas, uint32_t &useL, int iFileSize, int iBeginBlock, int iBeginStep, int& iBeginFrame, int iBlockCountToRead);

uint32_t	Analyse_Bchao2(FILE* pFileB, vector<BlockData_B>& vBDatas, uint32_t use_Size, int32_t iFileSize, int iBeginBlock, int iBeginStep, int iBlockCountToRead);

double		GetWalk(W_D wd);


#endif // ifnef