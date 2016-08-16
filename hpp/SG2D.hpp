#pragma once

#include "include/core.hpp"
#include "include/graphics.hpp"
#include "include/input.hpp"
#include "include/net.hpp"
#include "include/sound.hpp"
#include "include/util.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <functional>
#include <algorithm>
#include <deque>
#include <cmath>

/*NOTES
release: remove debugging symbols from exe, enable optimizations

non-images (textures) cant be rotated as of now
    can use gfx library for that, should be hardware accelerated though.
        eventually would be nice to have full featured hardware acceleration for all kinds of geometry
*/

/*@TODO
rewrite mover to move a certain distance, right now its brittle, probably buggy, and doesnt (cant) account for external forces on movement

make entities copyable? *should now be done, at least destructable!*
    right now you cant push a bunch into a vector on the stack for example (why?)

debugging features
    show info on hover
    show fps
    console?

easy keybinding, on entities?

collision detection for rotated rects
*/

/*things that are really bad
finalize (recomputing frame on a collision is retarded)
*/

/*networking overview
sendq (this is the only function you should call from a game)
adds message to a buffer to be sent to specified server, all sendq messages are sent at the end of the frame

send
wrapper around SDLNet_TCP_Send

recv
wrapper around SDLNet_TCP_Recv
appends a newline to the end of the string to signify end of message
*/

class Entity;
class Body;

namespace SG2D
{

    extern std::map<int,bool> entityWasDrawnLastFrame;
    //events
    extern std::function<void(Vec2 wh_p)> onResize;
    extern std::function<void()> preDraw;//used to draw background
    extern std::function<void()> postDraw;//used to draw hud
    //Also called for mutliplayer entities (IF SG2D::server), don't capture! If you need to do something client specific check key function.
    extern std::map<int,std::function<void(std::string who,long long time)>> onKeyUp;//null passed if its the client player
    extern std::map<int,std::function<void(std::string who,long long time)>> onKeyDown;
    extern std::map<std::string,std::function<void(std::string who,Vec2 pos,long long time)>> onMB_click;//null passed if its you
    extern std::map<std::string,std::function<void(std::string who,Vec2 pos,long long time)>> onMB_release;
    extern std::function<void(std::vector<std::string> tags)> handleMessage;//any other messages (custom messages, non key press)

    extern bool enableCamera;//must set player if true
    extern Vec2 cameraOffset;//uses getCurrentPos OF PLAYER
    extern Vec2 cameraCenter;//set in entity.update by player

    extern bool enableLogging;
    extern Clock logClock;

    extern int logTimer;
    extern bool enablelog0;

    extern Entity *player;//there can be only one
    extern std::vector<Entity*> ent;//keep this for now so we can easily do flat operations on all entities (probably would be better to just use children)

    extern bool enableSendTags;
    void sendtags(TCPsocket who, std::vector<std::string> data);
    void sortEnts(Entity *parent);
    void setCurFontPath(char const *fontPath);

    void initCamera(Vec2 cameraCenter_p);

    Vec2 getScreenSize();//updated dynamically

    void log0(std::string msg);//disableable cout (no time limit)
    void log(std::string label,int i);
    void log(std::string label,float i);
    void log(std::string label,double i);
    void log(std::string label,SDL_Rect s);
    void log(std::string label,Vec2 v);
    void log(std::string label,bool b);
    void log(std::string label,std::string s);
    void log(std::string label,std::vector<float> v);
}

class Body
{
    double renderangle;
    Vec2 renderpos;
    Vec2 rendersize;
    bool dirty;
    double angle;

    Vec2 origin;//same as relpos, .5,.5 for center
    Vec2 pos;
    Vec2 size;

    //@TODO RENDERSCALE ADD SCALE TO PARENT RENDERSCALE TO GET SCALE (SHOULD DEPDN ON RELCHILDREN/PARENT ?)
    //if we do this we have to make scale special, and separate it from renderpos, so that it doesnt effect children who update from parents renderpos
    Vec2 scale;
    Vec2 relposme;//this could be a vec4, but so could scale... can't think of a use for that though really

    //like relpos and relsize except can only scale x with x and y with y, easy vector operations
    Vec2 rp;
    Vec2 rs;

    std::vector <double> relpos;
    std::vector <double> relsize;

    int uid;
    public:
    bool listed;
    std::vector<Body *> bodyChildren;
    Body *parent;
    bool ignoreOrigin;

    Body(Body *parent_p=0,bool listed_p=true);
    virtual ~Body();

    virtual void update(bool inherit,Vec2 renderposOffset);

