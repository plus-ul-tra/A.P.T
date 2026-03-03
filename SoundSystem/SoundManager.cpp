#include <filesystem>
#include "SoundManager.h"
#include "Fmod_Error.h"
#include <unordered_map>
#include <chrono>

constexpr int ChannelCount = 128;

bool SoundManager::Init()
{
	FMOD_RESULT result = FMOD::System_Create(&m_CoreSystem);
	if (result != FMOD_OK) { FMOD_LOG(result); FMOD_ASSERT(result); return false; }

	//3D Sound 추가시 수정
	result = m_CoreSystem->init(ChannelCount, FMOD_INIT_NORMAL, nullptr);
	if (result != FMOD_OK) { FMOD_LOG(result); FMOD_ASSERT(result); return false; }
	 
	m_CoreSystem->getMasterChannelGroup(&m_MasterGroup);
	m_CoreSystem->createChannelGroup("BGM", &m_BGMGroup);
	m_CoreSystem->createChannelGroup("SFX", &m_SFXGroup);
	//m_CoreSystem->createChannelGroup("UI", &m_UIGroup);

	m_MasterGroup->addGroup(m_BGMGroup);
	m_MasterGroup->addGroup(m_SFXGroup);
	//m_MasterGroup->addGroup(m_UIGroup);

	//CreateBGMSource(m_SoundAssetManager.GetBGMPaths());
	//CreateSFXSource(m_SoundAssetManager.GetSFXPaths());
	//CreateUISource(m_SoundAssetManager.GetUISoundPaths());

	return true;
}

SoundManager::~SoundManager()
{
	m_BGMGroup->release();
	m_SFXGroup->release();
	//m_UIGroup->release();
	m_MasterGroup->release();
	m_CoreSystem->release();
}

void SoundManager::SetDirty()
{
	m_SoundDirty = true;
	m_Dirty_BGM = true;
	m_Dirty_SFX = true;
	//m_Dirty_UI = true;
}

//3D 설정시 변경 //FMOD_3D
void SoundManager::CreateBGMSource(const std::unordered_map<std::wstring, std::filesystem::path>& bgms)
{
	for (auto& bgm : bgms)
	{
		FMOD::Sound* temp;
		m_CoreSystem->createSound(bgm.second.string().c_str(), FMOD_LOOP_NORMAL | FMOD_2D, nullptr, &temp);

		m_BGMs.emplace(bgm.first, temp);
	}
}

void SoundManager::CreateSFXSource(const std::unordered_map<std::wstring, std::filesystem::path>& sfxs)
{
	for (auto& sfx : sfxs)
	{
		FMOD::Sound* temp;
		m_CoreSystem->createSound(sfx.second.string().c_str(), FMOD_DEFAULT | FMOD_2D, nullptr, &temp);

		m_SFXs.emplace(sfx.first, temp);
	}
}

void SoundManager::CreateUISource(const std::unordered_map<std::wstring, std::filesystem::path>& uis)
{
	for (auto& ui : uis)
	{
		FMOD::Sound* temp;
		m_CoreSystem->createSound(ui.second.string().c_str(), FMOD_DEFAULT | FMOD_2D, nullptr, &temp);

		m_UIs.emplace(ui.first, temp);
	}
}



void SoundManager::Update() //매 프레임 마다 필수 호출
{
	auto now = std::chrono::steady_clock::now();

	if (m_FadeOutChannel)
	{
		float elapsed = std::chrono::duration<float>(now - m_FadeOutStartTime).count();
		float t = elapsed / m_FadeOutDuration;
		float volume = 1.0f - t;

		if (volume < 0.0f) volume = 0.0f;
		m_FadeOutChannel->setVolume(volume);

		if (t >= 1.0f)
		{
			m_FadeOutChannel->stop();
			m_FadeOutChannel = nullptr;
			m_FadeOutActive = false;
		}
	}

	if (m_FadeInChannel)
	{
		float elapsed = std::chrono::duration<float>(now - m_FadeInStartTime).count();
		float t = elapsed / m_FadeInDuration;
		float volume = t;

		if (volume > 1.0f) volume = 1.0f;
		m_FadeInChannel->setVolume(volume);

		if (t >= 1.0f)
		{
			m_FadeInChannel = nullptr;
			m_FadeInActive = false;
		}
	}

	m_CoreSystem->update();
}

