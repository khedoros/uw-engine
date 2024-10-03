#include<string>
#include<vector>
#include "oplSequencer.h"
#include "opl/opl.h"

class audioManager {
public:
    audioManager(const std::string& adFile, const std::string& dataPath, const std::string& dataPath2);
    void playMusic(const std::string& xmiFile); // Stop current queue, play music
    void playWav(const std::string& vocFile);
    void playSfx(int number);
    void enqueueSong(const std::string& xmiFile);
    void clearQueue();
    void startMusic();
    void stopMusic();
    void muteMusic();
    void restartMusic();
    void setVol(int level);

    struct callbackStruct {
        int sdlDevId;
        oplSequencer* seq;
    } cbs;
private:
    int volumeLevel;
    int nextDelay;
    std::vector<std::string> musicQueue;
    oplSequencer seq;
    std::mutex oplMutex;
    int sdlDevId;

    static const int stereoChannels = 2;
    static const int sampleRate = OPL_SAMPLE_RATE;
    static const int processPerSecond = 120;
    static const int sampleChunkSize = (sampleRate * stereoChannels) / processPerSecond;
    static const int sampleChunkTimeInMs = 1000 / processPerSecond;
};