    void setdirty(bool dirty_p);
    void setangle(double angle_p);
    void setpos(Vec2 const &pos_p);
    void setsize(Vec2 const &size_p);
    void setrelpos(std::vector<double> relpos_p);
    void setrelsize(std::vector<double> relsize_p);
    void setscale(Vec2 const &scale_p);
    void setscale(float scale_p);
    void setrelposme(Vec2 const &relposme_p);
    void setorigin(Vec2 const &origin_p);
    void setrp(Vec2 const &rp_p);
    void setrs(Vec2 const &rs_p);
    void setrenderpos(Vec2 const &rp);
    void setrendersize(Vec2 const &rs);
    void setrenderangle(double renderangle_p);

    //all getters return copies, have to be set

    bool getdirty();
    int getuid();

    double getangle();
    Vec2 getpos();
    Vec2 getsize();
    std::vector<double> getrelpos();
    std::vector<double> getrelsize();
    Vec2 getscale();
    Vec2 getrelposme();
    Vec2 getorigin();
    Vec2 getrp();
    Vec2 getrs();

    Vec2 getrenderpos();
    Vec2 getrendersize();
    double getrenderangle();
};

void drawImageFromBody(Image *img_p,byte renderalpha_p,Body *b_p,SDL_Rect &dest_p,Vec2 &rs);
struct Animation
{
    bool loop;
    int index;
    int FPS;
    bool ranOnce;
    long long timer;
    std::vector<Image *> *imgv;

    Animation();
    Animation(std::vector<Image *> *imgv_p,bool loop_p=false,int FPS=60);
    void draw(long long deltaMilli,byte renderalpha,Body *b,SDL_Rect &dest,Vec2 &rs);
};
class Renderer
{
    byte alpha;//0-255
    byte renderalpha;
    SDL_Rect dest;
    std::string text;
    SDL_Point shapeCenter;
    SDL_Rect shapeDest;
    Color color;//last byte always replaced with alpha before rendering

    public:
    std::shared_ptr<Texture> shape;
    bool alwaysdraw;
    Animation anim;
    SDL_Surface *maskSurface;//dont free this, reference to single allocated surface in main
    /*@TODO
    add a variable you can use that will recreate the textures if the size changes from a certain factor
    e.g. renderer.resizeFact=5
    if(abs(size-lastResize)>size/5)recreatetexture();
    *should apply to text and shapes, anything that is allocated in code*
    */
    Vec2 shapeScale;
    Image **img;

    Text textImage;
    bool withinScreen;
    bool dirty;//will recreate all children (or set their dirty) if true
    //@todo circle, other geometry
    enum class ShapeType{RECT,CIRCLE,NONE};
    ShapeType st;

    Renderer();
    Renderer(Image **img_p);
    Renderer(Animation anim_p);
    //@note if you want to use text on an entity you have to call the text constructor, then add shapes and images after
    Renderer(std::string text_p, Color color_p, bool fastRender=true);
    Renderer(ShapeType st_p, Color color_p={255,255,255,255});

    virtual void draw(long long deltaMilli,Body *b,int alphaOffset,Vec2 origin,Vec2 positionOffset={});

    void settext(std::string text_p);
    std::string gettext();

    void setcolor(Color color_p);
    Color getcolor();
    SDL_Rect getdest();
    void setalpha(byte alpha_p);
    byte getalpha();
    byte getrenderalpha();
};

/*
in non-update code all references to parent must be null checked. root is parentless.
update is not called on root.
*/
template<typename T>
struct Motion
{
    static const double airDensity;
    static const double gravity;
    std::function<double(double vel)> drag;//takes velocity returns value substracted from acceleration
    double dragCoeff;
    double mass;
    double acceleration;//added to velocity every frame, passed and set to the result of drag
    double velocity;//multiplied by direction and returned as the delta from update
    double area;
    enum PhysicsType{GRAVITY,DRAG,CONSTANT};
    PhysicsType pt;
    T direction;//constant value

    Motion() : acceleration(0),velocity(0),direction(0),mass(100),dragCoeff(.75),area(1),pt(PhysicsType::GRAVITY)
    {
    }

    T update(int deltaMilli)//returns the delta to move
    {
        //@TODO drag coefficient is the typical one for a model rocket, finding the real one is not worth it especially in 2d
        if(pt==PhysicsType::GRAVITY)//@TODO fix for games that actually have gravity
        {
            auto weight=mass*gravity;
            auto drag=dragCoeff*(airDensity/2)*pow(velocity,2)/2*area;
            acceleration=(weight-drag)/mass;
            velocity+=acceleration;
        }
        else if(pt==PhysicsType::DRAG)
        {
            if(drag)
            {
                acceleration-=drag(velocity);
            }
            else
            {
                acceleration-=velocity * deltaMilli/1000 /1000;
            }
            velocity+=acceleration;
        }

        if(velocity<=0)
        {
            velocity=0;
            acceleration=0;
        }

        return direction.unit()*velocity;
    }
    bool addForce(double forceVelocity,T forceDirection)
    {//@TODO keep velocity in direction if depending on current direction/velocity and new force
        if(forceVelocity<velocity)return false;//@TODO replace this with  ^
        acceleration=0;
        direction=forceDirection;
        T diff=forceDirection+direction;
        velocity=forceVelocity;
        return true;
    }
};
template <typename T>
const double Motion<T>::airDensity=1.225;
template <typename T>
const double Motion<T>::gravity=9.8;

