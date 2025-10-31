#include "SoundEngine.h"
#include <iostream>

// === COSTRUTTORI ===
SoundEngine::SoundEngine()
{
    this->volMusic = 1.0f;
    this->volSound = 1.0f;
}

SoundEngine::SoundEngine(const float volMusic, const float volSound)
{
    this->volMusic = volMusic;
    this->volSound = volSound;
    this->device = nullptr;
    this->context = nullptr;
}


// === INIZIALIZZAZIONE AUDIO ===
int SoundEngine::startEngine()
{
    device = alcOpenDevice(nullptr);
    if (!device)
    {
        std::cerr << "[OpenAL] Errore: impossibile aprire il dispositivo audio.\n";
        return 0;
    }

    context = alcCreateContext(device, nullptr);
    if (!context || !alcMakeContextCurrent(context))
    {
        std::cerr << "[OpenAL] Errore: impossibile creare il contesto audio.\n";
        return 0;
    }

    std::cout << "[OpenAL] Inizializzazione completata.\n";
    return 1;
}


// === CHIUSURA AUDIO ===
void SoundEngine::endEngine() const {
    alcMakeContextCurrent(nullptr);
    if (context) alcDestroyContext(context);
    if (device) alcCloseDevice(device);
    std::cout << "[OpenAL] Audio chiuso correttamente.\n";
}


// === VOLUME ===
void SoundEngine::setVolMusica(const float newVolume)
{
    volMusic = newVolume;

    alListenerf(AL_GAIN, newVolume);
}


void SoundEngine::setVolSuono(const float newVolume)
{
    volSound = newVolume;
}



void SoundEngine::setVolMaster(const int newVolume)
{
    masterVolume = static_cast<float>(newVolume) / 100.0f;
    alListenerf(AL_GAIN, masterVolume);
}


// === OPERATORE ASSEGNAMENTO ===
SoundEngine& SoundEngine::operator=(const SoundEngine &op)
{
    this->volMusic = op.volMusic;
    this->volSound = op.volSound;
    return *this;
}
