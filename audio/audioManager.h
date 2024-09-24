#include<std::string>

class audioManager {
public:
    audioManager(std::string& adFile, std::string& dataPath, std::string& dataPath2);
    void playMusic(std::string& xmiFile); // Stop current queue, play music
    void playWav(std::string& vocFile);
    void playSfx(int number);
    void enqueueSong(std::string& xmiFile);
    void clearQueue();
    void startMusic()
    void stopMusic()
    void muteMusic()
    void restartMusic()
    void setVol(int level);
private:
    int volumeLevel;
    superOpl opl;
};
