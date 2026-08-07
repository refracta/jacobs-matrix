TARGET = jacob_bin

OFILES = ../ai.o \
	 ../buf.o \
	 ../chooser.o \
	 ../config.o \
	 ../dircontrol.o \
	 ../display.o \
	 ../dpdf.o \
	 ../engine.o \
	 ../fire.o \
	 ../gfxengine.o \
	 ../glbdef.o \
	 ../grammar.o \
	 ../item.o \
	 ../main.o \
	 ../map.o \
	 ../mob.o \
	 ../msg.o \
	 ../panel.o \
	 ../rand.o \
	 ../scrpos.o \
	 ../speed.o \
	 ../text.o \
	 ../thread.o \
	 ../thread_linux.o

.cpp.o:
	g++ -O3 -DLINUX -I../lib/libtcod-1.4.0/include `sdl-config --cflags` -c $< -o $@

all: $(TARGET)

$(TARGET): $(OFILES)
	g++ -O3 -o $(TARGET) $(OFILES) -L../lib/libtcod-1.4.0 `sdl-config --libs` -ltcod -ltcod++ -lSDL_mixer

clean:
	rm -f ../*.o
	rm -f $(TARGET)

premake:
	cd .. ; ./support/enummaker/enummaker source.txt
