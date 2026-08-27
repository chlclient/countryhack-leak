#pragma once
#include "../../../SDK/SDK.h"

//#define SEEDPRED_DEBUG

namespace DebugJitter
{
	inline double g_dLastRTT = 0.0;
	inline double g_dJitter = 0.0;
}

class CNoSpreadHitscan
{
public:
	bool ShouldRun(CTFWeaponBase* pWeapon = nullptr);
	int GetSeed(CUserCmd* pCmd);
	float CalcMantissaStep(float flV);
	std::string GetFormat(int iServerTime);

	bool m_bWaitingForPlayerPerf = false;
	int m_bSynced = 0;
	double m_dRequestTime = 0.0;
	float m_flServerTime = 0.f;
	double m_dTimeDelta = 0.0;
	std::deque<double> m_vTimeDeltas = {};

public:
	void Reset();

	void AskForPlayerPerf();
	bool ParsePlayerPerf(const std::string& sMsg);

	void Run(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd);
	void Draw(CTFPlayer* pLocal);
	
	float Spread = 0.f;
	int m_iSeed = 0;
	float m_flMantissaStep = 0.f;
	double CalculateJitter();
};

ADD_FEATURE(CNoSpreadHitscan, NoSpreadHitscan);