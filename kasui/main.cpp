#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <clocale>

#include <time.h>
#include <unistd.h>
#include <stdint.h>

#include <SDL.h>
#ifdef ENABLE_SOUND
#include <SDL_mixer.h>
#endif

#include "guava2d/g2dgl.h"

#include "panic.h"
#include "common.h"
#include "kasui.h"
#include "in_game.h"

#ifdef DUMP_FRAMES
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}
#endif

static const char *WINDOW_CAPTION = "K-RAD";
static bool mouse_button_down = false;

const char *options_file_path = "data/options";
const char *jukugo_hits_file_path = "data/jukugo-hits";

#ifdef DUMP_FRAMES
static uint32_t last_frame_dump;
#endif

#ifdef DUMP_FRAMES
static AVCodec *codec;
static AVCodecContext *codec_ctx;
static AVFrame *frame;

static unsigned char *rgb_buffer;
static unsigned char *yuv_buffer;
static unsigned char out_buf[500*1024];

static FILE *out_fp;

static void
dump_frames_init(int width, int height)
{
	rgb_buffer = new unsigned char[3*width*height];
	yuv_buffer = new unsigned char[width*height*3/2];

	av_register_all();

	codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
	if (!codec)
		panic("failed to find codec");

	codec_ctx = avcodec_alloc_context();
	codec_ctx->bit_rate = 1000000;
	codec_ctx->width = width;
	codec_ctx->height = height;
	codec_ctx->time_base = (AVRational){1,FPS};
	codec_ctx->gop_size = 10;
	codec_ctx->pix_fmt = PIX_FMT_YUV420P;

	if (avcodec_open(codec_ctx, codec) < 0)
		panic("failed to open codec");

	frame = avcodec_alloc_frame();
	frame->data[0] = yuv_buffer;
	frame->data[1] = frame->data[0] + width*height;
	frame->data[2] = frame->data[1] + width*height/4;
	frame->linesize[0] = width;
	frame->linesize[1] = width/2;
	frame->linesize[2] = width/2;

	out_fp = fopen("out.avi", "w");
	if (!out_fp)
		panic("failed to open output file");
}

static void
dump_frame()
{
	glReadPixels(0, 0, viewport_width, viewport_height, GL_RGB,
	  GL_UNSIGNED_BYTE, rgb_buffer);

	unsigned char *p;
	unsigned char *q = yuv_buffer;

	// y plane
	for (int i = 0; i < viewport_height; i++) {
		p = &rgb_buffer[(viewport_height - i - 1)*3*viewport_width];
		for (int j = 0; j < viewport_width; j++) {
			int r = *p++;
			int g = *p++;
			int b = *p++;

                        int y = r*.299 + g*.587 + b*.114;
                        if (y < 0)
                                y = 0;
                        if (y > 255)
				y = 255;
			*q++ = y;
		}
	}

	// u plane
	for (int i = 0; i < viewport_height; i += 2) {
		p = &rgb_buffer[(viewport_height - i - 1)*3*viewport_width];
		for (int j = 0; j < viewport_width; j += 2) {
			int r = *p++;
			int g = *p++;
			int b = *p++;
			p += 3;

			int u = r*-.169 + g*-.332 + b*.5 + 128;
			if (u < 0)
				u = 0;
			if (u > 255)
				u = 255;
			*q++ = u;
		}
	}

	// v plane
	for (int i = 0; i < viewport_height; i += 2) {
		p = &rgb_buffer[(viewport_height - i - 1)*3*viewport_width];
		for (int j = 0; j < viewport_width; j += 2) {
			int r = *p++;
			int g = *p++;
			int b = *p++;
			p += 3;

                        int v = r*.5 + g*-.419 + b*-.0813 + 128;
                        if (v < 0)
				v = 0;
			if (v > 255)
				v = 255;
			*q++ = v;
		}
	}

	int out_size = avcodec_encode_video(codec_ctx, out_buf,
	  sizeof out_buf, frame);
	fwrite(out_buf, 1, out_size, out_fp);
}

static void
dump_frames_end()
{
	int out_size;
	static const char trailer[] = { 0, 0, 1, 0xb7 };

	while ((out_size = avcodec_encode_video(codec_ctx, out_buf, sizeof out_buf, NULL)) > 0) {
		fwrite(out_buf, 1, out_size, out_fp);
	}

	fwrite(trailer, 1, 4, out_fp);

	avcodec_close(codec_ctx);
	av_free(codec_ctx);
	av_free(frame);
}
#endif

