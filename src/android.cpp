#include <cstdio>

#include <allegro5/allegro.h>

#include <jni.h>

#include "android.h"
#include "engine.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

extern "C" {
JNIEnv *_al_android_get_jnienv();
void __jni_checkException(JNIEnv *env, const char *file, const char *fname, int line);
jobject _al_android_activity_object();
}

#define _jni_checkException(env) __jni_checkException(env, __FILE__, __FUNCTION__, __LINE__)

#define _jni_call(env, rett, method, args...) ({ \
   rett ret = env->method(args); \
   _jni_checkException(env); \
   ret; \
})

#define _jni_callv(env, method, args...) ({ \
   env->method(args); \
   _jni_checkException(env); \
})

#define _jni_callVoidMethodV(env, obj, name, sig, args...) ({ \
   jclass class_id = _jni_call(env, jclass, GetObjectClass, obj); \
   \
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig); \
   if(method_id == NULL) { \
   } else { \
      _jni_callv(env, CallVoidMethod, obj, method_id, ##args); \
   } \
   \
   _jni_callv(env, DeleteLocalRef, class_id); \
})

#define _jni_callBooleanMethodV(env, obj, name, sig, args...) ({ \
   jclass class_id = _jni_call(env, jclass, GetObjectClass, obj); \
   \
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig); \
   \
   bool ret = false; \
   if(method_id == NULL) { \
   } \
   else { \
      ret = _jni_call(env, bool, CallBooleanMethod, obj, method_id, ##args); \
   } \
   \
   _jni_callv(env, DeleteLocalRef, class_id); \
   \
   ret; \
})

#define _jni_callIntMethodV(env, obj, name, sig, args...) ({ \
   jclass class_id = _jni_call(env, jclass, GetObjectClass, obj); \
   \
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig); \
   \
   int ret = -1; \
   if(method_id == NULL) { \
   } \
   else { \
      ret = _jni_call(env, int, CallIntMethod, obj, method_id, ##args); \
   } \
   \
   _jni_callv(env, DeleteLocalRef, class_id); \
   \
   ret; \
})

#define _jni_callIntMethod(env, obj, name) _jni_callIntMethodV(env, obj, name, "()I")
#define _jni_callBooleanMethod(env, obj, name) _jni_callBooleanMethodV(env, obj, name, "()Z")

static jobject _jni_callObjectMethod(JNIEnv *env, jobject object, const char *name, const char *sig)
{
   jclass class_id = _jni_call(env, jclass, GetObjectClass, object);
   jmethodID method_id = _jni_call(env, jmethodID, GetMethodID, class_id, name, sig);
   jobject ret = _jni_call(env, jobject, CallObjectMethod, object, method_id);
   _jni_callv(env, DeleteLocalRef, class_id);

   return ret;
}

void logString(const char *s)
{
	JNIEnv *env = _al_android_get_jnienv();

	jstring S = env->NewStringUTF(s);

	_jni_callVoidMethodV(
		env,
		_al_android_activity_object(),
		"logString",
		"(Ljava/lang/String;)V",
		S
	);

	env->DeleteLocalRef(S);
}

int isPurchased()
{
#if defined OUYA && defined NO_DRM
	return true;
#else
	int ret = _jni_callIntMethod(
		_al_android_get_jnienv(),
		_al_android_activity_object(),
		"isPurchased"
	);

	return ret;
#endif
}

void queryPurchased()
{
	_jni_callVoidMethodV(
		_al_android_get_jnienv(),
		_al_android_activity_object(),
		"queryPurchased",
		"()V"
	);
}

void doIAP()
{
	_jni_callVoidMethodV(
		_al_android_get_jnienv(),
		_al_android_activity_object(),
		"doIAP",
		"()V"
	);
}

int checkPurchased()
{
	queryPurchased();

	int purchased = -1;

	do {
		purchased = isPurchased();
		if (purchased == -1) {
			al_rest(0.01);
		}
	} while (purchased == -1);

	logString((std::string("purchased=") + (purchased == 0 ? "false" : "true")).c_str());

	return purchased;
}

const char * get_sdcarddir()
{
	static char buf[2000];

	jstring s =
		(jstring)_jni_callObjectMethod(
			_al_android_get_jnienv(),
			_al_android_activity_object(),
			"getSDCardPrivateDir",
			"()Ljava/lang/String;"
		);

	if (s == NULL)
		return "";

	const char *native = _al_android_get_jnienv()->GetStringUTFChars(s, 0);

	strcpy(buf, native);

	_al_android_get_jnienv()->ReleaseStringUTFChars(s, native);

	_al_android_get_jnienv()->DeleteLocalRef(s);

	return buf;
}

