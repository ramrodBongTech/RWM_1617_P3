#pragma once
#include <string>
#include <functional>

using namespace std;

struct Resource
{
public:
	Resource(string key) : m_key(key){}

	virtual string getKey(){ return m_key; }

protected:
	string m_key;

};

struct Texture : Resource
{
	Texture(string key, string textureDir) : Resource(key), m_textureDir(textureDir) {}

	string m_textureDir;
};

struct Music : Resource
{
	Music(string key, string musicDir) : Resource(key), m_musicDir(musicDir) {}

	string m_musicDir;
};

struct SoundEffect : Resource
{
	SoundEffect(string key, string soundEffectDir) : Resource(key), m_soundEffectDir(soundEffectDir) {}

	string m_soundEffectDir;
};
