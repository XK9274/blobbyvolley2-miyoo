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

#if HAVE_LIBGL

#include <SDL.h>

#if __MACOSX__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#elif (defined _MSC_VER)
#include <windows.h>
#include <GL/gl.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <vector>
#include <list>
#include <set>

#include "RenderManager.h"

/*! \class RenderManagerGL2D
	\brief RenderManager on top of OpenGL
	\details This render manager uses OpenGL for drawing, SDL is only used for loading
			the images.
*/
class RenderManagerGL2D : public RenderManager
{
	public:
		RenderManagerGL2D();
		~RenderManagerGL2D();

		void init(int xResolution, int yResolution, bool fullscreen) override;
		void refresh() override;

		bool setBackground(const std::string& filename) override;
		void setBlobColor(int player, Color color) override;
		void showShadow(bool shadow) override;
#ifdef MIYOO_MINI
		void drawText(const std::string& text, Vector2 position, unsigned int flags = TF_SMALL_FONT) override;
#else
		void drawText(const std::string& text, Vector2 position, unsigned int flags = TF_NORMAL) override;
#endif
		void drawImage(const std::string& filename, Vector2 position, Vector2 size) override;
		void drawOverlay(float opacity, Vector2 pos1, Vector2 pos2, Color col) override;
		void drawBlob(const Vector2& pos, const Color& col) override;
		void startDrawParticles() override;
		void drawParticle(const Vector2& pos, int player) override;
		void endDrawParticles() override;
		void drawGame(const DuelMatchState& gameState) override;

	private:
		// Make sure this object is created before any opengl call
		SDL_GLContext mGlContext;

		struct Texture
		{
			float indices[8];
			float w, h ;
			GLuint texture;

			Texture( GLuint tex, int x, int y, int w, int h, int tw, int th );
		};

		GLuint mBackground;
		GLuint mBallShadow;

		std::vector<GLuint> mBall;
		std::vector<GLuint> mBlob;
		std::vector<GLuint> mBlobSpecular;
		std::vector<GLuint> mBlobShadow;
		std::vector<Texture> mFont;
		std::vector<Texture> mHighlightFont;
		GLuint mParticle;

		std::list<Vector2> mLastBallStates;

		bool mShowShadow;

		Color mLeftBlobColor;
		Color mRightBlobColor;

		void drawQuad(float x, float y, float width, float height);
		void drawQuad(float x, float y, const Texture& tex);
		GLuint loadTexture(SDL_Surface* surface, bool specular);
		int getNextPOT(int npot);

		void glEnable(unsigned int flag);
		void glDisable(unsigned int flag);
		void glBindTexture(GLuint texture);

		GLuint mCurrentTexture = 0;
		std::set<unsigned int> mCurrentFlags;
};


#endif
