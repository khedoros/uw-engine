all: adlib_bank uw_patch dro_parse sine_gen dro_play

clean:
	/bin/rm adlib_bank uw_patch dro_parse sine_gen dro_play
adlib_bank: adlib_bank.cpp ../util.h
	g++ -Wall adlib_bank.cpp -I.. -o adlib_bank

uw_patch: uw_patch.cpp ../util.h
	g++ -Wall -std=c++11 uw_patch.cpp -I.. -o uw_patch

dro_parse: dro_parse.cpp ../util.h
	g++ -Wall -I.. dro_parse.cpp -o dro_parse

sine_gen: sine_gen.cpp sine_gen.h
	g++ -Wall -DSTAND_ALONE -std=c++11 -I /home/khedoros/include -L /home/khedoros/lib64 sine_gen.cpp -lsfml-audio -lsfml-system -o sine_gen

dro_play: dro_play.cpp sine_gen.cpp sine_gen.h ../util.h
	g++ -Wall -std=c++11 dro_play.cpp sine_gen.cpp -I.. -L/usr/local/lib64 -lsfml-audio -lsfml-system -o dro_play


dro_play_g: dro_play.cpp sine_gen.cpp sine_gen.h ../util.h
	g++ -g -O0 -Wall -std=c++11 dro_play.cpp sine_gen.cpp -I.. -L/usr/local/lib64 -lsfml-audio -lsfml-system -o dro_play

dro_analyze: dro_analyze.cpp ../util.h
	g++ -Wall dro_analyze.cpp -I.. -o dro_analyze
