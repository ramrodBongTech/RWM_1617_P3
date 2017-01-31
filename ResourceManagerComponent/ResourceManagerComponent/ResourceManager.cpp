#include "stdafx.h"
#include "ResourceManager.h"

ResourceManager * ResourceManager::m_instance = nullptr;

const int MAX_DELAY = 3;

ResourceManager::~ResourceManager()
{
	for (vector<Resource*>::iterator _it = m_resourceQueue.begin(); _it != m_resourceQueue.end(); ++_it)
	{
		Resource* _r = *_it;
		delete _r;
		(*_it) = nullptr;
	}

	for (map<string, pair<SDL_Texture*, tm>>::iterator _it = m_textures.begin(); _it != m_textures.end(); ++_it)
	{
		SDL_Texture* _t = (*_it).second.first;
		SDL_DestroyTexture(_t);
		(*_it).second.first = NULL;
	}
	
	for (map<string, Mix_Music*>::iterator _it = m_music.begin(); _it != m_music.end(); ++_it)
	{
		Mix_Music* _m = (*_it).second;
		Mix_FreeMusic(_m);
		(*_it).second = NULL;
	}

	for (map<string, Mix_Chunk*>::iterator _it = m_soundEffects.begin(); _it != m_soundEffects.end(); ++_it)
	{
		Mix_Chunk* _s = (*_it).second;
		Mix_FreeChunk(_s);
		(*_it).second = NULL;
	}

	m_renderer = nullptr;

	Mix_CloseAudio();
	Mix_Quit();
	IMG_Quit();	
}

ResourceManager* ResourceManager::getInstance()
{
	if (m_instance == nullptr)
		m_instance = new ResourceManager;

	return m_instance;
}

void ResourceManager::init(SDL_Renderer* renderer)
{
	//Initialize SDL_mixer
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1)
		std::cout << "Error initialising SDL audio!!" << std::endl;

	m_renderer = renderer;
}

void ResourceManager::destroy()
{
	delete m_instance;
	m_instance = nullptr;
}

void ResourceManager::update(float dt)
{
	m_fileCheckDelay += dt;
	if (m_fileCheckDelay >= MAX_DELAY)
	{
		//check underlying file changes
		for (map<string, pair<SDL_Texture*, tm>>::iterator _it = m_textures.begin(); _it != m_textures.end(); ++_it)
		{
			tm _timeInfo = getTimeInfo(m_path[_it->first].c_str());
			
			if (isOutOfDate(_it->second.second, _timeInfo))
				reloadTexture(_it->first);
		}

		tm _fileTimeInfo = getTimeInfo(m_source.c_str());
		if (isOutOfDate(m_sourceTimeInfo, _fileTimeInfo))
		{
			m_sourceTimeInfo = _fileTimeInfo;
			if (m_source.find(".xml") != string::npos)
				reloadFromXML();
			else if (m_source.find(".json") != string::npos)
				reloadFromJSON();
			else if (m_source.find(".txt") != string::npos)
				reloadFromText();
		}

		m_fileCheckDelay = 0;
	}
}

SDL_Texture* ResourceManager::getTextureByKey(string key)
{
	auto _texture = m_textures.find(key);

	if (_texture != m_textures.end())
		return _texture->second.first;
	else
		return m_textures["placeholder"].first;
}

Mix_Music* ResourceManager::getMusicByKey(string key)
{
	auto _music = m_music.find(key);

	if (_music != m_music.end())
		return _music->second;
	else
		return m_music["placeholder"];
}

Mix_Chunk* ResourceManager::getSoundEffectByKey(string key)
{
	auto _soundEffect = m_soundEffects.find(key);

	if (_soundEffect != m_soundEffects.end())
		return _soundEffect->second;
	else
		return m_soundEffects["placeholder"];
}

pair<SDL_Texture*, vector<SDL_Rect>> ResourceManager::getAnimationByKey(string key)
{
	pair<SDL_Texture*, vector<SDL_Rect>> _temp;
	_temp.first = getTextureByKey(key);
	_temp.second = getAnimationFrames(key);
	return _temp;
}

void ResourceManager::loadResourcesFromText(string fileName)
{
	m_source = fileName;
	m_sourceTimeInfo = getTimeInfo(m_source.c_str());

	string _key, _path, _type;
	ifstream _myFile(fileName);

	while (_myFile >> _type >> _key >> _path)
	{
		if (_type == "texture")
			addResourceToQueue(new Texture(_key, _path));
		else if (_type == "music")
			addResourceToQueue(new Music(_key, _path));
		else if (_type == "sound_effect")
			addResourceToQueue(new SoundEffect(_key, _path));
		else
		{
			addResourceToQueue(new Texture(_key, _path));

			string _line;
			_myFile >> _line;

			vector<SDL_Rect> _animationList;

			int _frames = stoi(_line);
			for (int i = 1; i <= _frames; i++)
				loadAnimations(&_myFile, &_animationList);

			m_animations[_key] = _animationList;
		}
	}

	_myFile.close();
}

