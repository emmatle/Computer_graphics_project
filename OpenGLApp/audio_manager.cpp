#include "audio_manager.h"

AudioManager& AudioManager::instance() {
    static AudioManager mgr;
    return mgr;
}

void AudioManager::init() {
    sf::Listener::setGlobalVolume(100.f);
}

void AudioManager::loadSound(const std::string& name, const sf::SoundBuffer& buffer) {
    buffers[name] = buffer; // Copies the buffer
    auto s = std::make_unique<sf::Sound>(buffers[name]);
    sounds[name] = std::move(s);
}

void AudioManager::playSound(const std::string& name, float volume, bool loop) {
    auto it = sounds.find(name);
    if (it == sounds.end() || !it->second) return;
    it->second->setVolume(volume);
    it->second->setLooping(loop);
    it->second->play();
}

void AudioManager::stopSound(const std::string& name) {
    auto it = sounds.find(name);
    if (it == sounds.end() || !it->second) return;
    it->second->stop();
}

void AudioManager::setGlobalVolume(float volume) {
    sf::Listener::setGlobalVolume(volume);
}
