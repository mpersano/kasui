#include <jni.h>
#include <errno.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <android_native_app_glue.h>

#include "log.h"
#include "kasui.h"
#include "options.h"
#include "common.h"

const char *options_file_path = "options";
const char *leaderboard_file_path = "hiscores";
const char *jukugo_hits_file_path = "jukugo-hits";

AAssetManager *g_asset_manager;

static android_app *g_app;

void
start_sound(int id, bool loop)
{
	extern options *cur_options;

	if (!cur_options->enable_sound)
		return;

	JNIEnv *env;
	g_app->activity->vm->AttachCurrentThread(&env, NULL);

	jclass clazz = env->GetObjectClass(g_app->activity->clazz);
	jmethodID method = env->GetStaticMethodID(clazz, "startSound", "(IZ)V");
	env->CallStaticVoidMethod(clazz, method, static_cast<jint>(id), static_cast<jboolean>(loop));

	g_app->activity->vm->DetachCurrentThread();
}

void
stop_sound(int id)
{
	extern options *cur_options;

	if (!cur_options->enable_sound)
		return;

	JNIEnv *env;
	g_app->activity->vm->AttachCurrentThread(&env, NULL);

	jclass clazz = env->GetObjectClass(g_app->activity->clazz);
	jmethodID method = env->GetStaticMethodID(clazz, "stopSound", "(I)V");
	env->CallStaticVoidMethod(clazz, method, static_cast<jint>(id));

	g_app->activity->vm->DetachCurrentThread();
}

void
stop_all_sounds()
{
	JNIEnv *env;
	g_app->activity->vm->AttachCurrentThread(&env, NULL);

	jclass clazz = env->GetObjectClass(g_app->activity->clazz);
	jmethodID method = env->GetStaticMethodID(clazz, "stopAllSounds", "()V");
	env->CallStaticVoidMethod(clazz, method);

	g_app->activity->vm->DetachCurrentThread();
}
       	

void
on_rate_me_clicked()
{
	JNIEnv *env;
	g_app->activity->vm->AttachCurrentThread(&env, NULL);

	jclass clazz = env->GetObjectClass(g_app->activity->clazz);
	jmethodID method = env->GetStaticMethodID(clazz, "onRateMeClicked", "()V");
	env->CallStaticVoidMethod(clazz, method);

	g_app->activity->vm->DetachCurrentThread();
}

class demo
{
public:
	demo(android_app *app);
	~demo();

	demo(const demo&) = delete;
	demo& operator=(const demo&) = delete;

	void run();

	static int32_t handle_input(android_app *app, AInputEvent *event);
	static void handle_cmd(android_app *app, int32_t cmd);

private:
	void draw_frame();
	int32_t handle_input(AInputEvent *event);
	void handle_cmd(int32_t cmd);

	void chdir_to_cache_dir();
	bool init_display();
	void term_display();

	android_app *app_;

	EGLDisplay display_;
	EGLSurface surface_;
	EGLContext context_;

	int viewport_width_;
	int viewport_height_;
};

demo::demo(android_app *app)
: app_(app)
, display_(EGL_NO_DISPLAY)
, surface_(EGL_NO_SURFACE)
, context_(EGL_NO_CONTEXT)
, viewport_width_(0)
, viewport_height_(0)
{
	app->userData = this;
	app->onAppCmd = handle_cmd;
	app->onInputEvent = handle_input;

	chdir_to_cache_dir();
}

demo::~demo()
{
	term_display();
}

void
demo::chdir_to_cache_dir()
{
	JNIEnv *env;
	app_->activity->vm->AttachCurrentThread(&env, NULL);

	// http://en.wikibooks.org/wiki/OpenGL_Programming/Android_GLUT_Wrapper

	jclass activityClass = env->GetObjectClass(app_->activity->clazz);
	jmethodID getCacheDir = env->GetMethodID(activityClass, "getCacheDir", "()Ljava/io/File;");
	jobject file = env->CallObjectMethod(app_->activity->clazz, getCacheDir);
	jclass fileClass = env->FindClass("java/io/File");
	jmethodID getAbsolutePath = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
	jstring jpath = (jstring)env->CallObjectMethod(file, getAbsolutePath);

	const char *app_dir = env->GetStringUTFChars(jpath, NULL);

	// chdir in the application cache directory
	log_debug("app_dir: %s", app_dir);

	chdir(app_dir);

	env->ReleaseStringUTFChars(jpath, app_dir);

	app_->activity->vm->DetachCurrentThread();
}

