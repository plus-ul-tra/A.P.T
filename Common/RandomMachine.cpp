#include "RandomMachine.h"
#include <numeric>

RandomMachine::RandomMachine()
{
	std::random_device device;
	SetSeed(device());
}

RandomMachine::RandomMachine(uint32_t seed)
{
	SetSeed(seed);
}

void RandomMachine::SetSeed(uint32_t seed)
{
	m_Seed = seed;
	ReseedAll();
}

uint32_t RandomMachine::NextUInt(RandomDomain domain)
{
	return GetStream(domain)();
}

int RandomMachine::NextInt(int minInclusive, int maxInclusive, RandomDomain domain)
{
	if (minInclusive > maxInclusive)
		std::swap(minInclusive, maxInclusive);

	std::uniform_int_distribution<int> dist(minInclusive, maxInclusive);
	return dist(GetStream(domain));
}

float RandomMachine::NextFloat(float minInclusive, float maxInclusive, RandomDomain domain)
{
	if (minInclusive > maxInclusive)
		std::swap(minInclusive, maxInclusive);

	std::uniform_real_distribution<float> dist(minInclusive, maxInclusive);
	return dist(GetStream(domain));
}

std::mt19937& RandomMachine::GetStream(RandomDomain domain)
{
	return m_Streams[static_cast<size_t>(domain)];
}

void RandomMachine::ReseedAll()
{
	for (uint32_t i = 0; i < static_cast<uint32_t>(RandomDomain::COUNT); ++i)
	{
		const uint32_t derived = DeriveSeed(m_Seed, i);
		m_Streams[i].seed(derived);
	}
}

// 간단한 32-bit 믹스(도메인별 파생 시드)
uint32_t RandomMachine::DeriveSeed(uint32_t masterSeed, uint32_t domainIndex)
{
	// masterSeed와 domainIndex를 섞어서 서로 다른 스트림 시드 생성
	uint32_t x = masterSeed ^ (0x9E3779B9u * (domainIndex + 1u));
	x ^= x >> 16;
	x *= 0x7FEB352Du;
	x ^= x >> 15;
	x *= 0x846CA68Bu;
	x ^= x >> 16;
	return x;
}
