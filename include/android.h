#ifndef ANDROID_H
#define ANDROID_H

#ifdef __cplusplus
extern "C" {
#endif

void logString(const char *s);
const char * get_sdcarddir();
int isPurchased();
void queryPurchased();
void doIAP();
int checkPurchased();
bool gamepadConnected();
void achieve(const char *id);
void init_play_services();
void show_achievements();
int amazon_initialized();
const char *get_android_language();

#ifdef ADMOB
void showAd();
void requestNewInterstitial();
#endif

#ifdef __cplusplus
}
#endif

#endif
