#include "../include/graphics.hpp"
#include "../include/util.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
/*
TODO: Add rotation for images
TODO: Add support for multiple windows? Would have to change init
	and add some kind of resource management for windows etc.
*/

/*
@perf
for text, could move all settexturealpha mods etc. into separate code that only happens when dirty, simply rendercopy if its not dirty

images do unnecessary work as well, could track dirty

*/
//input polling function defined in input.cpp
int poll( void * );

namespace
{
SDL_Renderer *renderer = 0;
SDL_Window *window = 0;
SDL_DisplayMode mode;
Color clearColor={0,0,0,255};
}

namespace SG2D
{
SDL_Cursor *hand;
SDL_Cursor *arrow;
}

Color::Color():r(0),g(0),b(0),a(255){}
Color::Color(Uint32 hex):r(0),g(0),b(0),a(255)
{
	byte bpp=8;//@todo actually handle variable size pixel
	if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
	{
		r=(hex >> bpp*3);
		g=(hex >> bpp*2);
		b=(hex >> bpp*1);
		a=(hex >> bpp*0);
	}
	else
	{
		a=(hex >> bpp*3);
		b=(hex >> bpp*2);
		g=(hex >> bpp*1);
		r=(hex >> bpp*0);
	}
}
Color::Color(byte r_p,byte g_p,byte b_p,byte a_p):r(r_p),g(g_p),b(b_p),a(a_p){}
Color::Color(byte r_p,byte g_p,byte b_p):r(r_p),g(g_p),b(b_p),a(255){}
byte &Color::operator[](int index)
{
	switch(index){
		case 0:
		return r;
		break;
		case 1:
		return g;
		break;
		case 2:
		return b;
		break;
		case 3:
		return a;
		break;
		default:
		return a;
		break;
	}
}

Uint32 Color::toHex()//@bug assumes 32bit pixel
{
	byte bpc=8;//@todo actually handle variable size pixel
	if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
		return ((r & 0xff) << bpc*3) + ((g & 0xff) << bpc*2) + ((b & 0xff) << bpc*1) + (a & 0xff << bpc*0);
	else
		return ((a & 0xff) << bpc*3) + ((b & 0xff) << bpc*2) + ((g & 0xff) << bpc*1) + (r & 0xff << bpc*0);
}

Texture::Texture()
{

}
Texture::Texture( char const *path )
{
	SDL_Surface* surf = IMG_Load( path );
	rawTexture = SDL_CreateTextureFromSurface( renderer, surf );
    if(!rawTexture)
    {
        msgBox("Invalid texture!",SDL_GetError());
    }
	SDL_FreeSurface( surf );
}
Texture::Texture( SDL_Texture *t )
{
	rawTexture = t;
}
Texture::~Texture()
{
	if(rawTexture)
	{
		SDL_DestroyTexture( rawTexture );
	}
}

Image::Image()
{
	angle=0;
	flip=SDL_FLIP_NONE;
	center={};
	dest={};
	src={};
}
Image::Image( char const *path, int srcX, int srcY, int srcW, int srcH ) : Image()
{
	texture = std::make_shared<Texture>( path );
	SDL_Surface* surf = IMG_Load( path );//needs free
    src.x = 0;
    src.y = 0;
    if(srcX&&srcY)
	{
		src.x=srcX;
		src.y=srcY;
	}
	else
	{
		src.w = surf->w;
		src.h = surf->h;
	}
	dest.x = 0;
    dest.y = 0;
    dest.w = src.w;
    dest.h = src.h;
	SDL_FreeSurface( surf );
	if(!texture.get())
    {
        msgBox("Image ctor error",SDL_GetError());
    }

    SDL_SetTextureBlendMode(texture->rawTexture, SDL_BLENDMODE_BLEND);
}
Image::~Image()
{

}
void Image::draw()
{
	SDL_RenderCopyEx( renderer, texture.get()->rawTexture, &src, &dest, angle, &center, flip );
}


Font::Font():font(0){}
Font::Font(std::string path_p,int fontsize):Font()
{
	path=path_p;
	font=TTF_OpenFont(path.c_str(),fontsize);
    if(!font)
    {
        msgBox("Error opening font",SDL_GetError());
    }
}
Font::~Font()
{
	if(font)TTF_CloseFont(font);
}
void Font::setfontsize(int fontsize)
{
	if(font)TTF_CloseFont(font);
	font=TTF_OpenFont(path.c_str(),fontsize);
}

