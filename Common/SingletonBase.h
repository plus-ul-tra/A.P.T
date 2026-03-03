#pragma once
#include <mutex>

template<typename T>
class SingletonBase
{
public:
	inline static T& Instance()
	{
		static T instance;
		return instance;
	}

protected:
	SingletonBase() = default;
	virtual ~SingletonBase() = default;

private:
	SingletonBase(const SingletonBase&)            = delete;
	SingletonBase& operator=(const SingletonBase&) = delete;
	SingletonBase(SingletonBase&&)                 = delete;
	SingletonBase& operator=(SingletonBase&&)      = delete;

	friend T;
};