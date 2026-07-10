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

// FILE: GeneralsOnlineCustomMatch.cpp ////////////////////////////////////////////////////////////
// Desc:   GeneralsOnline Custom Match lobby browser (Android port). Lists
//         open lobbies via SearchForLobbies() and can create/join one.
//
//         NOTE: joining or creating a lobby succeeds server-side, but there
//         is no in-lobby "room" screen yet (player slots, chat, ready-up,
//         map/side pickers) -- that is upstream's WOLGameSetupMenu.wnd
//         equivalent, deliberately not ported in this pass (see
//         GeneralsOnline_AndroidGlue.cpp's header comment). This screen says
//         so plainly via a message box rather than silently doing nothing.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"

#include "GameClient/Shell.h"
#include "GameClient/GUICallbacks.h"
#include "GameClient/GameWindow.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/Gadget.h"
#include "GameClient/GadgetListBox.h"
#include "GameClient/GadgetStaticText.h"
#include "GameClient/KeyDefs.h"
#include "GameClient/WindowLayout.h"
#include "Common/NameKeyGenerator.h"
#include "GameNetwork/GeneralsOnline/NGMP_interfaces.h"
#include "GameNetwork/GameSpyOverlay.h"

static GameWindow *listboxLobbies = nullptr;

static std::vector<LobbyEntry> s_lastLobbies;

