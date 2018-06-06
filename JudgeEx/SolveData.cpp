// SolveA2.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "SolveData.h"
#include <stdlib.h>
#include <string.h>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define F_RLE	0x8000000000000000
#define ROW		68
#define COLUMN	1000



//FILE* pFileA = NULL;
//FILE* pFileB = NULL;

#define WAVE_LEN	512					// A超波形数据长度
enum _FILE_ACCESS
{
	WRITE_B_SIZE = 0x1000,
	WRITE_A_SIZE = 0x4000,
	WRITE_A_LEN = 0x2000, // B 超文文件一一次写入入 4K
	// A 超文文件一一次写入入 16K

	ACCESS_SIZE = 0x8000,
	ACCESS_LEN = 0x4000, // 32K 一一次获取字节大大小小

	// 一一次读写的⻓长度(16bit)
	CASH_N = 32, // Achao 共有多少个 cash(意思是)
	CASH_SIZE = (CASH_N*ACCESS_SIZE),//0x00100000 32 * 32 K = 1024 K 1M 空间 指的是 B 超的压缩后整体的最大大数据量量

	CASH_LEN = (CASH_SIZE >> 1),//0x00080000 512K 空间
};

uint16_t			g_wave_F0[WAVE_LEN*CH_N];				// 双端RAM中的回波数据
uint16_t			g_Cache_A[CASH_LEN];					// A超读取、写入缓存，M空间
static uint16_t		*pTail_A = g_Cache_A + CASH_LEN;

enum COMPRESS {
	F_CMPR = 0x8000,				// A超每帧压缩包标志，表示接下来的数据是一个压缩包

	MAX_ZERO = 0x3ff0,				// A超压缩
	F_ZERO = 0x4000,				// A超压缩标志，BIT14+x个(x<=MAX_ZERO)

	F_MASK = 0xC000,

	MSAK_L = 0x3fff,
};


B_POINT	g_BchaoBuf2[ROW*COLUMN];
int32_t g_ckB[30000];
int32_t	g_ckA[30000];


uint8_t	g_Cache_B[CASH_SIZE];			// B 超读取、写入入	缓存,1M 空间 是一一个介质,作为第一一次读取的数据,未加压的原始数据
uint8_t	*pTail_B = (g_Cache_B + CASH_SIZE);	//g_Cache_B[CASH_SIZE]数组对应的最后一一个地址

static int64_t	bufRLE[1024 * 12];


