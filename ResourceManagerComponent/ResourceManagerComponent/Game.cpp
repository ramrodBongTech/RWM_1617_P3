#include "stdafx.h"
#include <iostream>
#include <queue>
#include "Game.h"

const int SCREEN_FPS = 100;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;
const float	SCREEN_WIDTH = 1200.0f;
const float	SCREEN_HEIGHT = 1200.0f;

Game::Game():
m_lastTime(0),
m_startTicks(0),
m_currentFrame(0),
m_animationDelay(0),
m_quit(false), 
m_resourceManager(nullptr)
{}

Game::~Game(){}

bool Game::init()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		cout << "Could not init SDL: " << SDL_GetError() << std::endl;
		return false;
	}

	m_startTicks = SDL_GetTicks();

	m_window = SDL_CreateWindow(
		"Resource Manager Component",		// window title
		SDL_WINDOWPOS_UNDEFINED,			// initial x position
		SDL_WINDOWPOS_UNDEFINED,			// initial y position
		(int)SCREEN_WIDTH,					// width, in pixels
		(int)SCREEN_HEIGHT,                 // height, in pixels
		SDL_WINDOW_OPENGL					// flags - see below
		);

	if (m_window == NULL)
	{
		cout << "Could not create window: " << SDL_GetError() << std::endl;
		return false;
	}

	m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
	if (m_renderer == NULL)
	{
		cout << "Could not create renderer: " << SDL_GetError() << std::endl;
		return false;
	}

	SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);

	// get a pointer to the resource manager and initialiase it with the renderer
	m_resourceManager = ResourceManager::getInstance();
	m_resourceManager->init(m_renderer);

	return true;
}

void Game::destroy()
{
	m_resourceManager->destroy();
	m_resourceManager = nullptr;
	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
	SDL_Quit;
}

void Game::loop()
{
	while (!m_quit)
	{
		processInput();

		if (m_filesLoaded)
		{
			update();
			render();
		}
	}
}

void Game::update()
{
	float _currentTime = SDL_GetTicks();						//millis since game started
	float _deltaTime = (_currentTime - m_lastTime) / 1000.0;	//time since last update

	m_animationDelay += _deltaTime;

	// Update the resource manager to monitor changes in the files
	m_resourceManager->update(_deltaTime);

	m_lastTime = _currentTime;
}

void Game::render()
{
	//Clear screen 
	SDL_RenderClear(m_renderer);

	renderSprite();
	renderAnimation();

	//Update screen 
	SDL_RenderPresent(m_renderer);
}

