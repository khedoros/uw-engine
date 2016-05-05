#Yet Another Ultima Underworld Recreation Project

##Purpose
Ultimately, I'd like to create a modern game engine that uses unmodified game files from Origin+Looking Glass's Ultima Underworld to replicate the original gameplay. This is comparable to projects like ScummVm and various Doom engines. It's also a great deal of work, and there are some things that I haven't figured out yet. I intend this to be a long-term project, probably taking a few years of my spare time to complete.

##Method
Separate the game into a Model-View-Controller paradigm. Import files to provide the model of the game state and the assets for graphics and audio, to be used in the view. 

###View+Model
- Interpret+import as many of the file formats that make up the Ultima Underworld game as possible, mostly based on documentation of previous reverse-engineering work
- Reverse engineer and document the remaining file formats, to whatever degree possible
- Use data contained in save files to inform my guesses about important game state information
- Render the game state in a way analogous to the original program

###Control
- Construct work-alike game logic for every feature in the original game
- This will probably be an iterative trial-and-error process

##Status
###Music
I can convert the XMI files to MIDI, read the ad-lib instrument definition files, and sequence music into register writes for an emulated YMF262 synthesizer. This also opens the door for sending the music to an emulated Roland CM-32L (MT-32) or to a modern MIDI server, like Timidity.