//-------------------------------------------------------------------------------------------------
static void refreshLobbyList()
{
	if (listboxLobbies == nullptr)
		return;

	NGMP_OnlineServices_LobbyInterface *pLobby = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_LobbyInterface>();
	if (pLobby == nullptr)
		return;

	GadgetListBoxReset(listboxLobbies);
	s_lastLobbies.clear();

	pLobby->SearchForLobbies(
		[]() { /* search started -- no dedicated "loading" row, list just stays empty briefly */ },
		[](std::vector<LobbyEntry> vecLobbies)
		{
			if (listboxLobbies == nullptr)
				return;

			s_lastLobbies = vecLobbies;
			GadgetListBoxReset(listboxLobbies);

			for (const LobbyEntry &entry : vecLobbies)
			{
				UnicodeString name, map, players;
				name.translate(AsciiString(entry.name.c_str()));
				map.translate(AsciiString(entry.map_name.c_str()));
				players.format(UnicodeString(L"%d/%d"), entry.current_players, entry.max_players);

				Int row = GadgetListBoxAddEntryText(listboxLobbies, name, GameMakeColor(255, 255, 255, 255), -1, 0);
				GadgetListBoxAddEntryText(listboxLobbies, map, GameMakeColor(255, 255, 255, 255), row, 1);
				GadgetListBoxAddEntryText(listboxLobbies, players, GameMakeColor(255, 255, 255, 255), row, 2);
			}
		});
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlineCustomMatchInit(WindowLayout *layout, void *userData)
{
	NameKeyType listboxID = TheNameKeyGenerator->nameToKey("GeneralsOnlineCustomMatch.wnd:ListboxLobbies");
	listboxLobbies = TheWindowManager->winGetWindowFromId(nullptr, listboxID);

	NGMP_OnlineServices_LobbyInterface *pLobby = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_LobbyInterface>();
	if (pLobby != nullptr)
	{
		pLobby->RegisterForCreateLobbyCallback([](bool bSuccess)
			{
				ClearGSMessageBoxes();
				if (bSuccess)
				{
					GSMessageBoxOk(UnicodeString(L"Custom Match"),
						UnicodeString(L"Lobby created.\n\n(There's no in-lobby room screen yet -- match setup, chat and ready-up need a follow-up screen.)"), nullptr);
				}
				else
				{
					GSMessageBoxOk(UnicodeString(L"Custom Match"), UnicodeString(L"Failed to create lobby."), nullptr);
				}
			});

		pLobby->RegisterForJoinLobbyCallback([](EJoinLobbyResult result)
			{
				ClearGSMessageBoxes();
				switch (result)
				{
					case EJoinLobbyResult::JoinLobbyResult_Success:
						GSMessageBoxOk(UnicodeString(L"Custom Match"),
							UnicodeString(L"Joined lobby.\n\n(There's no in-lobby room screen yet -- match setup, chat and ready-up need a follow-up screen.)"), nullptr);
						break;
					case EJoinLobbyResult::JoinLobbyResult_FullRoom:
						GSMessageBoxOk(UnicodeString(L"Custom Match"), UnicodeString(L"That lobby is full."), nullptr);
						break;
					case EJoinLobbyResult::JoinLobbyResult_BadPassword:
						GSMessageBoxOk(UnicodeString(L"Custom Match"), UnicodeString(L"That lobby needs a password."), nullptr);
						break;
					default:
						GSMessageBoxOk(UnicodeString(L"Custom Match"), UnicodeString(L"Failed to join lobby."), nullptr);
						break;
				}
			});
	}

	refreshLobbyList();
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlineCustomMatchUpdate(WindowLayout *layout, void *userData)
{
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlineCustomMatchShutdown(WindowLayout *layout, void *userData)
{
	NGMP_OnlineServices_LobbyInterface *pLobby = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_LobbyInterface>();
	if (pLobby != nullptr)
	{
		pLobby->DeregisterForCreateLobbyCallback();
		pLobby->DeregisterForJoinLobbyCallback();
	}

	listboxLobbies = nullptr;
	s_lastLobbies.clear();
}

//-------------------------------------------------------------------------------------------------
WindowMsgHandledType GeneralsOnlineCustomMatchSystem(GameWindow *window, UnsignedInt msg,
																				WindowMsgData mData1, WindowMsgData mData2)
{
	static NameKeyType listboxID = NAMEKEY_INVALID;
	static NameKeyType buttonRefreshID = NAMEKEY_INVALID;
	static NameKeyType buttonCreateID = NAMEKEY_INVALID;
	static NameKeyType buttonJoinID = NAMEKEY_INVALID;
	static NameKeyType buttonBackID = NAMEKEY_INVALID;

	switch (msg)
	{
		case GWM_CREATE:
		{
			listboxID = TheNameKeyGenerator->nameToKey("GeneralsOnlineCustomMatch.wnd:ListboxLobbies");
			buttonRefreshID = TheNameKeyGenerator->nameToKey("GeneralsOnlineCustomMatch.wnd:ButtonRefresh");
			buttonCreateID = TheNameKeyGenerator->nameToKey("GeneralsOnlineCustomMatch.wnd:ButtonCreate");
			buttonJoinID = TheNameKeyGenerator->nameToKey("GeneralsOnlineCustomMatch.wnd:ButtonJoin");
			buttonBackID = TheNameKeyGenerator->nameToKey("GeneralsOnlineCustomMatch.wnd:ButtonBack");
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

			if (controlID == buttonRefreshID)
			{
				refreshLobbyList();
			}
			else if (controlID == buttonCreateID)
			{
				NGMP_OnlineServices_LobbyInterface *pLobby = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_LobbyInterface>();
				NGMP_OnlineServices_AuthInterface *pAuth = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_AuthInterface>();
				if (pLobby != nullptr && pAuth != nullptr)
				{
					UnicodeString lobbyName;
					lobbyName.format(UnicodeString(L"%hs's Game"), pAuth->GetDisplayName().c_str());
					pLobby->CreateLobby(lobbyName, UnicodeString(L""), AsciiString::TheEmptyString,
						/*bIsOfficial*/ false, /*initialMaxSize*/ 8, /*bVanillaTeamsOnly*/ false,
						/*bTrackStats*/ true, /*startingCash*/ 10000, /*bPassworded*/ false,
						/*strPassword*/ std::string(), /*bAllowObservers*/ true);
				}
			}
			else if (controlID == buttonJoinID)
			{
				Int selected[1] = { -1 };
				GadgetListBoxGetSelected(listboxLobbies, selected);
				if (selected[0] >= 0 && selected[0] < (Int)s_lastLobbies.size())
				{
					NGMP_OnlineServices_LobbyInterface *pLobby = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_LobbyInterface>();
					if (pLobby != nullptr)
					{
						pLobby->JoinLobby(s_lastLobbies[selected[0]], std::string());
					}
				}
				else
				{
					ClearGSMessageBoxes();
					GSMessageBoxOk(UnicodeString(L"Custom Match"), UnicodeString(L"Select a lobby first."), nullptr);
				}
			}
			else if (controlID == buttonBackID)
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
WindowMsgHandledType GeneralsOnlineCustomMatchInput(GameWindow *window, UnsignedInt msg,
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