//A超分析函数-将A超文件分析，并将数据解压到一个txt文件中
//pInFile：原始数据文件
//pOutFile：解析后的文件;
uint32_t Analyse_Achao(FILE* pFileA, BlockData_A& vADatas, vector<BlockData_B>& vBDatas, int32_t& readL, int &useL, int iFileSize, int iBeginBlock, int& iBeginFrame, vector<Read_Info>& vInfo, int iBlockCountToRead /* = 100 */)
{
	uint16_t			*pData = NULL;
	uint16_t			*pDest = NULL;
	uint32_t			readN;
	uint32_t			isblock = 0, Num = 0;
	BLOCK_A				head;
	uint32_t			i, k, len;
	uint32_t			err = 0;
	uint32_t			errF = 0;
	uint16_t			sum;

	int n = 0, m = 0, Adataflag = 0;

	//定位到开始读取文件的位置，肯定是某个米块头
	//fseek(pFileA, iBeginIndexToRead, SEEK_SET);
	uint32_t	blockIndex = iBeginBlock;
	uint32_t s_FileL_A = (iFileSize - sizeof(F_HEAD)) >> 1;
	int iBlockRead = 0;
	while (iBlockRead < iBlockCountToRead)
	{
		if (useL >= s_FileL_A)
			break;

		if (readL < (useL + ACCESS_LEN) && readL < s_FileL_A)
		{
			fseek(pFileA, (readL << 1) + sizeof(F_HEAD), SEEK_SET);

			pDest = g_Cache_A + readL%CASH_LEN;
			readN = fread(pDest, 1, ACCESS_SIZE, pFileA);
			//printf("readN %d \n",readN);
			if (readN <= ACCESS_SIZE)
			{
				readL += (readN >> 1);
				if (readL >= s_FileL_A)
				{
					fclose(pFileA);
					pFileA = NULL;
				}
				else if (ACCESS_SIZE != readN)
				{
					err++;
				}
			}
			else
			{
				fclose(pFileA);
				pFileA = NULL;
				break;
			}
		}

		if (readL >= s_FileL_A || readL >= (useL + ACCESS_LEN))
		{
			pData = g_Cache_A + useL%CASH_LEN;
			while (1)
			{
				if (0xffff == *pData)
				{
					if (pData + 1 >= pTail_A)
					{
						if (0xffff == g_Cache_A[0])
						{
							isblock = 1;
							printf("一般不会出现这种情况\n");
							break;
						}
					}
					else
					{
						if (0xffff == *(pData + 1))
						{
							isblock = 1;
							break;
						}
					}
				}

				pData++;
				if (pData >= pTail_A)
					pData = g_Cache_A;

				useL++;
				if (useL >= readL)
				{
					isblock = 0;
					break;
				}
			}
		}

		if (isblock == 0)
		{
			continue;
		}

		Read_Info ri;
		ri.UsedL = useL;
		ri.ReadL = readL;
		vInfo.push_back(ri);

		isblock = 0;
		if (0 == g_ckB[0])
			err++;
		if (39 == Num)
			err++;

		pDest = (UINT16*)&head;
		for (i = 0; i < (sizeof(BLOCK_A) >> 1); ++i)
		{
			*pDest++ = *pData++;
			if (pData >= pTail_A)
				pData = g_Cache_A;
		}
		iBeginFrame += head.fNum;

		useL += (head.len + (sizeof(BLOCK_A) >> 1));

		k = 0;
		errF = 0;
		vector<int> iStepIndexs;
		for (i = 0; i < head.fNum; ++i)
		{
			if (k >= head.len)
				break;

			A_Step step;
			step.Block = blockIndex;
			step.Index = i;
			step.Index2 = iBeginFrame + i;
			if (*pData & F_CMPR)
			{
				len = *pData & MSAK_L;
				pData++;
				if (pData >= pTail_A)
					pData = g_Cache_A;

				step.Step = vBDatas[iBlockRead].vBStepDatas[0].Step + *pData;
				//step.Step2 = vBDatas[iBlockRead].vBStepDatas[0].Step2 + *pData;
				iStepIndexs.push_back(head.index + *pData);

				pData++;
				if (pData >= pTail_A)
					pData = g_Cache_A;
				sum = *pData;
				pData++;
				if (pData >= pTail_A)
					pData = g_Cache_A;
				if (sum != RLE16_Decompress_Loop2Buf(g_wave_F0, pData, g_Cache_A, pTail_A, len))
					errF++;
				else
				{

				}
				pData += len;
				k += len + 3;
			}
			else
			{
				while (1)
				{
					k++;
					pData++;
					if (pData >= pTail_A)
						pData = g_Cache_A;
					if (*pData & F_CMPR)
						break;
				}
			}

			A_Frame frame;
			for (int k = 0; k < WAVE_LEN; ++k)
			{
				frame.Horizon = k;
				uint32_t t = 0;
				for (int j = 0; j < CH_N; ++j)
				{
					frame.F[j] = g_wave_F0[j * WAVE_LEN + k];
					t += frame.F[j];
				}

				if (t > 0)
				{
					step.Frames.push_back(frame);
				}
			}

			if (step.Frames.size() > 0)
			{
				vADatas.vAStepDatas.push_back(step);
			}
		}

		if (errF)
			//	DebugMSG(1,(L"%d, %d\n", Num, head.index));
			g_ckA[Num] = head.index;
		Num++;

		++iBlockRead;
		++blockIndex;
		//DataA.Index = blockIndex++;
		//DataA.AFrameCount = head.fNum;
		//DataA.push_back(DataA);
	}
	return iBlockRead;
}
uint32_t Analyse_Achao2(FILE* pFileA, BlockData_A& vADatas, uint32_t &useL, int iFileSize, int iBeginBlock, int iBeginStep, int& iBeginFrame, int iBlockCountToRead)
{
	uint16_t			*pData = NULL;
	uint16_t			*pDest = NULL;
	uint32_t			readN;
	uint32_t			isblock = 0, Num = 0;
	uint32_t			i, k, len;
	uint32_t			err = 0;
	uint32_t			errF = 0;
	uint16_t			sum;

	int n = 0, m = 0, Adataflag = 0;

	//定位到开始读取文件的位置，肯定是某个米块头
	//fseek(pFileA, iBeginIndexToRead, SEEK_SET);
	uint32_t	blockIndex = iBeginBlock;

	uint32_t nFileHeadSize = sizeof(F_HEAD);
	int szHead = sizeof(BLOCK_A);
	uint32_t s_FileL_A = (iFileSize - sizeof(F_HEAD)) >> 1;
	int iBlockRead = 0;
	BLOCK_A head;

	while (iBlockRead < iBlockCountToRead)
	{
		if (useL >= s_FileL_A)
			break;

		fseek(pFileA, (useL << 1) + sizeof(F_HEAD), SEEK_SET);
		readN = fread(g_Cache_A, 1, ACCESS_SIZE, pFileA);

		pDest = (UINT16*)&head;
		pData = g_Cache_A;
		//memcpy(&head, g_Cache_A, szHead);
		for (i = 0; i < (sizeof(BLOCK_A) >> 1); ++i)
		{
			*pDest++ = *pData++;
			if (pData >= pTail_A)
				pData = g_Cache_A;
		}
		pData = g_Cache_A + (sizeof(BLOCK_A) >> 1);
		head.index = iBeginStep;
		//iBeginFrame += head.fNum;
		//TRACE("%d %d\n", blockIndex, head.len);
		useL += (head.len + (sizeof(BLOCK_A) >> 1));

		k = 0;
		errF = 0;
		vector<int> iStepIndexs;
		for (i = 0; i < head.fNum; ++i)
		{
			if (k >= head.len)
				break;

			A_Step step;
			step.Block = blockIndex;
			step.Index = i;
			step.Index2 = i + (iBeginFrame++);
			if (*pData & F_CMPR)
			{
				len = *pData & MSAK_L;
				pData++;
				if (pData >= pTail_A)
					pData = g_Cache_A;

				//step.Step = *pData;
				step.Step = iBeginStep + *pData;

				pData++;
				if (pData >= pTail_A)
					pData = g_Cache_A;
				sum = *pData;
				pData++;
				if (pData >= pTail_A)
					pData = g_Cache_A;
				uint16_t check_t = RLE16_Decompress_Loop2Buf(g_wave_F0, pData, g_Cache_A, pTail_A, len);
				if (sum != check_t)
					errF++;
				else
				{

				}
				pData += len;
				k += len + 3;
			}
			else
			{
				while (1)
				{
					k++;
					pData++;
					if (pData >= pTail_A)
						pData = g_Cache_A;
					if (*pData & F_CMPR)
						break;
				}
			}

			A_Frame frame;
			frame.Used = false;
			for (int k = 0; k < WAVE_LEN; ++k)
			{
				frame.Horizon = k;
				uint32_t t = 0;
				for (int j = 0; j < CH_N; ++j)
				{
					frame.F[j] = g_wave_F0[j * WAVE_LEN + k];
					t += frame.F[j];
				}

				if (t > 0)
				{
					step.Frames.push_back(frame);
				}
			}

			if (step.Frames.size() > 0)
			{
				vADatas.vAStepDatas.push_back(step);
			}
		}

		if (errF)
			g_ckA[Num] = head.index;
		Num++;

		++iBlockRead;
		++blockIndex;
	}
	return iBlockRead;
}



