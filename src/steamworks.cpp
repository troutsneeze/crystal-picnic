#include <cstdio>
#include <string>

#include <steam/steam_api.h>

#include <allegro5/allegro.h>

#include "engine.h"
#include "snprintf.h"

#define STEAM_DEBUG 0

static bool steam_init_failed = false;
static std::string steam_language;

static std::string log_path(void)
{
#if STEAM_DEBUG
	char tmp[5000];
	ALLEGRO_PATH *user_path = al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
	snprintf(tmp, 5000, "%s%clog.txt", al_path_cstr(user_path, ALLEGRO_NATIVE_PATH_SEP), ALLEGRO_NATIVE_PATH_SEP);
	al_destroy_path(user_path);
	return tmp;
#else
	return "";
#endif
}

void steam_log_string(std::string s)
{
	std::string path = log_path();
	FILE *f = fopen(path.c_str(), "a");
	if (f) {
		fprintf(f, "%s\n", s.c_str());
		fclose(f);
	}
}

#define _ACH_ID( id, name ) { id, #id, name, "", 0, 0 }
struct Achievement_t
{
	int m_eAchievementID;
	const char *m_pchAchievementID;
	char m_rgchName[128];
	char m_rgchDescription[256];
	bool m_bAchieved;
	int m_iIconImage;
};

class CSteamAchievements
{
private:
	int64 m_iAppID; // Our current AppID
	Achievement_t *m_pAchievements; // Achievements data
	int m_iNumAchievements; // The number of Achievements
	bool m_bInitialized; // Have we called Request stats and received the callback?

public:
	CSteamAchievements(Achievement_t *Achievements, int NumAchievements);
	~CSteamAchievements() {}
	
	bool RequestStats();
	bool SetAchievement(const char* ID);

	STEAM_CALLBACK( CSteamAchievements, OnUserStatsReceived, UserStatsReceived_t, 
		m_CallbackUserStatsReceived );
	STEAM_CALLBACK( CSteamAchievements, OnUserStatsStored, UserStatsStored_t, 
		m_CallbackUserStatsStored );
	STEAM_CALLBACK( CSteamAchievements, OnAchievementStored, 
		UserAchievementStored_t, m_CallbackAchievementStored );
};

CSteamAchievements::CSteamAchievements(Achievement_t *Achievements, int NumAchievements): 				
 m_iAppID( 0 ),
 m_bInitialized( false ),
 m_CallbackUserStatsReceived( this, &CSteamAchievements::OnUserStatsReceived ),
 m_CallbackUserStatsStored( this, &CSteamAchievements::OnUserStatsStored ),
 m_CallbackAchievementStored( this, &CSteamAchievements::OnAchievementStored )
{
     m_iAppID = SteamUtils()->GetAppID();
     m_pAchievements = Achievements;
     m_iNumAchievements = NumAchievements;
     RequestStats();
}

bool CSteamAchievements::RequestStats()
{
	// Is Steam loaded? If not we can't get stats.
	if ( NULL == SteamUserStats() || NULL == SteamUser() )
	{
		return false;
	}
	// Is the user logged on?  If not we can't get stats.
	if ( !SteamUser()->BLoggedOn() )
	{
		return false;
	}
	// Request user stats.
	return SteamUserStats()->RequestCurrentStats();
}

bool CSteamAchievements::SetAchievement(const char* ID)
{
	// Have we received a call back from Steam yet?
	if (m_bInitialized)
	{
		SteamUserStats()->SetAchievement(ID);
		return SteamUserStats()->StoreStats();
	}
	// If not then we can't set achievements yet
	return false;
}