static void
init_sdl(int width, int height)
{
	Uint32 flags = SDL_INIT_VIDEO;
#ifdef ENABLE_AUDIO
	flags |= SDL_INIT_AUDIO;
#endif

	if (SDL_Init(flags) < 0)
		panic("SDL_Init: %s", SDL_GetError());

	if (SDL_SetVideoMode(width, height, 0, SDL_OPENGL) == 0)
		panic("SDL_SetVideoMode: %s", SDL_GetError());

#ifdef ENABLE_AUDIO
	if (Mix_OpenAudio(22050, AUDIO_S16, 2, 4096))
		fprintf(stderr, "Mix_OpenAudio failed\n");
#endif

	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1); 

	SDL_WM_SetCaption(WINDOW_CAPTION, 0);
}

static void
init_glew()
{
	GLenum rv;
	if ((rv = glewInit()) != GLEW_OK)
		panic("glewInit: %s", glewGetErrorString(rv));
}

static void
tear_down_sdl()
{
#ifdef ENABLE_AUDIO
	Mix_CloseAudio();
#endif
	SDL_Quit();
}

static void
init(int width, int height)
{
#ifdef ENABLE_AUDIO
	extern void sounds_initialize();
#endif

	setlocale(LC_ALL, "ja_JP.UTF-8");

	srand(time(NULL));

	init_sdl(width, height);
	init_glew();
#ifdef ENABLE_AUDIO
	sounds_initialize();
#endif
#ifdef DUMP_FRAMES
	dump_frames_init(width, height);
#endif
	kasui& k = kasui::get_instance();

	k.resize(width, height);
	k.on_resume();

	dpad_state = 0;

#ifdef DUMP_FRAMES
	last_frame_dump = SDL_GetTicks();
#endif
}

static void
tear_down()
{
#ifdef ENABLE_AUDIO
	extern void sounds_release();
#endif

	kasui::get_instance().on_pause();

#ifdef ENABLE_AUDIO
	sounds_release();
#endif
	tear_down_sdl();
#ifdef DUMP_FRAMES
	dump_frames_end();
#endif
}

static bool running = false;

static void
redraw(kasui& k)
{
	k.redraw();

#ifdef DUMP_FRAMES
	dump_frame();
#endif

	SDL_GL_SwapBuffers();
}

static void
handle_events(kasui& k)
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				running = false;
				break;

			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_LEFT:
						dpad_state |= DPAD_LEFT;
						break;

					case SDLK_RIGHT:
						dpad_state |= DPAD_RIGHT;
						break;

					case SDLK_UP:
						dpad_state |= DPAD_UP;
						break;

					case SDLK_DOWN:
						dpad_state |= DPAD_DOWN;
						break;

					case SDLK_BACKSPACE:
						k.on_back_key_pressed();
						break;

					case SDLK_TAB:
						k.on_menu_key_pressed();
						break;

					default:
						break;
				}
				break;

			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
					case SDLK_LEFT:
						dpad_state &= ~DPAD_LEFT;
						break;

					case SDLK_RIGHT:
						dpad_state &= ~DPAD_RIGHT;
						break;

					case SDLK_UP:
						dpad_state &= ~DPAD_UP;
						break;

					case SDLK_DOWN:
						dpad_state &= ~DPAD_DOWN;
						break;

					default:
						break;
				}

			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == SDL_BUTTON_LEFT) {
					mouse_button_down = true;
					k.on_touch_down(event.button.x, event.button.y);
				}
				break;

			case SDL_MOUSEBUTTONUP:
				if (event.button.button == SDL_BUTTON_LEFT) {
					mouse_button_down = false;
					k.on_touch_up();
				}
				break;

			case SDL_MOUSEMOTION:
				if (mouse_button_down) {
					k.on_touch_move(event.motion.x, event.motion.y);
				}
				break;

			default:
				break;
		}
	}
}

static void
event_loop()
{
	kasui& k = kasui::get_instance();

	running = true;

	while (running) {
		handle_events(k);
		redraw(k);
	}
}

void
on_rate_me_clicked()
{
	printf("rate me!\n");
}

int
main(int argc, char *argv[])
{
	int width = 320, height = 480;
	int opt;

	while ((opt = getopt(argc, argv, "w:h:")) != -1) {
		switch (opt) {
			case 'w':
				width = atoi(optarg);
				break;

			case 'h':
				height = atoi(optarg);
				break;
		}
	}

	init(width, height);
	event_loop();
	tear_down();

	return 0;
}
