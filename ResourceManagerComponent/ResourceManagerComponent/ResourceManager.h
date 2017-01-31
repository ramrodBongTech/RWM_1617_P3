#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <time.h>
#include <sys/stat.h>
#include <rapidjson\document.h>
#include <rapidjson\filereadstream.h>
#include "rapidjson\reader.h"
#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_iterators.hpp"

#include "Resource.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

using namespace std;
using namespace rapidjson;
using namespace rapidxml;

struct LoadException : public std::exception
{
	LoadException(string ss) { printf(ss.c_str()); }
	~LoadException() throw () {}
};

inline bool doesFileExists(const string& name)
{
	ifstream _ifs(name.c_str());
	if (_ifs.good())
	{
		_ifs.close();
		return true;
	}
	else
	{
		_ifs.close();
		return false;
	}
}

inline bool isOutOfDate(tm td1, tm td2)
{
	if (td1.tm_sec != td2.tm_sec ||
		td1.tm_min != td2.tm_min ||
		td1.tm_hour != td2.tm_hour)
	{
		return true;
	}
	else
	{
		return false;
	}
}

class ResourceManager
{
public:
	~ResourceManager();
	
	static ResourceManager*					getInstance();

	void									init(SDL_Renderer* renderer);
	void									destroy();
	void									update(float dt);
	
	SDL_Texture*							getTextureByKey(string key);
	Mix_Music*								getMusicByKey(string key);
	Mix_Chunk*								getSoundEffectByKey(string key);

	pair<SDL_Texture*, vector<SDL_Rect>>	getAnimationByKey(string key);

	void									loadResourcesFromText(string fileName);
	void									loadResourcesFromJSON(string fileName);
	void									loadResourcesFromXML(string fileName);

	void									loadResourceQueue();

private:
	static ResourceManager*					m_instance;

	map<string, pair<SDL_Texture*, tm>>		m_textures;
	map<string, Mix_Music*>					m_music;
	map<string, Mix_Chunk*>					m_soundEffects;

	map<string, vector<SDL_Rect>>			m_animations;

	map<string, string>						m_path;

	vector<Resource*>						m_resourceQueue;

	float									m_resourcesLoaded;
	float									m_fileCheckDelay;
	string									m_source;
	tm										m_sourceTimeInfo;

	SDL_Renderer*							m_renderer;

	void									addResourceToQueue(Resource* resource);
	void									loadResource(Resource* resource);

	void									addTexture(string key);
	void									addMusic(string key);
	void									addSoundEffect(string key);

	void									checkJsonObject(const Value& object, string type);

	void									reloadTexture(string key);
	void									loadAnimations(ifstream* file, vector<SDL_Rect>* list);

	tm										getTimeInfo(const char* path);
	vector<SDL_Rect>						getAnimationFrames(string key);

	void									reloadFromXML();
	void									reloadFromJSON();
	void									reloadFromText();

	ResourceManager();
};
