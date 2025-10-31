#include "SoundManager.h"
#include "SoundEngine.h"
#include <iostream>
#include <utility>
#include <vector>
#include <sndfile.h> // per caricare WAV/OGG

SoundManager::SoundManager()
{
    this->filename = "";
    this->volume = 1.0f;
    this->isMusic = false;
    this->engine_ptr = nullptr;
}

SoundManager::SoundManager(std::string filename, float volume, bool isMusic, SoundEngine* engine_ptr)
{
    this->filename = std::move(filename);
    this->volume = volume;
    this->isMusic = isMusic;
    this->engine_ptr = engine_ptr;
    this->buffer = 0;
    this->source = 0;
    this->is_playing = false;
}

void SoundManager::playSound()
{
    if (engine_ptr == nullptr) return;

    SF_INFO sfInfo;
    SNDFILE* sndFile = sf_open(filename.c_str(), SFM_READ, &sfInfo);
    if (!sndFile) {
        std::cerr << "[OpenAL] Errore: impossibile aprire file audio " << filename << "\n";
        return;
    }

    std::vector<short> samples(sfInfo.frames * sfInfo.channels);
    sf_readf_short(sndFile, samples.data(), sfInfo.frames);
    sf_close(sndFile);

    alGenBuffers(1, &buffer);
    alBufferData(buffer, sfInfo.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
                 samples.data(), samples.size() * sizeof(short), sfInfo.samplerate);

    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);

    float vol = (isMusic ? engine_ptr->volMusic : engine_ptr->volSound) / 100.0f;
    alSourcef(source, AL_GAIN, vol);
    alSourcePlay(source);

    is_playing = true;
}

void SoundManager::stopSound()
{
    if (is_playing) {
        alSourceStop(source);
        alDeleteSources(1, &source);
        alDeleteBuffers(1, &buffer);
        is_playing = false;
    }
}

bool SoundManager::isPlaying() const
{
    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    return (state == AL_PLAYING);
}

void SoundManager::changeVolume() const {
    const float newVolume = (isMusic ? engine_ptr->volMusic : engine_ptr->volSound) / 100.0f;
    alSourcef(source, AL_GAIN, newVolume);
}

SoundManager& SoundManager::operator=(const SoundManager &op)
{
    this->engine_ptr = op.engine_ptr;
    this->filename = op.filename;
    this->volume = op.volume;
    this->isMusic = op.isMusic;
    this->buffer = op.buffer;
    this->source = op.source;
    this->is_playing = op.is_playing;

    return *this;
}