bool gamepadConnected()
{
	bool ret = _jni_callBooleanMethod(
		_al_android_get_jnienv(),
		_al_android_activity_object(),
		"gamepadAlwaysConnected"
	);

	if (ret) {
		return true;
	}

	if (!al_is_joystick_installed()) {
		return false;
	}

	int numjoy = al_get_num_joysticks();
	for (int i = 0; i < numjoy; i++) {
		ALLEGRO_JOYSTICK *joy = al_get_joystick(i);
		if (al_get_joystick_num_buttons(joy) != 0) {
			return true;
		}
	}

	return false;
}

struct Achievement {
	const char *id;
	const char *google_id;
};

#ifdef GOOGLEPLAY
#define NUM_ACHIEVEMENTS 10

static Achievement achievements[NUM_ACHIEVEMENTS] =
{
	{ "chests", "CgkIrOKD8owfEAIQAQ" },
	{ "credits", "CgkIrOKD8owfEAIQAg" },
	{ "bonus", "CgkIrOKD8owfEAIQAw" },
	{ "crystals", "CgkIrOKD8owfEAIQBA" },
	{ "crystal", "CgkIrOKD8owfEAIQBQ" },
	{ "whack", "CgkIrOKD8owfEAIQBg" },
	{ "attack", "CgkIrOKD8owfEAIQBw" },
	{ "defense", "CgkIrOKD8owfEAIQCA" },
	{ "egberts", "CgkIrOKD8owfEAIQCQ" },
	{ "frogberts", "CgkIrOKD8owfEAIQCg" }
};
#endif

void achieve(const char *id)
{
	JNIEnv *env = _al_android_get_jnienv();

#ifdef GOOGLEPLAY
	const char *s = NULL;

	for (int i = 0; i < NUM_ACHIEVEMENTS; i++) {
		if (!strcmp(achievements[i].id, id)) {
			s = achievements[i].google_id;
			break;
		}
	}

	if (s == NULL) {
		return;
	}
#else
	const char *s = id;
#endif

	if (engine->milestone_is_complete(id)) {
		return;
	}
	engine->set_milestone_complete(id, true);

	jstring S = env->NewStringUTF(s);

	_jni_callVoidMethodV(
		env,
		_al_android_activity_object(),
		"unlock_achievement",
		"(Ljava/lang/String;)V",
		S
	);

	env->DeleteLocalRef(S);
}

void init_play_services()
{
	_jni_callVoidMethodV(
		_al_android_get_jnienv(),
		_al_android_activity_object(),
		"init_play_services",
		"()V"
	);
}

void show_achievements()
{
	_jni_callVoidMethodV(
		_al_android_get_jnienv(),
		_al_android_activity_object(),
		"show_achievements",
		"()V"
	);
}

int amazon_initialized()
{
	return _jni_callIntMethod(
		_al_android_get_jnienv(),
		_al_android_activity_object(),
		"initialized"
	);
}

const char *get_android_language()
{
	static char buf[100];

	jstring s =
		(jstring)_jni_callObjectMethod(
			_al_android_get_jnienv(),
			_al_android_activity_object(),
			"get_android_language",
			"()Ljava/lang/String;"
		);

	if (s == NULL)
		return "";

	const char *native = _al_android_get_jnienv()->GetStringUTFChars(s, 0);

	strcpy(buf, native);

	_al_android_get_jnienv()->ReleaseStringUTFChars(s, native);

	_al_android_get_jnienv()->DeleteLocalRef(s);

	std::string str = buf;
	str = str.substr(0, 2);

	// convert to steam style since that was the first one we did
	if (str == "de") {
		str = "german";
	}
	else if (str == "fr") {
		str = "french";
	}
	else if (str == "ko") {
		str = "korean";
	}
	else {
		str = "english";
	}

	strcpy(buf, str.c_str());

	return buf;
}

#ifdef ADMOB
void showAd()
{
	_jni_callVoidMethodV(
		_al_android_get_jnienv(),
		_al_android_activity_object(),
		"showAd",
		"()V"
	);
}

void requestNewInterstitial()
{
	_jni_callVoidMethodV(
		_al_android_get_jnienv(),
		_al_android_activity_object(),
		"requestNewInterstitial",
		"()V"
	);
}
#endif
