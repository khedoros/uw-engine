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

###Sound Effects
These are defined in the instrument definition files, but don't follow any format that I've been able to find documentation for. I contacted John Miles, who wrote the Audio Interface Library (AIL) that the game uses, and was informed that sound effects are defined in terms of twiddling OPL2 registers, but he didn't have any further details. The format is refered to as OSI ALE or OSI TVFX (OSI = Origin Systems, Inc), and it is a proprietary format made by Origin themselves.

Dosbox can record register writes to an Ad-Lib card into a file format called DRO, and I've included some code to interpret those. It's been a while since I've looked at it (a couple of years), and it seems like it needs some love to get into a properly-working state.

###Graphics
I can load most of the graphics in the game, including cut scenes, wall, floor and 3d object textures, sprites for in-game items, etc. Foreground/in-hand weapon graphics use a variant on the other graphic formats, so that isn't working properly yet.

###Map
I can read the map, load item/character locations, etc. The function of the minimap needs to be documented.

###Engine
I've got a fly-through view, level-switching, and a few other things written. 3D objects aren't imported yet (the Ankh, barrels+furniture, pillars, etc). Some things use placeholder graphics from the game's level editor and haven't been properly replaced and/or hidden from view (traps, items representing 3D objects, etc). The current engine is based on fixed pipeline OpenGL (1.x), and it's horrifically inefficient. I wrote it very naively. The next iteration will use OpenGL ES 2 and be developed to run on a Raspberry Pi. That will give me a stepping stone to using shader pipeline code on the desktop (GL ES 2 being a close subset to WebGL and OpenGL 2.x). 

###3D objects
These are actually contained in structs in the executable itself. I can interpret them and output .obj 3D models. The next step for the engine is to build them into arrays of vertexes and attributes, so that they can be processed by shaders and displayed in the game engine.

##Legality
I encourage you to go to gog.com (or a similar site) and buy Ultima Underworld. It's cheap, and it includes the sequel. Effort will be taken to allow the demo files to work as well, but I'm sure that EA would like to see sales of the original game. I can't give anyone copies of any game files, and this project will *never* distribute actual game data or information that could be used to reconstruct game data.