void Game::processInput()
{
	SDL_Event _e;

	while (SDL_PollEvent(&_e) != 0)
	{
		switch (_e.type)
		{
		case SDL_QUIT:
			m_quit = true;
			break;

		case SDL_KEYDOWN:
			switch (_e.key.keysym.sym)
			{
			
			}
			break;

		case SDL_KEYUP:
			switch (_e.key.keysym.sym)
			{
			case SDLK_1:
				if (!m_filesLoaded)
				{
					m_resourceManager->loadResourcesFromText("Resources/resources.txt");
					m_resourceManager->loadResourceQueue();

					m_gameMusic = m_resourceManager->getMusicByKey("game_music");
					m_jump = m_resourceManager->getSoundEffectByKey("jump");
					m_land = m_resourceManager->getSoundEffectByKey("land");

					m_filesLoaded = true;
				}
				break;
			case SDLK_2:
				if (!m_filesLoaded)
				{
					m_resourceManager->loadResourcesFromXML("Resources/resources.xml");
					m_resourceManager->loadResourceQueue();

					m_gameMusic = m_resourceManager->getMusicByKey("game_music");
					m_jump = m_resourceManager->getSoundEffectByKey("jump");
					m_land = m_resourceManager->getSoundEffectByKey("land");

					m_filesLoaded = true;
				}
				break;
			case SDLK_3:
				if (!m_filesLoaded)
				{
					m_resourceManager->loadResourcesFromJSON("Resources/resources.json");
					m_resourceManager->loadResourceQueue();
					
					m_gameMusic = m_resourceManager->getMusicByKey("game_music");
					m_jump = m_resourceManager->getSoundEffectByKey("jump");
					m_land = m_resourceManager->getSoundEffectByKey("land");

					m_filesLoaded = true;
				}
				break;
			case SDLK_d:
				if (m_filesLoaded)
				{
					m_gameMusic = nullptr;
					m_jump = nullptr;
					m_land = nullptr;

					m_resourceManager->destroy();
					m_resourceManager = ResourceManager::getInstance();
					m_resourceManager->init(m_renderer);

					m_filesLoaded = false;
				}
				break;
			case SDLK_p:					// Play / Pause music
				if (Mix_PlayingMusic() == 0)
				{
					if (Mix_PlayMusic(m_gameMusic, -1) == -1)
						cout << "Problem playing game music!!" << endl;
				}
				else
				{
					if (Mix_PausedMusic() == 1)
						Mix_ResumeMusic();
					else
						Mix_PauseMusic();
				}

				break;
			case SDLK_j:					// Play jump sound effect
				if (Mix_PlayChannel(-1, m_jump, 0) == -1)
					cout << "Problem playing jump sound effect!!" << endl;
				
				break;
			case SDLK_l:					// Play shoot sound effect
				if (Mix_PlayChannel(-1, m_land, 0) == -1)
					cout << "Problem playing land sound effect!!" << endl;

				break;
			}
			break;

		default:
			break;
		}
	}
}

void Game::renderSprite()
{
	SDL_Texture* _playerTexture = m_resourceManager->getTextureByKey("player_texture");

	int _w, _h;
	SDL_QueryTexture(_playerTexture, NULL, NULL, &_w, &_h);

	SDL_Rect _src = SDL_Rect();
	_src.h = _h;
	_src.w = _w;
	_src.x = 0;
	_src.y = 0;

	SDL_Rect _dest = SDL_Rect();
	_dest.h = _h;
	_dest.w = _w;
	_dest.x = 600;
	_dest.y = 600;

	SDL_RenderCopy(m_renderer, _playerTexture, &_src, &_dest);

	SDL_Texture* _placeholder = m_resourceManager->getTextureByKey("bob");

	SDL_QueryTexture(_placeholder, NULL, NULL, &_w, &_h);

	_src.h = _h;
	_src.w = _w;
	_src.x = 0;
	_src.y = 0;

	_dest.h = _h;
	_dest.w = _w;
	_dest.x = 900;
	_dest.y = 300;

	SDL_RenderCopy(m_renderer, _placeholder, &_src, &_dest);
}

void Game::renderAnimation()
{
	pair<SDL_Texture*, vector<SDL_Rect>> _placeholder = m_resourceManager->getAnimationByKey("bob");

	SDL_Rect _src = _placeholder.second[m_currentFrame % _placeholder.second.size()];

	SDL_Rect _dest = SDL_Rect();
	_dest.w = _src.w;
	_dest.h = _src.h;
	_dest.x = 300;
	_dest.y = 300;

	SDL_RenderCopy(m_renderer, _placeholder.first, &_src, &_dest);

	pair<SDL_Texture*, vector<SDL_Rect>> _stickMan = m_resourceManager->getAnimationByKey("stick_man");

	_src = _stickMan.second[m_currentFrame % _stickMan.second.size()];

	_dest.w = _src.w;
	_dest.h = _src.h;
	_dest.x = 900;
	_dest.y = 900;

	SDL_RenderCopy(m_renderer, _stickMan.first, &_src, &_dest);

	if (m_animationDelay >= 0.25f)
	{
		m_currentFrame++;
		if (m_currentFrame >= _stickMan.second.size())
			m_currentFrame = 0;
		m_animationDelay = 0;
	}
}