class Entity : public Body
{
    /*
    struct Mover//@todo something better
    {
        Vec2 beginpos;
        Vec2 endpos;
        bool moving;
        double acceleration;
        double velConst;
        std::function<void()> callback;

        Mover()
        {
            moving=false;
        }

        void update(Entity *movee,int deltaMilli)
        {
            if(moving)
            {
                auto delta=endpos-beginpos;
                auto adj=delta.unit().scale(acceleration);
                if(!velConst)
                {
                    if((movee->getCurrentPos()-endpos).len()<(movee->getCurrentPos()-beginpos).len())
                    {
                        movee->vel-=adj;
                    }
                    else
                    {
                        movee->vel+=adj;
                    }
                }
                else
                {
                    movee->vel=movee->vel.unit()*velConst;
                }

                if(!delta.sameSign(movee->vel)||(movee->getCurrentPos()-endpos).len()<(movee->vel).len()*deltaMilli/1000+.00000001)
                {
                    auto val=(movee->getCurrentPos()-endpos).len();
                    moving=false;
                    movee->vel=0;
                    movee->setCurrentPos(endpos);
                    if(callback)
                    {
                        callback();
                    }
                }
            }
        }
    }mover;
    */

    Entity *parent;
    int order;


    public:

    std::string entityName;
    int resizeFactorDirty;//the amount you have to scale in size to redirty textures, such as text. uses lastResize rendersize (last time resizeFactor was triggered, or at creation)
    Vec2 lastResize;

    std::string userData;//not used in the class, should probably be removed.
    std::vector<Entity *> children;
    std::vector<Entity *> collider;

    double spin;//returns delta PER SECOND
    Motion<Vec2> positionMotion;//returns defaultPos delta PER SECOND

    bool visible;//here rather than in renderer since it stops absolutely everything. (returns from drawrecurs, no children draw)
    bool myvisible;//just stops my own draw
    bool enabled;//if its updated and events are called from world.update, effects children
    bool myenabled;//does not effect children
    bool solid;//whether or not it takes mouse events
    //visible is public on renderer
    Renderer r;
    bool offsetByCamera;
    std::function<void()> onclick;
    std::function<bool()> onupdate;
    SDL_Cursor *hoverCursor;
    std::function<void()> drawCallback;
    std::function<void()> onDrawBegin;
    int uid_e;
    enum class PosType{POS,RP}defaultPos;
    bool listed;
    Vec2 startrenderpos;
    Vec2 lastDelta;
    bool hasCollided;
    bool relparent;//you check this before inheriting from parent
    bool relchildren;//your children check this before inheriting

    Entity();
    //note: each image corresponds to a buffer of pixels, they are not managed at all internally. reuse images
    Entity(Renderer r_p,Entity *parent_p=0,bool listed=true,bool isPlayer=false);
    virtual ~Entity();

    virtual void update(int deltaMilli);
    bool finalize();

    virtual void draw(long long deltaMilli);
    bool decendedFrom(Entity *e);

    //this will break easily. dont use it unless there is nothing else effecting the position (until its rewritten)
    //void moveTo(Vec2 endpos, double acceleration, double velConst=0, std::function<void()> callback=[](){});

    //no setter
    Entity *getparent();

    byte getalpha();
    Vec2 getCurrentPos();
    int getorder();
    int getorderRecurs();

    void setCurrentPos(Vec2 v);
    void setorder(int order_p);
    void setalpha(byte alpha_p);
};

namespace SG2D
{
    extern TCPsocket serverIP;
    extern TCPsocket server;
    extern std::string userName;

    extern Entity root;
    bool initServer(char const *serverIP);
    Vec2 init(char const *title=0, int width=0, int height=0, int flags=0, char const *userName_p=0,char const *serverIP="127.0.0.1");
    void sortEnts(Entity *parent);
    void updateRecurs(Entity *e,long long deltaMilli);
    void finalizeRecurs(Entity *e);
    void drawRecurs(Entity *e);

    struct World
    {
        Entity *topHover;

        bool last_MB_RIGHT;
        bool last_MB_LEFT;
        bool last_MB_MIDDLE;

        std::map<int,bool> lastKey;

        World();

        void update(Clock *gameClock);
    }extern world;

    void frame(Clock *gameClock);
}














