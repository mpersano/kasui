ifdef WINDOZE
CROSS=i686-w64-mingw32-
endif

CXX = $(CROSS)g++
LD = $(CROSS)g++
AR = $(CROSS)ar

OBJS = $(CXXFILES:.cpp=.o)

CXXFILES = \
	pixmap.cpp \
	texture.cpp \
	texture_manager.cpp \
	font.cpp \
	font_manager.cpp \
	panic.cpp \
	xwchar.cpp \
	file.cpp \
	draw_queue.cpp \
	program.cpp \
	shader.cpp

CXXFLAGS = `pkg-config --cflags zlib libpng glew` -Wall -g -O2 -std=c++14

ifdef WINDOZE
CXXFLAGS += -DGLEW_STATIC
endif

TARGET = libguava2d.a

all: $(TARGET)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

$(TARGET): $(OBJS)
	$(AR) cr $@ $(OBJS)

depend: .depend

.depend: $(CXXFILES)
	rm -f .depend
	$(CXX) $(CXXFLAGS) -MM $^ > .depend;

clean:
	rm -f *o $(TARGET) .depend

include .depend