//B超解压文件函数
//pInFile：原始数据文件
//pOutFile：解析之后的文件 
uint32_t Analyse_Bchao(FILE* pFileB, vector<BlockData_B>& vBDatas, uint32_t& use_Size, uint32_t& read_Size, int iFileSize, int iBeginBlock, int& iBeginStep, vector<Read_Info>& vInfo, int iBlockCountToRead /* = 100 */)
{
	uint8_t			*pData = NULL;
	uint8_t			*pDest = NULL;
	uint32_t		readN;
	uint32_t		isblock = 0, Num = 0;
	uint16_t		sum;
	BLOCK			head;
	uint32_t		ii;
	uint32_t		err = 0;
	uint32_t		isRight;

	int nFileHeadSize = sizeof(F_HEAD);
	uint32_t s_File_SizeB = iFileSize - nFileHeadSize;

	uint32_t		StepIndex = 0;
	int				mikuai = 0;
	uint64_t		bujingmikuai = 0;

	//use_Size = iBeginIndexToRead - nFileHeadSize;

	int iBlockRead = 0;
	while (iBlockRead < iBlockCountToRead)
	{
		if (use_Size >= s_File_SizeB)
			break;

#pragma region 判断是否米块
		isblock = 0;

		if (read_Size < (use_Size + ACCESS_SIZE) && read_Size < s_File_SizeB)
		{
			fseek(pFileB, read_Size + nFileHeadSize, SEEK_SET);
			readN = fread(g_Cache_B + read_Size % CASH_SIZE, 1, ACCESS_SIZE, pFileB);
			if (readN >= 0)
			{
				read_Size += readN;
				if (read_Size >= s_File_SizeB)
				{
					fclose(pFileB);
					pFileB = NULL;

				}
				else if (ACCESS_SIZE != readN)
				{
					//err++;
				}
			}
			else
			{
				fclose(pFileB);
				pFileB = NULL;
				break;
			}
		}

		if (read_Size >= s_File_SizeB || read_Size >= (use_Size + ACCESS_SIZE))
		{
			pData = g_Cache_B + use_Size%CASH_SIZE;
			while (1)
			{
				if (0xff == *pData)
				{
					if (0xff == g_Cache_B[(use_Size + 1) % CASH_SIZE])
					{
						if (0xff == g_Cache_B[(use_Size + 2) % CASH_SIZE])
						{
							if (0xff == g_Cache_B[(use_Size + 3) % CASH_SIZE])
							{
								isblock = 1;
								break;
							}
						}
					}
				}

				pData++;
				if (pData >= pTail_B)
					pData = g_Cache_B;

				use_Size++;
				if (use_Size > read_Size)		// Èç¹ûÎÄŒþÃ»ÓÐ¶ÁÍê£¬Sleep
					continue;
			}
		}

		if (!isblock)
		{
			continue;
		}

		pDest = (uint8_t*)&head;
		for (ii = 0; ii < sizeof(BLOCK); ii++)
		{
			*pDest++ = *pData++;
			if (pData >= pTail_B)
				pData = g_Cache_B;
		}
		if (head.checkSum != Check_Sum(&head.checkD, (sizeof(BLOCK) - 6) >> 1))
		{
			use_Size += 4;
			TRACE("校验和失败\n");
			continue;
		}
#pragma endregion

		for (int i = 0; i < 12; ++i)
		{
			head.gain[i] = 160 - head.gain[i];
		}
		BlockData_B block;
		head.indexL = iBeginStep;
		block.BlockHead = head;
		block.Index = iBeginBlock + (mikuai++);
		block.StepCount = head.row;
		//fprintf(pFileOutB, "[%4d	%4d    %dKm%dm%dmm]\n", block.Index, head.indexL, head.walk.Km, head.walk.m, head.walk.mm);	

		sum = RLE64_Decompress_Loop2Loop((uint64_t*)g_BchaoBuf2, COLUMN, ROW, 0, head.row, pData, g_Cache_B, pTail_B, head.len);
		if (head.checkD != sum)
		{
			err++;
			mikuai--;
			use_Size += 4;
			continue;
		}
		else
		{
			Read_Info info;
			info.ReadL = read_Size;
			info.UsedL = use_Size;
			vInfo.push_back(info);

			for (int q = 0; q < head.row; ++q)
			{
				B_Step step;
				step.Step = iBeginStep++;
				//step.Step2 = ++iBeginStep;

				for (int qq = 0; qq < 66; ++qq)
				{
					B_RowData data;
					data.Row = qq;
					data.Point = g_BchaoBuf2[qq * COLUMN + q];
					if (data.Point.Draw1 > 0 || data.Point.Alarm > 0 || data.Point.Weight > 0 || data.Point.Draw2 > 0 || data.Point.Wound > 0)
					{
						step.vRowDatas.push_back(data);
					}
				}
				step.Mark = *(B_MARK*)&g_BchaoBuf2[66 * COLUMN + q];
				step.Wound = *(B_WOUND*)&g_BchaoBuf2[67 * COLUMN + q];
				block.vBStepDatas.push_back(step);
			}
		}

		use_Size += (head.len + sizeof(BLOCK));
		if (use_Size < s_File_SizeB)
		{
			pData = (g_Cache_B + use_Size%CASH_SIZE);

			if (0xff == *pData)
			{
				if (0xff == g_Cache_B[(use_Size + 1) % CASH_SIZE])
				{
					if (0xff == g_Cache_B[(use_Size + 2) % CASH_SIZE])
					{
						if (0xff == g_Cache_B[(use_Size + 3) % CASH_SIZE])
							isRight = 1;
					}
				}
			}
			if (0 == isRight)
			{
				use_Size -= head.len;
				continue;
			}
		}

		g_ckB[Num] = head.indexL;
		vBDatas.push_back(block);
		++iBlockRead;
		++Num;
	}
	return mikuai;
}

