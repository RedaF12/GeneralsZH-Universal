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

// FILE: GeneralsOnlineQuickMatch.cpp /////////////////////////////////////////////////////////////
// Desc:   GeneralsOnline QuickMatch screen (Android port). Kicks off
//         automatic matchmaking on the first playlist the server reports and
//         shows status updates as they arrive.
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

static GameWindow *labelStatus = nullptr;
static GameWindow *labelDetail = nullptr;
static GameWindow *buttonSearch = nullptr;

static bool s_isSearching = false;
static bool s_havePlaylists = false;
static uint16_t s_playlistID = 0;

//-------------------------------------------------------------------------------------------------
static void setStatus(const wchar_t *status)
{
	if (labelStatus != nullptr)
		GadgetStaticTextSetText(labelStatus, UnicodeString(status));
}

//-------------------------------------------------------------------------------------------------
static void setDetail(const std::string &detail)
{
	if (labelDetail != nullptr)
	{
		UnicodeString text;
		text.format(UnicodeString(L"%hs"), detail.c_str());
		GadgetStaticTextSetText(labelDetail, text);
	}
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlineQuickMatchInit(WindowLayout *layout, void *userData)
{
	NameKeyType labelStatusID = TheNameKeyGenerator->nameToKey("GeneralsOnlineQuickMatch.wnd:LabelStatus");
	NameKeyType labelDetailID = TheNameKeyGenerator->nameToKey("GeneralsOnlineQuickMatch.wnd:LabelDetail");
	NameKeyType buttonSearchID = TheNameKeyGenerator->nameToKey("GeneralsOnlineQuickMatch.wnd:ButtonSearch");

	labelStatus = TheWindowManager->winGetWindowFromId(nullptr, labelStatusID);
	labelDetail = TheWindowManager->winGetWindowFromId(nullptr, labelDetailID);
	buttonSearch = TheWindowManager->winGetWindowFromId(nullptr, buttonSearchID);

	s_isSearching = false;
	setStatus(L"Idle");
	setDetail("Fetching playlists...");

	NGMP_OnlineServices_LobbyInterface *pLobby = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_LobbyInterface>();
	if (pLobby != nullptr)
	{
		pLobby->RegisterForMatchmakingMessageCallback([](std::string strMsg)
			{
				setDetail(strMsg);
			});
		pLobby->RegisterForMatchmakingMatchFoundCallback([]()
			{
				setStatus(L"Match found!");
			});
		pLobby->RegisterForMatchmakingStartGameCallback([]()
			{
				setStatus(L"Starting game...");
			});
	}

	NGMP_OnlineServices_MatchmakingInterface *pMatchmaking = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_MatchmakingInterface>();
	if (pMatchmaking != nullptr)
	{
		pMatchmaking->RetrievePlaylists([](std::vector<PlaylistEntry> vecPlaylists)
			{
				s_havePlaylists = !vecPlaylists.empty();
				if (s_havePlaylists)
				{
					s_playlistID = vecPlaylists[0].PlaylistID;
					setDetail(std::string("Ready: ") + vecPlaylists[0].Name);
				}
				else
				{
					setDetail("No playlists available.");
				}
			});
	}
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlineQuickMatchUpdate(WindowLayout *layout, void *userData)
{
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlineQuickMatchShutdown(WindowLayout *layout, void *userData)
{
	NGMP_OnlineServices_LobbyInterface *pLobby = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_LobbyInterface>();
	if (pLobby != nullptr)
	{
		pLobby->RegisterForMatchmakingMessageCallback(nullptr);
		pLobby->DeRegisterForMatchmakingMatchFoundCallback();
		pLobby->RegisterForMatchmakingStartGameCallback(nullptr);
	}

	if (s_isSearching)
	{
		NGMP_OnlineServices_MatchmakingInterface *pMatchmaking = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_MatchmakingInterface>();
		if (pMatchmaking != nullptr)
		{
			pMatchmaking->CancelMatchmaking();
		}
		s_isSearching = false;
	}

	labelStatus = nullptr;
	labelDetail = nullptr;
	buttonSearch = nullptr;
}

//-------------------------------------------------------------------------------------------------
static void toggleSearch()
{
	NGMP_OnlineServices_MatchmakingInterface *pMatchmaking = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_MatchmakingInterface>();
	if (pMatchmaking == nullptr)
		return;

	if (!s_isSearching)
	{
		if (!s_havePlaylists)
		{
			setDetail("Playlists not loaded yet.");
			return;
		}

		s_isSearching = true;
		setStatus(L"Searching for match...");
		if (buttonSearch != nullptr)
			buttonSearch->winSetText(UnicodeString(L"Cancel Search"));

		pMatchmaking->StartMatchmaking(s_playlistID, std::vector<int>(), [](bool bSuccess)
			{
				if (!bSuccess)
				{
					s_isSearching = false;
					setStatus(L"Idle");
					setDetail("Matchmaking failed to start.");
					if (buttonSearch != nullptr)
						buttonSearch->winSetText(UnicodeString(L"Start Search"));
				}
			});
	}
	else
	{
		pMatchmaking->CancelMatchmaking();
		s_isSearching = false;
		setStatus(L"Idle");
		setDetail("Search cancelled.");
		if (buttonSearch != nullptr)
			buttonSearch->winSetText(UnicodeString(L"Start Search"));
	}
}

//-------------------------------------------------------------------------------------------------
WindowMsgHandledType GeneralsOnlineQuickMatchSystem(GameWindow *window, UnsignedInt msg,
																				WindowMsgData mData1, WindowMsgData mData2)
{
	static NameKeyType buttonSearchID = NAMEKEY_INVALID;
	static NameKeyType buttonBackID = NAMEKEY_INVALID;

	switch (msg)
	{
		case GWM_CREATE:
		{
			buttonSearchID = TheNameKeyGenerator->nameToKey("GeneralsOnlineQuickMatch.wnd:ButtonSearch");
			buttonBackID = TheNameKeyGenerator->nameToKey("GeneralsOnlineQuickMatch.wnd:ButtonBack");
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

			if (controlID == buttonSearchID)
			{
				toggleSearch();
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
WindowMsgHandledType GeneralsOnlineQuickMatchInput(GameWindow *window, UnsignedInt msg,
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
