#include<std::string>

class audioManager {
public:
    audioManager::audioManager(std::string& adFile, std::string& dataPath, std::string& dataPath2);
    void audioManager::playMusic(std::string& xmiFile); // Stop current queue, play music
    void audioManager::playWav(std::string& vocFile);
    void audioManager::playSfx(int number);
    void audioManager::enqueueSong(std::string& xmiFile);
    void audioManager::clearQueue();
    void audioManager::startMusic()
    void audioManager::stopMusic()
    void audioManager::muteMusic()
    void audioManager::restartMusic()
    void audioManager::setVol(int level);
private:
    int volumeLevel;
    superOpl opl;
    bool runMusic;
    bool runSfx;
};
