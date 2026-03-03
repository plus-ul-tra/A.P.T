#pragma once

#include <algorithm>
#include <cmath>

namespace BlendCurveFunc
{
	constexpr float PI = 3.14159265358979323846f;

	inline float Clamp01(float t)		// 입력을 [0, 1]로 고정
	{
		return std::clamp(t, 0.0f, 1.0f);
	}

	// -----------------------------------------------------------------------------
	// Sine
	// -----------------------------------------------------------------------------

	// Sine In: 시작이 느리고 끝으로 갈수록 가속
	inline float EaseInSine(float t)	
	{
		t = Clamp01(t);

		const float halfPi = PI * 0.5f;
		const float angle = t * halfPi;			// [0..pi/2]

		return 1.0f - std::cos(angle);
	}

	// Sine Out: 시작이 빠르고 끝으로 갈수록 감속
	inline float EaseOutSine(float t)  
	{
		t = Clamp01(t);

		const float halfPi = PI * 0.5f;
		const float angle = t * halfPi;			// [0..pi/2]

		return std::sin(angle);
	}

	// Sine InOut: 시작/끝은 느리고 중간이 빠름
	inline float EaseInOutSine(float t)	
	{
		t = Clamp01(t);

		const float angle = t * PI;				// [0..pi]

		// (1 - cos(pi*t)) / 2 와 동일
		return 0.5f * (1.0f - std::cos(angle));
	}


	// -----------------------------------------------------------------------------
	// Quad (t^2)
	// -----------------------------------------------------------------------------

	// Quad In: t^2로 가속
	inline float EaseInQuad(float t)
	{
		t = Clamp01(t);
		return t * t;				    // pow보다 빠름
	}

	// Quad Out: 1 - (1-t)^2 로 감속
	inline float EaseOutQuad(float t)
	{
		t = Clamp01(t);

		const float oneMinusT  = 1.0f - t;
		const float oneMinusT2 = oneMinusT * oneMinusT;

		return 1.0f - oneMinusT2;
	}

	// Quad InOut: 전반 가속, 후반 감속
	inline float EaseInOutQuad(float t)
	{
		t = Clamp01(t);

		const float inv = -2.0f * t + 2.0f;
		const float inv2 = inv * inv;

		return (t < 0.5f)
			? 2.0f * t * t
			: 1.0f - inv2 * 0.5f;
	}


	// -----------------------------------------------------------------------------
	// Cubic (t^3)
	// -----------------------------------------------------------------------------

	// Cubic In: t^3로 더 강한 가속
	inline float EaseInCubic(float t)
	{
		t = Clamp01(t);

		const float t2 = t * t;
		return t2 * t; 
	}

	// Cubic Out: 1 - (1-t)^3 로 더 강한 감속
	inline float EaseOutCubic(float t)	
	{
		t = Clamp01(t);

		const float oneMinusT  = 1.0f - t;
		const float oneMinusT2 = oneMinusT  * oneMinusT;
		const float oneMinusT3 = oneMinusT2 * oneMinusT;

		return 1.0f - oneMinusT3;
	}

	// Cubic InOut: 부드럽게 가속 후 감속
	inline float EaseInOutCubic(float t)
	{
		t = Clamp01(t);

		const float t2 = t  * t;
		const float t3 = t2 * t;

		const float twoMinus2T  = -2.0f * t + 2.0f;
		const float twoMinus2T2 = twoMinus2T * twoMinus2T;
		const float twoMinus2T3 = twoMinus2T2 * twoMinus2T;

		return (t < 0.5f)
			? 4.0f * t3
			: 1.0f - twoMinus2T3 * 0.5f;
	}


	// -----------------------------------------------------------------------------
	// Quart (t^4)
	// -----------------------------------------------------------------------------

	// Quart In: t^4로 가속 강도 증가
	inline float EaseInQuart(float t)
	{
		t = Clamp01(t);

		const float t2 = t * t;

		return t2 * t2;
	}

	// Quart Out: 1 - (1-t)^4 로 감속 강도 증가
	inline float EaseOutQuart(float t)	
	{
		t = Clamp01(t);

		const float oneMinusT  = 1.0f - t;
		const float oneMinusT2 = oneMinusT  * oneMinusT;
		const float oneMinusT4 = oneMinusT2 * oneMinusT2;

		return 1.0f - oneMinusT4;
	}

	// Quart InOut: 가속/감속
	inline float EaseInOutQuart(float t)
	{
		t = Clamp01(t);

		const float t2 = t * t;
		const float t4 = t2 * t2;

		const float oneMinusT = 1.0f - t;
		const float oneMinusT2 = oneMinusT * oneMinusT;
		const float oneMinusT4 = oneMinusT2 * oneMinusT2;

		return (t < 0.5f)
			? 8.0f * t4
			: 1.0f - oneMinusT4 * 0.5f;
	}


	// -----------------------------------------------------------------------------
	// Quint (t^5)
	// -----------------------------------------------------------------------------

