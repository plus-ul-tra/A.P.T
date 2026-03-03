#ifndef GAMETIMER_H
#define GAMETIMER_H

#include <chrono>

class GameTimer
{
public:
	GameTimer();

	float TotalTime()   const;
	float DeltaTime()   const;
	float DeltaTimeMs() const;

	void Reset();
	void Start();
	void Stop();
	void Tick();

private:
	using Clock = std::chrono::high_resolution_clock;
	using TimePoint = Clock::time_point;
	using Duration = std::chrono::duration<float>;

	TimePoint m_StartTime;
	TimePoint m_StopTime;
	TimePoint m_PrevTime;
	TimePoint m_CurrTime;
	Duration m_PausedDuration;

	Duration m_DeltaTime;
	bool m_Stopped = false;
};

#endif // GAMETIMER_H

