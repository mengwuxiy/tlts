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
	int16_t	h[6];			// 单位(点)，最大，显示时换算成深度
	int16_t	gS[6];			// 节点起始位置增益，单位/2db
	int16_t	gE[6];			// 节点结束位置增益，单位/2db
};


struct STATUS {
	FILE_TIME	creatT;		// 创建文件的时间
	FILE_TIME	ExitT;		// 关机时间
	int16_t		ExitKm;		// 关机里程
	int16_t		CreatKm;	// 创建里程
};

// 里程
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
gpsInfor[0]表示GPS定位状态（GPRMC） A有效V无效
gpsInfor[1]~[2]:有效卫星数量，[1]个位，[2]十位
gpsInfor[3]~[14]，纬度如："N30.42.6304";
gpsInfor[15]~[27]，经度，如"E104.02.6090"
gpsInfor[28]~[31]，UTC时间，表示：[28][29]时时，[30][31]日日
*/


//-----------------------------------------------
// 所有报警或判伤的标准
//-----------------------------------------------
typedef struct _STANDARD {
	uint8_t	speed;		// 速度上限*km/h
	uint8_t	speed_D;	// 超速距离低5位为超速距离，单位m，高3位保留
	uint8_t	coup;		// 耦合度，耦合不良的评判，分为1~5级（5级为最严格）
	uint8_t	coup_D;		// 失耦阈值，失偶超过此距离，报警，单位0.1m
	uint8_t	abrasion;	// 磨耗阈值mm,超过报警，保留
	uint8_t	QAB_M;		// 70度轨面鱼鳞门内的回波移动报警距离,单位/15us
	uint8_t	QDE_M;		// 37度轨底锈蚀门内的回波移动报警距离,单位/15us
	uint8_t	QDE1_Al;	// 37度螺孔交替波间距（垂直距离）
	uint8_t	QDE1_D;		// 37度螺孔双波报警间距
	uint8_t	QF1_D;		// 0度螺孔双波报警间距
	uint8_t	QF1_AL;		// 0度螺孔交替波间距（垂直）
	uint8_t	wDouble0_D;	// 0度螺孔双波伤损间距
	uint8_t	wDouble37_D;// 37度螺孔双波伤损间距
	uint8_t	rev1;		// 
	uint8_t	rev2;		//保留
	uint8_t	rev3;		// 
	uint8_t	revS[4];	// 
	uint8_t	rev4;		// 
	uint8_t	lose_Step;	// 失波距离控制
	uint16_t	rev5;
}STD;

enum  _ALARM_OC {
	A_OC = BIT1,	// 报警音频开关，开，关
	A_SPECH = BIT2,	// 语音提示
	A_0LOS = BIT3,	// 0度失波报警方式，0：正常失波，1：距离控制(步进在管理员设置中)
	A_SEW = BIT4,	// 轨逢回波，0-关，1-开,关表示轨缝附近出波或失波不报警。
	A_QDE = BIT5,	// QDE出波
	A_QAB = BIT6,	// QAB出波
	A_ALT = BIT7,	// 交替波
	A_SPEED = BIT8,	// 超速
	A_COUPLE = BIT9,// 耦合
	//	A_FG = BIT15,	// 0度穿透失波通道选择
};

enum  _DETECT_ELSE {
	R_LS = BIT0,		// 长短轨1长，短
	R_BS = BIT1,		// 抑制大小，1-大，0-小
	C_LR = BIT2,		// 车型，1-右手车，0-左手车
	D_DUB = BIT3,		// 是否双波识别
	D_CB = BIT4,		// 是否组合伤损识别
	D_MOV = BIT5,		// 是否移动伤损识别
	D_TYP = BIT6,		// 是否轨型识别
};

typedef struct _DETECT_PARAM {
	uint16_t	Alarm;			// 对照_ALARM_OC
	uint16_t	Identify;		// 对照_DETECT_ELSE
}DETECT;

//-----------------------------------------------
// 角度及偏角
//-----------------------------------------------
typedef struct _ANGLE {
	int16_t	Refrac;		// 折射角，单位0.5度
	int16_t	Angle;		// 斜70度的偏斜角，单位0.5度
}ANGLE;

