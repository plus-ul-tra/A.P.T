#include "pch.h"
#include "GameTimer.h"

GameTimer::GameTimer() 
{
	Reset();
}

float GameTimer::TotalTime() const
{
	if (m_Stopped)
	{
		return std::chrono::duration<float>(m_StopTime - m_StartTime - m_PausedDuration).count();
	}
	else
	{
		return std::chrono::duration<float>(Clock::now() - m_StartTime - m_PausedDuration).count();
	}
}

float GameTimer::DeltaTime() const
{
	return m_DeltaTime.count();
}

float GameTimer::DeltaTimeMs() const
{
	return m_DeltaTime.count() * 1000.0f;
}

void GameTimer::Reset()
{
	m_StartTime = Clock::now();
	m_PrevTime = m_StartTime;
	m_PausedDuration = Duration::zero();
	m_Stopped = false;
}

void GameTimer::Start()
{
	if (m_Stopped)
	{
		auto startAgain = Clock::now();
		m_PausedDuration += startAgain - m_StopTime;
		m_PrevTime = startAgain;
		m_Stopped = false;
	}
}

void GameTimer::Stop()
{
	if (!m_Stopped)
	{
		m_StopTime = Clock::now();
		m_Stopped = true;
	}
}

void GameTimer::Tick()
{
	if (m_Stopped)
	{
		m_DeltaTime = Duration::zero();
		return;
	}

	m_CurrTime = Clock::now();
	m_DeltaTime = m_CurrTime - m_PrevTime;
	m_PrevTime = m_CurrTime;

	if (m_DeltaTime.count() < 0.0f)
	{
		m_DeltaTime = Duration::zero();
	}
}
