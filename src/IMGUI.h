/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

#pragma once

#include <vector>
#include <string>
#include <queue>

#include "Vector.h"
#include "Global.h"

#include "RenderManager.h"
#include "InputManager.h"
#include "TextManager.h" /// needed because we can't forward declare that enum
#include "BlobbyDebug.h"

enum SelectBoxAction
{
	SBA_NONE = 0,
	SBA_SELECT,
	SBA_DBL_CLICK
};

struct QueueObject;

typedef std::queue<QueueObject> RenderQueue;

/*! \class IMGUI
	\brief GUI Manager
	\details This class manages drawing and input handling of the blobby GUI.
			It is poorly designed, does not use OOP and makes extension difficult, so
			it needs a complete rewrite.
*/
class IMGUI : public ObjectCounter<IMGUI>
{
	public:
		explicit IMGUI(InputManager* inputMgr);
		~IMGUI();

		void begin();
		void end(RenderManager& renderer);
		void resetSelection();

		void doImage(int id, const Vector2& position, const std::string& name, const Vector2& size = Vector2(0,0));
#ifdef MIYOO_MINI
		void doText(int id, const Vector2& position, const std::string& text, unsigned int flags = TF_SMALL_FONT);
		void doText(int id, const Vector2& position, TextManager::STRING text, unsigned int flags = TF_SMALL_FONT);
#else
		void doText(int id, const Vector2& position, const std::string& text, unsigned int flags = TF_NORMAL);
		void doText(int id, const Vector2& position, TextManager::STRING text, unsigned int flags = TF_NORMAL);
#endif
		void doOverlay(int id, const Vector2& pos1, const Vector2& pos2, const Color& col = Color(0, 0, 0), float alpha = 0.65);
		void doCursor(bool draw = true);
		
#ifdef MIYOO_MINI
		bool doButton(int id, const Vector2& position, const std::string& text, unsigned int flags = TF_SMALL_FONT, bool forceMenu = false);
		bool doButton(int id, const Vector2& position, TextManager::STRING text, unsigned int flags = TF_SMALL_FONT, bool forceMenu = false);
#else
		bool doButton(int id, const Vector2& position, const std::string& text, unsigned int flags = TF_NORMAL, bool forceMenu = false);
		bool doButton(int id, const Vector2& position, TextManager::STRING text, unsigned int flags = TF_NORMAL, bool forceMenu = false);
#endif		
		// draws an image that also works as a button
		// for now, it is not included in keyboard navigation, so it is more like a clickable image than a real button
		/// \todo the size parameter should be calculated from the passed image
		bool doImageButton(int id, const Vector2& position, const Vector2& size, const std::string& image);

		bool doScrollbar(int id, const Vector2& position, float& value);
#ifdef MIYOO_MINI
		bool doEditbox(int id, const Vector2& position, unsigned length, std::string& text, unsigned& cpos, unsigned int flags = TF_SMALL_FONT, bool force_active = false);
		SelectBoxAction doSelectbox(int id, const Vector2& pos1, const Vector2& pos2, const std::vector<std::string>& entries, unsigned& selected, unsigned int flags = TF_SMALL_FONT);
		void doChatbox(int id, const Vector2& pos1, const Vector2& pos2, const std::vector<std::string>& entries, unsigned& selected, const std::vector<bool>& local, unsigned int flags = TF_SMALL_FONT);
#else
		bool doEditbox(int id, const Vector2& position, unsigned length, std::string& text, unsigned& cpos, unsigned int flags = TF_NORMAL, bool force_active = false);
		SelectBoxAction doSelectbox(int id, const Vector2& pos1, const Vector2& pos2, const std::vector<std::string>& entries, unsigned& selected, unsigned int flags = TF_NORMAL);
		void doChatbox(int id, const Vector2& pos1, const Vector2& pos2, const std::vector<std::string>& entries, unsigned& selected, const std::vector<bool>& local, unsigned int flags = TF_NORMAL);
#endif
		bool doBlob(int id, const Vector2& position, const Color& col);

		bool usingCursor() const;
		void doInactiveMode(bool inactive) { mInactive = inactive; }

		int getNextId() { return mIdCounter++; };

		const TextManager& textMgr() const;
		const std::string& getText(TextManager::STRING id) const;
		void setTextMgr(std::string lang);

	private:
		int mActiveButton;
		int mHeldWidget;
		KeyAction mLastKeyAction;
		int mLastWidget;
		bool mDrawCursor;
		bool mButtonReset;
		bool mInactive;
		bool mUsingCursor;

		int mIdCounter;

		std::unique_ptr<TextManager> mTextManager;
		InputManager* mInputManager;

		std::unique_ptr<RenderQueue> mQueue;
};

