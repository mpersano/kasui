find_package(ZLIB REQUIRED)

include_directories(${ZLIB_INCLUDE_DIR})

set(PNG_SOURCES
	png.c
	pngerror.c
	pngget.c
	pngmem.c
	pngpread.c
	pngread.c
	pngrio.c
	pngrtran.c
	pngrutil.c
	pngset.c
	pngtrans.c
	pngwio.c
	pngwrite.c
	pngwtran.c
	pngwutil.c)

add_library(png ${PNG_SOURCES})

set(PNG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(PNG_LIBRARY png PARENT_SCOPE)
