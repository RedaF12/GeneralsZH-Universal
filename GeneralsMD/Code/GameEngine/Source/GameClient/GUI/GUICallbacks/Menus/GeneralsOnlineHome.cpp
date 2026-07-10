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

// FILE: GeneralsOnlineHome.cpp ///////////////////////////////////////////////////////////////////
// Desc:   GeneralsOnline lobby hub screen (Android port) -- entry point once
//         TryStartGeneralsOnline() (GeneralsOnline_AndroidGlue.cpp) succeeds.
//         QUICKMATCH / CUSTOM MATCH / MY PERSONA / COMMUNICATOR / OPTIONS,
//         mirroring the reference GeneralsOnline client's "Welcome" screen.
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

static GameWindow *labelWelcome = nullptr;
static GameWindow *labelStats = nullptr;

//-------------------------------------------------------------------------------------------------
static void refreshStatsLabel()
{
	if (labelStats == nullptr)
		return;

	NGMP_OnlineServices_AuthInterface *pAuth = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_AuthInterface>();
	NGMP_OnlineServices_StatsInterface *pStats = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_StatsInterface>();
	if (pAuth == nullptr || pStats == nullptr)
		return;

	int64_t myID = pAuth->GetUserID();

	PSPlayerStats stats;
	if (pStats->getPlayerStatsFromCache(myID, &stats))
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
		text.format(UnicodeString(L"Wins: %d   Losses: %d   Win %%: %d%%"), totalWins, totalLosses, winPct);
		GadgetStaticTextSetText(labelStats, text);
	}
	else
	{
		// Not cached yet -- kick off a fetch and try again once it lands.
		pStats->findPlayerStatsByID(myID, [](bool bSuccess, PSPlayerStats)
			{
				refreshStatsLabel();
			}, EStatsRequestPolicy::RESPECT_CACHE_ALLOW_REQUEST);
	}
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlineHomeInit(WindowLayout *layout, void *userData)
{
	NameKeyType labelWelcomeID = TheNameKeyGenerator->nameToKey("GeneralsOnlineHome.wnd:LabelWelcome");
	NameKeyType labelStatsID = TheNameKeyGenerator->nameToKey("GeneralsOnlineHome.wnd:LabelStats");

	labelWelcome = TheWindowManager->winGetWindowFromId(nullptr, labelWelcomeID);
	labelStats = TheWindowManager->winGetWindowFromId(nullptr, labelStatsID);

	NGMP_OnlineServices_AuthInterface *pAuth = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_AuthInterface>();
	if (labelWelcome != nullptr && pAuth != nullptr)
	{
		UnicodeString text;
		text.format(UnicodeString(L"Connected as %hs"), pAuth->GetDisplayName().c_str());
		GadgetStaticTextSetText(labelWelcome, text);
	}

	refreshStatsLabel();
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlineHomeUpdate(WindowLayout *layout, void *userData)
{
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlineHomeShutdown(WindowLayout *layout, void *userData)
{
	labelWelcome = nullptr;
	labelStats = nullptr;
}

//-------------------------------------------------------------------------------------------------
WindowMsgHandledType GeneralsOnlineHomeSystem(GameWindow *window, UnsignedInt msg,
																				WindowMsgData mData1, WindowMsgData mData2)
{
	static NameKeyType buttonQuickMatch = NAMEKEY_INVALID;
	static NameKeyType buttonCustomMatch = NAMEKEY_INVALID;
	static NameKeyType buttonMyPersona = NAMEKEY_INVALID;
	static NameKeyType buttonCommunicator = NAMEKEY_INVALID;
	static NameKeyType buttonOptions = NAMEKEY_INVALID;
	static NameKeyType buttonDisconnect = NAMEKEY_INVALID;

	switch (msg)
	{
		case GWM_CREATE:
		{
			buttonQuickMatch = TheNameKeyGenerator->nameToKey("GeneralsOnlineHome.wnd:ButtonQuickMatch");
			buttonCustomMatch = TheNameKeyGenerator->nameToKey("GeneralsOnlineHome.wnd:ButtonCustomMatch");
			buttonMyPersona = TheNameKeyGenerator->nameToKey("GeneralsOnlineHome.wnd:ButtonMyPersona");
			buttonCommunicator = TheNameKeyGenerator->nameToKey("GeneralsOnlineHome.wnd:ButtonCommunicator");
			buttonOptions = TheNameKeyGenerator->nameToKey("GeneralsOnlineHome.wnd:ButtonOptions");
			buttonDisconnect = TheNameKeyGenerator->nameToKey("GeneralsOnlineHome.wnd:ButtonDisconnect");
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

			if (controlID == buttonQuickMatch)
			{
				TheShell->push("Menus/GeneralsOnlineQuickMatch.wnd");
			}
			else if (controlID == buttonCustomMatch)
			{
				TheShell->push("Menus/GeneralsOnlineCustomMatch.wnd");
			}
			else if (controlID == buttonMyPersona)
			{
				TheShell->push("Menus/GeneralsOnlinePersona.wnd");
			}
			else if (controlID == buttonCommunicator)
			{
				TheShell->push("Menus/GeneralsOnlineCommunicator.wnd");
			}
			else if (controlID == buttonOptions)
			{
				// Same cached-layout pattern MainMenu uses for its own Options
				// button -- GeneralsOnline doesn't need its own options screen,
				// the game's normal one already covers video/audio/controls.
				WindowLayout *optLayout = TheShell->getOptionsLayout(TRUE);
				if (optLayout != nullptr)
				{
					optLayout->runInit();
					optLayout->hide(FALSE);
					optLayout->bringForward();
				}
			}
			else if (controlID == buttonDisconnect)
			{
				// Leaves the lobby UI only -- the GeneralsOnline session and
				// WebSocket connection stay up in the background (so friends
				// still see us online), matching how the main menu's own
				// "Online" button behaves once wired to a real lobby browser.
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
WindowMsgHandledType GeneralsOnlineHomeInput(GameWindow *window, UnsignedInt msg,
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
