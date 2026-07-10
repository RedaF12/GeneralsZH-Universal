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

// FILE: GeneralsOnlineCommunicator.cpp ///////////////////////////////////////////////////////////
// Desc:   GeneralsOnline Communicator (friends list) screen (Android port).
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"

#include "GameClient/Shell.h"
#include "GameClient/GUICallbacks.h"
#include "GameClient/GameWindow.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/Gadget.h"
#include "GameClient/GadgetListBox.h"
#include "GameClient/KeyDefs.h"
#include "GameClient/WindowLayout.h"
#include "Common/NameKeyGenerator.h"
#include "GameNetwork/GeneralsOnline/NGMP_interfaces.h"

static GameWindow *listboxFriends = nullptr;

//-------------------------------------------------------------------------------------------------
static void populateFriendsList()
{
	if (listboxFriends == nullptr)
		return;

	NGMP_OnlineServices_SocialInterface *pSocial = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_SocialInterface>();
	if (pSocial == nullptr)
		return;

	GadgetListBoxReset(listboxFriends);

	for (const auto &kv : pSocial->GetCachedFriendsList())
	{
		const FriendsEntry &friendEntry = kv.second;

		UnicodeString name, status;
		name.translate(AsciiString(friendEntry.display_name.c_str()));
		status.format(UnicodeString(L"%hs"), friendEntry.online ? "Online" : "Offline");

		Color color = friendEntry.online ? GameMakeColor(120, 255, 120, 255) : GameMakeColor(160, 160, 160, 255);

		Int row = GadgetListBoxAddEntryText(listboxFriends, name, color, -1, 0);
		GadgetListBoxAddEntryText(listboxFriends, status, color, row, 1);
	}
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlineCommunicatorInit(WindowLayout *layout, void *userData)
{
	NameKeyType listboxID = TheNameKeyGenerator->nameToKey("GeneralsOnlineCommunicator.wnd:ListboxFriends");
	listboxFriends = TheWindowManager->winGetWindowFromId(nullptr, listboxID);

	NGMP_OnlineServices_SocialInterface *pSocial = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_SocialInterface>();
	if (pSocial != nullptr)
	{
		// Friends/blocked lists were already fetched once at login (see
		// GeneralsOnline_AndroidGlue.cpp / OnLogin()); refresh here in case
		// they've changed since, then just render whatever's cached.
		pSocial->GetFriendsList(false, []()
			{
				populateFriendsList();
			});
	}

	populateFriendsList();
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlineCommunicatorUpdate(WindowLayout *layout, void *userData)
{
}

//-------------------------------------------------------------------------------------------------
void GeneralsOnlineCommunicatorShutdown(WindowLayout *layout, void *userData)
{
	listboxFriends = nullptr;
}

//-------------------------------------------------------------------------------------------------
WindowMsgHandledType GeneralsOnlineCommunicatorSystem(GameWindow *window, UnsignedInt msg,
																				WindowMsgData mData1, WindowMsgData mData2)
{
	static NameKeyType buttonRefreshID = NAMEKEY_INVALID;
	static NameKeyType buttonBackID = NAMEKEY_INVALID;

	switch (msg)
	{
		case GWM_CREATE:
		{
			buttonRefreshID = TheNameKeyGenerator->nameToKey("GeneralsOnlineCommunicator.wnd:ButtonRefresh");
			buttonBackID = TheNameKeyGenerator->nameToKey("GeneralsOnlineCommunicator.wnd:ButtonBack");
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
				NGMP_OnlineServices_SocialInterface *pSocial = NGMP_OnlineServicesManager::GetInterface<NGMP_OnlineServices_SocialInterface>();
				if (pSocial != nullptr)
				{
					pSocial->GetFriendsList(false, []()
						{
							populateFriendsList();
						});
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
WindowMsgHandledType GeneralsOnlineCommunicatorInput(GameWindow *window, UnsignedInt msg,
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