	// Quint In: t^5로 매우 강한 가속
	inline float EaseInQuint(float t)
	{
		t = Clamp01(t);

		const float t2 = t  * t;
		const float t4 = t2 * t2;

		return t4 * t;
	}

	// Quint Out: 1 - (1-t)^5 로 매우 강한 감속
	inline float EaseOutQuint(float t)	
	{
		t = Clamp01(t);

		const float oneMinusT  = 1.0f - t;
		const float oneMinusT2 = oneMinusT  * oneMinusT;
		const float oneMinusT4 = oneMinusT2 * oneMinusT2;
		const float oneMinusT5 = oneMinusT4 * oneMinusT;

		return 1.0f - oneMinusT5;
	}

	// Quint InOut: 강하게 가속 후 강하게 감속
	inline float EaseInOutQuint(float t)
	{
		t = Clamp01(t);

		const float t2 = t  * t;
		const float t4 = t2 * t2;
		
		const float oneMinusT = 1.0f - t;
		const float oneMinusT2 = oneMinusT * oneMinusT;
		const float oneMinusT4 = oneMinusT2 * oneMinusT2;
		const float oneMinusT5 = oneMinusT4 * oneMinusT;
		return (t < 0.5f)
			? 16.0f * t4 * t
			: 1.0f - oneMinusT5 * 0.5f;
	}


	// -----------------------------------------------------------------------------
	// Expo (2^x)
	// -----------------------------------------------------------------------------

	// Expo In: 초반 매우 느리고 이후 급가속
	inline float EaseInExpo(float t)
	{
		t = Clamp01(t);

		// 0에서 불연속을 피하기 위해 예외 처리
		if (t == 0.0f) return 0.0f;

		const float exponent = 10.0f * t - 10.0f; // [-10..0]
		return std::pow(2.0f, exponent);
	}


	// Expo Out: 초반 빠르고 갈수록 매우 느림
	inline float EaseOutExpo(float t)	
	{
		t = Clamp01(t);

		// 1에서 불연속을 피하기 위해 예외 처리
		if (t == 1.0f) return 1.0f;

		const float exponent = -10.0f * t;		  // [0..-10]
		return 1.0f - std::pow(2.0f, exponent);
	}

	// Expo InOut: 중간이 빠르고 양끝은 완만
	inline float EaseInOutExpo(float t) 
	{
		t = Clamp01(t);

		// 양끝 불연속 방지
		if (t == 0.0f) return 0.0f;
		if (t == 1.0f) return 1.0f;

		const float exponent1 =  20.0f * t - 10.0f; // [-10..0)
		const float exponent2 = -20.0f * t + 10.0f; // (0..-10]

		return (t < 0.5f)
			? 0.5f * std::pow(2.0f, exponent1)
			: 0.5f * (2.0f - std::pow(2.0f, exponent2));
	}


	// -----------------------------------------------------------------------------
	// Circ (원호)
	// -----------------------------------------------------------------------------

	// Circ In: 원호 형태로 부드럽게 시작
	inline float EaseInCirc(float t)
	{
		t = Clamp01(t);

		const float t2 = t * t;
		return 1.0f - std::sqrt(1.0f - t2);
	}

	// Circ Out: 원호 형태로 부드럽게 종료
	inline float EaseOutCirc(float t)	
	{
		t = Clamp01(t);

		const float oneMinusT  = t - 1.0f;
		const float oneMinusT2 = oneMinusT * oneMinusT;
		return std::sqrt(1.0f - oneMinusT2);
	}

	// Circ InOut: 원호 기반 가속 후 감속
	inline float EaseInOutCirc(float t) 
	{
		t = Clamp01(t);

		const float twoT = 2.0f * t;

		const float twoOneMinusT = -2.0f * t + 2.0f; // == 2(1-t)
		const float twoOneMinusT2 = twoOneMinusT * twoOneMinusT;
		return (t < 0.5f)
			? 0.5f * (1.0f - std::sqrt(1.0f - twoT * twoT))
			: 0.5f * (std::sqrt(1.0f - twoOneMinusT2) + 1.0f);
	}

	// -----------------------------------------------------------------------------
	// Back (오버슈트)
	// -----------------------------------------------------------------------------

	// Back In: 시작에서 뒤로 살짝 튀었다가 전진 가속
	inline float EaseInBack(float t)
	{
		t = Clamp01(t);

		const float overshoot = 1.70158f;			     // 오버슈트 강도
		const float overshootPlus1 = overshoot + 1.0f;

		const float t2 = t  * t;
		const float t3 = t2 * t;

		return overshootPlus1 * t3 - overshoot * t2;
	}

	// Back Out: 끝에서 살짝 오버슈트 후 되돌아옴
	inline float EaseOutBack(float t)	
	{
		t = Clamp01(t);
		const float overshoot = 1.70158f;
		const float overshootPlus1 = overshoot + 1.0f;

		const float u = 2.0f * t - 2.0f;           // [-2, 0] 구간으로 이동된 시간(끝 기준)
		const float u2 = u * u;
		const float u3 = u2 * u;

		return 1.0f + overshootPlus1 * u3 + overshoot * u2;
	}

