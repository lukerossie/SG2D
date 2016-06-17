#include "../../SG2D/SG2D.hpp"
#include <iostream>
#include <math.h>
#include <map>
#include <cstdio>
#include <fstream>

using namespace std;
using namespace SG2D;

//in windows you can remove the console by going into make.bat and changing SUBSYSTEM:CONSOLE to SUBSYSTEM:WINDOWS
int main(int argc, char **argv)
{
    setCurFontPath("res/fonts/FFFFORWA.ttf");//global in SG2D.hpp
    
    bool online=false;
    if(online)
    {
        map<string,string> conf=parseConfig("res/config.txt");
        Vec2 wh=SG2D::init("test",750,750,SDL_WINDOW_RESIZABLE,conf["username"].c_str(),conf["serverip"].c_str());//create window, wh is its size, runs tests if SG2D_TEST is defined
    }
    else
    {
        Vec2 wh=SG2D::init("test",750,750,SDL_WINDOW_RESIZABLE,0,0);//passing zero for the last two fields means offline, not passing them defaults to local server
    }

    
    Clock gameClock;//declare the clock here and you can have references to it in lambdas while it gets updated
    
    Sound s(Mix_LoadWAV("res/dub.ogg"));
    s.play();
    
    Image sb("res/test.bmp");
    
    Entity board(Renderer::ShapeType::RECT,&root);
    board.setrs({1,1});
    board.myvisible=false;
    
    Entity someimg(Renderer(&sb),&board);
    someimg.setrs({.2,.2});
    someimg.setCurrentPos({.5,.5});
    
    Entity sometext(Renderer("template",Color(255,0,0,255)),&someimg);
    sometext.setrs({1,1});
    
    onResize=[](Vec2 wh)
    {
    };
    //other handlers can be found in SG2D.hpp SG2D namespace (clicks, keydown, etc.)
    
    handleMessage=[](vector<string> tags)//custom handler
    {   
        if(tags[1]=="Disconnect")
        {
            //erase the player from an array or something here
        }
    };
    
    //initCamera({0,0}); requires you set SG2D::player to a pointer to an entity that the offset will be calculated from, {0,0} is the position of the camera as offset from the center of the screen
    player=&someimg;
    onKeyDown['w']=[&someimg](string who,long long timestamp)
    {
        someimg.vel+=Vec2(0,-.1);
    };
    onKeyDown['a']=[&someimg](string who,long long timestamp)
    {
        someimg.vel+=Vec2(-.1,0);
    };
    onKeyDown['s']=[&someimg](string who,long long timestamp)
    {
        someimg.vel+=Vec2(0,.1);
    };
    onKeyDown['d']=[&someimg](string who,long long timestamp)
    {
        someimg.vel+=Vec2(.1,0);
    };
    
    onKeyUp['w']=[&someimg](string who,long long timestamp)
    {
        someimg.vel-=Vec2(0,-.1);
    };
    onKeyUp['a']=[&someimg](string who,long long timestamp)
    {
        someimg.vel-=Vec2(-.1,0);
    };
    onKeyUp['s']=[&someimg](string who,long long timestamp)
    {
        someimg.vel-=Vec2(0,.1);
    };
    onKeyUp['d']=[&someimg](string who,long long timestamp)
    {
        someimg.vel-=Vec2(.1,0);
    };
    
    bool running=true;
    while(running)
	{
        SDL_Delay(1);//give cpu a break
        
        gameClock.update();
        frame(&gameClock);
        
        if(key(SDLK_ESCAPE))running=false;
	}
    
    if(server)
    {
        sendtags(server,{userName,"Disconnect"});//write your own message handler
        close(server);
    }
    
    //must return on windows
    return 0;
}


