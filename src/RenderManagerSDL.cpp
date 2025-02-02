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

/* header include */
#include "RenderManagerSDL.h"

/* includes */
#include "FileExceptions.h"
#include "DuelMatchState.h"

/* implementation */
SDL_Surface* RenderManagerSDL::colorSurface(SDL_Surface *surface, Color color)
{
	// Create new surface
	SDL_Surface *newSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);

	SDL_LockSurface(newSurface);

	Uint32* surface_pixels = (Uint32*)newSurface->pixels;

	for (int p = 0; p < newSurface->w * newSurface->h; ++p)
	{
		SDL_Color pixel;
		SDL_GetRGBA(surface_pixels[p], newSurface->format, &pixel.r, &pixel.g, &pixel.b, &pixel.a);

		const bool color_key = !(pixel.r | pixel.g | pixel.b);

		if(!color_key)
		{
			int rr = (int(pixel.r) * int(color.r)) >> 8;
			int rg = (int(pixel.g) * int(color.g)) >> 8;
			int rb = (int(pixel.b) * int(color.b)) >> 8;
			int fak = int(pixel.r) * 5 - 4 * 256 - 138;

			auto clamp = [fak](int value) {
				// Bright pixels in the original image should remain bright!
				if (fak > 0)
					value += fak;
				// This is clamped to 1 because dark colors may would be
				// color-keyed otherwise
				return std::max(1, std::min(value, 255));
			};

			surface_pixels[p] = SDL_MapRGBA(newSurface->format, clamp(rr), clamp(rg), clamp(rb), pixel.a);
		}


	}
	SDL_UnlockSurface(newSurface);

	// Use a black color-key
	SDL_SetColorKey(newSurface, SDL_TRUE, SDL_MapRGB(newSurface->format, 0, 0, 0));

	return newSurface;
}

RenderManagerSDL::RenderManagerSDL() = default;

std::unique_ptr<RenderManager> RenderManager::createRenderManagerSDL()
{
	return std::unique_ptr<RenderManager>{new RenderManagerSDL()};
}

void RenderManagerSDL::init(int xResolution, int yResolution, bool fullscreen)
{
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	// Set modesetting
	Uint32 screenFlags = 0;
#ifdef __SWITCH__
	screenFlags |= SDL_WINDOW_FULLSCREEN;
#else
	if (fullscreen)
	{
		screenFlags |= SDL_WINDOW_FULLSCREEN;
	}
	else
	{
		screenFlags |= SDL_WINDOW_RESIZABLE;
	}
#endif
	// Create window
	mWindow = SDL_CreateWindow(AppTitle,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		xResolution, yResolution,
		screenFlags);

#if !(defined __SWITCH__) && !(defined BUILD_MACOS_BUNDLE)
	// Set icon
	SDL_Surface* icon = loadSurface("Icon.bmp");
	SDL_SetColorKey(icon, SDL_TRUE,
			SDL_MapRGB(icon->format, 0, 0, 0));
	SDL_SetWindowIcon(mWindow, icon);
	SDL_FreeSurface(icon);
#endif

	// Create renderer to draw in window
#ifdef __SWITCH__
	mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#else
	mRenderer = SDL_CreateRenderer(mWindow, -1, 0);
#endif

	// Hide mousecursor
	SDL_ShowCursor(0);

	// Create rendertarget to make window resizeable
#ifdef MIYOO_MINI
	mRenderStreaming = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, xResolution, yResolution);
#else
	mRenderTarget = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, xResolution, yResolution);
#endif

	// Load all textures and surfaces to render the game
	SDL_Surface* tmpSurface;

