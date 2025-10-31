#pragma once
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

class SoundEngine
{
public:
	SoundEngine();
	SoundEngine( float volMusic, float volSound);

	int startEngine();
	void endEngine() const;

	void setVolMaster(int newVolume);

	SoundEngine &operator=(const SoundEngine &op);

	void setVolMusica(float newVolume);
	void setVolSuono(float newVolume);
	float volMusic = 1.0f;
	float volSound = 1.0f;

private:
	ALCdevice* device = nullptr;
	ALCcontext* context = nullptr;
	ALfloat masterVolume = 1.0f;

};