	// Back InOut: 시작/끝 오버슈트, 중간은 부드럽게
	inline float EaseInOutBack(float t)
	{
		t = Clamp01(t);
		const float overshoot = 1.70158f;
		const float overshootInOut = overshoot * 1.525f; // InOut용 보정

		const float twoT = 2.0f * t;
		const float twoT2 = twoT * twoT;

		const float u = 2.0f * t - 2.0f;         // [-1, 0] 구간
		const float u2 = u * u;
		return (t < 0.5f)
			? 0.5f * (twoT2 * ((overshootInOut + 1.0f) * twoT - overshootInOut))
			: 0.5f * (u2 * ((overshootInOut + 1.0f) * u + overshootInOut) + 2.0f);
	}


	// -----------------------------------------------------------------------------
	// Elastic (탄성)
	// -----------------------------------------------------------------------------

	// Elastic In: 탄성 흔들림 후 가속 시작
	inline float EaseInElastic(float t)	
	{
		t = Clamp01(t);


		// 양끝 불연속 방지
		if (t == 0.0f || t == 1.0f)
			return t;

		const float angularFreq    = (2.0f * PI) / 3.0f;	         	// 진동 각주파수(기본 주기)
		const float amplitudeScale = std::pow(2.0f, 10.0f * t - 10.0f); // 진폭 스케일(지수)

		const float phase = (t * 10.0f - 10.75f) * angularFreq;

		return -amplitudeScale * std::sin(phase);
	}

	// Elastic Out: 감속 후 탄성 흔들림으로 종료
	inline float EaseOutElastic(float t) 
	{
		t = Clamp01(t);

		//양끝 불연속 방지
		if (t == 0.0f || t == 1.0f)
			return t;

		const float angularFreq = (2.0f * PI) / 3.0f;			 // 진동 각주파수(기본 주기)
		const float amplitudeScale = std::pow(2.0f, -10.0f * t);		 // 진폭 스케일(지수)

		const float phase = (t * 10.0f - 0.75f) * angularFreq;

		return amplitudeScale * std::sin(phase) + 1.0f;
	}

	// Elastic InOut: 시작/끝에 흔들림, 중간은 부드럽게
	inline float EaseInOutElastic(float t) 
	{
		t = Clamp01(t);
		if (t == 0.0f || t == 1.0f)
			return t;

		const float angularFreq = (2.0f * PI) / 4.5f;

		// 앞/뒤 구간 지수항(성능상 분리)
		const float amplitudeScaleIn = std::pow(2.0f, 20.0f * t - 10.0f);
		const float amplitudeScaleOut = std::pow(2.0f, -20.0f * t + 10.0f);

		const float phase = (t * 20.0f - 11.125f) * angularFreq;

		return (t < 0.5f)
			? -0.5f * amplitudeScaleIn  * std::sin(phase)
			:  0.5f * amplitudeScaleOut * std::sin(phase) + 1.0f;
	}

	// -----------------------------------------------------------------------------
	// Bounce (바운스)
	// -----------------------------------------------------------------------------

	// Bounce Out: 끝으로 갈수록 바운스
	inline float EaseOutBounce(float t) 
	{
		t = Clamp01(t);
	

		// 표준 상수(고정된 바운스 형태)
		const float bounceFactor   = 7.5625f;
		const float segmentDivisor = 2.75f;

		if (t < 1.0f / segmentDivisor)
		{
			// 첫 구간
			return bounceFactor * t * t;
		}
			
		if (t < 2.0f / segmentDivisor)
		{
			// 두 번째 구간
			const float tShifted = t - (1.5f / segmentDivisor);
			return bounceFactor * tShifted * tShifted + 0.75f;
		}
		if (t < 2.5f / segmentDivisor)
		{
			// 세 번째 구간
			const float tShifted = t - (2.25f / segmentDivisor);
			return bounceFactor * tShifted * tShifted + 0.9375f;
		}
		else
		{
			// 네 번째 구간
			const float tShifted = t - (2.625f / segmentDivisor);
			return bounceFactor * tShifted * tShifted + 0.984375f;
		}
	}

	// Bounce In: Out 바운스를 반대로 적용
	inline float EaseInBounce(float t)	
	{
		t = Clamp01(t);

		return 1.0f - EaseOutBounce(1.0f - t);
	}


	// Bounce InOut: 시작/끝은 바운스, 중간은 부드럽게
	inline float EaseInOutBounce(float t) 
	{
		t = Clamp01(t);

		return (t < 0.5f)
			? 0.5f * (1.0f - EaseOutBounce(1.0f - 2.0f * t))
			: 0.5f * (1.0f + EaseOutBounce(2.0f * t - 1.0f));
		// 0 ~ 0.5 InBounce / 0.5 ~ 1 OutBounce
	}
}