//B超解压文件函数
//pInFile：原始数据文件
//pOutFile：解析之后的文件 
uint32_t Analyse_Bchao2(FILE* pFileB, vector<BlockData_B>& vBDatas, uint32_t use_Size, int32_t iFileSize, int iBeginBlock, int iBeginStep, int iBlockCountToRead)
{
	uint8_t			*pData = NULL;
	uint8_t			*pDest = NULL;
	uint32_t		isblock = 0, Num = 0;
	uint16_t		sum;
	BLOCK			head;
	uint32_t		err = 0;

	int nFileHeadSize = sizeof(F_HEAD);
	uint32_t s_File_SizeB = iFileSize - nFileHeadSize;

	int szHead = sizeof(BLOCK);

	uint32_t		StepIndex = 0;
	int				mikuai = 0;
	uint64_t		bujingmikuai = 0;

	//use_Size = iBeginIndexToRead - nFileHeadSize;

	int iBlockRead = 0;
	int read_Size = 0;
	while (iBlockRead < iBlockCountToRead)
	{
		fseek(pFileB, use_Size + nFileHeadSize, SEEK_SET);
		read_Size = fread(&head, 1, szHead, pFileB);

		for (int i = 0; i < 12; ++i)
		{
			head.gain[i] = 160 - head.gain[i];
		}
		head.indexL = iBeginStep;
		BlockData_B block;
		block.BlockHead = head;
		block.Index = iBeginBlock + (mikuai++);
		block.StepCount = head.row;


		memset(g_Cache_B, 0, CASH_SIZE);
		fread(g_Cache_B, 1, head.len, pFileB);
		//fprintf(pFileOutB, "[%4d	%4d    %dKm%dm%dmm]\n", block.Index, head.indexL, head.walk.Km, head.walk.m, head.walk.mm);	
		pData = g_Cache_B;
		pTail_B = g_Cache_B + CASH_SIZE;
		sum = RLE64_Decompress_Loop2Loop((uint64_t*)g_BchaoBuf2, COLUMN, ROW, 0, head.row, pData, g_Cache_B, pTail_B, head.len);
		if (head.checkD != sum)
		{
			err++;
		}
		else
		{
			for (int q = 0; q < head.row; ++q)
			{
				B_Step step;
				step.Step = head.indexL + q;

				for (int qq = 0; qq < 66; ++qq)
				{
					B_RowData data;
					data.Row = qq;
					data.Point = g_BchaoBuf2[qq * COLUMN + q];
					if (data.Point.Draw1 > 0 || data.Point.Alarm > 0 || data.Point.Weight > 0 || data.Point.Draw2 > 0 || data.Point.Wound > 0)
					{
						step.vRowDatas.push_back(data);
					}
				}
				step.Mark = *(B_MARK*)&g_BchaoBuf2[66 * COLUMN + q];
				step.Wound = *(B_WOUND*)&g_BchaoBuf2[67 * COLUMN + q];
				block.vBStepDatas.push_back(step);
			}
		}

		g_ckB[Num] = head.indexL;
		vBDatas.push_back(block);
		++iBlockRead;
		++Num;

		use_Size += szHead + head.len;
	}
	return mikuai;
}