bool
demo::init_display()
{
	//
	//   initialize surface
	//

	display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(display_, 0, 0);

	const EGLint attribs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, // request ES2.0
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};

	EGLConfig config;

	EGLint num_configs;
	eglChooseConfig(display_, attribs, &config, 1, &num_configs);

	if (!num_configs) {
		log_err("unable to retrieve EGL config");
		return false;
	}

	EGLint format;
	eglGetConfigAttrib(display_, config, EGL_NATIVE_VISUAL_ID, &format);
	ANativeWindow_setBuffersGeometry(app_->window, 0, 0, format);

	surface_ = eglCreateWindowSurface(display_, config, app_->window, NULL);

	//
	//   initialize context
	//

	const EGLint context_attribs[] = {
			EGL_CONTEXT_CLIENT_VERSION, 3, // request ES3.0
			EGL_NONE };
	context_ = eglCreateContext(display_, config, NULL, context_attribs);

	if (eglMakeCurrent(display_, surface_, surface_, context_) == EGL_FALSE) {
		log_err("eglMakeCurrent failed");
		return false;
	}

	eglQuerySurface(display_, surface_, EGL_WIDTH, &viewport_width_);
	eglQuerySurface(display_, surface_, EGL_HEIGHT, &viewport_height_);

	log_debug("initialized surface: %dx%d / OpenGL version: %s",
		viewport_width_, viewport_height_,
		reinterpret_cast<const char *>(glGetString(GL_VERSION)));

	return true;
}

void
demo::draw_frame()
{
	if (display_ == EGL_NO_DISPLAY)
		return;

	kasui::get_instance().redraw();

	eglSwapBuffers(display_, surface_);
}

void
demo::term_display()
{
	if (display_ != EGL_NO_DISPLAY) {
		eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if (context_ != EGL_NO_CONTEXT)
			eglDestroyContext(display_, context_);

		if (surface_ != EGL_NO_SURFACE)
			eglDestroySurface(display_, surface_);

		eglTerminate(display_);
	}

	display_ = EGL_NO_DISPLAY;
	context_ = EGL_NO_CONTEXT;
	surface_ = EGL_NO_SURFACE;
}

int32_t
demo::handle_input(struct android_app *app, AInputEvent *event)
{
	return static_cast<demo *>(app->userData)->handle_input(event);
}

int32_t
demo::handle_input(AInputEvent *event)
{
	switch (AInputEvent_getType(event)) {
		case AINPUT_EVENT_TYPE_MOTION:
			switch (AMotionEvent_getAction(event)) {
				case AMOTION_EVENT_ACTION_DOWN:
					{
					const float x = AMotionEvent_getX(event, 0);
					const float y = AMotionEvent_getY(event, 0);
					kasui::get_instance().on_touch_down(x, y);
					}
					return 1;

				case AMOTION_EVENT_ACTION_UP:
					kasui::get_instance().on_touch_up();
					return 1;

				case AMOTION_EVENT_ACTION_MOVE:
					{
					const float x = AMotionEvent_getX(event, 0);
					const float y = AMotionEvent_getY(event, 0);
					kasui::get_instance().on_touch_move(x, y);
					}
					return 1;
			}
			break;

		case AINPUT_EVENT_TYPE_KEY:
			if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN) {
				switch (AKeyEvent_getKeyCode(event)) {
					case AKEYCODE_MENU:
						kasui::get_instance().on_menu_key_pressed();
						return 1;

					case AKEYCODE_BACK:
						kasui::get_instance().on_back_key_pressed();
						return 1;
				}
			}
			break;
	}

	return 0;
}

void
demo::handle_cmd(struct android_app* app, int32_t cmd)
{
	static_cast<demo *>(app->userData)->handle_cmd(cmd);
}

void
demo::handle_cmd(int32_t cmd)
{
	switch (cmd) {
		case APP_CMD_SAVE_STATE:
			// The system has asked us to save our current state.
			log_debug("APP_CMD_SAVE_STATE");
			kasui::get_instance().on_pause();
			break;

		case APP_CMD_INIT_WINDOW:
			// The window is being shown, get it ready.
			log_debug("APP_CMD_INIT_WINDOW");
			if (app_->window != NULL) {
				init_display();
				kasui::get_instance().resize(viewport_width_, viewport_height_);
			}
			break;

		case APP_CMD_TERM_WINDOW:
			log_debug("APP_CMD_TERM_WINDOW");
			// The window is being hidden or closed, clean it up.
			term_display();
			break;

		case APP_CMD_GAINED_FOCUS:
			// When our app gains focus, we start monitoring the accelerometer.
			log_debug("APP_CMD_GAINED_FOCUS");
			break;

		case APP_CMD_LOST_FOCUS:
			log_debug("APP_CMD_LOST_FOCUS");
			break;
	}
}

void
demo::run()
{
	bool running = true;

	while (running) {
		int ident;
		int events;
		android_poll_source *source;

		while ((ident = ALooper_pollAll(0, NULL, &events, reinterpret_cast<void **>(&source))) >= 0) {
			if (source)
				source->process(app_, source);

			if (app_->destroyRequested) {
				log_debug("destroy requested");
				running = false;
				break;
			}
		}

		if (!running)
			break;

		draw_frame();
	}

	term_display();
}

void
android_main(struct android_app *state)
{
	// no idea what this shit is for
	app_dummy();

	g_app = state;
	g_asset_manager = state->activity->assetManager;

	demo(state).run();
}