void ResourceManager::loadResourcesFromJSON(string fileName)
{
	m_source = fileName;
	m_sourceTimeInfo = getTimeInfo(m_source.c_str());

	FILE* _file = new FILE();
	fopen_s(&_file, fileName.c_str(), "rb");
	char readBuffer[65536];
	FileReadStream _is(_file, readBuffer, sizeof(readBuffer));
	Document _document;
	_document.ParseStream(_is);
	fclose(_file);

	const Value& _resources = _document["resources"];

	const Value& _textures = _resources["textures"];
	for (Value::ConstMemberIterator _it = _textures.MemberBegin(); _it != _textures.MemberEnd(); ++_it)
	{
		string name = _it->name.GetString();
		const Value& _t = _textures[name.c_str()];

		checkJsonObject(_t, "texture");
	}

	const Value& _music = _resources["music"];
	for (Value::ConstMemberIterator _it = _music.MemberBegin(); _it != _music.MemberEnd(); ++_it)
	{
		string name = _it->name.GetString();
		const Value& _m = _music[name.c_str()];

		checkJsonObject(_m, "music");
	}

	const Value& _effects = _resources["effects"];
	for (Value::ConstMemberIterator _it = _effects.MemberBegin(); _it != _effects.MemberEnd(); ++_it)
	{
		string name = _it->name.GetString();
		const Value& _e = _effects[name.c_str()];

		checkJsonObject(_e, "effect");
	}

	const Value& _animations = _resources["animations"];
	for (Value::ConstMemberIterator _it = _animations.MemberBegin(); _it != _animations.MemberEnd(); ++_it)
	{
		string name = _it->name.GetString();
		const Value& _a = _animations[name.c_str()];

		checkJsonObject(_a, "animation");
	}
}

void ResourceManager::loadResourcesFromXML(string fileName)
{
	m_source = fileName;
	m_sourceTimeInfo = getTimeInfo(m_source.c_str());

	string _line;
	ifstream _myFile(fileName);

	xml_document<> _document;
	std::stringstream _buffer;
	_buffer << _myFile.rdbuf();
	_myFile.close();
	std::string content(_buffer.str());
	_document.parse<0>(&content[0]);

	xml_node<>* _root = _document.first_node();

	xml_node<>* _assets = _root->first_node("textures");

	xml_node<>* _texture = _assets->first_node("texture");
	while (_texture != 0)
	{
		string _key = _texture->first_node("key")->value();
		string _path = _texture->first_node("path")->value();
		addResourceToQueue(new Texture(_key, _path));
		_texture = _texture->next_sibling();
	}

	_assets = _assets->next_sibling();
	xml_node<>* _music = _assets->first_node("music");
	while (_music != 0)
	{
		string _key = _music->first_node("key")->value();
		string _path = _music->first_node("path")->value();
		addResourceToQueue(new Music(_key, _path));
		_music = _music->next_sibling();
	}
		
	_assets = _assets->next_sibling();
	xml_node<>* _effect = _assets->first_node("effect");
	while (_effect != 0)
	{
		string _key = _effect->first_node("key")->value();
		string _path = _effect->first_node("path")->value();
		addResourceToQueue(new SoundEffect(_key, _path));
		_effect = _effect->next_sibling();
	}

	_assets = _assets->next_sibling();
	xml_node<>* _animation = _assets->first_node("animation");
	while (_animation != 0)
	{
		string _key = _animation->first_node("key")->value();
		string _path = _animation->first_node("path")->value();
		addResourceToQueue(new Texture(_key, _path));

		vector<SDL_Rect> _animationList;
		xml_node<>* _frame = _animation->first_node("metaData")->first_node("frame");

		while (_frame != 0)
		{
			SDL_Rect _tempRect = SDL_Rect();
			_tempRect.w = stoi(_frame->first_node("width")->value());
			_tempRect.h = stoi(_frame->first_node("height")->value());
			_tempRect.x = stoi(_frame->first_node("x")->value());
			_tempRect.y = stoi(_frame->first_node("y")->value());

			_animationList.push_back(_tempRect);

			_frame = _frame->next_sibling();
		}

		m_animations[_key] = _animationList;
		_animation = _animation->next_sibling();
	}
}