uint16_t Check_Sum(uint16_t *p16Buffer, uint16_t uLength)
{
	uint32_t uSum = 0;

	while (uLength--)
	{
		uSum += *p16Buffer++;
		if (uSum & 0x80000000)
			uSum = (uSum & 0xffff) + (uSum >> 16);
	}

	while (uSum >> 16)
		uSum = (uSum & 0xffff) + (uSum >> 16);

	if (uSum & 0x8000)
		uSum = (uSum & 0xfff) + (uSum >> 12);

	return (uint16_t)uSum;

}

//B超解压函数
uint16_t RLE64_Decompress_Loop2Loop(uint64_t* pDestBuf, uint32_t width, uint32_t height, uint32_t start, uint32_t L, uint8_t *pSorc, uint8_t *pHead, uint8_t *pTail, uint32_t size)
{
	uint64_t	i, k, N;
	uint64_t	*pDest;
	uint64_t	data;
	uint32_t		H = 0, row = 0;
	uint8_t		*pBuf8;
	uint64_t	*pRLE;
	uint32_t		RLE_n = 0;
	uint32_t		u32Sum = 0;

	int myflag1 = 0;
	int myflag2 = 0;
	// xian
	pBuf8 = (uint8_t*)bufRLE;

	if (size > 1024 * 12 * 8 || L > 1000)
	{
		printf("记录米块尺寸过大\n");
		return 0;
	}

	for (i = 0; i < size; i++)//pSorc存储的是米块头的地址还是米块数据的开始地址 size 此米块数据压缩后多长, 采用bit压缩，所以len的单位为Byte
	{
		if (*pSorc)
		{
			u32Sum += *pSorc;
			*pBuf8++ = *pSorc++;
			if (pSorc >= pTail)
				pSorc = pHead;
		}
		else
		{
			*pSorc++;
			if (pSorc >= pTail)
				pSorc = pHead;

			for (k = 0; k < *pSorc; k++)
				*pBuf8++ = 0;

			pSorc++;
			if (pSorc >= pTail)
				pSorc = pHead;
			i++;
		}
	}

	while (u32Sum >> 16)
		u32Sum = (u32Sum & 0xffff) + (u32Sum >> 16);

	if (u32Sum & 0x8000)
		u32Sum = (u32Sum & 0xfff) + (u32Sum >> 12);

	pRLE = (uint64_t *)bufRLE;
	pDest = pDestBuf + start;
	data = *pRLE;
	RLE_n = (pBuf8 - (uint8_t*)bufRLE) >> 3;
	//printf("每个米块对应多少个点%d\n",RLE_n);
	for (i = 0; i < RLE_n; i++)
	{
		if (0 == (*pRLE&F_RLE))//0x8000000000000000  读取当前字节的最高位，如果为1 则表示为压缩的数据，如果不是，则表示未压缩
		{
			//对压缩的数据进行存储。
			data = *pRLE;
			*pDest++ = data;
			row++;
			if (row >= L)//L存的是米块有多少列，340列，读完340列的时候，就转到下1000行进行存储数据
			{
				myflag1++;
				row = 0;
				H++;
				if (H >= height)
				{
					//printf("!!!!!!!!!!!!!出现意外！！！！！！！！！！ %d\n RLE_n %d i %I64d",H,RLE_n,i);					
					return (uint16_t)u32Sum;
				}
				pDest = pDestBuf + start + H*width;
			}
			if (pDest >= pDestBuf + (H + 1)*width)
			{
				pDest = pDestBuf + H*width;
			}
		}
		else
		{
			//对压缩数据进行解压操作
			N = (*pRLE & (~F_RLE));
			//printf("循环次数%I64d  data%I64d\n",N,data);
			for (k = 0; k < N; k++)
			{
				*pDest++ = data;
				row++;
				if (row >= L)  //L = head.row记录着行数 ---------------
				{
					myflag2++;
					row = 0;
					H++;
					if (H >= height)//height存的是68,说明要进去多少次 解析的数据已经超过68列，有错误
					{
						//printf("!!!!!!!!!!!!!出现意外！！！！！！！！！！ %d\n RLE_n %d i %I64d",H,RLE_n,i);								
						return (uint16_t)u32Sum;
					}
					pDest = pDestBuf + start + H*width;//width是1000
				}
				if (pDest >= pDestBuf + (H + 1)*width)
				{
					pDest = pDestBuf + H*width;
				}
			}
		}

		pRLE++;
	}
	//printf("myflag1	%d\n myflag2	%d\n",myflag1,myflag2);
	return (uint16_t)u32Sum;
}