void CSteamAchievements::OnUserStatsReceived( UserStatsReceived_t *pCallback )
{
 // we may get callbacks for other games' stats arriving, ignore them
 if ( m_iAppID == pCallback->m_nGameID )
 {
   if ( k_EResultOK == pCallback->m_eResult )
   {
     steam_log_string("Received stats and achievements from Steam\n");
     m_bInitialized = true;

     // load achievements
     for ( int iAch = 0; iAch < m_iNumAchievements; ++iAch )
     {
       Achievement_t &ach = m_pAchievements[iAch];

       SteamUserStats()->GetAchievement(ach.m_pchAchievementID, &ach.m_bAchieved);
       _snprintf( ach.m_rgchName, sizeof(ach.m_rgchName), "%s", 
          SteamUserStats()->GetAchievementDisplayAttribute(ach.m_pchAchievementID, 
          "name"));
       _snprintf( ach.m_rgchDescription, sizeof(ach.m_rgchDescription), "%s", 
          SteamUserStats()->GetAchievementDisplayAttribute(ach.m_pchAchievementID, 
          "desc"));			
     }
   }
   else
   {
     char buffer[128];
     _snprintf( buffer, 128, "RequestStats - failed, %d\n", pCallback->m_eResult );
     steam_log_string( buffer );
   }
 }
}

void CSteamAchievements::OnUserStatsStored( UserStatsStored_t *pCallback )
{
 // we may get callbacks for other games' stats arriving, ignore them
 if ( m_iAppID == pCallback->m_nGameID )	
 {
   if ( k_EResultOK == pCallback->m_eResult )
   {
     steam_log_string( "Stored stats for Steam\n" );
   }
   else
   {
     char buffer[128];
     _snprintf( buffer, 128, "StatsStored - failed, %d\n", pCallback->m_eResult );
     steam_log_string( buffer );
   }
 }
}

void CSteamAchievements::OnAchievementStored( UserAchievementStored_t *pCallback )
{
	// we may get callbacks for other games' stats arriving, ignore them
	if (m_iAppID == pCallback->m_nGameID)	{
		steam_log_string( "Stored Achievement for Steam\n" );
	}
}

#define NUM_ACHIEVEMENTS 10

Achievement_t g_Achievements[NUM_ACHIEVEMENTS] = 
{
	{ 0, "chests", "Thar Be Treasure", "", 0, 0 },
	{ 1, "credits", "The Accused", "", 0, 0 },
	{ 2, "bonus", "Honey Bunny", "", 0, 0 },
	{ 3, "crystals", "All Decked Out", "", 0, 0 },
	{ 4, "crystal", "Beefed Up", "", 0, 0 },
	{ 5, "whack", "Bonk Bonk", "", 0, 0 },
	{ 6, "attack", "Whale On 'Em", "", 0, 0 },
	{ 7, "defense", "Tank Topped", "", 0, 0 },
	{ 8, "egberts", "Egbert's Pond", "", 0, 0 },
	{ 9, "frogberts", "Frogbert's 'Pad", "", 0, 0 }
};

CSteamAchievements *g_SteamAchievements = NULL;

void achieve(std::string name)
{
	if (steam_init_failed) {
		steam_log_string("steam init failed, not achieving\n");
		return;
	}
	if (engine->milestone_is_complete(name)) {
		steam_log_string("milestone already complete, not achieving\n");
		return;
	}
	engine->set_milestone_complete(name, true);
	if (g_SteamAchievements) {
		steam_log_string("SetAchievement(" + name + ")");
		static std::string s;
		s = name;
		g_SteamAchievements->SetAchievement(s.c_str());
	}
	else {
		steam_log_string("g_SteamAchievements=NULL");
	}
}

void init_steamworks()
{
#if STEAM_DEBUG
	FILE *f = fopen(log_path().c_str(), "w");
	if (f) {
		fclose(f);
	}
#endif

	// Initialize Steam
	bool bRet = SteamAPI_Init();
	// Create the SteamAchievements object if Steam was successfully initialized
	if (bRet) {
		g_SteamAchievements = new CSteamAchievements(g_Achievements, NUM_ACHIEVEMENTS);
		steam_language = SteamApps()->GetCurrentGameLanguage();
	}
	else {
		steam_log_string("Steam init failed!");
		steam_init_failed = true;
	}
}

std::string get_steam_language()
{
	return steam_language;
}