#ifdef MIYOO_MINI
	mOverlaySurface = SDL_CreateRGBSurface(0, 1, 1, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if (!mOverlaySurface) {
		SDL_Log("Failed to create overlay surface: %s", SDL_GetError());
	} else {
		SDL_SetSurfaceBlendMode(mOverlaySurface, SDL_BLENDMODE_BLEND);
		SDL_FillRect(mOverlaySurface, NULL, SDL_MapRGBA(mOverlaySurface->format, 0, 0, 0, 0x7F));
	}
#else
	tmpSurface = SDL_CreateRGBSurface(0, 1, 1, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	// Because of SDL bug we can't check at the moment if color mod is available... no risk no fun ;)
	SDL_FillRect(tmpSurface, nullptr, SDL_MapRGB(tmpSurface->format, 255, 255, 255));
	mOverlayTexture = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
	SDL_FreeSurface(tmpSurface);
#endif

	// Load background
	tmpSurface = loadSurface("backgrounds/strand2.bmp");
	if (!tmpSurface) {
		DEBUG_STATUS("Unable to load background image.");
		return;
	}
	
	BufferedImage* bgImage = new BufferedImage;

#ifdef MIYOO_MINI
	mMiyooSurface = SDL_CreateRGBSurface(0, xResolution, yResolution, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if (!mMiyooSurface) {
		SDL_Log("Failed to create miyoo surface: %s", SDL_GetError());
	} else {
		SDL_SetSurfaceBlendMode(mMiyooSurface, SDL_BLENDMODE_BLEND);
		SDL_FillRect(mMiyooSurface, NULL, SDL_MapRGBA(mMiyooSurface->format, 0, 0, 0, 0));
	}

	mBackgroundSurface = SDL_CreateRGBSurface(0, xResolution, yResolution, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if (!mBackgroundSurface) {
		DEBUG_STATUS("Unable to create background surface.");
		delete bgImage;
		return;
	}

	SDL_Rect destRect = {0, 0, tmpSurface->w, tmpSurface->h};
	SDL_BlitSurface(tmpSurface, NULL, mBackgroundSurface, &destRect);
	bgImage->w = mBackgroundSurface->w;
	bgImage->h = mBackgroundSurface->h;
	bgImage->sdlSurface = mBackgroundSurface; 
	mImageMap["background"] = bgImage;
	SDL_BlitSurface(mBackgroundSurface, NULL, mMiyooSurface, NULL);
#else
	mBackground = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
	if (mBackground) {
		bgImage->w = tmpSurface->w;
		bgImage->h = tmpSurface->h;
		bgImage->sdlImage = mBackground;
		mImageMap["background"] = bgImage;
	} else {
		DEBUG_STATUS("Unable to create background texture.");
		delete bgImage;
	}
#endif

	SDL_FreeSurface(tmpSurface);

    // Create marker texture for mouse and ball
    tmpSurface = SDL_CreateRGBSurface(0, 5, 5, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

#ifndef MIYOO_MINI
    SDL_FillRect(tmpSurface, nullptr, SDL_MapRGB(tmpSurface->format, 255, 255, 255));
    mMarker[0] = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);

    SDL_FillRect(tmpSurface, nullptr, SDL_MapRGB(tmpSurface->format, 0, 0, 0));
    mMarker[1] = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
#endif

    SDL_FreeSurface(tmpSurface);

	// Load ball
	for (int i = 1; i <= 16; ++i)
		{
			char filename[64];
			sprintf(filename, "gfx/ball%02d.bmp", i);
			tmpSurface = loadSurface(filename);
			SDL_SetColorKey(tmpSurface, SDL_TRUE, SDL_MapRGB(tmpSurface->format, 0, 0, 0));

#ifdef MIYOO_MINI
			mBallSurfaces.push_back(tmpSurface);
#else
			SDL_Texture *ballTexture = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
			SDL_FreeSurface(tmpSurface);
			mBall.push_back(ballTexture);
#endif
	}

	// Load ball shadow
	tmpSurface = loadSurface("gfx/schball.bmp");
	if (!tmpSurface) {
			DEBUG_STATUS("Unable to load ball shadow image.");
			return;
	}

	SDL_SetColorKey(tmpSurface, SDL_TRUE, SDL_MapRGB(tmpSurface->format, 0, 0, 0));
	SDL_SetSurfaceAlphaMod(tmpSurface, 127);

#ifdef MIYOO_MINI
	mBallShadowSurf = tmpSurface;
#else
	mBallShadow = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
	SDL_FreeSurface(tmpSurface);
#endif

	// Load blobby and shadows surface
	// Load streamed textures for coloring
	for (int i = 1; i <= 5; ++i)
	{
		char filename[64];
		sprintf(filename, "gfx/blobbym%d.bmp", i);
		SDL_Surface* blobImage = loadSurface(filename);
		if (!blobImage) {
				continue;
		}

		SDL_Surface* formattedBlobImage = SDL_ConvertSurfaceFormat(blobImage, SDL_PIXELFORMAT_ABGR8888, 0);
		SDL_FreeSurface(blobImage);

		SDL_SetColorKey(formattedBlobImage, SDL_TRUE, SDL_MapRGB(formattedBlobImage->format, 0, 0, 0));
		for (int j = 0; j < formattedBlobImage->w * formattedBlobImage->h; ++j) {
				SDL_Color* pixel = &(((SDL_Color*)formattedBlobImage->pixels)[j]);
				if (!(pixel->r | pixel->g | pixel->b)) {
						pixel->a = 0;
				} 
		}

#ifdef MIYOO_MINI
		mBlobSurfaces.push_back(formattedBlobImage);
#else
		SDL_Texture* blobTexture = SDL_CreateTextureFromSurface(mRenderer, formattedBlobImage);
		SDL_FreeSurface(formattedBlobImage); 
		mStandardBlob.push_back(blobTexture);
#endif

        // Load blobby shadow surface
        sprintf(filename, "gfx/sch1%d.bmp", i);
        SDL_Surface* blobShadow = loadSurface(filename);
        SDL_Surface* formattedBlobShadowImage = SDL_ConvertSurfaceFormat(blobShadow, SDL_PIXELFORMAT_ABGR8888, 0);
        SDL_FreeSurface(blobShadow);

        SDL_SetSurfaceAlphaMod(formattedBlobShadowImage, 127);
        SDL_SetColorKey(formattedBlobShadowImage, SDL_TRUE, SDL_MapRGB(formattedBlobShadowImage->format, 0, 0, 0));
        for (int j = 0; j < formattedBlobShadowImage->w * formattedBlobShadowImage->h; j++)
        {
                SDL_Color* pixel = &(((SDL_Color*)formattedBlobShadowImage->pixels)[j]);
                if (!(pixel->r | pixel->g | pixel->b))
                {
                        pixel->a = 0;
                }
                else
                {
                        pixel->a = 127;
                }
        }

#ifdef MIYOO_MINI
        mBlobShadowSurfaces.push_back(formattedBlobShadowImage);
#else
        SDL_Texture* leftBlobShadowTex = SDL_CreateTexture(mRenderer,
                        SDL_PIXELFORMAT_ABGR8888,
                        SDL_TEXTUREACCESS_STREAMING,
                        formattedBlobShadowImage->w, formattedBlobShadowImage->h);
        SDL_SetTextureBlendMode(leftBlobShadowTex, SDL_BLENDMODE_BLEND);
        SDL_UpdateTexture(leftBlobShadowTex, nullptr, formattedBlobShadowImage->pixels, formattedBlobShadowImage->pitch);

        mLeftBlobShadow.emplace_back(
                        leftBlobShadowTex,
                        Color(255, 255, 255));

        SDL_Texture* rightBlobShadowTex = SDL_CreateTexture(mRenderer,
                        SDL_PIXELFORMAT_ABGR8888,
                        SDL_TEXTUREACCESS_STREAMING,
                        formattedBlobShadowImage->w, formattedBlobShadowImage->h);
        SDL_SetTextureBlendMode(rightBlobShadowTex, SDL_BLENDMODE_BLEND);
        SDL_UpdateTexture(rightBlobShadowTex, nullptr, formattedBlobShadowImage->pixels, formattedBlobShadowImage->pitch);

        mRightBlobShadow.emplace_back(
                        rightBlobShadowTex,
                        Color(255, 255, 255));
                        
		// Prepare blobby textures
		SDL_Texture* leftBlobTex = SDL_CreateTexture(mRenderer,
				SDL_PIXELFORMAT_ABGR8888,
				SDL_TEXTUREACCESS_STREAMING,
				formattedBlobImage->w, formattedBlobImage->h);
		SDL_SetTextureBlendMode(leftBlobTex, SDL_BLENDMODE_BLEND);
		SDL_UpdateTexture(leftBlobTex, nullptr, formattedBlobImage->pixels, formattedBlobImage->pitch);

		mLeftBlob.emplace_back(
				leftBlobTex,
				Color(255, 255, 255));

		SDL_Texture* rightBlobTex = SDL_CreateTexture(mRenderer,
				SDL_PIXELFORMAT_ABGR8888,
				SDL_TEXTUREACCESS_STREAMING,
				formattedBlobImage->w, formattedBlobImage->h);
		SDL_SetTextureBlendMode(rightBlobTex, SDL_BLENDMODE_BLEND);
		SDL_UpdateTexture(rightBlobTex, nullptr, formattedBlobImage->pixels, formattedBlobImage->pitch);

		mRightBlob.emplace_back(
				rightBlobTex,
				Color(255, 255, 255));

		// Prepare blobby shadow textures
		SDL_Texture* leftBlobShadowTex = SDL_CreateTexture(mRenderer,
				SDL_PIXELFORMAT_ABGR8888,
				SDL_TEXTUREACCESS_STREAMING,
				formattedBlobShadowImage->w, formattedBlobShadowImage->h);
		SDL_SetTextureBlendMode(leftBlobShadowTex, SDL_BLENDMODE_BLEND);
		mLeftBlobShadow.emplace_back(
				leftBlobShadowTex,
				Color(255, 255, 255));
		SDL_UpdateTexture(leftBlobShadowTex, nullptr, formattedBlobShadowImage->pixels, formattedBlobShadowImage->pitch);

		SDL_Texture* rightBlobShadowTex = SDL_CreateTexture(mRenderer,
				SDL_PIXELFORMAT_ABGR8888,
				SDL_TEXTUREACCESS_STREAMING,
				formattedBlobShadowImage->w, formattedBlobShadowImage->h);
		SDL_SetTextureBlendMode(rightBlobShadowTex, SDL_BLENDMODE_BLEND);
		mRightBlobShadow.emplace_back(
				rightBlobShadowTex,
				Color(255, 255, 255));
		SDL_UpdateTexture(rightBlobShadowTex, nullptr, formattedBlobShadowImage->pixels, formattedBlobShadowImage->pitch);
#endif
	}


	// Load font
	for (int i = 0; i <= 58; ++i) {
		char filename[64];
		sprintf(filename, "gfx/font%02d.bmp", i);
		SDL_Surface* tempFont = loadSurface(filename);

		SDL_SetColorKey(tempFont, SDL_TRUE, SDL_MapRGB(tempFont->format, 0, 0, 0));

#ifdef MIYOO_MINI
		mFontSurfaces.push_back(tempFont);

		SDL_Surface* tempFontHighlighted = highlightSurface(tempFont, 60);
		if (tempFontHighlighted != nullptr) {
			mHighlightFontSurfaces.push_back(tempFontHighlighted);
		}
#else
		SDL_Texture* fontTexture = SDL_CreateTextureFromSurface(mRenderer, tempFont);
		mFont.push_back(fontTexture); 

		SDL_Surface* tempFont2 = highlightSurface(tempFont, 60);
		SDL_Texture* fontTextureHighlighted = SDL_CreateTextureFromSurface(mRenderer, tempFont2);
		mHighlightFont.push_back(fontTextureHighlighted);

		SDL_FreeSurface(tempFont);
		SDL_FreeSurface(tempFont2);
#endif
	}
    
	// Load blood surface
	SDL_Surface* blobStandardBlood = loadSurface("gfx/blood.bmp");
	SDL_Surface* formattedBlobStandardBlood = SDL_ConvertSurfaceFormat(blobStandardBlood, SDL_PIXELFORMAT_ABGR8888, 0);
	SDL_FreeSurface(blobStandardBlood);

	SDL_SetColorKey(formattedBlobStandardBlood, SDL_TRUE, SDL_MapRGB(formattedBlobStandardBlood->format, 0, 0, 0));
	for(int j = 0; j < formattedBlobStandardBlood->w * formattedBlobStandardBlood->h; j++)
	{
		SDL_Color* pixel = &(((SDL_Color*)formattedBlobStandardBlood->pixels)[j]);
		if (!(pixel->r | pixel->g | pixel->b))
		{
			pixel->a = 0;
		} else {
			pixel->a = 255;
		}
	}

#ifdef MIYOO_MINI
    mStandardBlobBloodSurface = formattedBlobStandardBlood;
#else
    mStandardBlobBlood = formattedBlobStandardBlood;
    
	// Create streamed textures for blood
	SDL_Texture* leftBlobBlood = SDL_CreateTexture(mRenderer,
			SDL_PIXELFORMAT_ABGR8888,
			SDL_TEXTUREACCESS_STREAMING,
			formattedBlobStandardBlood->w, formattedBlobStandardBlood->h);
	SDL_SetTextureBlendMode(leftBlobBlood, SDL_BLENDMODE_BLEND);
	mLeftBlobBlood = DynamicColoredTexture(
			leftBlobBlood,
			Color(255, 0, 0));
	SDL_UpdateTexture(leftBlobBlood, nullptr, formattedBlobStandardBlood->pixels, formattedBlobStandardBlood->pitch);

	SDL_Texture* rightBlobBlood = SDL_CreateTexture(mRenderer,
			SDL_PIXELFORMAT_ABGR8888,
			SDL_TEXTUREACCESS_STREAMING,
			formattedBlobStandardBlood->w, formattedBlobStandardBlood->h);
	SDL_SetTextureBlendMode(rightBlobBlood, SDL_BLENDMODE_BLEND);
	mRightBlobBlood = DynamicColoredTexture(
			rightBlobBlood,
			Color(255, 0, 0));
	SDL_UpdateTexture(rightBlobBlood, nullptr, formattedBlobStandardBlood->pixels, formattedBlobStandardBlood->pitch);
#endif

SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
}

RenderManagerSDL::~RenderManagerSDL()
{
	
#ifdef MIYOO_MINI
	if (mMiyooSurface) {
		SDL_FreeSurface(mMiyooSurface);
	}
	if (mBackgroundSurface) {
		SDL_FreeSurface(mBackgroundSurface);
	}
	for (auto& surface : mFontSurfaces) {
		SDL_FreeSurface(surface);
	}
	for (auto& surface : mHighlightFontSurfaces) {
		SDL_FreeSurface(surface);
	}
	// for(const auto& image : mImageMap) { // causes segs
		// SDL_FreeSurface(image.second->sdlSurface);
		// delete image.second;
	// }
	for (auto& surface : mBlobShadowSurfaces) {
		SDL_FreeSurface(surface);
	}		
#else
	for (unsigned int i = 0; i < mFont.size(); ++i)
	{
		SDL_DestroyTexture(mFont[i]);
		SDL_DestroyTexture(mHighlightFont[i]);
	}
		
	SDL_DestroyTexture(mBallShadow);
		for(auto& i : mMarker) {
		SDL_DestroyTexture(i);
	}

	for(auto& i : mBall)
		SDL_DestroyTexture(i);

	for (unsigned int i = 0; i < mStandardBlob.size(); ++i)
	{
		SDL_FreeSurface(mStandardBlob[i]);
		SDL_FreeSurface(mStandardBlobShadow[i]);
		SDL_DestroyTexture(mLeftBlob[i].mSDLsf);
		SDL_DestroyTexture(mLeftBlobShadow[i].mSDLsf);
		SDL_DestroyTexture(mRightBlob[i].mSDLsf);
		SDL_DestroyTexture(mRightBlobShadow[i].mSDLsf);
	}

	SDL_FreeSurface(mStandardBlobBlood);
	SDL_DestroyTexture(mLeftBlobBlood.mSDLsf);
	SDL_DestroyTexture(mRightBlobBlood.mSDLsf);
		
		for(const auto& image : mImageMap) {
		SDL_DestroyTexture(image.second->sdlImage);
		delete image.second;
	}
    SDL_DestroyTexture(mOverlayTexture);
    
#endif

#ifdef MIYOO_MINI
	SDL_DestroyTexture(mRenderStreaming);
#else
    SDL_DestroyTexture(mRenderTarget);
#endif

	SDL_DestroyRenderer(mRenderer);
	SDL_DestroyWindow(mWindow);
}

bool RenderManagerSDL::setBackground(const std::string& filename)
{
    try
    {
        SDL_Surface* tempBackgroundSurface = loadSurface(filename);
 #ifdef MIYOO_MINI
        if (mBackgroundSurface) {
            SDL_FreeSurface(mBackgroundSurface); 
        }
        mBackgroundSurface = tempBackgroundSurface; 
#else
        SDL_Texture* tempBackgroundTexture = SDL_CreateTextureFromSurface(mRenderer, tempBackgroundSurface);
        SDL_FreeSurface(tempBackgroundSurface);

        BufferedImage* oldBackground = mImageMap["background"];
        if (oldBackground) {
            SDL_DestroyTexture(oldBackground->sdlImage);
            delete oldBackground;
        }

        BufferedImage* newImage = new BufferedImage;
        newImage->w = tempBackgroundSurface->w;
        newImage->h = tempBackgroundSurface->h;
        newImage->sdlImage = tempBackgroundTexture;
        mBackground = tempBackgroundTexture;
        mImageMap["background"] = newImage;
#endif
        return true;
    }
    catch (const FileLoadException& e)
    {
        SDL_Log("Error loading background: %s", e.what());
        return false;
    }
}

void RenderManagerSDL::setBlobColor(int player, Color color)
{
	if (color != mBlobColor[player]) {
		mBlobColor[player] = color;
#ifdef MIYOO_MINI
        return;
#endif
	} else {
		return;
	}
    
#ifndef MIYOO_MINI
	DynamicColoredTexture* handledBlobBlood;

	if (player == LEFT_PLAYER)
	{
		handledBlobBlood = &mLeftBlobBlood;
	}
	if (player == RIGHT_PLAYER)
	{
		handledBlobBlood = &mRightBlobBlood;
	}

    SDL_Surface* tempSurface = colorSurface(mStandardBlobBlood, mBlobColor[player]);
	SDL_UpdateTexture(handledBlobBlood->mSDLsf, nullptr, tempSurface->pixels, tempSurface->pitch);
	SDL_FreeSurface(tempSurface);
#endif

}

void RenderManagerSDL::colorizeBlobs(int player, int frame)
{
#ifndef MIYOO_MINI
	std::vector<DynamicColoredTexture> *handledBlob = nullptr;
	std::vector<DynamicColoredTexture> *handledBlobShadow = nullptr;

	if (player == LEFT_PLAYER)
	{
		handledBlob = &mLeftBlob;
		handledBlobShadow = &mLeftBlobShadow;
	}
	if (player == RIGHT_PLAYER)
	{
		handledBlob = &mRightBlob;
		handledBlobShadow = &mRightBlobShadow;
	}

	if( (*handledBlob)[frame].mColor != mBlobColor[player])
	{
		SDL_Surface* tempSurface = colorSurface(mStandardBlob[frame], mBlobColor[player]);
		SDL_UpdateTexture((*handledBlob)[frame].mSDLsf, nullptr, tempSurface->pixels, tempSurface->pitch);
		SDL_FreeSurface(tempSurface);

		SDL_Surface* tempSurface2 = colorSurface(mStandardBlobShadow[frame], mBlobColor[player]);
		SDL_UpdateTexture((*handledBlobShadow)[frame].mSDLsf, nullptr, tempSurface2->pixels, tempSurface2->pitch);
		SDL_FreeSurface(tempSurface2);

		(*handledBlob)[frame].mColor = mBlobColor[player];
	}
#endif
}

void RenderManagerSDL::showShadow(bool shadow)
{
	mShowShadow = shadow;
}

void RenderManagerSDL::drawText(const std::string& text, Vector2 position, unsigned int flags)
{
	drawTextImpl(text, position, flags);
}

void RenderManagerSDL::drawTextImpl(const std::string& text, Vector2 position, unsigned int flags) {
    int FontSize = (flags & TF_SMALL_FONT ? FONT_WIDTH_SMALL : FONT_WIDTH_NORMAL);
    int length = 0;

    for (auto iter = text.cbegin(); iter != text.cend();) {
        int index = getNextFontIndex(iter);

        if (flags & TF_OBFUSCATE) {
            index = FONT_INDEX_ASTERISK;
        }

        bool isHighlight = flags & TF_HIGHLIGHT;
        
#ifdef MIYOO_MINI
        SDL_Surface* fontSurface = nullptr;
        fontSurface = isHighlight ? mHighlightFontSurfaces[index] : mFontSurfaces[index];
#else
        SDL_Texture* fontTexture = nullptr;
        fontTexture = isHighlight ? mHighlightFont[index] : mFont[index];
#endif

        SDL_Rect charRect = {
            lround(position.x) + length,
            lround(position.y),
            FontSize,
            FontSize
        };

#ifdef MIYOO_MINI
        if (SDL_MUSTLOCK(mMiyooSurface)) SDL_LockSurface(mMiyooSurface);
        SDL_BlitSurface(fontSurface, nullptr, mMiyooSurface, &charRect);
        if (SDL_MUSTLOCK(mMiyooSurface)) SDL_UnlockSurface(mMiyooSurface);
#else
        SDL_QueryTexture(fontTexture, nullptr, nullptr, &charRect.w, &charRect.h);
        SDL_RenderCopy(mRenderer, fontTexture, nullptr, &charRect);
#endif

        length += FontSize;
    }
}

void RenderManagerSDL::drawImage(const std::string& filename, Vector2 position, Vector2 size)
{
	BufferedImage* imageBuffer = mImageMap[filename];

	if (!imageBuffer)
	{
		imageBuffer = new BufferedImage;
		SDL_Surface* tmpSurface = loadSurface(filename);
		SDL_SetColorKey(tmpSurface, SDL_TRUE,
				SDL_MapRGB(tmpSurface->format, 0, 0, 0));
		
#ifdef MIYOO_MINI
		imageBuffer->sdlSurface = tmpSurface;
#else
		imageBuffer->sdlImage = SDL_CreateTextureFromSurface(mRenderer, tmpSurface);
		SDL_FreeSurface(tmpSurface);
#endif

		imageBuffer->w = tmpSurface->w;
		imageBuffer->h = tmpSurface->h;
		mImageMap[filename] = imageBuffer;
	}

	SDL_Rect blitRect;
	if (size == Vector2(0,0))
	{
		// No scaling
		blitRect = {
			(short)lround(position.x - float(imageBuffer->w) / 2.0),
			(short)lround(position.y - float(imageBuffer->h) / 2.0),
			(short)imageBuffer->w,
			(short)imageBuffer->h
		};
	}
	else
	{
		// Scaling
		blitRect = {
			(short)lround(position.x - float(size.x) / 2.0),
			(short)lround(position.y - float(size.y) / 2.0),
			(short)size.x,
			(short)size.y
		};
	}

#ifdef MIYOO_MINI
    if (mMiyooSurface && imageBuffer->sdlSurface)
    {
        // SDL_Log("Drawing image %s at position (%d, %d).", filename.c_str(), blitRect.x, blitRect.y);
        if (filename == "background") // what's this? i hear you say. it's a dirty fat hack - surface lock/unlock issue
        {
            if (!SDL_MUSTLOCK(mMiyooSurface) || SDL_LockSurface(mMiyooSurface) == 0)
            {                
                if (mBackgroundSurface)
                {
                    SDL_BlitSurface(mBackgroundSurface, nullptr, mMiyooSurface, &blitRect);
                }
                
                if (SDL_MUSTLOCK(mMiyooSurface)) SDL_UnlockSurface(mMiyooSurface);
            }
        }
        else
        {
            if (SDL_MUSTLOCK(mMiyooSurface)) SDL_LockSurface(mMiyooSurface);
            SDL_BlitSurface(imageBuffer->sdlSurface, nullptr, mMiyooSurface, &blitRect);
            if (SDL_MUSTLOCK(mMiyooSurface)) SDL_UnlockSurface(mMiyooSurface);
        }
    }
#else
        SDL_RenderCopy(mRenderer, imageBuffer->sdlImage, nullptr, &blitRect);
#endif
}


void RenderManagerSDL::drawOverlay(float opacity, Vector2 pos1, Vector2 pos2, Color col)
{
	SDL_Rect ovRect;
	ovRect.x = static_cast<int>(lround(pos1.x));
	ovRect.y = static_cast<int>(lround(pos1.y));
	ovRect.w = static_cast<int>(lround(pos2.x - pos1.x));
	ovRect.h = static_cast<int>(lround(pos2.y - pos1.y));

#ifdef MIYOO_MINI
	if (mOverlaySurface != nullptr) {
		SDL_Surface* resizedOverlay = SDL_CreateRGBSurface(0, ovRect.w, ovRect.h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		if (resizedOverlay) {
			SDL_FillRect(resizedOverlay, NULL, SDL_MapRGBA(resizedOverlay->format, col.r, col.g, col.b, static_cast<Uint8>(lround(opacity * 255))));
			SDL_SetSurfaceBlendMode(resizedOverlay, SDL_BLENDMODE_BLEND);
			SDL_BlitSurface(resizedOverlay, NULL, mMiyooSurface, &ovRect);
			SDL_FreeSurface(resizedOverlay);
		}
	}
#else
	SDL_SetTextureAlphaMod(mOverlayTexture, static_cast<Uint8>(lround(opacity * 255)));
	SDL_SetTextureColorMod(mOverlayTexture, col.r, col.g, col.b);
	SDL_RenderCopy(mRenderer, mOverlayTexture, nullptr, &ovRect);
#endif
}

void RenderManagerSDL::drawBlob(const Vector2& pos, const Color& col)
{
	static int toDraw = 0;

	setBlobColor(toDraw, col);
	/// \todo this recolores the current frame (0)
	/// + shadows; that's not exactly what we want
    // we only want to set the blob colour here in MM mode
#ifndef MIYOO_MINI 
	colorizeBlobs(toDraw, 0);
    SDL_Rect position;
	position.x = (int)lround(pos.x);
	position.y = (int)lround(pos.y);

	//  Second dirty workaround in the function to have the right position of blobs in the GUI
	position.x = position.x - (int)(75/2);
	position.y = position.y - (int)(89/2);

	if(toDraw == 1)
	{
		SDL_QueryTexture(mRightBlob[0].mSDLsf, nullptr, nullptr, &position.w, &position.h);
		SDL_RenderCopy(mRenderer, mRightBlob[0].mSDLsf, nullptr, &position);
		toDraw = 0;
	}
	else
	{
		SDL_QueryTexture(mLeftBlob[0].mSDLsf, nullptr, nullptr, &position.w, &position.h);
		SDL_RenderCopy(mRenderer, mLeftBlob[0].mSDLsf, nullptr, &position);
		toDraw = 1;
	}
#endif
}

void RenderManagerSDL::drawParticle(const Vector2& pos, int player)
{
    SDL_Rect blitRect = {
        (short)lround(pos.x - float(9) / 2.0),
        (short)lround(pos.y - float(9) / 2.0),
        (short)9,
        (short)9,
    };

#ifdef MIYOO_MINI
    Color bloodColor = (player == LEFT_PLAYER) ? mBlobColor[LEFT_PLAYER] : mBlobColor[RIGHT_PLAYER];
    
    if (mStandardBlobBloodSurface) {
        SDL_Surface* coloredBloodSurface = colorSurface(mStandardBlobBloodSurface, bloodColor);

        if (SDL_MUSTLOCK(mMiyooSurface)) SDL_LockSurface(mMiyooSurface);
        SDL_BlitSurface(coloredBloodSurface, nullptr, mMiyooSurface, &blitRect);
        if (SDL_MUSTLOCK(mMiyooSurface)) SDL_UnlockSurface(mMiyooSurface);
        
        SDL_FreeSurface(coloredBloodSurface);
    }
#else
    DynamicColoredTexture blood = player == LEFT_PLAYER ? mLeftBlobBlood : mRightBlobBlood;
    SDL_RenderCopy(mRenderer, blood.mSDLsf, nullptr, &blitRect);
#endif
}

void RenderManagerSDL::refresh()
{
#ifdef MIYOO_MINI
	SDL_RenderClear(mRenderer);
	SDL_UpdateTexture(mRenderStreaming, NULL, mMiyooSurface->pixels, mMiyooSurface->pitch);
	SDL_RenderCopy(mRenderer, mRenderStreaming, NULL, NULL);
	SDL_RenderPresent(mRenderer);
#else
	SDL_SetRenderTarget(mRenderer, nullptr);

	// We have a resizeable window
	// Resize renderer if needed
	// TODO: We should catch the resize event
	SDL_Rect renderRect;
	int windowX;
	int windowY;
	SDL_RenderGetViewport(mRenderer, &renderRect);
	SDL_GetWindowSize(mWindow, &windowX, &windowY);
	if (renderRect.w != windowX || renderRect.h != windowY)
	{
		renderRect.w = windowX;
		renderRect.h = windowY;
		SDL_RenderSetViewport(mRenderer, &renderRect);
	}

	SDL_RenderCopy(mRenderer, mRenderTarget, nullptr, nullptr);
	SDL_RenderPresent(mRenderer);
	SDL_SetRenderTarget(mRenderer, mRenderTarget);
#endif
}

void RenderManagerSDL::drawGame(const DuelMatchState& gameState)
{
	SDL_Rect position;

#ifdef MIYOO_MINI
    if (SDL_MUSTLOCK(mMiyooSurface)) SDL_LockSurface(mMiyooSurface);

    // Draw the background
    SDL_BlitSurface(mBackgroundSurface, NULL, mMiyooSurface, NULL);

    // Ball marker
    position.y = 5;
    position.x = (int)lround(gameState.getBallPosition().x - 2.5);
    position.w = 10;
    position.h = 10;
    if((int)SDL_GetTicks() % 1000 >= 500)
        SDL_FillRect(mMiyooSurface, &position, SDL_MapRGB(mMiyooSurface->format, 0x00, 0x00, 0x00));
    else
        SDL_FillRect(mMiyooSurface, &position, SDL_MapRGB(mMiyooSurface->format, 255, 255, 255));    

    // Mouse marker
    position.y = 590;
    position.x = (int)lround(mMouseMarkerPosition - 2.5);
    if((int)SDL_GetTicks() % 1000 >= 500)
        SDL_FillRect(mMiyooSurface, &position, SDL_MapRGB(mMiyooSurface->format, 0x00, 0x00, 0x00));
    else
        SDL_FillRect(mMiyooSurface, &position, SDL_MapRGB(mMiyooSurface->format, 255, 255, 255));   
    
    if (SDL_MUSTLOCK(mMiyooSurface)) SDL_UnlockSurface(mMiyooSurface);
#else
    SDL_RenderCopy(mRenderer, mBackground, nullptr, nullptr);

    // Ball marker
    position.y = 5;
    position.x = (int)lround(gameState.getBallPosition().x - 2.5);
    position.w = 5;
    position.h = 5;
    SDL_RenderCopy(mRenderer, mMarker[(int)SDL_GetTicks() % 1000 >= 500], nullptr, &position);

    // Mouse marker
    position.y = 590;
    position.x = (int)lround(mMouseMarkerPosition - 2.5);
    SDL_RenderCopy(mRenderer, mMarker[(int)SDL_GetTicks() % 1000 >= 500], nullptr, &position);
#endif

	if(mShowShadow)
	{
		// Ball Shadow
		SDL_Rect position = ballShadowRect(ballShadowPosition(gameState.getBallPosition()));
        
#ifdef MIYOO_MINI
        if (SDL_MUSTLOCK(mMiyooSurface)) SDL_LockSurface(mMiyooSurface);
		SDL_BlitSurface(mBallShadowSurf, nullptr, mMiyooSurface, &position);
		if (SDL_MUSTLOCK(mMiyooSurface)) SDL_UnlockSurface(mMiyooSurface);
        
        // Drawing left blob shadow
        int leftFrame = int(gameState.getBlobState(LEFT_PLAYER)) % mBlobShadowSurfaces.size();
        SDL_Rect shadowPosition = blobShadowRect(blobShadowPosition(gameState.getBlobPosition(LEFT_PLAYER)));
        SDL_BlitSurface(mBlobShadowSurfaces[leftFrame], nullptr, mMiyooSurface, &shadowPosition);

        // Drawing right blob shadow
        int rightFrame = int(gameState.getBlobState(RIGHT_PLAYER)) % mBlobShadowSurfaces.size();
        shadowPosition = blobShadowRect(blobShadowPosition(gameState.getBlobPosition(RIGHT_PLAYER)));
        SDL_BlitSurface(mBlobShadowSurfaces[rightFrame], nullptr, mMiyooSurface, &shadowPosition);
#else
        SDL_RenderCopy(mRenderer, mBallShadow, nullptr, &position);
        
		// Left blob shadow
		position = blobShadowRect(blobShadowPosition(gameState.getBlobPosition(LEFT_PLAYER)));
		int animationState = int(gameState.getBlobState(LEFT_PLAYER)) % 5;
		SDL_RenderCopy(mRenderer, mLeftBlobShadow[animationState].mSDLsf, nullptr, &position);

		// Right blob shadow
		position = blobShadowRect(blobShadowPosition(gameState.getBlobPosition(RIGHT_PLAYER)));
		animationState = int(gameState.getBlobState(RIGHT_PLAYER)) % 5;
		SDL_RenderCopy(mRenderer, mRightBlobShadow[animationState].mSDLsf, nullptr, &position);
#endif

	}

#ifndef MIYOO_MINI
	// Restore the rod
	position.x = 400 - 7;
	position.y = 300;
	SDL_Rect rodPosition;
	rodPosition.x = 400 - 7;
	rodPosition.y = 300;
	rodPosition.w = 14;
	rodPosition.h = 300;
	SDL_RenderCopy(mRenderer, mBackground, &rodPosition, &rodPosition);
#endif
	// Drawing the Ball
	position = ballRect(gameState.getBallPosition());
	int animationState = int(gameState.getBallRotation() / M_PI / 2 * 16) % 16;

#ifdef MIYOO_MINI
	if (SDL_MUSTLOCK(mMiyooSurface)) SDL_LockSurface(mMiyooSurface);
	SDL_Rect srcRect = {0, 0, mBallSurfaces[animationState]->w, mBallSurfaces[animationState]->h};
	SDL_BlitSurface(mBallSurfaces[animationState], &srcRect, mMiyooSurface, &position);
	if (SDL_MUSTLOCK(mMiyooSurface)) SDL_UnlockSurface(mMiyooSurface);
#else
	SDL_RenderCopy(mRenderer, mBall[animationState], nullptr, &position);
#endif

#ifdef MIYOO_MINI
    // update blob colors for MM
    Color leftBlobColor = mBlobColor[LEFT_PLAYER];
    Color rightBlobColor = mBlobColor[RIGHT_PLAYER];

    int leftFrame = int(gameState.getBlobState(LEFT_PLAYER)) % mBlobSurfaces.size();
    SDL_Surface* leftBlobColored = colorSurface(mBlobSurfaces[leftFrame], leftBlobColor);
    SDL_Rect leftBlobPosition = blobRect(gameState.getBlobPosition(LEFT_PLAYER));

    if (SDL_MUSTLOCK(mMiyooSurface)) SDL_LockSurface(mMiyooSurface);
    SDL_BlitSurface(leftBlobColored, NULL, mMiyooSurface, &leftBlobPosition);
    SDL_FreeSurface(leftBlobColored);
    if (SDL_MUSTLOCK(mMiyooSurface)) SDL_UnlockSurface(mMiyooSurface);

    int rightFrame = int(gameState.getBlobState(RIGHT_PLAYER)) % mBlobSurfaces.size();
    SDL_Surface* rightBlobColored = colorSurface(mBlobSurfaces[rightFrame], rightBlobColor);
    SDL_Rect rightBlobPosition = blobRect(gameState.getBlobPosition(RIGHT_PLAYER));

    if (SDL_MUSTLOCK(mMiyooSurface)) SDL_LockSurface(mMiyooSurface);
    SDL_BlitSurface(rightBlobColored, NULL, mMiyooSurface, &rightBlobPosition);
    SDL_FreeSurface(rightBlobColored);
    if (SDL_MUSTLOCK(mMiyooSurface)) SDL_UnlockSurface(mMiyooSurface);
#else
	// update blob colors
	int leftFrame = int(gameState.getBlobState(LEFT_PLAYER)) % 5;
	int rightFrame = int(gameState.getBlobState(RIGHT_PLAYER)) % 5;
	colorizeBlobs(LEFT_PLAYER, leftFrame);
	colorizeBlobs(RIGHT_PLAYER, rightFrame);
    
	// Drawing left blob
	position = blobRect(gameState.getBlobPosition(LEFT_PLAYER));
	SDL_RenderCopy(mRenderer, mLeftBlob[leftFrame].mSDLsf, nullptr, &position);
		
	// Drawing right blob
	position = blobRect(gameState.getBlobPosition(RIGHT_PLAYER));
	SDL_RenderCopy(mRenderer, mRightBlob[rightFrame].mSDLsf, nullptr, &position);
#endif
}
