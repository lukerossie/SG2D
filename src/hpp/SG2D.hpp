#pragma once
//@todo move these to cpp file
#include "../../../OSAL/src/hpp/system.hpp"
#include "../../../OSAL/src/hpp/graphics.hpp"
#include "../../../OSAL/src/hpp/input.hpp"
#include "../../../OSAL/src/hpp/net.hpp"
#include "../../../OSAL/src/hpp/sound.hpp"
#include "../../../OSAL/src/hpp/util.hpp"
#include "widgets.hpp"
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

/*
@NOTES
release: remove debugging symbols from exe, enable optimizations

@TODO
make entities copyable? *should now be done, at least destructable!*

debugging features
    show info on hover
    show fps
    console?

easy keybinding, on entities?

collision detection for rotated rects, gjk

@things that are really bad
finalize (recomputing frame on a collision is retarded)


@networking overview
sendq (this is the only function you should call from a game)
adds message to a buffer to be sent to specified server, all sendq messages are sent at the end of the frame

send
wrapper around SDLNet_TCP_Send

recv
wrapper around SDLNet_TCP_Recv
appends a newline to the end of the string to signify end of message

@rework
key state code can stay the same, just need to send them to a server that runs a sim instead of broadcasting them
    then need an entirely new system for recieving state from the server, could use udp

remove redundancies (keyState, keyDown, onKeyEvent...)

make naming and convention consistent (e.g. constructors start with ctor, destructors start with dtor, followed by name of class followed optionally by something else)

perhaps entity should compose body instead of inherit it

factor out neon needle specific code into the game

factor out non-essential code into other modules (e.g. gettags)
*/

std::string gettag(std::string s, int tagnumber, int index);
std::string gettags(std::string s, int begin, int end);
std::pair<std::vector<std::string>,std::string> separate(char c,std::string stream,bool exclude);

class Entity;
class Body;

namespace SG2D
{

    extern std::map<int,bool> entityWasDrawnLastFrame;
    //events
    extern std::function<void(vec2 wh_p)> onResize;
    extern std::function<void()> preDraw;//used to draw background
    extern std::function<void()> postDraw;//used to draw hud
    extern std::function<void(int key, bool down)> keyEvent;
    //Also called for mutliplayer entities (IF SG2D::server), don't capture! If you need to do something client specific check key function.
    extern std::map<int,std::function<void(std::string who,long long time)>> onKeyUp;//null passed if its the client player
    extern std::map<int,std::function<void(std::string who,long long time)>> onKeyDown;
    extern std::map<std::string,std::function<void(std::string who,vec2 pos,long long time)>> onMB_click;//null passed if its you
    extern std::map<std::string,std::function<void(std::string who,vec2 pos,long long time)>> onMB_release;
    extern std::function<void(std::vector<std::string> tags)> handleMessage;//any other messages (custom messages, non key press)

    extern bool enableCamera;//must set player if true
    extern vec2 cameraOffset;//uses getCurrentPos OF PLAYER
    extern vec2 cameraCenter;//set in entity.update by player

    extern bool enableLogging;

    extern int logTimer;
    extern bool enablelog0;

    extern Entity *player;//there can be only one
    extern std::vector<Entity*> ent;//keep this for now so we can easily do flat operations on all entities (probably would be better to just use children)

    extern bool enableSendTags;
    void sendtags(tcp_socket *who, std::vector<std::string> data);
    void sortEnts(Entity *parent);
    void setCurFontPath(char const *fontPath);

    void initCamera(vec2 cameraCenter_p);

    vec2 getScreenSize();//updated dynamically

    void log0(std::string msg);//disableable cout (no time limit)
    void log(std::string label,int i);
    void log(std::string label,float i);
    void log(std::string label,double i);
    void log(std::string label,rect s);
    void log(std::string label,vec2 v);
    void log(std::string label,bool b);
    void log(std::string label,std::string s);
    void log(std::string label,std::vector<float> v);
}

class Body
{
    double renderangle;
    vec2 renderpos;
    vec2 rendersize;
    bool dirty;
    double angle;

    vec2 origin;//same as relpos, .5,.5 for center
    vec2 pos;
    vec2 size;

    //@TODO RENDERSCALE ADD SCALE TO PARENT RENDERSCALE TO GET SCALE (SHOULD DEPDN ON RELCHILDREN/PARENT ?)
    //if we do this we have to make scale special, and separate it from renderpos, so that it doesnt effect children who update from parents renderpos
    vec2 scale;
    vec2 relposme;//this could be a vec4, but so could scale... can't think of a use for that though really

    //like relpos and relsize except can only scale x with x and y with y, easy vector operations
    vec2 rp;
    vec2 rs;

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

