#ifndef OPL_H
#define OPL_H

// Abstract base class for OPL emulators

class OPLEmul
{
public:
	OPLEmul() {}
	virtual ~OPLEmul() {}

	virtual void Reset() = 0;
	virtual void WriteReg(int reg, int v) = 0;
	virtual void Update(float *buffer, int length) = 0;
        virtual void Update(int16_t *buffer, int length) = 0;
	virtual void SetPanning(int c, float left, float right) = 0;
	//virtual FString GetVoiceString() { return FString(); }
};

OPLEmul *YM3812Create(bool stereo);
OPLEmul *DBOPLCreate(bool stereo);
OPLEmul *JavaOPLCreate(bool stereo);
OPLEmul *YamahaYm3812Create(bool stereo);

#define NATIVE_OPL_SAMPLE_RATE			49716.0

#define OPL_SAMPLE_RATE                       48000.0 //Standard rate that's: a. close to 49716Hz, b. evenly divisible by 120Hz (so I can generate 400 samples per XMIDI tick)
//#define OPL_SAMPLE_RATE 49716.0
// #define OPL_SAMPLE_RATE 22050.0
//#define OPL_SAMPLE_RATE			44100.0
#define CENTER_PANNING_POWER	0.70710678118	/* [RH] volume at center for EQP */


#endif
