
/* Include the SDL main definition header */
#include "SDL_main.h"

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/
#include <jni.h>

// Called before SDL_main() to initialize JNI bindings in SDL library
extern "C" void SDL_Android_Init(JNIEnv* env);

// Library init
extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    return JNI_VERSION_1_4;
}

// Start up the SDL app
extern "C" void Java_org_libsdl_app_SDLActivity_nativeInit( JNIEnv* env, jobject obj )
{
    /* This interface could expand with ABI negotiation, calbacks, etc. */
    SDL_Android_Init(env);

    /* Run the application code! */
    char *argv[2];
    argv[0] = strdup("SDL_app");
    argv[1] = NULL;
    SDL_main(1, argv);
}
