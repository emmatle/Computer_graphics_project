#pragma once
#include <string>
#include <iostream>
#include "SoundEngine.h"

using namespace std;


class SoundManager
{
public:
	SoundManager();
	SoundManager(std::string filename, float volume, bool isMusic, SoundEngine* engine_ptr);

	void playSound();
	void stopSound();
	bool isPlaying() const;
	void changeVolume() const;

	SoundManager &operator=(const SoundManager &op);


protected:
	unsigned int buffer = 0;
	unsigned int source = 0;
	bool is_playing = false;
	SoundEngine* engine_ptr = nullptr;
	std::string filename;
	float volume = 1.0f;
	bool isMusic = false;
};
