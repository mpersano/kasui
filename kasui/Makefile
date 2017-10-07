ifdef WINDOZE
CROSS=i686-w64-mingw32-
endif

CXX = $(CROSS)g++
LD = $(CROSS)g++
OBJS = $(CXXFILES:.cpp=.o)

# DUMP_FRAMES = 1

LIBS = -L../guava2d -lguava2d

ifdef WINDOZE
LIBS += `pkg-config --libs libpng zlib sdl` -lglew32
else
LIBS += `pkg-config --libs libpng zlib sdl glew gl`
endif

ifdef DUMP_FRAMES
LIBS += \
  -L/opt/ffmpeg/lib \
  -lavdevice -lavfilter -lavformat -lavcodec \
  -lswscale -lavutil -lbz2
endif

CXXFLAGS = `pkg-config --cflags zlib libpng sdl glew gl` -Wall -O2 -g -I.. -std=c++14
ifdef WINDOZE
CXXFLAGS += -U__STRICT_ANSI__ -DGLEW_STATIC
endif

LDFLAGS =

ifdef DUMP_FRAMES
CXXFLAGS += \
  -I/opt/ffmpeg/include -DDUMP_FRAMES -D__STDC_CONSTANT_MACROS
endif

CXXFILES = \
	main.cpp \
	sound.cpp \
	kasui.cpp \
	panic.cpp \
	world.cpp \
	in_game.cpp \
	background.cpp \
	jukugo.cpp \
	kanji_info.cpp \
	theme.cpp \
	falling_leaves_theme.cpp \
	flowers_theme.cpp \
	clouds_theme.cpp \
	settings_parser.cpp \
	settings_lexer.cpp \
	action.cpp \
	options.cpp \
	sakura.cpp \
	kasui_logo.cpp \
	title_background.cpp \
	utf8.cpp \
	score_display.cpp \
	jukugo_info_sprite.cpp \
	bakudan_sprite.cpp \
	program_wrapper.cpp \
	combo_sprite.cpp \
	timer_display.cpp \
	main_menu.cpp \
	menu.cpp \
	pause_button.cpp \
	in_game_menu.cpp \
	line_splitter.cpp \
	credits.cpp \
	stats_page.cpp \
	hiscore_list.cpp \
	text_box.cpp \
	hint_animation.cpp \
	tutorial.cpp \
	hiscore_input.cpp \
	http_request.cpp \
	leaderboard_page.cpp \
	render.cpp \
	program_manager.cpp

TARGET = demo

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

settings_parser.cpp settings_parser.h: settings.y
	bison -y -d settings.y && mv y.tab.c settings_parser.cpp && mv y.tab.h settings_parser.h

settings_lexer.cpp: settings.l settings_parser.h
	lex settings.l && mv lex.yy.c settings_lexer.cpp

depend: .depend

.depend: $(CXXFILES)
	rm -f .depend
	$(CXX) $(CXXFLAGS) -MM $^ > .depend;

clean:
	rm -f *o $(TARGET) .depend

include .depend

.PHONY: all clean depend