PlainText::PlainText(Font *f_p,Color color_p,Vec2 pos_p):f(f_p),color(color_p),pos(pos_p),texture(NULL)
{
	//if constructor constructs state that gets destroyed in destructor then copying the thing wont keep that state
}
PlainText::~PlainText()
{
	if(texture)
	{
		SDL_DestroyTexture(texture);
		texture=NULL;
	}
}
void PlainText::draw()
{
	if(!texture)
	{
		settext(text);//@perf check for new text, stupid move semantics destruct texture and settext isnt called
		return;
	}
	dest.x=pos.x;
	dest.y=pos.y;
	SDL_RenderCopy( renderer, texture, 0, &dest );
}
void PlainText::settext(std::string text_p)
{
	text=text_p;
	if(texture)
	{
		SDL_DestroyTexture(texture);
		texture=NULL;
	}
	if(!text.size())
	{
		return;
	}
	color_impl.r=color[0];
	color_impl.g=color[1];
	color_impl.b=color[2];
	color_impl.a=color[3];
	SDL_Surface *s=TTF_RenderText_Blended( f->font, text.c_str(), color_impl );
	if(!s)msgBox("PlainText error",SDL_GetError());
	texture=SDL_CreateTextureFromSurface( renderer, s );
	SDL_SetTextureAlphaMod(texture, color_impl.a);
	dest.w=s->w;
	dest.h=s->h;
	SDL_FreeSurface( s );
}
std::string PlainText::gettext()
{
	return text;
}

Text::Text():origin({.5,.5}),rendertext(nullptr),constFS(false),fontsize(0),fontsizeOverride(NULL),last_fontsizeOverride(0){}
Text::Text( char const *path, Color color_p, bool fastRender_p ):Text()
{
	color.r=color_p[0];
	color.g=color_p[1];
	color.b=color_p[2];
	color.a=color_p[3];

	fastRender=fastRender_p;
	renderW=0;
	renderH=0;
	f=std::make_shared<Font>( path );
	dest={};
	src={};
	center={0,0};
	angle=0;
	flip=SDL_FLIP_NONE;
}
Text::~Text()
{
	if(rendertext)SDL_DestroyTexture( rendertext );
}
void Text::draw()
{
	if(fontsizeOverride)
	{
		if(*fontsizeOverride!=last_fontsizeOverride)
		{
			dirty=true;
			last_fontsizeOverride=*fontsizeOverride;
		}
	}

	if(dirty)
	{
		if(fontsizeOverride)
		{
			f->setfontsize(*fontsizeOverride);
			TTF_SizeText(f->font,text.c_str(),&renderW,&renderH);
		}
		else if(constFS&&fontsize)
		{
			TTF_SizeText(f->font,text.c_str(),&renderW,&renderH);
			//reuse same fs as first calculated (fs is zero first frame), shouldnt even need to setfontsize
		}
		else
		{
			//std::cout<<"RERENDERING TEXT"<<std::endl;//@perf
			int fs=dest.h;
			if(fastRender)
			{
				f->setfontsize(fs);
				TTF_SizeText(f->font,text.c_str(),&renderW,&renderH);
				if(renderW>renderH)
				{
					//int len=text.length()*dest.h;
					fs=fs/text.length()/.7;//magic number relating line height and width approx (different for every font, ratio should never go above .7)
					TTF_SizeText(f->font,text.c_str(),&renderW,&renderH);
				}
			}
			f->setfontsize(fs);

			TTF_SizeText(f->font,text.c_str(),&renderW,&renderH);

			if(!fastRender)
			{
				while(renderW>dest.w||renderH>dest.h)
				{
					fs--;
					if(fs<=0)
					{
						std::cout<<"WARNING: ZERO FONTSIZE"<<std::endl;
						break;
					}

					f->setfontsize( fs );
					TTF_SizeText( f->font,text.c_str(),&renderW,&renderH );
				}
			}

			fontsize=fs;
		}

		{//buffers
			SDL_Surface *s=TTF_RenderText_Blended( f->font, text.c_str(), color );

			if(!s)
			{//if there is no text or error s will be null
                printf("Null text surface: %s\n",SDL_GetError());
				return;
			}

			if(rendertext)SDL_DestroyTexture( rendertext );

			int offset=0;//@bug set this to 1 to remove bugged pixel border around texture, or 0 to disable
			src={ offset,offset,s->w-offset*2,s->h-offset*2 };

			rendertext=SDL_CreateTextureFromSurface( renderer, s );
            if(!rendertext)printf("Render text texture error: %s\n",SDL_GetError());
			SDL_SetTextureAlphaMod(rendertext, color.a);
			SDL_FreeSurface( s );
		}
		dirty=false;
	}
	dest.x+=(dest.w-renderW)*origin.x;//align with private text origin
	dest.y+=(dest.h-renderH)*origin.y;//this is the origin of the text inside the dest box
	dest.w=renderW;//have to set even if its not dirty
	dest.h=renderH;
	SDL_RenderCopyEx( renderer, rendertext, &src, &dest, angle, &center, flip );
}
std::string Text::gettext()
{
	return text;
}
void Text::settext(std::string s)
{
	if(text==s)
	{
		return;
	}
	text=s;
	dirty=true;
}

