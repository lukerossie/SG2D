#pragma once

#include "../../../OSAL/src/hpp/graphics.hpp"
#include "../../../OSAL/src/hpp/util.hpp"
#include <memory>
#include <string>
#include <vector>

texture *ctor_texture_arc(int radius, int degreeBegin, int degreeEnd, color draw_color);
texture *ctor_texture_circle(int radius, color draw_color);//(if surfMask overwrites radius with size of surf.w/2) will have to recreate to get decent looking edges
texture *ctor_texture_rect(color draw_color,std::vector<std::vector<color>> *surfMask=0);//allocs 1x1 rect (unless surfMask, then size of surfMask), can then be stretched

/*
texture wrapper
shared_ptr needs a defined struct (why? who knows).
texture's implementation isn't known by the compiler.
This wrapper solves that problem.
*/
struct Texture
{//NOT COPYABLE
	texture *rawTexture;

	Texture();
	Texture( char const *path );
	Texture( texture *t );
	~Texture();
};
/*
Copyable/assignable thing that is easy to draw.
*/
struct Image
{
	rect dest, src;
	vec2 center;
	double angle;

	std::shared_ptr<Texture> image_texture;

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
	ttf_font *font;
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
	rect dest;
	texture *rendertext;
	color color_impl;

	public:
	vec2 pos;
	Font *f;
	color text_color;

	PlainText(Font *f_p,color color_p,vec2 pos_p={0,0});
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
	rect src;
	std::string text;
	int renderW,renderH;
	int fontsize;
	public:
	vec2 origin;//this is the origin of the text surface within the dest rect of the entity owning it. it defaults to .5,.5 (centering the text in the rectangle). it is necessary to do this to align text perfectly. if you want to rotate the rectangle and you change this it will not appear to be spinning from the center, even if the center is w/2,h/2
	bool fastRender;
	int last_fontsizeOverride;
	int *fontsizeOverride;
	bool constFS;
	bool dirty;
	texture *rendertext;
	rect dest;//this must be set before every draw (its written to)
	vec2 center;
	double angle;
	std::shared_ptr<Font> f;
	color text_color;

	Text();
	Text( char const *path,  color color_p, bool fastRender_p=true );
	~Text();
	//Copy the text to the renderer
	void draw();
	void settext(std::string);
	std::string gettext();
};
