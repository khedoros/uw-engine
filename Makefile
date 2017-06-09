SFML_VERSION:=$(shell pkg-config sfml-all --modversion|grep -Eo "[[:digit:]]+\.[[:digit:]]+\.[[:digit:]]+")
SFML_LIB:=$(shell pkg-config sfml-all --libs)
CXX_VERSION:=$(word 1, $(shell $(CXX) --version|grep -Eo "[[:digit:]]+\.[[:digit:]]+\.[[:digit:]]+"))
OGL_LIB := -lGL -lGLU
CXX_VER:=$(shell $(CXX) -c -std=c++11 util.cpp -o flagtest.o 2> /dev/null; echo $$?)
ifeq "$(CXX_VER)" "0"
    CXX_VER:=-std=c++11
else
    CXX_VER:=-std=c++0x
endif

default:
	@echo SFML Version: $(SFML_VERSION)
	@echo CXX Version: $(CXX_VERSION)

CPPFLAGS:=-I/usr/local/include
CXXFLAGS:=$(CXX_VER)
LDFLAGS:=-L/usr/local/lib $(SFML_LIB) -lpthread $(OGL_LIB)

all: texfile play_xmi sfml-fixed-engine sfml-shader-engine simple_map UwText critfile convfile engine uw_model uwfont convvm

texfile: texfile.cpp texfile.h util.o util.h weapon_offset.o weapon_offset.h
	$(CXX) $(CXXFLAGS) texfile.cpp util.o weapon_offset.o -DSTAND_ALONE_TEX -o texfile $(LDFLAGS)

play_xmi: play_xmi.o
	$(CXX) $(CXXFLAGS) play_xmi.o -ltimidity -o play_xmi $(LDFLAGS)

sfml-fixed-engine: sfml-fixed-engine.cpp simple_map.o simple_map.h util.o util.h texfile.o texfile.h UwText.o UwText.h critfile.o critfile.h uw_model.o uw_model.h palette.o palette.h
	$(CXX) $(CXXFLAGS) -DDEVBUILD sfml-fixed-engine.cpp simple_map.o util.o texfile.o UwText.o critfile.o uw_model.o palette.o -o sfml-fixed-engine $(LDFLAGS)

sfml-shader-engine: sfml-shader-engine.o simple_map.o simple_map.h util.o util.h texfile.o texfile.h UwText.o UwText.h critfile.o critfile.h glutil.o glutil.h uw_model.o uw_model.h
	$(CXX) $(CXXFLAGS) sfml-shader-engine.o glutil.o uw_model.o util.o -o sfml-shader-engine $(LDFLAGS)

simple_map: simple_map.cpp simple_map.h util.o util.h UwText.o UwText.h
	$(CXX) $(CXXFLAGS) simple_map.cpp util.o UwText.o -DSTAND_ALONE_MAP -o simple_map $(LDFLAGS)

lpfcut: lpfcut.cpp lpfcut.h util.o util.h
	$(CXX) $(CXXFLAGS) lpfcut.cpp util.o  -DSTAND_ALONE_LPF -o lpfcut $(LDFLAGS)

mainplay: main.o lpfcut.o lpfcut.h util.o util.h audio/vocfile.o audio/vocfile.h
	$(CXX) $(CXXFLAGS) main.o lpfcut.o util.o audio/vocfile.o -Iaudio -o mainplay $(LDFLAGS)

UwText: UwText.cpp UwText.h util.h
	$(CXX) $(CXXFLAGS) UwText.cpp -DSTAND_ALONE_TEXT -o UwText $(LDFLAGS)

critfile: critfile.cpp critfile.h util.o util.h
	$(CXX) $(CXXFLAGS) -std=c++0x -DSTAND_ALONE_CRIT critfile.cpp util.o -o critfile $(LDFLAGS)

convfile: convfile.cpp convfile.h util.o util.h UwText.o UwText.h
	$(CXX) $(CXXFLAGS) convfile.cpp util.o UwText.o -DSTAND_ALONE_CONV -o convfile $(LDFLAGS)

convvm: convvm.cpp convvm.h convfile.o convfile.h util.o util.h UwText.o UwText.h globfile.o globfile.h savefile.o savefile.h
	$(CXX) $(CXXFLAGS) convvm.cpp convfile.o util.o UwText.o globfile.o savefile.o -DSTAND_ALONE_VM -o convvm $(LDFLAGS)

engine: engine.o
	$(CXX) $(CXXFLAGS) engine.o -lGL -lGLU -lglut -o engine $(LDFLAGS)

uw_model: uw_model.cpp uw_model.h util.o util.h palette.o palette.h
	$(CXX) $(CXXFLAGS) uw_model.cpp util.o palette.o -DSTAND_ALONE_MODEL -o uw_model $(LDFLAGS)

uwfont: uwfont.cpp util.o util.h uwfont.h
	$(CXX) $(CXXFLAGS) -DSTAND_ALONE_FONT uwfont.cpp util.o -o uwfont $(LDFLAGS)

main: main.o lpfcut.o lpfcut.h audio/vocfile.o audio/vocfile.h
	$(CXX) $(CXXFLAGS) -o main main.o lpfcut.o audio/vocfile.o $(LDFLAGS)

cutscene: cutscene.cpp uwfont.cpp util.cpp lpfcut.cpp UwText.cpp cutscene.h uwfont.h util.h lpfcut.h UwText.h audio/vocfile.cpp audio/vocfile.h
	$(CXX) $(CXXFLAGS) -DSTAND_ALONE_CS -L/usr/local/lib cutscene.cpp uwfont.cpp util.cpp lpfcut.cpp UwText.cpp audio/vocfile.cpp -o cutscene $(LDFLAGS)

start: start.o lpfcut.o lpfcut.h palette.o palette.h texfile.o texfile.h util.o util.h uwfont.o uwfont.h UwText.o UwText.h cutscene.o cutscene.h audio/midi_event.o audio/midi_event.h audio/opl_music.o audio/opl_music.h audio/uw_patch.o audio/uw_patch.h audio/vocfile.o audio/vocfile.h audio/xmi.o audio/xmi.h audio/opl/OPL3.o audio/opl/opl.h
	$(CXX) $(CXXFLAGS) -o start start.o lpfcut.o palette.o texfile.o util.o uwfont.o UwText.o cutscene.o audio/midi_event.o audio/opl_music.o audio/uw_patch.o audio/vocfile.o audio/xmi.o audio/opl/OPL3.o $(LDFLAGS)