- The XMI format and format of the \*.ad file were gleaned from the AIL 2.0 (aka Miles Sound System 2.0) documentation and source code.
- I'm using a C++ port of Robson Cozenday's Java OPL3 emulator, which was distributed under an LGPL 2.1+ license
- I also include some code from MAME, which I acquired through one of the Doom engine implementations (don't remember which one). I don't think that I actually *use* their OPL3 code, but I'm using their interface to Cozenday's emulator, and I think that it was that project which ported it to C++.
- I'm actually not completely happy with my current sequencing code; the output doesn't sound perfectly like the music in the original game. There are some sour notes, and some MIDI control commands that I ignore (FOR loops, velocity on start and stop, probably some others like pitch bends). For that reason, I'm currently working on porting the original AIL2 C+ASM code into modern-ish C++. The whole audio subsystem will be put into its own shared library. Eventually, it'll be configurable to output to an included OPL3 emulator (operating in OPL2 mode, since that's what the game supports), or to talk to the MUNT emulator for Roland music and sound effects.

###Sound Effects
These are defined in the instrument definition files, but don't follow any format that I've been able to find documentation for. I contacted John Miles, who wrote the Audio Interface Library (AIL) that the game uses, and was informed that sound effects are defined in terms of twiddling OPL2 registers, but he didn't have any further details. The format is refered to as OSI ALE or OSI TVFX (OSI = Origin Systems, Inc), and it is a proprietary format made by Origin themselves.

Dosbox can record register writes to an Ad-Lib card into a file format called DRO, and I've included some code to interpret those. It's been a while since I've looked at it (a couple of years), and it seems like it needs some love to get into a properly-working state.

####Update
Having looked at some of the audio code, I've identified the routines that handle the TVFX sound effects, and I've looked at them some. I haven't completely figured out their operation. Here's what I think I've found, though:
- The sizes of sound effects are variable, but based on multiples of 16 plus 2 (the 2 is a word specifying the size of the effect block).
- Like all the other parts of the AIL interface, the sound effects are handled based on a 120Hz timer. The difference is that over time, the values that get written to the OPL2 change.
- There seem to be 8 word values that get updated per iteration of the effect handler.
- TVFX blocks in a timbre definition file start with a 2-byte word showing the size (like all the other timbre types). After that, there are 16-byte blocks.
- The smallest effect has 0xC2 bytes (so 12 * 16-byte blocks, plus 2 size). The next is 0xD2 bytes (13 * 16 + 2). The largest is 0x162 (22 * 16 + 2).

####Playing an effect in the code
- Checks for an available SFX callback (the code allows up to 4 to play at once)
- Checks if the timbre is already in the cache
- Load it if it isn't
- Get a lock on a MIDI channel (so it won't be interfered with due to music playback or other SFX)
- Do some bookkeeping for SFX/Music note tracking (the music device is always the one rendering sounds effects in UW1; that optionally changes in UW2)
- Sets the bank to 1 (the SFX bank)
- Sets the program to the SFX patch number
- Clears all MIDI controls on the locked channel
- Max out the volume
- Max out the expression
- Set panpot to level defined in sounds.dat
- Play note# with specified velocity (both given in sounds.dat)

####Where does it get some of those arguments?
- UW1's sounds.dat has 24 5-byte structs. The first byte of the file tells how many items there are (0x18 = 24)
- For each struct: Byte 0: Timbre number from bank 1, Byte 1: MIDI note number, Byte 2: Velocity for the note, Byte 3: Not sure. Haven't seen it used. Byte 4: Seems to be a pan value

###Graphics
I can load most of the graphics in the game, including cut scenes, wall, floor and 3d object textures, sprites for in-game items, etc. Foreground/in-hand weapon graphics use a variant on the other graphic formats, so that isn't working properly yet.

###Map
I can read the map, load item/character locations, etc. The function of the minimap needs to be documented.

###Engine
I've got a fly-through view, level-switching, and a few other things written. 3D objects aren't imported yet (the Ankh, barrels+furniture, pillars, etc). Some things use placeholder graphics from the game's level editor and haven't been properly replaced and/or hidden from view (traps, items representing 3D objects, etc). The current engine is based on fixed pipeline OpenGL (1.x), and it's horrifically inefficient. I wrote it very naively. The next iteration will use OpenGL ES 2 and be developed to run on a Raspberry Pi. That will give me a stepping stone to using shader pipeline code on the desktop (GL ES 2 being a close subset to WebGL and OpenGL 2.x). 

###3D objects
These are actually contained in structs in the executable itself. I can interpret them and output .obj 3D models. The next step for the engine is to build them into arrays of vertexes and attributes, so that they can be processed by shaders and displayed in the game engine.

###Cutscenes
- N00 files contain the "scripts" for how to run each cutscene. NXX files with higher numbers are DeluxePaint ANM files, aka LPFs (Long Page Files). They contain actual frames of animation. They're also documented elsewhere, so I won't go into their exact format here.
- For a file CSxyz.Nab, 'xyz' is the cutscene number. 'ab' is the script file (ab == 00), or the image files (ab > 00).
- N00 files are a series of entries. The first value of an entry is a 1-based frame number. (frames numbered 0 are run before processing any image data). The next value is a function number. In UW1, it's in the range 0-15 (0x0-0xF). In UW2, the range is larger, but I'm not sure how large (maybe 0-31?). Functions have between 0 and 3 arguments that immediately follow the function numbers.
- A lot of the functions operate on a byte-size bitfield, setting and clearing options that change the behavior of the cutscene state-machine

Function 0: 2 arguments. The first argument is a palette index from the current nXX file. The second argument is an index into the game's string table (strings.pak is also documented elsewhere). It reads flag bit0, and returns immediately if it isn't set.

Function 1: 0 arguments. I don't know what it does.

Function 2: 2 arguments. Seems to be a no-op.

Function 3: 1 argument. The argument is a number of seconds to pause, displaying the current frame. Current frame is used as a kind of hidden second argument. Clears flag bit1.

Function 4: 2 arguments. The first seems to be a frame number. The second seems to be a time in seconds. It seems to play the frames from current to the first argument, probably over the course of arg2 seconds.

Function 5: 1 argument. I'm not sure what it does, but the argument isn't used in the function. It just clears a bit in an 8-bit flag that controls flow of the cutscene. Might mean that the next frame's audio shouldn't block playback of the frames.

Function 6: 0 arguments. Marks the end of a cutscene.

Function 7: 1 argument. The argument is the number of times to repeat from the beginning of the file to this point (the start point is speculation on my part, but cs011 is the only example)

Function 8: 2 arguments. The first argument is a cutscene number, and the second is an animation file number. It instructs the file to load the given cutscene file.

Function 9: 1 argument. The argument is a rate to fade to black at. A higher number is a slower rate (1 fades in 8 steps, 2 fades in 16, 0 is instant-black, etc)

Function A: 1 argument. The argument is a rate to fade from black to the current frame. Same rules apply as in the fade-out function.

Function B: 1 argument. I'm not sure what it does. Argument is a frame number that shouldn't be the "current" frame. This clears flag bit0 and sets flag bit1.

Function C: 1 argument. Not sure what it does. The argument is used as a boolean value. Odd numbers are true. Even are false (i.e. it just looks at bit0 of the value). It sets/unsets bit4 of the flag.

Function D: 3 arguments. First arg is a palette index. Second is text (these two are the same as function 0). Third arg is the number of a .voc sound file to play.

Function E: 2 arguments. Might pause on current frame for different amounts of time, depending on whether audio is enabled. Current frame is used as a frame number arg. arg0 is used as a time if flag bit5 is clear, arg1 is used if flag bit5 is set. I think bit5 is related to whether digital sound is active or not.

Function F: 0 arguments. Plays the "Klang" sound effect (MIDI bank 1, patch 3, using 0-based numbering)

Function 10+: Only available in UW2, and I haven't begun my investigations of that binary.

###VOC file format
The file format is straight-forward, but not necessarily only raw WAV data. It's used in its simplest form in UW 1+2, though. Open the file, seek to byte 32 to skip the header. The remaining data is 8-bit unsigned mono PCM (except the final "00" byte that marks the end of the file). In UW1, they're recorded at 12048Hz. In UW2, they're recorded at 11111Hz.

##Legality
I encourage you to go to gog.com (or a similar site) and buy Ultima Underworld. It's cheap, and it includes the sequel (which I plan to support eventually anyhow). Effort will be taken to allow the demo files to work as well, but I'm sure that EA would like to see sales of the original game. I can't give anyone copies of any game files, and this project will *never* distribute actual game data or information that could be used to reconstruct game data.
