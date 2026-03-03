#pragma once
#include <random>
#include <cstdint>
#include <array>

enum class RandomDomain : uint8_t { Combat, Loot, Shop, World, COUNT };

class RandomMachine
{
public:
	RandomMachine();
	explicit RandomMachine(uint32_t seed);

	void     SetSeed(uint32_t seed);
	uint32_t GetSeed() const		{ return m_Seed; }

	// 기본: World 스트림을 기본 스트림으로 사용
	uint32_t NextUInt(RandomDomain domain = RandomDomain::World);
	int      NextInt(int minInclusive, int maxInclusive, RandomDomain domain = RandomDomain::World);
	float    NextFloat(float minInclusive, float maxInclusive, RandomDomain domain = RandomDomain::World);

	std::mt19937& GetStream(RandomDomain domain);
private:
	void ReseedAll();

	static uint32_t DeriveSeed(uint32_t masterSeed, uint32_t domainIndex);

private:
	uint32_t     m_Seed = 0;
	std::array<std::mt19937, static_cast<size_t>(RandomDomain::COUNT)> m_Streams{};
};

