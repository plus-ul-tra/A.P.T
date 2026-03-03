#pragma once
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <string> 
#include "fmod.hpp"

class SoundManager
{
public:
	SoundManager()/*SoundAssetManager& soundAssetManager) : m_SoundAssetManager(soundAssetManager)*/ {}
	~SoundManager();
	bool Init();
	void Update();
	void Shutdown();

	void BGM_Shot(const std::wstring& eventPath, float fadeTime);
	void SFX_Shot(const std::wstring& eventPath);
	void UI_Shot(const std::wstring& eventPath);

	void SetVolume_Master(float volume); //other volume = mainV * otherV;
	void SetVolume_BGM(float volume);
	void SetVolume_SFX(float volume);
	void SetVolume_UI(float volume);

	void SetDirty(); // dirty = true;

	void CreateBGMSource(const std::unordered_map<std::wstring, std::filesystem::path>& container);
	void CreateSFXSource(const std::unordered_map<std::wstring, std::filesystem::path>& container);
	void CreateUISource(const std::unordered_map<std::wstring, std::filesystem::path>& container);

private:
	//SoundAssetManager& m_SoundAssetManager;
	
	SoundManager            (const SoundManager&) = delete;
	SoundManager& operator= (const SoundManager&) = delete;
	SoundManager            (SoundManager&&)      = delete;
	SoundManager& operator= (SoundManager&&)      = delete;

	//FMOD::Studio::System* m_studioSystem = nullptr;
	FMOD::System* m_CoreSystem = nullptr;

	std::unordered_map<std::wstring, FMOD::Sound*> m_BGMs;
	std::unordered_map<std::wstring, FMOD::Sound*> m_SFXs;
	std::unordered_map<std::wstring, FMOD::Sound*> m_UIs;

	FMOD::Sound* m_CurrentSound = nullptr;
	std::wstring m_CurrentFileName;
	FMOD::Channel* m_CurrentChannel = nullptr;
	FMOD::Channel* m_FadeOutChannel = nullptr;
	std::chrono::steady_clock::time_point m_FadeOutStartTime;
	float m_FadeOutDuration;
	bool m_FadeOutActive;
	FMOD::Channel* m_FadeInChannel = nullptr;
	std::chrono::steady_clock::time_point m_FadeInStartTime;
	float m_FadeInDuration;
	bool m_FadeInActive;

	FMOD::ChannelGroup* m_MasterGroup = nullptr;
	FMOD::ChannelGroup* m_BGMGroup = nullptr;
	FMOD::ChannelGroup* m_SFXGroup = nullptr;
	FMOD::ChannelGroup* m_UIGroup = nullptr;

	float m_Volume_Master = 1.0f;
	float m_Volume_BGM    = 1.0f;
	float m_Volume_SFX    = 1.0f;
	float m_Volume_UI     = 1.0f;

	bool m_SoundDirty = false;
	bool m_Dirty_BGM  = false;
	bool m_Dirty_SFX  = false;
	bool m_Dirty_UI   = false;

};