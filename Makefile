SFML_LIB :=-L/usr/local/lib64 -lsfml-window -lsfml-graphics -lsfml-system -lsfml-audio

all: texfile play_xmi sfml-engine simple_map UwText critfile convfile engine 3dmodel

texfile: texfile.cpp texfile.h util.cpp util.h
	g++ texfile.cpp util.cpp -DSTAND_ALONE -o texfile $(SFML_LIB)

play_xmi: play_xmi.cpp
	g++ play_xmi.cpp -ltimidity $(SFML_LIB) -o play_xmi

sfml-engine: sfml-engine.cpp simple_map.cpp simple_map.h util.cpp util.h texfile.cpp texfile.h UwText.cpp UwText.h critfile.cpp critfile.h
	g++ -std=c++11 sfml-engine.cpp simple_map.cpp util.cpp texfile.cpp UwText.cpp critfile.cpp $(SFML_LIB) -lGL -lGLU -o sfml-engine

simple_map: simple_map.cpp simple_map.h util.cpp util.h UwText.cpp UwText.h
	 g++ simple_map.cpp util.cpp UwText.cpp -DSTAND_ALONE $(SFML_LIB) -o simple_map

lpfcut: lpfcut.cpp lpfcut.h util.cpp util.h
	g++ -std=c++11 lpfcut.cpp util.cpp  -DSTAND_ALONE $(SFML_LIB) -o lpfcut

mainplay: main.cpp lpfcut.cpp lpfcut.h util.cpp util.h audio/vocfile.cpp audio/vocfile.h
	g++ -std=c++11 main.cpp lpfcut.cpp util.cpp audio/vocfile.cpp -Iaudio $(SFML_LIB) -o mainplay

UwText: UwText.cpp UwText.h util.h
	g++ UwText.cpp -DSTAND_ALONE_TEXT -o UwText

critfile: critfile.cpp critfile.h util.cpp util.h
	g++ -std=c++0x -DSTAND_ALONE critfile.cpp util.cpp $(SFML_LIB) -o critfile

convfile: convfile.cpp convfile.h util.cpp util.h UwText.cpp UwText.h
	g++ convfile.cpp util.cpp UwText.cpp -std=c++11 -DSTAND_ALONE -o convfile

engine: engine.cpp
	g++ engine.cpp -lGL -lGLU -lglut -o engine

3dmodel: 3dmodel.cpp 3dmodel.h
	g++ -std=c++11 3dmodel.cpp util.cpp -o 3dmodel
