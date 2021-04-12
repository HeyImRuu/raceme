// RaceMe for Discovery FLHook
// April 2021 by HeyImRuu
//
// speed racer.
//
// This is free software; you can redistribute it and/or modify it as
// you wish without restriction. If you do then I would appreciate
// being notified and/or mentioned somewhere.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Includes
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include <math.h>
#include <list>
#include <map>
#include <algorithm>
#include <FLHook.h>
#include <plugin.h>
#include <PluginUtilities.h>
#include "Main.h"
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../../Public/hookext_plugin/hookext_exports.h"
using namespace boost::posix_time;
struct RaceTrack
{
	int iTrackId;
	string sTrackNameFriendly;
	Vector vStartPos;
	Vector vFinishPos;
	string sSystem;
};
struct Racer
{
	string sRacerNick;
	int iRacerId;
	int iTrackId;
	ptime iTimeStart;
	bool bRacing;
	bool bWaiting;
};

vector<string> vTrackList;
float buffer;
static map<int, RaceTrack> mapRegisteredRaceTracks;
static map<int, Racer> mapRegisteredRacers;

static int set_iPluginDebug = 0;

/// A return code to indicate to FLHook if we want the hook processing to continue.
PLUGIN_RETURNCODE returncode;

void LoadSettings();

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	srand((uint)time(0));
	// If we're being loaded from the command line while FLHook is running then
	// set_scCfgFile will not be empty so load the settings as FLHook only
	// calls load settings on FLHook startup and .rehash.
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		if (set_scCfgFile.length() > 0)
			LoadSettings();
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
	}
	return true;
}

/// Hook will call this function after calling a plugin function to see if we the
/// processing to continue
EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

bool bPluginEnabled = true;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Loading Settings
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;
	char szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	string File_FLHook = string(szCurDir) + "\\flhook_plugins\\raceme.cfg";
	int iLoaded = 0;

	INI_Reader ini;
	if (ini.open(File_FLHook.c_str(), false))
	{
		while (ini.read_header())
		{
			if (ini.is_header("config"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("enabled"))
					{
						bPluginEnabled = ini.get_value_bool(0);
					}
					if (ini.is_value("buffer"))
					{
						buffer = ini.get_value_float(0);
					}
				}
			}
			if (ini.is_header("data"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("tracklist"))
					{
						vTrackList.push_back(ini.get_value_string(0));
					}
				}
			}

			for (vector<string>::iterator it = vTrackList.begin(); it != vTrackList.end(); ++it)
			{
				if (ini.is_header(it->c_str()))
				{
					while (ini.read_value())
					{
						RaceTrack track;
						if (ini.is_value("track"))
						{
							track.iTrackId = ini.get_value_int(0);
							track.sTrackNameFriendly = ini.get_value_string(1);
							track.vStartPos.x = ini.get_value_int(2);
							track.vStartPos.y = ini.get_value_int(3);
							track.vStartPos.z = ini.get_value_int(4);
							track.vFinishPos.x = ini.get_value_int(5);
							track.vFinishPos.y = ini.get_value_int(6);
							track.vFinishPos.z = ini.get_value_int(7);
							track.sSystem = ini.get_value_string(8);
							mapRegisteredRaceTracks[track.iTrackId] = track;
							++iLoaded;
						}
					}
				}
			}
		}
		ini.close();
	}

	ConPrint(L"RaceMe: Loaded %u Tracks\n", iLoaded);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


string ftos(float f)
{
	return boost::lexical_cast<string>(f);
}

bool UserCmd_StartRace(uint iClientID, const wstring &wscCmd, const wstring &wscParam, const wchar_t *usage)
{
	if (!bPluginEnabled)
	{
		PrintUserCmdText(iClientID, L"RaceMe is disabled.");
		return true;
	}

	int iRaceTrackId = stoi(wstos(GetParam(wscParam, L' ', 0)));
	string PlayerCurrentsSystem = HkGetPlayerSystemS(iClientID);
	
	//pub::Player::GetSystem(iClientID, PlayerCurrentiSystem);
	//wstring wsRaceSystem = HkGetSystemNickByID(mapRegisteredRaceTracks[iRaceTrackId].sSystem.c_str());//CreateID(mapRegisteredRaceTracks[iRaceTrackId].sSystem.c_str());

	//debug: ConPrint(L"track: " + wscParam + L" RaceSystem:" + stows(mapRegisteredRaceTracks[iRaceTrackId].sSystem) + L" player sys:" + stows(PlayerCurrentsSystem) + L" trackid:" + stows(itos(mapRegisteredRaceTracks[iRaceTrackId].iTrackId)));//debug

	//check if user entered an id, that it is loaded into map, and the user in in the correct system
	if (iRaceTrackId > 0 && mapRegisteredRaceTracks[iRaceTrackId].iTrackId == iRaceTrackId && PlayerCurrentsSystem == mapRegisteredRaceTracks[iRaceTrackId].sSystem)
	{
		mapRegisteredRacers[iClientID].iRacerId = iClientID;
		mapRegisteredRacers[iClientID].bWaiting = true;
		mapRegisteredRacers[iClientID].iTrackId = iRaceTrackId;
		PrintUserCmdText(iClientID, L"OK " + wscParam);
		return true;
	}
	else {
		PrintUserCmdText(iClientID, L"err.");
	}

	return true;
}