void SoundManager::Shutdown()
{
	for (auto& bgm : m_BGMs) { bgm.second->release(); }
	for (auto& sfx : m_SFXs) { sfx.second->release(); }
	//for (auto& ui : m_UIs) { ui.second->release(); }
	m_CoreSystem->release();
}

void SoundManager::BGM_Shot(const std::wstring& fileName, float fadeTime)
{
	if (m_CurrentFileName == fileName)
		return;

	// 기존 페이드 채널 있으면 즉시 정리
	if (m_FadeOutActive && m_FadeOutChannel)
	{
		m_FadeOutChannel->stop();
		m_FadeOutChannel = nullptr;
		m_FadeOutActive = false;
	}

	if (m_FadeInActive && m_FadeInChannel)
	{
		m_FadeInChannel->stop();
		m_FadeInChannel = nullptr;
		m_FadeInActive = false;
	}

	auto it = m_BGMs.find(fileName);

	if (it == m_BGMs.end())
	{
		return;
	}
	
	FMOD::Sound* newSound = it->second;

	if (m_CurrentChannel)
	{
		m_FadeOutChannel = m_CurrentChannel;
		m_FadeOutStartTime = std::chrono::steady_clock::now();
		m_FadeOutDuration = fadeTime;
		m_FadeOutActive = true;
	}

	m_FadeInChannel = nullptr;
	m_CoreSystem->playSound(newSound, m_BGMGroup, true, &m_FadeInChannel);
	m_FadeInChannel->setVolume(0.0f);
	m_FadeInChannel->setPaused(false);

	m_CurrentChannel = m_FadeInChannel;
	m_CurrentSound = newSound;
	m_CurrentFileName = fileName;

	m_FadeInStartTime = std::chrono::steady_clock::now();
	m_FadeInDuration = fadeTime;
	m_FadeInActive = true;
}


void SoundManager::SFX_Shot(const std::wstring& fileName)
{
	FMOD::Channel* channel = nullptr;
	auto it = m_SFXs.find(fileName);

	if (it != m_SFXs.end())
	{
		m_CoreSystem->playSound(it->second, m_SFXGroup, false, &channel);
	}
}

void SoundManager::UI_Shot(const std::wstring& fileName)
{
	FMOD::Channel* channel = nullptr;
	auto it = m_UIs.find(fileName);

	if (it != m_UIs.end())
	{
		m_CoreSystem->playSound(it->second, m_UIGroup, false, &channel);
	}
}


void SoundManager::SetVolume_Master(float volume)
{
	if (m_SoundDirty == false) return;
	if (volume < 0) { volume = 0; }
	else if (volume > 1.0f) { volume = 1.0f; }
	m_Volume_Master = volume;

	m_MasterGroup->setVolume(m_Volume_Master);

	m_SoundDirty = false;
}

void SoundManager::SetVolume_BGM(float volume)
{
	if (m_Dirty_BGM == false) return;
	if (volume < 0) { volume = 0; }
	else if (volume > 1.0f) { volume = 1.0f; }
	m_Volume_BGM = m_Volume_Master * volume;

	m_BGMGroup->setVolume(m_Volume_BGM);

	m_Dirty_BGM = false;
}

void SoundManager::SetVolume_SFX(float volume)
{
	if (m_Dirty_SFX == false) return;
	if (volume < 0) { volume = 0; }
	else if (volume > 1.0f) { volume = 1.0f; }
	m_Volume_SFX = m_Volume_Master * volume;

	m_SFXGroup->setVolume(m_Volume_SFX);

	m_Dirty_SFX = false;
}

void SoundManager::SetVolume_UI(float volume)
{
	if (m_Dirty_UI == false) return;
	if (volume < 0) { volume = 0; }
	else if (volume > 1.0f) { volume = 1.0f; }
	m_Volume_UI = m_Volume_Master * volume;

	m_UIGroup->setVolume(m_Volume_UI);

	m_Dirty_UI = false;
}

