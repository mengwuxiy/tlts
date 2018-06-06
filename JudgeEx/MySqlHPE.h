#pragma once
#ifndef MYSQL_HPE
#define MYSQL_HPE

#include "mysql.h"

#include "Judge.h"

class CMySqlHPE
{
public:
	CMySqlHPE();
	~CMySqlHPE();

public:
	bool ConnectDatabase(CString host, CString user, CString pwd, CString database);

	bool CloseConn();

	bool Query(CString strSql, MYSQL_RES** rs);

	bool Execute(CString strSql);

	const char* GetErrorMsg();

	uint64_t	GetTimeStamp(char* pStr);

	CString		ToHexCString(int data, int length);

	CString		ToCString(int data, int length);

public:
	bool		IsConnected(){ return m_bConnected; }
protected:
	MYSQL		m_conn;

	bool		m_bConnected;

	uint64_t	GetLastInsertID();

	CString		GetTimeString(uint32_t date, uint32_t time = 0);

	CString		GetGwdNo(CString railNo, uint8_t xingbie, double startwd);

public:
	CString		GetFileName(CString strFile, F_HEAD& head, uint16_t& gwd, double& gps_log, double& gps_lat);

public:
	bool	Addwork(CString strFile, F_HEAD& head, uint64_t& fileID, CString& strGwdNo, double& gps_log, double& gps_lat);

	bool	GetBlocks(uint64_t fileID, vector<BLOCK_SQL>& blocks);

	bool	AddBlocks(vector<BLOCK>& blocks, vector<Read_Info>& vA, vector<Read_Info>& vB);

	bool	AddSteps_A(vector<A_Step>& vSteps);

	bool	AddSteps_B(vector<WaveData>* vDatas);

public:
	bool	AddCRs(vector<Connected_Region> & vcr);

	bool	UpdateCR(Connected_Region& cr);

public:
	bool	AddPositionMarks(vector<Position_Mark>& vPMs);

	bool	GetPositionMarks(CString strRailNo, uint8_t xingbie, uint8_t gubie, double start, double end, vector<Position_Mark>& vPMs);

public:
	bool	GetWound(uint64_t id, Wound_Judged& wd);

	bool	AddWounds(vector<Wound_Judged>& vWounds);

	bool	ModifyWound(int woundID, Wound_Judged& wd);

	bool	DeleteWound(int woundID);

public:
	bool	GetCurves(vector<Curve>& curves, CString railNo, uint8_t xingbie, double start, double end);

	bool	GetBridges(vector<Bridge>& bridges, CString railNo, uint8_t xingbie, double start, double end);

	bool	GetTunnels(vector<Tunnel>& tunnels, CString railNo, uint8_t xingbie, double start, double end);
};

#endif