    virtual void update(bool inherit,vec2 renderposOffset);

    void setdirty(bool dirty_p);
    void setangle(double angle_p);
    void setpos(vec2 const &pos_p);
    void setsize(vec2 const &size_p);
    void setrelpos(std::vector<double> relpos_p);
    void setrelsize(std::vector<double> relsize_p);
    void setscale(vec2 const &scale_p);
    void setscale(float scale_p);
    void setrelposme(vec2 const &relposme_p);
    void setorigin(vec2 const &origin_p);
    void setrp(vec2 const &rp_p);
    void setrs(vec2 const &rs_p);
    void setrenderpos(vec2 const &rp);
    void setrendersize(vec2 const &rs);
    void setrenderangle(double renderangle_p);

    //all getters return copies, have to be set

    bool getdirty();
    int getuid();

    double getangle();
    vec2 getpos();
    vec2 getsize();
    std::vector<double> getrelpos();
    std::vector<double> getrelsize();
    vec2 getscale();
    vec2 getrelposme();
    vec2 getorigin();
    vec2 getrp();
    vec2 getrs();

    vec2 getrenderpos();
    vec2 getrendersize();
    double getrenderangle();
};

void drawImageFromBody(Image *img_p,u8 renderalpha_p,Body *b_p,rect &dest_p,vec2 &rs);
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
    void draw(long long deltaMilli,u8 renderalpha,Body *b,rect &dest,vec2 &rs);
};
class Renderer
{
    u8 alpha;//0-255
    u8 renderalpha;
    rect dest;
    std::string text;
    rect shapeCenter;
    rect shapeDest;
    color draw_color;//last u8 always replaced with alpha before rendering

    public:
    bool use_prealloc_color;
    std::shared_ptr<Texture> prealloc_red;
    std::shared_ptr<Texture> prealloc_green;
    std::shared_ptr<Texture> prealloc_blue;

    std::shared_ptr<Texture> shape;
    bool alwaysdraw;
    Animation anim;
    surface *maskSurface;//dont free this, reference to single allocated surface in main
    surface *prealloc_mask_surface;//dont free this, reference to single allocated surface in main
    /*@TODO
    add a variable you can use that will recreate the textures if the size changes from a certain factor
    e.g. renderer.resizeFact=5
    if(abs(size-lastResize)>size/5)recreatetexture();
    *should apply to text and shapes, anything that is allocated in code*
    */
    vec2 shapeScale;
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
    Renderer(std::string text_p, color color_p, bool fastRender=true);
    Renderer(ShapeType st_p, color color_p={255,255,255,255});

    void use_prealloc_color_mask(surface *s);
    virtual void draw(long long deltaMilli,Body *b,vec2 renderpos,vec2 rendersize,int alphaOffset,vec2 positionOffset={},bool just_mask=false);

    void settext(std::string text_p);
    std::string gettext();

    void setcolor(color color_p);
    color getcolor();
    rect getdest();
    void setalpha(u8 alpha_p);
    u8 getalpha();
    u8 getrenderalpha();
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

    Entity *parent;
    int order;
    std::vector<vec2> image_fade_previous;
    int image_fade_frame_drag_counter;
    vec2 unusedOffset;
    bool fading=false;


    public:

    int image_fade;
    int image_fade_frame_drag;
    std::string entityName;
    int resizeFactorDirty;//the amount you have to scale in size to redirty textures, such as text. uses lastResize rendersize (last time resizeFactor was triggered, or at creation)
    vec2 lastResize;

    std::string userData;//not used in the class, should probably be removed.
    std::vector<Entity *> children;
    std::vector<Entity *> collider;

    double spin;//returns delta PER SECOND
    Motion<vec2> positionMotion;//returns defaultPos delta PER SECOND

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
    std::function<void()> drawCallback;
    std::function<void()> onDrawBegin;
    int uid_e;
    enum class PosType{POS,RP}defaultPos;
    bool listed;
    vec2 startrenderpos;
    vec2 lastDelta;
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
    //void moveTo(vec2 endpos, double acceleration, double velConst=0, std::function<void()> callback=[](){});

    //no setter
    Entity *getparent();

    u8 getalpha();
    vec2 getCurrentPos();
    int getorder();
    int getorderRecurs();

    void setCurrentPos(vec2 v);
    void setorder(int order_p);
    void setalpha(u8 alpha_p);
};

namespace SG2D
{
    extern tcp_socket *serverIP;
    extern tcp_socket *server;
    extern std::string userName;

    extern Entity root;
    bool initServer(char const *serverIP);
    vec2 init(char const *userName_p=0,char const *serverIP="127.0.0.1");
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

        void update(long long deltatime);
    }extern world;

    void frame(long long deltatime);
}