//-----------------------------------------------
// 上道设置
//-----------------------------------------------
typedef struct _TRACK_PARAM {
	uint8_t	lineType;			// 上道类型0: 站线, 1: 正线
	uint8_t	lineWay;			// 行别0：单线、1：上，2：下行
	uint8_t	leftRight;			// 左右股0: 右1：左
	uint8_t	WalkWay;			// 行走方向0: 逆里程, 1: 顺里程
	uint8_t	railType;			// 轨型0~3依次表示43, 50, 60, 75
	uint8_t	rev1;
	uint8_t	rev2;
	uint8_t	rev3;

	uint16_t	railNum;		// 钢轨编号，4个BCD
	uint16_t	plyNum;			// 股道编号（站线）4个BCD
	uint16_t	KMm[2];			// 里程， KMm[0]：Km, KMm[1]：m
	uint32_t	stationNum;		// 车站编号（站线）4个BCD

	uint32_t	sectionNum;		// 单位编号5个BCD
	uint16_t	regionNum;		// 工区4个BCD
	uint16_t	lineNum;		// 线编号4个BCD
	uint16_t	teamNum;		// 班组4个BCD
	uint16_t	workerNum;		// 工号4个BCD
}TRACK_P;


//-----------------------------------------------
// 门
//-----------------------------------------------
struct DOOR {
	int16_t	start;		// 方门的起始位置，用声程表示，单位/15us
	int16_t	end;		// 方门的结束位置
	int16_t	isOn;		// 方门开关
};


//-----------------------------------------------
typedef struct _B_POINT {
	uint16_t	Draw1;			// 正常画图
	uint16_t	Draw2;			// 3dB画图
	uint16_t	Alarm;
	uint8_t		Weight;			// 权值只用低位，其他保留,
	uint8_t		Wound;			// 伤损标记（_B_WOUND_DEF）BIT7不能用！！*****
}B_POINT;



typedef struct _B_MARK {
	uint32_t	Mark;
	uint16_t	Couple;		// BIT0~BIT11: 12个通道的耦合情况，BIT15: 仪器判断耦合不良(表示有一个通道欧合不良已经达到一定距离)，
	uint16_t	rev;		// 保留，Bit15不可用！！*****
}B_MARK;

enum _B_MARK_DEF {
	ALARM_M = BIT0,				// 出波报警点
	LOSE_0 = BIT1,				// 0度失波
	LOSE_37 = BIT2,				// 37度失波
	SP_P = BIT3,				// 超速点>
	SEW = BIT4,					// 轨缝#
	BACK_P = BIT5,				// 回退@
	START = BIT6,				// 上道S
	CHECK = BIT7,				// 效验C

	CK_KM = BIT9,				// 里程校对

	SEW2 = BIT16,				// 手动焊缝轨缝*
	FORK = BIT17,				// 道岔Y
	CURVE = BIT18,				// 曲线$
	QIAO = BIT19,				// 桥梁Q
	SEW_N = BIT20 | BIT21 | BIT22 | BIT23,// 手动*标记的编号，比如*1,*2....
};

typedef struct _B_WOUND {
	uint16_t	W_Mark;			// BIT0~BIT7各种伤损的标记参照_B_WOUND_DEF, BIT8~BIT11伤损程度(人工设定，仪器的伤不设定), BIT12~BIT15作为其他备注（1：37度穿透伤，2：0度失波伤）
	uint16_t	W_Code;			// 4个BCD码，从低到高依次表示：长度位置、截面位置、伤损状态、细化
	uint32_t	Other;			// 低位表示处理情况，未处理，钻孔上夹板，鼓包上夹，原位复焊。其他保留，Bit31不可用！！*****
}B_WOUND;

enum _B_WOUND_DEF {
	W_MAN = BIT0,			// 手动标伤
	W_D_SER = BIT1,			// 多通道组合重伤，权值大于7
	W_D_SLI = BIT2,			// 多通道组合轻伤，权值<7
	W_SIG = BIT3,			// 自动单通道伤损
	W_HOL = BIT4,			// 螺孔双波伤\交替波\，记录为Z_WOUND，伤损等级为轻伤
	W_ADD = BIT6			// 回放添加
};


//-----------------------------------------------
// 仪器所有参数
//-----------------------------------------------
struct ALL_P {
	DOOR	Doors[4][DOR_NUM];	// 小门
	uint8_t	Gain[CH_N];			// 手动增益
	DETECT	DetectSet;			// 探测设置
	STD		Standard;			// 智能探伤报警标准
	uint16_t	fork;			// 道岔号，4个BCD
	uint16_t	rev16;			// 保留
	uint8_t	disMode;			// 显示模式
	uint8_t	volume;				// 音量16级
	uint8_t	autoGain;			// BIT0~1是否自动增益，BIT2~3探头选择(普通/复合材料)，其他保留
	uint8_t	BitState;			// BIT0道岔右左，BIT1道岔直曲
};

