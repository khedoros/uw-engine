opl_droplay: dro_play.cpp OPL3.cpp util.h
	g++ dro_play.cpp OPL3.cpp fmopl.cpp -lsfml-system -lsfml-audio -o opl_droplay
	g++ -g dro_play.cpp OPL3.cpp fmopl.cpp -lsfml-system -lsfml-audio -o opl_droplay_g

sine_gen: sine_gen.cpp sine_gen.h
	g++ sine_gen.cpp -L /home/khedoros/lib -lsfml-audio -lsfml-system -DSTAND_ALONE -o sine_gen
