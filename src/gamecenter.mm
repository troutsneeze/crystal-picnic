#include <allegro5/allegro.h>

#ifdef ALLEGRO_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

#import <GameKit/GameKit.h>

#if defined ALLEGRO_MACOSX
#include <allegro5/allegro_osx.h>
#elif defined ALLEGRO_IPHONE
#include <allegro5/allegro_iphone.h>
#include <allegro5/allegro_iphone_objc.h>
#endif

#include "engine.h"
#include "gamecenter.h"
#include "general.h"

#define NOTYET 2

static int is_authenticated = NOTYET;

static NSMutableDictionary *achievementsDictionary;

bool isGameCenterAPIAvailable()
{
#ifndef GAMECENTER
	return FALSE;
#else
	// Check for presence of GKLocalPlayer class.
	BOOL localPlayerClassAvailable = (NSClassFromString(@"GKLocalPlayer")) != nil;

	BOOL osVersionSupported = FALSE;

#ifdef ALLEGRO_IPHONE
	// The device must be running iOS 4.1 or later.
	NSString *reqSysVer = @"4.1";
	NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
	osVersionSupported = ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending);
#else
	OSErr err;
	SInt32 systemVersion;
	if ((err = Gestalt(gestaltSystemVersion, &systemVersion)) == noErr) {
		if (systemVersion >= 0x1080) {
			osVersionSupported = TRUE;
		}
	}
#endif

	return (localPlayerClassAvailable && osVersionSupported);
#endif
}

#ifdef GAMECENTER
void reportAchievementIdentifier(NSString* identifier, bool notification);
#endif

void authenticatePlayer(void)
{
	if (!isGameCenterAPIAvailable()) {
		is_authenticated = 0;
		return;
	}

#ifdef GAMECENTER
	GKLocalPlayer *localPlayer = [GKLocalPlayer localPlayer];

#ifdef ALLEGRO_IPHONE
	BOOL osVersionSupported;
	NSString *reqSysVer = @"6.0";
	NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
	osVersionSupported = ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending);

	if (osVersionSupported) {
		[localPlayer setAuthenticateHandler:^(UIViewController *viewController, NSError *error) {
			if (viewController != nil) {
				al_iphone_set_statusbar_orientation(ALLEGRO_IPHONE_STATUSBAR_ORIENTATION_PORTRAIT);
				UIWindow *win = al_iphone_get_window(engine->get_display());
				[win.rootViewController presentModalViewController:viewController animated:TRUE];
			}
			else if (localPlayer.isAuthenticated) {
				is_authenticated = true;
			}
			else {
				is_authenticated = false;
			}
		}];
	}
	else
#endif
	{
		[localPlayer authenticateWithCompletionHandler:^(NSError *error) {
			if (localPlayer.isAuthenticated)
			{
				// Perform additional tasks for the authenticated player.
				is_authenticated = 1;
			}
			else {
				printf("Game Center authentication error: code %ld\n", (long)[error code]);
				is_authenticated = 0;
			}
		}];
	}
#endif
}

#ifdef GAMECENTER
void reportAchievementIdentifier(NSString* identifier, bool notification)
{
	if (!isGameCenterAPIAvailable() || !is_authenticated)
		return;

	if ([achievementsDictionary objectForKey:identifier] != nil) {
		return;
	}
	
	float percent = 100;

	GKAchievement *achievement = [[GKAchievement alloc] initWithIdentifier: identifier];
	if (achievement)
	{
        achievement.showsCompletionBanner = notification ? YES : NO;
		[achievementsDictionary setObject:achievement forKey:identifier];
		achievement.percentComplete = percent;
        NSArray *achievementsToComplete = @[achievement];
        [GKAchievement reportAchievements:achievementsToComplete withCompletionHandler:^(NSError *error)
		 {
			 if (error != nil)
			 {
			 }
		 }];
	}
}
#endif

#define NUM_ACHIEVEMENTS 10

static std::string achievement_names[NUM_ACHIEVEMENTS] = {
	"chests",
	"credits",
	"bonus",
	"crystals",
	"crystal",
	"whack",
	"attack",
	"defense",
	"egberts",
	"frogberts"
};

void achieve(std::string name)
{
    /*
    // FIXME: this resets all achievements for user for the game!
    [GKAchievement resetAchievementsWithCompletionHandler:^(NSError *error)
     {
         if (error != nil)
             NSLog(@"Could not reset achievements due to %@", error);
     }];
    return;
    */
    
    
#ifndef GAMECENTER
	return;
#else

	if (engine->milestone_is_complete(name)) {
		return;
	}
	engine->set_milestone_complete(name, true);

	for (int i = 0; i < NUM_ACHIEVEMENTS; i++)
	{
		if (achievement_names[i] == name) {
            std::string s = "grp.CP." + General::itos(i);
            NSString *stringFromUTFString = [[NSString alloc] initWithUTF8String:s.c_str()];
			reportAchievementIdentifier(stringFromUTFString, true);
#ifdef ALLEGRO_IPHONE
			engine->show_trophy();
#endif
			return;
		}
	}
#endif
}