//--------------------------------------------------------------
// ALL_P2
// 此参数只存于文件头，不存米块头
//--------------------------------------------------------------
struct ALL_P2 {
	ANGLE	Angle[CH_N];	// 探头角度及偏角
	int16_t	Place[CH_N];	// 探头位置
	int16_t	Zero[CH_N];		// 探头零点, 单位/15us
	int16_t	Restr[CH_N];	// 抑制，小于此值的回波不显示
	int16_t	Trig[CH_N];		// 阈值，小于此值的回波不画B超。这个值的一半 = 抑制值
	DOOR	Gate[4][GA_NUM];// 大门, 单位/15us
	DGC		Dgc[4][4];		// 定制DGC，保留
	TRACK_P	TrackSet;		// 上道设置
	STATUS	dev_S;			// 设备状态，保留
	int16_t	S_factor;		// 里程系数
	uint16_t	rev;
};


//-----------------------------------------------
// 文件头
//-----------------------------------------------
struct F_HEAD
{
	uint32_t		CheckSum;
	int8_t			Name[32];			// "SZT-800_RailTrack_Bchao"
	int8_t			DataVerS[8];		// 数据版本，DataVerS[8] = {'2', '.', '4', 0, }; V2.2之前的数据结构是2015年7月之前的
	int8_t			SoftVerS[8];		// 软件版本
	int8_t			FPGAVerS[8];		// FPGA版本
	int8_t			DeviceNum[8];		// 机器编号
	uint32_t		Reserved[30];		// 保留
	W_D				startKm;
	uint32_t		startT;				// 开始作业时间：D23~D16ANGLEAnalysisPrint(srcdeviceP2.Place[i],pFile);	时,D15~D8分,D7~D0秒
	uint32_t		startD;				// 年月日
	uint32_t		endT;				// 结束作业时间：时分秒
	uint32_t		endD;				// 结束作业时间：年月日
	int32_t			distance;			// 总共行走多少距离, 步进
	uint32_t		block;				// 总共多少米块,
	uint32_t		alarm;				// 总共多少报警,保留
	uint32_t		wound;				// 总共多少伤损,保留
	uint32_t		run;				// 超速次数
	uint32_t		orderNum;			// 探伤命令号,暂定8位BCD,保留
	float			step;				// 每个步进多少毫米
	W_D				endKm;				// 结束里程
	ALL_P			deviceP;			// 仪器参数
	ALL_P2			deviceP2;			// 仪器参数2
	DGC				DGC_Sd[R_N][4];		// 所有DGC节点位置, 幅度,4种轨型，每种轨型4条，保留
	DGC				DGC_St[R_N][4];		// 强补偿，目前不用，保留。
	uint8_t			zero_AD[2];			// AD零点.
	uint8_t			rev8[2];			// 从此处开始100个字节保留作回放软件用。
	uint32_t		rev[395];			// 保留

};

//----------------------------------------------------------------------------------------
//tpA米块头
//----------------------------------------------------------------------------------------
typedef struct _BLOCK_WAVE {
	uint32_t	symbol;			// 米块标志 0xFFFFFFFF 所以要保证所有的数据小于0xFFFFFFFF
	int32_t		index;			// 米块起始步进，和B超中的步进相对应的
	uint32_t	len;			// 此米块数据压缩后多少字节

	uint16_t	checkD;			// 米块压缩后数据的校验和
	uint16_t	fNum;			// 此米块多少帧A超

	uint8_t		log;			// 操作日志v2.2
	uint8_t		rev;			// 保留
	uint16_t	rev1;			// 保留
}BLOCK_A;