void ResourceManager::loadResourceQueue()
{
	cout << "Number of resources to load: " + to_string(m_resourceQueue.size()) << endl;
	cout << "Loading... 0%" << endl << endl;
	for (auto& resource : m_resourceQueue)
		loadResource(resource);
}

void ResourceManager::addResourceToQueue(Resource* resource)
{
	Texture* _textureResource = dynamic_cast<Texture*>(resource);
	Music* _musicResource = dynamic_cast<Music*>(resource);
	SoundEffect* _soundEffectResource = dynamic_cast<SoundEffect*>(resource);

	if (_textureResource)
		m_path[_textureResource->getKey()] = _textureResource->m_textureDir.c_str();
	else if (_musicResource)
		m_path[_musicResource->getKey()] = _musicResource->m_musicDir.c_str();
	else
		m_path[_soundEffectResource->getKey()] = _soundEffectResource->m_soundEffectDir.c_str();

	m_resourceQueue.push_back(resource);
}

void ResourceManager::loadResource(Resource* resource)
{
	Texture* _textureResource = dynamic_cast<Texture*>(resource);
	Music* _musicResource = dynamic_cast<Music*>(resource);
	SoundEffect* _soundEffectResource = dynamic_cast<SoundEffect*>(resource);

	m_resourcesLoaded++;

	if (_textureResource)
		addTexture(_textureResource->getKey());
	else if (_musicResource)
		addMusic(_musicResource->getKey());
	else
		addSoundEffect(_soundEffectResource->getKey());

	float _percentage = m_resourcesLoaded / m_resourceQueue.size();
	cout << "Current Resource: " + resource->getKey() << endl;
	cout << "Loading... " + to_string(_percentage) + "%" << endl << endl;
}

void ResourceManager::addTexture(string key)
{
	SDL_Texture* _temp = nullptr;

	if (!doesFileExists(m_path[key]))
		throw(LoadException("Could not load texture " + key + " from " + m_path[key]));

	_temp = IMG_LoadTexture(m_renderer, m_path[key].c_str());
	if (_temp == 0)
		throw(LoadException("Could not load texture " + key + " from " + m_path[key] + "\n" + IMG_GetError() + "\n"));

	m_textures[key].first = _temp;
	m_textures[key].second = getTimeInfo(m_path[key].c_str());
}

void ResourceManager::addMusic(string key)
{
	Mix_Music* _temp = nullptr;

	if (!doesFileExists(m_path[key].c_str()))
		throw(LoadException("Could not load music " + key + " from " + m_path[key]));

	_temp = Mix_LoadMUS(m_path[key].c_str());
	if (_temp == 0)
		throw(LoadException("Could not load music " + key + " from " + m_path[key] + "\n" + Mix_GetError() + "\n"));

	m_music[key] = _temp;
}

void ResourceManager::addSoundEffect(string key)
{
	Mix_Chunk* _temp = nullptr;

	if (!doesFileExists(m_path[key].c_str()))
		throw(LoadException("Could not load sound effect " + key + " from " + m_path[key]));

	_temp = Mix_LoadWAV(m_path[key].c_str());
	if (_temp == 0)
		throw(LoadException("Could not load sound effect " + key + " from " + m_path[key] + "\n" + Mix_GetError() + "\n"));

	m_soundEffects[key] = _temp;
}

void ResourceManager::checkJsonObject(const Value& object, string type)
{
	string _key = object["key"].GetString();
	string _path = object["path"].GetString();

	m_path[_key] = _path;

	if (type == "texture")
		addResourceToQueue(new Texture(_key, _path));
	else if (type == "music")
		addResourceToQueue(new Music(_key, _path));
	else if (type == "effect")
		addResourceToQueue(new SoundEffect(_key, _path));
	else
	{
		addResourceToQueue(new Texture(_key, _path));

		vector<SDL_Rect>  animationList;
		const Value& _meta = object["metaData"];

		for (Value::ConstMemberIterator _it = _meta.MemberBegin(); _it != _meta.MemberEnd(); ++_it)
		{
			SDL_Rect _tempRect = SDL_Rect();
			_tempRect.w = _it->value["width"].GetDouble();
			_tempRect.h = _it->value["height"].GetDouble();
			_tempRect.x = _it->value["x"].GetDouble();
			_tempRect.y = _it->value["y"].GetDouble();

			animationList.push_back(_tempRect);
		}

		m_animations[_key] = animationList;
	}
}

