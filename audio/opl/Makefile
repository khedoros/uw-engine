SFML_PATH:=/usr/local/lib64
SFML_LIB:=-L$(SFML_PATH) -lsfml-system -lsfml-audio
INC:=-I./external
EXT_PATH:=./external
CPPFLAGS = -std=c++11

default: xmi uw_patch_file xmi_player midi_freqs opl_droplay sine_gen

opl_droplay: dro_play.cpp $(EXT_PATH)/OPL3.cpp $(EXT_PATH)/fmopl.cpp util.h
	g++ dro_play.cpp $(INC) $(EXT_PATH)/OPL3.cpp $(SFML_LIB) -o opl_droplay
	g++ -g dro_play.cpp $(INC) $(EXT_PATH)/OPL3.cpp $(SFML_LIB) -o opl_droplay_g

sine_gen: sine_gen.cpp sine_gen.h
	g++ sine_gen.cpp $(SFML_LIB) -DSTAND_ALONE -o sine_gen

xmi: midi_event.o midi_event.h xmi-standalone.o xmi.h util.h
	g++ xmi-standalone.o midi_event.o -o $@

uw_patch_file: uw_patch-standalone.o uw_patch.h
	g++ uw_patch-standalone.o -o $@

xmi_player: xmi_player.o midi_event.o midi_event.h xmi.o xmi.h uw_patch.o uw_patch.h $(EXT_PATH)/OPL3.o $(EXT_PATH)/opl.h $(EXT_PATH)/xs_Float.h util.h
	g++ $(SFML_LIB) xmi_player.o midi_event.o xmi.o uw_patch.o $(EXT_PATH)/OPL3.o -lsfml-system -lsfml-audio -o $@

midi_freqs: midi_freqs.o
	g++ midi_freqs.o -o $@

%-standalone.o: %.cpp
	g++ -c $(CPPFLAGS) -DSTAND_ALONE $< -o $@

%.o: %.cpp
	g++ -c $(CPPFLAGS) $< -o $@