//校验和函数
uint16_t RLE16_Decompress_Loop2Buf(uint16_t *pDest, uint16_t *pSrc, uint16_t *pSrcHead, uint16_t *pSrcTail, uint32_t uLength)
{
	uint32_t		i, cnt;
	uint32_t		u32Sum = 0;
	uint16_t		*pDestTail = NULL;

	pDestTail = pDest + 512 * 12;
	for (i = 0; i < uLength; i++)
	{
		if (*pSrc & F_ZERO)//0x4000
		{
			cnt = *pSrc & MSAK_L;//0x3fff
			memset(pDest, 0x0, 2 * cnt);
			pDest += cnt;
		}
		else
		{
			*pDest++ = *pSrc;
			u32Sum += *pSrc;
		}
		pSrc++;
		if (pSrc >= pSrcTail)
			pSrc = pSrcHead;
		if (pDest > pDestTail)
			break;
	}

	while (u32Sum >> 16)
		u32Sum = (u32Sum & 0xffff) + (u32Sum >> 16);

	if (u32Sum & 0x8000)		//避免最高位为，与压缩标志等冲突方便统一压缩。
		u32Sum = (u32Sum & 0xfff) + (u32Sum >> 12);

	return u32Sum;
}

double		GetWalk(W_D wd)
{
	return wd.Km + 0.001 * wd.m + 0.000001 * wd.mm;
}