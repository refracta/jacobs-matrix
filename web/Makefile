ROOT := ..
TCOD_DIR := $(ROOT)/linux/lib/libtcod-1.4.0
BUILD_DIR := build
DIST_DIR := dist

EMCC ?= emcc
EMXX ?= em++

COMMON_FLAGS := -O2 -pthread -DLINUX -D_GNU_SOURCE=1 -fno-strict-aliasing \
	-I$(TCOD_DIR)/include -MMD -MP
C_FLAGS := $(COMMON_FLAGS)
CXX_FLAGS := $(COMMON_FLAGS) -std=gnu++98
PORT_FLAGS := -sUSE_SDL=1 -sUSE_LIBPNG=1 -sUSE_SDL_MIXER=1 -sUSE_OGG=1

TCOD_C_FILES := \
	bresenham_c.c bsp_c.c color_c.c console_c.c fov_c.c heightmap_c.c \
	image_c.c lex_c.c list_c.c mersenne_c.c noise_c.c parser_c.c path_c.c \
	sys_sdl_c.c sys_sdl_img_bmp.c sys_sdl_img_png.c tree_c.c zip_c.c
TCOD_CPP_FILES := \
	bresenham.cpp bsp.cpp color.cpp console.cpp fov.cpp heightmap.cpp \
	image.cpp lex.cpp mersenne.cpp mouse.cpp noise.cpp parser.cpp path.cpp \
	sys.cpp zip.cpp
APP_CPP_FILES := \
	ai.cpp buf.cpp chooser.cpp config.cpp dircontrol.cpp display.cpp dpdf.cpp \
	engine.cpp fire.cpp gfxengine.cpp glbdef.cpp grammar.cpp item.cpp main.cpp \
	map.cpp mob.cpp msg.cpp panel.cpp rand.cpp scrpos.cpp speed.cpp text.cpp \
	thread.cpp thread_linux.cpp

TCOD_C_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/tcod-c/%.o,$(TCOD_C_FILES))
TCOD_CPP_OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/tcod-cpp/%.o,$(TCOD_CPP_FILES))
APP_OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/app/%.o,$(APP_CPP_FILES))
OBJECTS := $(TCOD_C_OBJECTS) $(TCOD_CPP_OBJECTS) $(APP_OBJECTS)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean serve

all: $(DIST_DIR)/index.html

$(DIST_DIR)/index.html: $(OBJECTS) shell.html pre.js coi-serviceworker.js COI-SERVICEWORKER-LICENSE.txt
	@mkdir -p $(DIST_DIR)
	$(EMXX) $(OBJECTS) -o $@ \
		-O2 -pthread $(PORT_FLAGS) \
		-sPTHREAD_POOL_SIZE=4 \
		-sPROXY_TO_PTHREAD \
		-sINITIAL_MEMORY=67108864 \
		-sSTACK_SIZE=2097152 \
		-sASSERTIONS=1 \
		-sSTACK_OVERFLOW_CHECK=1 \
		-sENVIRONMENT=web,worker \
		-sEXIT_RUNTIME=0 \
		-sFORCE_FILESYSTEM=1 \
		-lidbfs.js \
		--use-preload-plugins \
		--shell-file shell.html \
		--pre-js pre.js \
		--preload-file $(ROOT)/src/jacob.cfg@/jacob.cfg \
		--preload-file $(ROOT)/linux/terminal.png@/terminal.png \
		--preload-file $(ROOT)/music@/music \
		--preload-file $(ROOT)/rooms@/rooms \
		--preload-file $(ROOT)/text.txt@/text.txt
	cp coi-serviceworker.js COI-SERVICEWORKER-LICENSE.txt $(DIST_DIR)/
	touch $(DIST_DIR)/.nojekyll

$(BUILD_DIR)/tcod-c/%.o: $(TCOD_DIR)/src/%.c
	@mkdir -p $(@D)
	$(EMCC) $(C_FLAGS) $(PORT_FLAGS) -c $< -o $@

$(BUILD_DIR)/tcod-cpp/%.o: $(TCOD_DIR)/src/%.cpp
	@mkdir -p $(@D)
	$(EMXX) $(CXX_FLAGS) $(PORT_FLAGS) -c $< -o $@

$(BUILD_DIR)/app/%.o: $(ROOT)/src/%.cpp
	@mkdir -p $(@D)
	$(EMXX) $(CXX_FLAGS) $(PORT_FLAGS) -c $< -o $@

serve: all
	python3 serve.py --directory $(DIST_DIR)

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)

-include $(DEPS)