/*
If width or height is 0 sets width and height from screen size (displaymode).
Returns the screen width and height.
*/
Vec2 initGraphics( char const *title, int width, int height, int flags )
{
	if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
	{//if SDL_Init fails you can't render a msgBox
		std::cout << "Critical Error\n" << "SDL_Init failed: " << SDL_GetError() << ". Application will now exit.\n";
		exit(-1);
	}
	if( SDL_GetCurrentDisplayMode(0, &mode) < 0 )
	{
		msgBox( "Warning", "SDL_GetCurrentDisplayMode failed." );
	}
	//Linear texture filtering
	if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
	{
		msgBox( "Warning", "SDL_SetHint failed." );
	}

	if( width==0 || height==0 )
	{
		width=mode.w;
		height=mode.h;
	}
	if( !( window = SDL_CreateWindow( title,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width, height, SDL_WINDOW_SHOWN | flags ) ) )//show the window by default
	{
		msgBox( "Critical Error", "SDL_CreateWindow failed. Application will now exit." );
		exit(-1);
	}
	if( !( renderer = SDL_CreateRenderer( window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC ) ) )//can or with SDL_RENDERER_PRESENTVSYNC here
	{
		msgBox( "Error", "SDL_CreateRenderer failed." );
	}
	//White renderer
	if( SDL_SetRenderDrawColor( renderer, 255, 255, 255, 255 ) < 0 )
	{
		msgBox( "Error", (std::string("SDL_CreateRenderer failed: ") + SDL_GetError() + ". Will now try fallback software renderer." ).c_str() );
		renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_SOFTWARE );
	}
	if( !IMG_Init( IMG_INIT_PNG ) )//returns bitmask on success, 0 is failure
	{
		msgBox( "Error", "IMG_Init failed." );
	}

	if( TTF_Init() ) {//returns 0 on success, nonzero on error
		msgBox( "Error", (std::string("TTF_Init: ")+TTF_GetError()).c_str() );
	}

	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1"); 
	SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "1", SDL_HINT_OVERRIDE); 
	if(SDL_GL_SetSwapInterval(-1)==-1)
	{
		SDL_GL_SetSwapInterval(1);
	}

	SG2D::hand=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	SG2D::arrow=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	SDL_SetCursor(SG2D::arrow);
	SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);

	//no longer spawning a polling thread here - must be polled on the main thread on all platforms
	return Vec2(width, height);
}

SDL_Renderer *getRenderer()
{
	return renderer;
}
SDL_Window *getWindow()
{
	return window;
}
void clear()
{
	SDL_SetRenderDrawColor(renderer, clearColor[0],clearColor[1],clearColor[2],clearColor[3]);
	SDL_RenderClear( renderer );
}
void flip()
{
	SDL_RenderPresent( renderer );
}
void msgBox( char const *title, char const *msg )
{
	SDL_ShowSimpleMessageBox( 0, title, msg, 0 );
}
void drawRect(SDL_Rect *rect,Color color)
{
	SDL_SetRenderDrawColor(renderer, color[0],color[1],color[2],color[3]);
	SDL_RenderFillRect(renderer,rect);
}

void setPixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    Uint8 *pixelToWrite=(Uint8 *)surface->pixels + y * surface->pitch + x * 4;
    *(Uint32*)pixelToWrite=pixel;
}
Uint32 getPixel(SDL_Surface *surface, int x, int y)
{
    return *(Uint32 *)((Uint8 *)surface->pixels + y * surface->pitch + x * 4);
}

static void surfDrawLine(SDL_Surface *surf, Vec2 begin, Vec2 end, Color color)
{
    Vec2 pixel=begin;

    int length=(begin-end).len();
    Vec2 direction=(end-begin).unit();

	SDL_LockSurface(surf);
    for( int i=0; i<length; i++ )
    {
        setPixel(surf,pixel.x,pixel.y,color.toHex());
        pixel+=direction;
    }
    SDL_UnlockSurface(surf);
}
SDL_Surface *makeRGB_surface(int size_x,int size_y)
{
    Uint32 rmask, gmask, bmask, amask;
	if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
	{
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
	}
	else
	{
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0xff000000;
	}

    SDL_Surface *surf = SDL_CreateRGBSurface(0, size_x,size_y, 32, rmask, gmask, bmask, amask);

	if(surf == NULL)
	{
        fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
		msgBox("Error","CreateRGBSurface failed.");
    }
    return surf;
}