void ResourceManager::reloadTexture(string key)
{
	SDL_Texture* _temp = IMG_LoadTexture(m_renderer, m_path[key].c_str());
	if (_temp == 0)
		throw(LoadException("Could not load texture " + key + " from " + m_path[key] + "\n" + IMG_GetError() + "\n"));

	SDL_DestroyTexture(m_textures[key].first);
	m_textures[key].first = _temp;
	m_textures[key].second = getTimeInfo(m_path[key].c_str());
}

void ResourceManager::loadAnimations(ifstream* file, vector<SDL_Rect>* list)
{
	string _line;
	*file >> _line;
	int _width = stoi(_line);
	*file >> _line;
	int _height = stoi(_line);
	*file >> _line;
	int _x = stoi(_line);
	*file >> _line;
	int _y = stoi(_line);

	SDL_Rect _tempRect = SDL_Rect();
	_tempRect.w = _width;
	_tempRect.h = _height;
	_tempRect.x = _x;
	_tempRect.y = _y;

	list->push_back(_tempRect);
}


tm ResourceManager::getTimeInfo(const char* path)
{
	struct stat _result;
	if (stat(path, &_result) == 0)
	{
		tm _timeInfo = tm();
		localtime_s(&_timeInfo, &_result.st_mtime);
		return _timeInfo;
	}
}

vector<SDL_Rect> ResourceManager::getAnimationFrames(string key)
{
	auto _animations = m_animations.find(key);

	if (_animations != m_animations.end())
		return _animations->second;
	else
		return m_animations["placeholder"];
}

void ResourceManager::reloadFromXML()
{
	string _line;
	ifstream _myFile(m_source);

	xml_document<> _document;
	std::stringstream _buffer;
	_buffer << _myFile.rdbuf();
	_myFile.close();
	std::string content(_buffer.str());
	_document.parse<0>(&content[0]);

	xml_node<>* _root = _document.first_node();
	xml_node<>* _assets = _root->first_node("textures");
	_assets = _assets->next_sibling(); 
	_assets = _assets->next_sibling();
	_assets = _assets->next_sibling(); 

	xml_node<>* _animation = _assets->first_node("animation");
	while (_animation != 0)
	{
		string _key = _animation->first_node("key")->value();

		vector<SDL_Rect> _animationList;
		xml_node<>* _frame = _animation->first_node("metaData")->first_node("frame");

		while (_frame != 0)
		{
			SDL_Rect _tempRect = SDL_Rect();
			_tempRect.w = stoi(_frame->first_node("width")->value());
			_tempRect.h = stoi(_frame->first_node("height")->value());
			_tempRect.x = stoi(_frame->first_node("x")->value());
			_tempRect.y = stoi(_frame->first_node("y")->value());

			_animationList.push_back(_tempRect);

			_frame = _frame->next_sibling();
		}

		m_animations[_key] = _animationList;
		_animation = _animation->next_sibling();
	}
}

void ResourceManager::reloadFromJSON()
{
	FILE* _file = new FILE();
	fopen_s(&_file, m_source.c_str(), "rb");
	char readBuffer[65536];
	FileReadStream _is(_file, readBuffer, sizeof(readBuffer));
	Document _document;
	_document.ParseStream(_is);
	fclose(_file);

	const Value& _resources = _document["resources"];

	const Value& _animations = _resources["animations"];
	for (Value::ConstMemberIterator _it = _animations.MemberBegin(); _it != _animations.MemberEnd(); ++_it)
	{
		string name = _it->name.GetString();
		const Value& _a = _animations[name.c_str()];

		string _key = _a["key"].GetString();

		vector<SDL_Rect>  animationList;
		const Value& _meta = _a["metaData"];

		for (Value::ConstMemberIterator _it = _meta.MemberBegin(); _it != _meta.MemberEnd(); ++_it)
		{
			SDL_Rect _tempRect = SDL_Rect();
			_tempRect.w = _it->value["width"].GetDouble();
			_tempRect.h = _it->value["height"].GetDouble();
			_tempRect.x = _it->value["x"].GetDouble();
			_tempRect.y = _it->value["y"].GetDouble();

			animationList.push_back(_tempRect);
		}

		m_animations[_key] = animationList;
	}
}

void ResourceManager::reloadFromText()
{
	string _key, _path, _type;
	ifstream _myFile;
	_myFile.open(m_source);

	while (_myFile >> _type >> _key >> _path)
	{
		if (_type == "animation")
		{
			string _line;
			_myFile >> _line;

			vector<SDL_Rect> _animationList;

			int _frames = stoi(_line);
			for (int i = 1; i <= _frames; i++)
				loadAnimations(&_myFile, &_animationList);

			m_animations[_key] = _animationList;
		}
	}
	_myFile.close();
}

ResourceManager::ResourceManager(){}