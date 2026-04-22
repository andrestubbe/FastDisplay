#ifndef FASTTHEME_H
#define FASTTHEME_H

#include <jni.h>
#include <jawt.h>
#include <jawt_md.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// FASTTHEME MONITORING API
// ============================================================================

JNIEXPORT jboolean JNICALL Java_fasttheme_FastTheme_startMonitoring(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_fasttheme_FastTheme_stopMonitoring(JNIEnv* env, jobject obj);

// ============================================================================
// WINDOW STYLING API (Generic, usable by any demo/app)
// ============================================================================

JNIEXPORT jlong JNICALL Java_fasttheme_FastTheme_getWindowHandle(JNIEnv* env, jobject obj, jobject component);
JNIEXPORT jboolean JNICALL Java_fasttheme_FastTheme_setWindowTransparency(JNIEnv* env, jobject obj, jlong hwndLong, jint alpha);
JNIEXPORT jboolean JNICALL Java_fasttheme_FastTheme_setTitleBarColor(JNIEnv* env, jobject obj, jlong hwndLong, jint r, jint g, jint b);
JNIEXPORT jboolean JNICALL Java_fasttheme_FastTheme_setTitleBarTextColor(JNIEnv* env, jobject obj, jlong hwndLong, jint r, jint g, jint b);
JNIEXPORT jboolean JNICALL Java_fasttheme_FastTheme_setTitleBarDarkMode(JNIEnv* env, jobject obj, jlong hwndLong, jboolean enabled);
JNIEXPORT jstring JNICALL Java_fasttheme_FastTheme_getSystemResolution(JNIEnv* env, jobject obj);
JNIEXPORT jint JNICALL Java_fasttheme_FastTheme_getSystemDPI(JNIEnv* env, jobject obj);
JNIEXPORT jboolean JNICALL Java_fasttheme_FastTheme_isSystemDarkMode(JNIEnv* env, jobject obj);
JNIEXPORT jint JNICALL Java_fasttheme_FastTheme_getSystemRefreshRate(JNIEnv* env, jobject obj);

#ifdef __cplusplus
}
#endif

#endif // FASTTHEME_H