SDL_Texture *ALLOC_arc(int radius,int degreeBegin, int degreeEnd,Color color)
{
    if(radius==0)radius=1;//texture dimensions cant be zero

    SDL_Surface *surf=makeRGB_surface(radius*2,radius*2);

	SDL_LockSurface(surf);
	for(int x=0;x<radius*2;x++)
    {
        for(int y=0; y<radius*2; y++)
        {
            setPixel(surf,x,y,Color(0,0,0,0).toHex());
        }
    }
	Vec2 center({(double)radius,(double)radius});
	//if(surf->format->BitsPerPixel!=8)msgBox("!!!",std::to_string(surf->format->BitsPerPixel).c_str());
	Uint32 clear=Color(255,0,0,0).toHex();

	//1/distance between lines
	double rate=1/ sqrt(pow(radius,2)+pow(radius,2)-(2*radius*radius)*cos(1));

	for(double i=degreeBegin; i<degreeEnd; i+=rate)
	{
        surfDrawLine(surf,center,center+angleToVec2(i).unit().scale(radius),color);
	}
	SDL_UnlockSurface(surf);

	SDL_Texture *texture=SDL_CreateTextureFromSurface(renderer,surf);
    if(!texture)printf("ALLOC_circle texture error: %s\n",SDL_GetError());
	SDL_FreeSurface(surf);
	return texture;
}
SDL_Texture *ALLOC_circle(int radius, Color color)//@perf
{
    if(radius==0)radius=1;//texture dimensions cant be zero

    SDL_Surface *surf=makeRGB_surface(radius*2,radius*2);

	SDL_LockSurface(surf);
	Vec2 center({(double)radius,(double)radius});
	//if(surf->format->BitsPerPixel!=8)msgBox("!!!",std::to_string(surf->format->BitsPerPixel).c_str());
	Uint32 clear=Color(255,0,0,0).toHex();
	for(int a=0; a<radius*2; a++)
	{
		for(int b=0; b<radius*2; b++)
		{
			if((Vec2({(double)a,(double)b})-center).len()<radius)
			{
				setPixel(surf,a,b,color.toHex());
			}
			else
			{
				setPixel(surf,a,b,clear);
			}
		}
	}
	SDL_UnlockSurface(surf);

	SDL_Texture *texture=SDL_CreateTextureFromSurface(renderer,surf);
    if(!texture)printf("ALLOC_circle texture error: %s\n",SDL_GetError());
	SDL_FreeSurface(surf);
	return texture;
}
SDL_Texture *ALLOC_rect(Color color,SDL_Surface *surfMask)//@perf
{
	const int SIZE_X=surfMask?surfMask->w:1;
	const int SIZE_Y=surfMask?surfMask->h:1;

    SDL_Surface *surf=makeRGB_surface(SIZE_X,SIZE_Y);

	Uint32 clear=Color(255,0,0,0).toHex();
	if(surf == NULL) {
        fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
		msgBox("Error","CreateRGBSurface failed.");
    }

    /*
	if(surfMask)
	{
		if(rmask!=surfMask->format->Rmask||gmask!=surfMask->format->Gmask||
			bmask!=surfMask->format->Bmask||amask!=surfMask->format->Amask)
		{
			std::cout<<"surfMask rbga mask does not match RBGsurface mask.\n";
			//exit(-1); @bug @todo this is triggering on mac
		}
		if(surfMask->format->BytesPerPixel!=surf->format->BytesPerPixel)
		{
			std::cout<<"surfMask BytesPerPixel mask does not match RBGsurface BytesPerPixel.\n";
			exit(-1);
		}
		if(surfMask->format->BitsPerPixel!=surf->format->BitsPerPixel)
		{
			std::cout<<"surfMask BitsPerPixel mask does not match RBGsurface BitsPerPixel.\n";
			exit(-1);
		}
	}
    */
	SDL_LockSurface(surf);
	if(surfMask)SDL_LockSurface(surfMask);

	for(int a=0; a<SIZE_X; a++)
	{
		for(int b=0; b<SIZE_Y; b++)
		{
			if(surfMask)
			{
				auto pixel=Color(getPixel(surfMask,a,b));
				//std::cout<<(int)pixel.r<<','<<(int)pixel.g<<','<<(int)pixel.b<<','<<(int)pixel.a<<','<<std::endl;
				if(pixel.a!=0)
				{
					color.a=pixel.a;
					setPixel(surf,a,b,color.toHex());
				}
				else
				{
					setPixel(surf,a,b,clear);
				}
			}
			else
			{
				setPixel(surf,a,b,color.toHex());
			}
		}
	}
	SDL_UnlockSurface(surf);
	if(surfMask)SDL_UnlockSurface(surfMask);

	SDL_Texture *texture=SDL_CreateTextureFromSurface(renderer,surf);
    if(!texture)printf("ALLOC_rect texture error: %s\n",SDL_GetError());
	SDL_FreeSurface(surf);
	return texture;
}

#ifdef SG2D_TEST
void graphics_test()
{
}
#endif