//----------------------------------------------------------------------------------------
//tpB米块头，目前B超数据采用位压缩，米块头size应是的整数倍！！！！！
//----------------------------------------------------------------------------------------
typedef struct _BLOCK {
	uint32_t	symbol;			// 米块标志0xFFFFFFFF,所以要保证所有的其他数据小于0xFFFFFFFF，
	uint16_t	checkSum;		// 米块头效验和,保留, 位置不能改
	uint16_t	checkD;			// 米块数据的效验和
	int32_t		indexL;			// 当前步进
	uint32_t	len;			// 此米块数据压缩后字节数	
	uint32_t	time;			// 时间：高位至低位每8位依次为：日、时、分、秒
	uint16_t	row;			// 此米块多少个步进
	uint16_t	railNum;		// 钢轨编号,4个BCD码表示
	uint16_t	swNum;			// 道岔号，4个BCD表示，一般3位
	uint16_t	user;			// 工号,4个BCD
	DETECT		detectSet;		// 探测设置
	uint16_t	door[DOR_NUM][2];	// 小门设置，DOR_NUM=7，door[x][0]的最高位为on/off	
	uint16_t	addVal[CH_N];		// 累计值，CH_N=12 de通道不用，保留
	int8_t		probOff[CH_N];		// 探头位置偏移
	W_D			walk;			// 即时里程
	int16_t		speed;			// 速度，单位0.1km/h
	uint8_t		gpsInfor[32];	// GPS经纬度、UTC时间，字符串
	uint8_t		gain[CH_N];		// 12通道，0~160表示0~80.0dB，单位0.5dB。
	uint8_t		railType;		// BIT0~1: 轨型(0为43轨，1-50，2-60，3-75), BIT4:0逆里程、1顺里程，BIT5:0右股、1左股，BIT6~7：单线、上行、下行，其他预留
	uint8_t		railH;			// 当前轨高mm
	uint8_t		autoGain;		// BIT0~1:是否自动增益, BIT2~3探头选择，其他预留
	uint8_t		log;			// 操作日志
	uint8_t		volume;			// BIT0~6：音量32级，其他预留,高位可做显示模式
	uint8_t		BitS;			// BIT0:道岔右左，BIT1：道岔直曲，BIT2:道岔后前
	uint8_t		rev2;			// 用作站线的股道编号的个位
	uint8_t		rev3;			// 用作站线的股道编号的十位
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


//记录米块的起始位置
typedef struct _ReadFile_Info
{
	uint32_t	ReadL;
	uint32_t	UsedL;
}Read_Info;
typedef vector<Read_Info> VRI;

/************************************************************************/
/*                           导出数据格式                               */
/************************************************************************/

//B超中每行数据
typedef struct _B_RowData
{
	uint8_t		Row;			//行
	B_POINT		Point;			//画图信息
}B_RowData;

//B超每步进数据
typedef struct _B_Step
{
	//总的步进数
	int			Step;

	//总的步进数2，回退不减
	//int			Step2;		

	//标记
	B_MARK		Mark;

	//伤损
	B_WOUND		Wound;

	//步进数据
	vector<B_RowData>	vRowDatas;
}B_Step;


//A超每帧数据
typedef struct _A_Frame
{
	uint16_t	Horizon;	//横坐标
	uint8_t		Used;		//是否已使用
	uint16_t	F[CH_N];	//声波能量	
}A_Frame;

typedef struct _A_Step
{
	uint32_t		Block;		//米块索引
	int32_t			Step;		//步进偏移量
	uint16_t		Index;		//米块内帧ID
	int32_t			Index2;		//帧ID
	vector<A_Frame> Frames;
}A_Step;

typedef struct _BlockData_A
{
	vector<A_Step>		vAStepDatas;//A超帧数据
}BlockData_A;

typedef struct _BlockData_B
{
	BLOCK	BlockHead;			//米块头
	int		Index;				//米块索引，即第几个米块
	int		StepCount;			//米块中步进总数
	vector<B_Step>		vBStepDatas;//B超步进数据
}BlockData_B;


/*******************************************************/
//函数声明

//A超数据解析
uint32_t	Analyse_Achao(FILE* pFileA, BlockData_A& vADatas, vector<BlockData_B>& vBDatas, int32_t& readL, int &useL, int iFileSize, int iBeginBlock, int& iBeginFrame, vector<Read_Info>& vInfo, int iBlockCountToRead = 100);

//B超数据解析
uint32_t	Analyse_Bchao(FILE* pFileB, vector<BlockData_B>& vBDatas, uint32_t& use_Size, uint32_t& read_Size, int iFileSize, int iBeginBlock, int& iBeginStep, vector<Read_Info>& vInfo, int iBlockCountToRead = 100);

//校验和函数
uint16_t 	Check_Sum(uint16_t *p16Buffer, uint16_t uLength);

//B超解压函数
uint16_t	RLE64_Decompress_Loop2Loop(uint64_t *pDestBuf, uint32_t width, uint32_t height, uint32_t start, uint32_t L, uint8_t *pSorc, uint8_t *pHead, uint8_t *pTail, uint32_t size);

//A超解压函数
uint16_t 	RLE16_Decompress_Loop2Buf(uint16_t *pDest, uint16_t *pSrc, uint16_t *pSrcHead, uint16_t *pSrcTail, uint32_t uLength);


uint32_t	Analyse_Achao2(FILE* pFileA, BlockData_A& vADatas, uint32_t &useL, int iFileSize, int iBeginBlock, int iBeginStep, int& iBeginFrame, int iBlockCountToRead);

uint32_t	Analyse_Bchao2(FILE* pFileB, vector<BlockData_B>& vBDatas, uint32_t use_Size, int32_t iFileSize, int iBeginBlock, int iBeginStep, int iBlockCountToRead);

double		GetWalk(W_D wd);


#endif // ifnef