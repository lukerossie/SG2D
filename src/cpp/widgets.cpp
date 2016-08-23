#include "../hpp/widgets.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include "../../../OSAL/src/hpp/util.hpp"
#include "../../../OSAL/src/hpp/system.hpp"

Texture::Texture()
{

}
Texture::Texture( char const *path )
{
	rawTexture=ctor_texture(path);
}
Texture::Texture( texture *t )
{
	rawTexture = t;
}
Texture::~Texture()
{
	if(rawTexture)
	{
		dtor_texture( rawTexture );
	}
}

Image::Image()
{
	angle=0;
	center={};
	dest={};
	src={};
}
Image::Image( char const *path, int srcX, int srcY, int srcW, int srcH ) : Image()
{
	texture = std::make_shared<Texture>( path );
	surface *surf = new surface(path);//needs free
    src.x = 0;
    src.y = 0;
    if(srcX&&srcY)
	{
		src.x=srcX;
		src.y=srcY;
	}
	else
	{
		src.w = surf->pixels.size();
		src.h = surf->pixels[0].size();
	}
	dest.x = 0;
    dest.y = 0;
    dest.w = src.w;
    dest.h = src.h;
	delete ( surf );
	if(!texture.get())
    {
     	print(std::string("Image ctor error")+get_error());
    }
}
Image::~Image()
{

}
void Image::draw()
{
	draw_texture( texture.get()->rawTexture, &dest, angle, &center, &src );
}


Font::Font():font(0){}
Font::Font(std::string path_p,int fontsize):Font()
{
	path=path_p;
	font=ctor_ttf_font(path.c_str(),fontsize);
    if(!font)
    {
        print(std::string("Error opening font")+get_error());
    }
}
Font::~Font()
{
	if(font)dtor_ttf_font(font);
}
void Font::setfontsize(int fontsize)
{
	if(font)dtor_ttf_font(font);
	font=ctor_ttf_font(path.c_str(),fontsize);
}

PlainText::PlainText(Font *f_p,color color_p,vec2 pos_p):f(f_p),text_color(color_p),pos(pos_p),rendertext(NULL)
{
	//if constructor constructs state that gets destroyed in destructor then copying the thing wont keep that state
}
PlainText::~PlainText()
{
	if(rendertext)
	{
		dtor_texture(rendertext);
		rendertext=NULL;
	}
}
void PlainText::draw()
{
	if(!rendertext)
	{
		settext(text);//@perf check for new text, stupid move semantics destruct texture and settext isnt called
		return;
	}
	dest.x=pos.x;
	dest.y=pos.y;
	draw_texture( rendertext, &dest );
}
void PlainText::settext(std::string text_p)
{
	text=text_p;
	if(rendertext)
	{
		dtor_texture(rendertext);
		rendertext=NULL;
	}
	if(!text.size())
	{
		return;
	}
	color_impl.r=text_color[0];
	color_impl.g=text_color[1];
	color_impl.b=text_color[2];
	color_impl.a=text_color[3];
	surface *s=new surface( f->font, text.c_str(), color_impl );
	if(!s)print(std::string("PlainText error")+get_error());//these should be moved into the lower level code that handles sdl
	rendertext=ctor_texture( s );
	set_texture_alpha(rendertext, color_impl.a);
	dest.w=s->pixels.size();
	dest.h=s->pixels[0].size();
	delete ( s );
}
std::string PlainText::gettext()
{
	return text;
}

Text::Text():origin({.5,.5}),rendertext(nullptr),constFS(false),fontsize(0),fontsizeOverride(NULL),last_fontsizeOverride(0){}
Text::Text( char const *path, color color_p, bool fastRender_p ):Text()
{
	text_color.r=color_p[0];
	text_color.g=color_p[1];
	text_color.b=color_p[2];
	text_color.a=color_p[3];

	fastRender=fastRender_p;
	renderW=0;
	renderH=0;
	f=std::make_shared<Font>( path );
	dest={};
	src={};
	center={0,0};
	angle=0;
}
Text::~Text()
{
	if(rendertext)dtor_texture( rendertext );
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
			size_ttf_font(f->font,text.c_str(),&renderW,&renderH);
		}
		else if(constFS&&fontsize)
		{
			size_ttf_font(f->font,text.c_str(),&renderW,&renderH);
			//reuse same fs as first calculated (fs is zero first frame), shouldnt even need to setfontsize
		}
		else
		{
			//std::cout<<"RERENDERING TEXT"<<std::endl;//@perf
			int fs=dest.h;
			if(fastRender)
			{
				f->setfontsize(fs);
				size_ttf_font(f->font,text.c_str(),&renderW,&renderH);
				if(renderW>renderH)
				{
					//int len=text.length()*dest.h;
					fs=fs/text.length()/.7;//magic number relating line height and width approx (different for every font, ratio should never go above .7)
					size_ttf_font(f->font,text.c_str(),&renderW,&renderH);
				}
			}
			f->setfontsize(fs);

			size_ttf_font(f->font,text.c_str(),&renderW,&renderH);

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
					size_ttf_font( f->font,text.c_str(),&renderW,&renderH );
				}
			}

			fontsize=fs;
		}

		{//buffers
			surface *s=new surface( f->font, text.c_str(), text_color );

			if(!s)
			{//if there is no text or error s will be null
                printf("Null text surface: %s\n",get_error());
				return;
			}

			if(rendertext)dtor_texture( rendertext );

			int offset=0;//@bug set this to 1 to remove bugged pixel border around texture, or 0 to disable
			src={ (f64)offset,(f64)offset,(f64)(s->pixels.size()-offset*2),(f64)(s->pixels[0].size()-offset*2) };

			rendertext=ctor_texture( s );
            if(!rendertext)printf("Render text texture error: %s\n",get_error());
			set_texture_alpha(rendertext, text_color.a);
			delete ( s );
		}
		dirty=false;
	}
	dest.x+=(dest.w-renderW)*origin.x;//align with private text origin
	dest.y+=(dest.h-renderH)*origin.y;//this is the origin of the text inside the dest box
	dest.w=renderW;//have to set even if its not dirty
	dest.h=renderH;
	draw_texture( rendertext, &dest, angle, &center, &src );
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
