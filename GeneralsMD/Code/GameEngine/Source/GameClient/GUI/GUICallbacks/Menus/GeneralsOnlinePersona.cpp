/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// FILE: GeneralsOnlinePersona.cpp ////////////////////////////////////////////////////////////////
// Desc:   GeneralsOnline "My Persona" stats screen (Android port).
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"

#include "GameClient/Shell.h"
#include "GameClient/GUICallbacks.h"
#include "GameClient/GameWindow.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/Gadget.h"
#include "GameClient/GadgetStaticText.h"
#include "GameClient/KeyDefs.h"
#include "GameClient/WindowLayout.h"
#include "Common/NameKeyGenerator.h"
#include "GameNetwork/GeneralsOnline/NGMP_interfaces.h"

static GameWindow *labelName = nullptr;
static GameWindow *labelUserID = nullptr;
static GameWindow *labelWins = nullptr;
static GameWindow *labelLosses = nullptr;
static GameWindow *labelWinPct = nullptr;
static GameWindow *labelElo = nullptr;

//-------------------------------------------------------------------------------------------------
static void populateFromStats(int64_t userID, const PSPlayerStats &stats)
{
	Int totalWins = 0;
	Int totalLosses = 0;
	PerGeneralMap::const_iterator it;
	for (it = stats.wins.begin(); it != stats.wins.end(); ++it)
		totalWins += it->second;
	for (it = stats.losses.begin(); it != stats.losses.end(); ++it)
		totalLosses += it->second;

	Int totalGames = totalWins + totalLosses;
	Int winPct = (totalGames > 0) ? (totalWins * 100 / totalGames) : 0;

	UnicodeString text;

	if (labelWins != nullptr)
	{
		text.format(UnicodeString(L"Wins: %d"), totalWins);
		GadgetStaticTextSetText(labelWins, text);
	}
	if (labelLosses != nullptr)
	{
		text.format(UnicodeString(L"Losses: %d"), totalLosses);
		GadgetStaticTextSetText(labelLosses, text);
	}
	if (labelWinPct != nullptr)
	{
		text.format(UnicodeString(L"Win %%: %d%%"), winPct);
		GadgetStaticTextSetText(labelWinPct, text);
	}
#if defined(GENERALS_ONLINE)
	if (labelElo != nullptr)
	{
		text.format(UnicodeString(L"ELO: %d (%d matches)"), stats.elo_rating, stats.elo_num_matches);
		GadgetStaticTextSetText(labelElo, text);
	}
#endif
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlinePersonaInit(WindowLayout *layout, void *userData)
{
	labelName = TheWindowManager->winGetWindowFromId(nullptr, TheNameKeyGenerator->nameToKey("GeneralsOnlinePersona.wnd:LabelName"));
	labelUserID = TheWindowManager->winGetWindowFromId(nullptr, TheNameKeyGenerator->nameToKey("GeneralsOnlinePersona.wnd:LabelUserID"));
	labelWins = TheWindowManager->winGetWindowFromId(nullptr, TheNameKeyGenerator->nameToKey("GeneralsOnlinePersona.wnd:LabelWins"));
	labelLosses = TheWindowManager->winGetWindowFromId(nullptr, TheNameKeyGenerator->nameToKey("GeneralsOnlinePersona.wnd:LabelLosses"));
	labelWinPct = TheWindowManager->winGetWindowFromId(nullptr, TheNameKeyGenerator->nameToKey("GeneralsOnlinePersona.wnd:LabelWinPct"));
	labelElo = TheWindowManager->winGetWindowFromId(nullptr, TheNameKeyGenerator->nameToKey("GeneralsOnlinePersona.wnd:LabelElo"));

	NGMP_OnlineServices_AuthInterface *pAuth = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_AuthInterface>();
	NGMP_OnlineServices_StatsInterface *pStats = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_StatsInterface>();
	if (pAuth == nullptr)
		return;

	int64_t myID = pAuth->GetUserID();

	if (labelName != nullptr)
	{
		UnicodeString text;
		text.format(UnicodeString(L"Name: %hs"), pAuth->GetDisplayName().c_str());
		GadgetStaticTextSetText(labelName, text);
	}
	if (labelUserID != nullptr)
	{
		UnicodeString text;
		text.format(UnicodeString(L"User ID: %lld"), (long long)myID);
		GadgetStaticTextSetText(labelUserID, text);
	}

	if (pStats == nullptr)
		return;

	PSPlayerStats stats;
	if (pStats->getPlayerStatsFromCache(myID, &stats))
	{
		populateFromStats(myID, stats);
	}
	else
	{
		pStats->findPlayerStatsByID(myID, [myID](bool bSuccess, PSPlayerStats stats)
			{
				if (bSuccess)
					populateFromStats(myID, stats);
			}, EStatsRequestPolicy::RESPECT_CACHE_ALLOW_REQUEST);
	}
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlinePersonaUpdate(WindowLayout *layout, void *userData)
{
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlinePersonaShutdown(WindowLayout *layout, void *userData)
{
	labelName = nullptr;
	labelUserID = nullptr;
	labelWins = nullptr;
	labelLosses = nullptr;
	labelWinPct = nullptr;
	labelElo = nullptr;
}

//-------------------------------------------------------------------------------------------------
WindowMsgHandledType GeneralsOnlinePersonaSystem(GameWindow *window, UnsignedInt msg,
																				WindowMsgData mData1, WindowMsgData mData2)
{
	static NameKeyType buttonBackID = NAMEKEY_INVALID;

	switch (msg)
	{
		case GWM_CREATE:
		{
			buttonBackID = TheNameKeyGenerator->nameToKey("GeneralsOnlinePersona.wnd:ButtonBack");
			break;
		}

		case GWM_DESTROY:
			break;

		case GWM_INPUT_FOCUS:
		{
			if (mData1 == TRUE)
				*(Bool *)mData2 = TRUE;
			return MSG_HANDLED;
		}

		case GBM_SELECTED:
		{
			GameWindow *control = (GameWindow *)mData1;
			Int controlID = control->winGetWindowId();

			if (controlID == buttonBackID)
			{
				TheShell->pop();
			}
			break;
		}

		default:
			break;
	}

	return MSG_IGNORED;
}

//-------------------------------------------------------------------------------------------------
WindowMsgHandledType GeneralsOnlinePersonaInput(GameWindow *window, UnsignedInt msg,
																				WindowMsgData mData1, WindowMsgData mData2)
{
	switch (msg)
	{
		case GWM_CHAR:
		{
			UnsignedByte key = mData1;
			UnsignedByte state = mData2;

			if (key == KEY_ESC)
			{
				if (BitIsSet(state, KEY_STATE_UP))
				{
					TheShell->pop();
				}
				return MSG_HANDLED;
			}
			break;
		}
	}

	return MSG_IGNORED;
}
