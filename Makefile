SFML_VERSION:=$(shell pkg-config sfml-all --modversion)
SFML_LIB:=$(shell pkg-config sfml-all --libs)
CXX_VERSION:=$(shell g++ --version|grep -Eo "[[:digit:]]+\.[[:digit:]]+\.[[:digit:]]+")
OGL_LIB := -lGL -lGLU -lGLEW
CXX_VER:=$(shell g++ -c -std=c++11 util.cpp -o flagtest.o 2> /dev/null; echo $$?)
ifeq "$(CXX_VER)" "0"
    CXX_VER:=-std=c++11
else
    CXX_VER:=-std=c++0x
endif

all: texfile play_xmi sfml-fixed-engine sfml-shader-engine simple_map UwText critfile convfile engine uw_model uwfont convvm

texfile: texfile.cpp texfile.h util.cpp util.h weapon_offset.cpp weapon_offset.h
	g++ $(CXX_VER) texfile.cpp util.cpp weapon_offset.cpp -DSTAND_ALONE_TEX -o texfile $(SFML_LIB)

play_xmi: play_xmi.cpp
	g++ play_xmi.cpp -ltimidity $(SFML_LIB) -o play_xmi

sfml-fixed-engine: sfml-fixed-engine.cpp simple_map.cpp simple_map.h util.cpp util.h texfile.cpp texfile.h UwText.cpp UwText.h critfile.cpp critfile.h uw_model.cpp uw_model.h palette.cpp palette.h
	g++ $(CXX_VER) -DDEVBUILD sfml-fixed-engine.cpp simple_map.cpp util.cpp texfile.cpp UwText.cpp critfile.cpp uw_model.cpp palette.cpp $(SFML_LIB) -lGL -lGLU -o sfml-fixed-engine

sfml-shader-engine: sfml-shader-engine.cpp simple_map.cpp simple_map.h util.cpp util.h texfile.cpp texfile.h UwText.cpp UwText.h critfile.cpp critfile.h glutil.cpp glutil.h uw_model.cpp uw_model.h
	g++ $(CXX_VER) sfml-shader-engine.cpp glutil.cpp uw_model.cpp util.cpp $(SFML_LIB) $(OGL_LIB) -o sfml-shader-engine

simple_map: simple_map.cpp simple_map.h util.cpp util.h UwText.cpp UwText.h
	 g++ simple_map.cpp util.cpp UwText.cpp -DSTAND_ALONE_MAP $(SFML_LIB) -o simple_map

lpfcut: lpfcut.cpp lpfcut.h util.cpp util.h
	g++ $(CXX_VER) lpfcut.cpp util.cpp  -DSTAND_ALONE_LPF $(SFML_LIB) -o lpfcut

mainplay: main.cpp lpfcut.cpp lpfcut.h util.cpp util.h audio/vocfile.cpp audio/vocfile.h
	g++ $(CXX_VER) main.cpp lpfcut.cpp util.cpp audio/vocfile.cpp -Iaudio $(SFML_LIB) -o mainplay

UwText: UwText.cpp UwText.h util.h
	g++ UwText.cpp -DSTAND_ALONE_TEXT -o UwText

critfile: critfile.cpp critfile.h util.cpp util.h
	g++ -std=c++0x -DSTAND_ALONE_CRIT critfile.cpp util.cpp $(SFML_LIB) -o critfile

convfile: convfile.cpp convfile.h util.cpp util.h UwText.cpp UwText.h
	g++ convfile.cpp util.cpp UwText.cpp $(CXX_VER) -DSTAND_ALONE_CONV -o convfile

convvm: convvm.cpp convvm.h convfile.cpp convfile.h util.cpp util.h UwText.cpp UwText.h globfile.cpp globfile.h savefile.cpp savefile.h
	g++ convvm.cpp convfile.cpp util.cpp UwText.cpp globfile.cpp savefile.cpp $(CXX_VER) -DSTAND_ALONE_VM -o convvm

engine: engine.cpp
	g++ engine.cpp -lGL -lGLU -lglut -o engine

uw_model: uw_model.cpp uw_model.h util.cpp util.h palette.cpp palette.h
	g++ $(CXX_VER) uw_model.cpp util.cpp palette.cpp -DSTAND_ALONE_MODEL -o uw_model

uwfont: uwfont.cpp util.cpp util.h uwfont.h
	g++ $(CXX_VER) -DSTAND_ALONE_FONT uwfont.cpp util.cpp `pkg-config --libs sfml-all` -o uwfont

main: main.cpp lpfcut.cpp lpfcut.h audio/vocfile.cpp audio/vocfile.h
	g++ -o main $(CXX_VER) main.cpp lpfcut.cpp audio/vocfile.cpp -lsfml-audio -lsfml-system -lsfml-window -lsfml-graphics

cutscene: cutscene.cpp uwfont.cpp util.cpp lpfcut.cpp UwText.cpp cutscene.h uwfont.h util.h lpfcut.h UwText.h audio/vocfile.cpp audio/vocfile.h
	g++ $(CXX_VER) -DSTAND_ALONE_CS -L/usr/local/lib cutscene.cpp uwfont.cpp util.cpp lpfcut.cpp UwText.cpp audio/vocfile.cpp -o cutscene -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

start:
	g++ -o start $(CXX_VER) start.cpp lpfcut.cpp palette.cpp texfile.cpp util.cpp uwfont.cpp UwText.cpp cutscene.cpp audio/midi_event.cpp audio/opl_music.cpp audio/uw_patch.cpp audio/vocfile.cpp audio/xmi.cpp audio/opl/OPL3.cpp -L/usr/local/lib -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system -lpthread
