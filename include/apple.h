#ifndef APPLE_H
#define APPLE_H

#include <string>

#ifdef ADMOB
void showAd();
void requestNewInterstitial();

int isPurchased();
void queryPurchased();
void doIAP();
int checkPurchased();
void restore_purchases();

// defined in engine.cpp
void show_please_connect_dialog(bool is_network_test);
int isPurchased_engine();
#endif

const char *get_apple_language();

bool ios_show_license();

#ifdef ALLEGRO_MACOSX
void macosx_open_with_system(std::string filename);
#endif

#endif // APPLE_H
