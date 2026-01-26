#pragma once

#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <memory>

class AudioManager {
public:
    static AudioManager& instance();
    void init();
    void loadSound(const std::string& name, const sf::SoundBuffer& buffer);
    void playSound(const std::string& name, float volume = 100.f, bool loop = false);
    void stopSound(const std::string& name);
    void setGlobalVolume(float volume);

private:
    AudioManager() = default;
    ~AudioManager() = default;
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    std::unordered_map<std::string, sf::SoundBuffer> buffers;
    std::unordered_map<std::string, std::unique_ptr<sf::Sound>> sounds;
};
