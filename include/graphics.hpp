#pragma once

#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <SDL2_ttf/SDL_ttf.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#endif

#include "util.hpp"
#include <memory>
#include <string>

//not all flags are here
enum reducedFlags
{
	FULLSCREEN = SDL_WINDOW_FULLSCREEN,
	OPENGL = SDL_WINDOW_OPENGL,
	FULLSCREEN_DESKTOP = SDL_WINDOW_FULLSCREEN_DESKTOP,
	RESIZABLE = SDL_WINDOW_RESIZABLE,
	BORDERLESS = SDL_WINDOW_BORDERLESS
};

namespace SG2D
{
	extern SDL_Cursor *hand;
	extern SDL_Cursor *arrow;

	extern TTF_Font *curFont;
}
struct Color;

//Open SDL and create window - last parameter is a bitflag that goes straight into SDL
//Returns the screen width and height.
Vec2 initGraphics( char const *title, int height, int width, int flags );
SDL_Renderer *getRenderer();
SDL_Window *getWindow();
//Clear screen
void clear();
//Copy your renderer to the window
void flip();
void msgBox( char const *title, char const *msg );
void drawRect(SDL_Rect *rect,Color color);
void setPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

SDL_Texture *ALLOC_arc(int radius, int degreeBegin, int degreeEnd, Color color);
SDL_Texture *ALLOC_circle(int radius, Color color);//(if surfMask overwrites radius with size of surf.w/2) will have to recreate to get decent looking edges
SDL_Texture *ALLOC_rect(Color color,SDL_Surface *surfMask=0);//allocs 1x1 rect (unless surfMask, then size of surfMask), can then be stretched

struct Color
{
	byte r,g,b,a;
	Color();
	Color(Uint32 hex);
	Color(byte r_p,byte g_p,byte b_p,byte a_p);
	Color(byte r_p,byte g_p,byte b_p);
	byte &operator[](int index);
	Uint32 toHex();
};
/*
SDL_Texture wrapper
shared_ptr needs a defined struct (why? who knows).
SDL_Texture's implementation isn't known by the compiler.
This wrapper solves that problem.
*/
struct Texture
{//NOT COPYABLE
	SDL_Texture *rawTexture;

	Texture();
	Texture( char const *path );
	Texture( SDL_Texture *t );
	~Texture();
};
/*
Copyable/assignable thing that is easy to draw.
*/
struct Image
{
	SDL_Rect dest, src;
	SDL_Point center;
	SDL_RendererFlip flip;
	double angle;

	std::shared_ptr<Texture> texture;

    Image();
	Image( char const *path, int srcX=0, int srcY=0, int srcW=0, int srcH=0 );
	//Image( const Image& );
	//Image( const Image&, int srcX, int srcY, int srcW, int srcH );
	~Image();
	//Copy the image to the renderer
	void draw();
};

//holds resource, similar to texture
struct Font
{//NOT COPYABLE
	TTF_Font *font;
	std::string path;

	Font(std::string path_p,int fontsize=16);
	Font();
	~Font();

	void setfontsize(int fs);
};
//performant text object that just draws text at a position using a reference to a Font
class PlainText
{
	std::string text;
	SDL_Rect dest;
	SDL_Texture *texture;
	SDL_Color color_impl;

	public:
	Vec2 pos;
	Font *f;
	Color color;

	PlainText(Font *f_p,Color color_p,Vec2 pos_p={0,0});
	~PlainText();

	void draw();
	void settext(std::string text_p);
	std::string gettext();
};
//similar to image
//each one loads a font just like each image loads a bitmap
//holds font and other information associated with rendering text, pass a reference to it so you dont reload the font (similar to image)
//renders text inside a block, compatible with image
class Text
{
	SDL_Rect src;
	std::string text;
	int renderW,renderH;
	int fontsize;
	public:
	Vec2 origin;//this is the origin of the text surface within the dest rect of the entity owning it. it defaults to .5,.5 (centering the text in the rectangle). it is necessary to do this to align text perfectly. if you want to rotate the rectangle and you change this it will not appear to be spinning from the center, even if the center is w/2,h/2
	bool fastRender;
	int last_fontsizeOverride;
	int *fontsizeOverride;
	bool constFS;
	bool dirty;
	SDL_Texture *rendertext;
	SDL_Rect dest;//this must be set before every draw (its written to)
	SDL_Point center;
	SDL_RendererFlip flip;
	double angle;
	std::shared_ptr<Font> f;
	SDL_Color color;

	Text();
	Text( char const *path,  Color color_p, bool fastRender_p=true );
	~Text();
	//Copy the text to the renderer
	void draw();
	void settext(std::string);
	std::string gettext();
};