bool UserCmd_ShowTracks(uint iClientID, const wstring &wscCmd, const wstring &wscParam, const wchar_t *usage)
{
	if (!bPluginEnabled)
	{
		PrintUserCmdText(iClientID, L"RaceMe is disabled.");
		return true;
	}
	
	for (uint i = 0; i < vTrackList.size(); i++)
	{
		PrintUserCmdText(iClientID, stows(itos(i+1)) + L": " + stows(vTrackList[i]));
	}
	/*
	for (vector<string>::iterator it = vTrackList.begin(); it != vTrackList.end(); ++it)
	{
		
		PrintUserCmdText(iClientID, stows(*it));
	}
	*/
	PrintUserCmdText(iClientID, L"OK");
	return true;
}

bool UserCmd_dev(uint iClientID, const wstring &wscCmd, const wstring &wscParam, const wchar_t *usage)
{
	if (!bPluginEnabled)
	{
		PrintUserCmdText(iClientID, L"RaceMe is disabled.");
		return true;
	}
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Actual Code
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void playerNotification(int iClientID, wstring wszText)
{
	HkChangeIDSString(iClientID, 526999, wszText);//526999 is the mission objective text that pops up with 'n'

	FmtStr caption(0, 0);
	caption.begin_mad_lib(526999);
	caption.end_mad_lib();

	pub::Player::DisplayMissionMessage(iClientID, caption, MissionMessageType_Type1, true);
}

/** Clean up when a client disconnects */
void ClearClientInfo(uint iClientID)
{
	returncode = DEFAULT_RETURNCODE;

	mapRegisteredRacers.erase(iClientID);
}

// you really gotta pull a sharp turn thru the gates to make sure you trigger this update,
// min size for the buffer really has to be 50, an arrow pulling a full turn in cruise only updates every 40m or so.
// unless you're activley controlling the movement of your ship these updates can slow down to well over 150m,
// which means you'll miss the gate by a lot. A 50m buffer is roughly the correct size to use for the race gates,
// can use bigger but trying to avoid allowing skipping a gate.
void __stdcall SPObjUpdate(struct SSPObjUpdateInfo const &ui, unsigned int iClientID)
{
	//save the x,y,z in this update
	Vector shippyPos;
	shippyPos = ui.vPos;
	
	if (mapRegisteredRacers[iClientID].bWaiting)//we're in a waiting state before start of race
	{
		Vector diff;
		diff.x = shippyPos.x - mapRegisteredRaceTracks[mapRegisteredRacers[iClientID].iTrackId].vStartPos.x;//target pos
		diff.y = shippyPos.y - mapRegisteredRaceTracks[mapRegisteredRacers[iClientID].iTrackId].vStartPos.y;//start position of track that user is registerd for
		diff.z = shippyPos.z - mapRegisteredRaceTracks[mapRegisteredRacers[iClientID].iTrackId].vStartPos.z;
		float d = pow(pow(diff.x, 2.0f) + pow(diff.y, 2.0f) + pow(diff.z, 2.0f), 0.5f);//calulate 3d distance
		//check if the player has started moving - before beginning race
		if (d < buffer)
		{
			if (set_iPluginDebug)
			{
				PrintUserCmdText(iClientID, L"RACEME: movement event detected: ");
				PrintUserCmdText(iClientID, stows(ftos(diff.x)));
				PrintUserCmdText(iClientID, stows(ftos(diff.y)));
				PrintUserCmdText(iClientID, stows(ftos(diff.z)));
				PrintUserCmdText(iClientID, stows(ftos(d)));
			}
			mapRegisteredRacers[iClientID].bRacing = true;//start race
			mapRegisteredRacers[iClientID].bWaiting = false;//not waiting

			PrintUserCmdText(iClientID, L"Timer started.");
			//timer start
			mapRegisteredRacers[iClientID].iTimeStart = microsec_clock::universal_time();

			//play some music
			uint MusictoID = CreateID("music_race_loop");
			pub::Audio::Tryptich music;
			music.iDunno = 0;
			music.iDunno2 = 0;
			music.iDunno3 = 0;
			music.iMusicID = MusictoID;
			pub::Audio::SetMusic(iClientID, music);
			return;

		}
	}
	ptime ptimeNow = microsec_clock::universal_time();
	if (mapRegisteredRacers[iClientID].bRacing)//during race
	{
		if (ptimeNow - mapRegisteredRacers[iClientID].iTimeStart < boost::posix_time::seconds(2))
		{
			//skip - grace period in s if start = finish
			return;
		}
		//finish race
		Vector diff;
		diff.x = shippyPos.x - mapRegisteredRaceTracks[mapRegisteredRacers[iClientID].iTrackId].vFinishPos.x;//target pos
		diff.y = shippyPos.y - mapRegisteredRaceTracks[mapRegisteredRacers[iClientID].iTrackId].vFinishPos.y;//finish position of track that user is registered for
		diff.z = shippyPos.z - mapRegisteredRaceTracks[mapRegisteredRacers[iClientID].iTrackId].vFinishPos.z;
		float d = pow(pow(diff.x,2.0f)+ pow(diff.y, 2.0f)+ pow(diff.z, 2.0f),0.5f);//calculate 3d distance
		//is the user in the finish buffer zone?
		if (d < buffer)
		{
			if (set_iPluginDebug)
			{
				PrintUserCmdText(iClientID, L"RACEME: movement event detected: ");
				PrintUserCmdText(iClientID, stows(ftos(diff.x)));
				PrintUserCmdText(iClientID, stows(ftos(diff.y)));
				PrintUserCmdText(iClientID, stows(ftos(diff.z)));
				PrintUserCmdText(iClientID, stows(ftos(d)));
			}
			mapRegisteredRacers[iClientID].bRacing = false;//race finished
			time_duration finTime = ptimeNow - mapRegisteredRacers[iClientID].iTimeStart;//save timer
			wstring wsFinMsg = L" Has completed " + stows(mapRegisteredRaceTracks[mapRegisteredRacers[iClientID].iTrackId].sTrackNameFriendly) + L" in " + to_simple_wstring(finTime);
			PrintUserCmdText(iClientID, wsFinMsg);//tell the user
			PrintLocalUserCmdText(iClientID, wsFinMsg, 15000.f);//tell everyone within 15k
			playerNotification(iClientID, L"Final Time: " + to_simple_wstring(finTime));
			pub::Audio::CancelMusic(iClientID);
		}
	}
	//nevermind; not ours
	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Client command processing
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef bool(*_UserCmdProc)(uint, const wstring &, const wstring &, const wchar_t*);

struct USERCMD
{
	wchar_t *wszCmd;
	_UserCmdProc proc;
	wchar_t *usage;
};

USERCMD UserCmds[] =
{
	{ L"/startrace", UserCmd_StartRace, L"Usage: /startrace id" },
	{ L"/showtracks", UserCmd_ShowTracks, L"Usage: /showtracks" },
	{ L"/dev", UserCmd_dev, L"Usage: /showtracks" },
};

/**
This function is called by FLHook when a user types a chat string. We look at the
string they've typed and see if it starts with one of the above commands. If it
does we try to process it.
*/
bool UserCmd_Process(uint iClientID, const wstring &wscCmd)
{
	returncode = DEFAULT_RETURNCODE;

	wstring wscCmdLineLower = ToLower(wscCmd);

	// If the chat string does not match the USER_CMD then we do not handle the
	// command, so let other plugins or FLHook kick in. We require an exact match
	for (uint i = 0; (i < sizeof(UserCmds) / sizeof(USERCMD)); i++)
	{

		if (wscCmdLineLower.find(UserCmds[i].wszCmd) == 0)
		{
			// Extract the parameters string from the chat string. It should
			// be immediately after the command and a space.
			wstring wscParam = L"";
			if (wscCmd.length() > wcslen(UserCmds[i].wszCmd))
			{
				if (wscCmd[wcslen(UserCmds[i].wszCmd)] != ' ')
					continue;
				wscParam = wscCmd.substr(wcslen(UserCmds[i].wszCmd) + 1);
			}

			// Dispatch the command to the appropriate processing function.
			if (UserCmds[i].proc(iClientID, wscCmd, wscParam, UserCmds[i].usage))
			{
				// We handled the command tell FL hook to stop processing this
				// chat string.
				returncode = SKIPPLUGINS_NOFUNCTIONCALL; // we handled the command, return immediatly
				return true;
			}
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions to hook
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO* p_PI = new PLUGIN_INFO();
	p_PI->sName = "RaceMe by HeyImRuu";
	p_PI->sShortName = "RaceMe";
	p_PI->bMayPause = true;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;

	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ClearClientInfo, PLUGIN_ClearClientInfo, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&SPObjUpdate, PLUGIN_HkIServerImpl_SPObjUpdate, 0));

	return p_PI;
}
