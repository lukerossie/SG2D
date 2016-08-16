#include "SG2D.hpp"

using namespace SG2D;

namespace
{
    char *curFontPath=0;
    Vec2 initWH={};
    Vec2 toorigin(Body *e);
    Vec2 fromorigin(Body *e);
    constexpr int SDL_lowScancode=0;
    constexpr int SDL_highScancode=282;

    std::map<TCPsocket, std::vector<std::string>> payload;
    void sendq(TCPsocket who, std::string data)//add to a queue to be sent at the end of the frame (for performance/stability)
    {
        payload[who].push_back(data);
    }

    int idCounter=0;
    std::map<std::string,long long> logMap;
    void logBase(std::string label, std::string content)
    {
        if(!enableLogging)return;
        if(!logMap[label] ||
            (logMap[label]&&logClock.getNow()-logMap[label]>=logTimer))
        {
            std::cout<<label<<": "<<content<<std::endl;
            logMap[label]=logClock.getNow();
        }
    }

    std::vector<std::string> tagsToVector(std::string str)
    {
        std::vector<std::string> retval;
        int i=0;
        std::string iterval;
        while((iterval=gettag(str,i,1)).length())
        {
            retval.push_back(iterval);
            i++;
        }
        return retval;
    }
}

namespace SG2D
{

    std::map<int,bool> entityWasDrawnLastFrame;
    std::function<void(Vec2 wh_p)> onResize;
    //called in frame
    std::function<void()> preDraw;//used to draw background
    std::function<void()> postDraw;//used to draw hud

    //Also called for mutliplayer entities (IF SG2D::server), don't capture! If you need to do something client specific check key function.
    std::map<int,std::function<void(std::string who,long long time)>> onKeyUp;//null passed if its the client player
    std::map<int,std::function<void(std::string who,long long time)>> onKeyDown;
    std::map<std::string,std::function<void(std::string who,Vec2 pos,long long time)>> onMB_click;//null passed if its you
    std::map<std::string,std::function<void(std::string who,Vec2 pos,long long time)>> onMB_release;
    std::function<void(std::vector<std::string> tags)> handleMessage;//any other messages (custom messages, non key press)

    bool enableCamera=false;//must set player if true
    Vec2 cameraOffset;//uses getCurrentPos OF PLAYER
    Vec2 cameraCenter;//set in entity.update by player

    bool enableLogging=true;
    Clock logClock;

    int logTimer=1000;
    bool enablelog0=true;

    Entity *player=nullptr;//there can be only one
    std::vector<Entity*> ent=std::vector<Entity*>();//keep this for now so we can easily do flat operations on all entities (probably would be better to just use children)

    bool enableSendTags=true;
    void sendtags(TCPsocket who, std::vector<std::string> tags)
    {
        if(!enableSendTags)return;
        std::string s;
        for(int i=0; i<tags.size(); i++)
        {
            s+="<";
            s+=tags[i];
            s+=">";
        }
        sendq(who,s);
    }
    void setCurFontPath(char const *fontPath)
    {
        curFontPath=(char*)fontPath;
    }

    Vec2 getScreenSize()//updated dynamically
    {
        return initWH;
    }
    void initCamera(Vec2 cameraCenter_p)
    {
        enableCamera=true;
        cameraCenter=cameraCenter_p;
    }

    void log0(std::string msg)//disableable cout
    {
        if(enablelog0)
        {
            std::cout<<msg<<std::endl;
        }
    }
    void log(std::string label,int i)
    {
        logBase(label,std::to_string(i));
    }
    void log(std::string label,float i)
    {
        logBase(label,std::to_string(i));
    }
    void log(std::string label,double i)
    {
        logBase(label,std::to_string(i));
    }
    void log(std::string label,SDL_Rect s)
    {
        if(!enableLogging)return;
        std::string str="{"+std::to_string(s.x)+", "+std::to_string(s.y)+", "+std::to_string(s.w)+", "+std::to_string(s.h)+"}";
        logBase(label, str);
    }
    void log(std::string label,Vec2 v){
        std::string str="{"+std::to_string(v.x)+", "+std::to_string(v.y)+"}";
        logBase(label,str);
    }

    void log(std::string label,bool b){
        if(b)
        {
            logBase(label,"true");
        }
        else
        {
            logBase(label,"false");
        }
    }
    void log(std::string label,std::string s){
        logBase(label,s);
    }
    void log(std::string label,std::vector<float> v){
        std::string s;
        s+='{';
        for(auto &ele: v)
        {
            s+=std::to_string(ele)+',';
        }
        s+='}';
        logBase(label,s);
    }
}//</SG2D>

//body
Body::Body(Body *parent_p,bool listed_p)
{
    listed=listed_p;
    renderangle=0;
    ignoreOrigin=false;
    parent=parent_p;
    angle=0;
    pos={};
    size={};
    relposme={};
    relpos={};
    relsize={};
    renderpos={};
    rendersize={};
    origin={.5,.5};
    scale={1,1};
    rp={};
    rs={};
    dirty=true;
    setdirty(true);

    uid=idCounter++;

    if(parent)
    {
        parent->bodyChildren.push_back(this);
    }
    setdirty(true);//update on first frame
}
Body::~Body()
{
    if(!parent)return;
    //if(!listed)return;
    using namespace SG2D;
    auto &bc=parent->bodyChildren;
    for(int a=0; a<bc.size(); a++)
    {
        if(bc[a]->getuid()==uid)
        {
            bc.erase(bc.begin()+a);
        }
    }

    SG2D::log("WARNING",std::string("BODY DESTRUCTOR CALLED"));
}

void Body::update(bool inherit,Vec2 renderposOffset)
{
    if(!dirty)return;//if size is dirty pos is likely dirty (bodyChildren relpos, relposme, etc)

    {//rendersize
        //relsize
        rendersize=size;
        if(relsize.size()>=1)
        {
            rendersize[0]+=relsize[0]*parent->rendersize[0];
        }
        if(relsize.size()>=2)
        {
            rendersize[1]+=relsize[1]*parent->rendersize[1];
        }
        if(relsize.size()>=3)
        {
            rendersize[0]+=relsize[2]*parent->rendersize[1];
        }
        if(relsize.size()>=4)
        {
            rendersize[1]+=relsize[3]*parent->rendersize[0];
        }

        //rs
        rendersize[0]+=rs[0]*parent->rendersize[0];
        rendersize[1]+=rs[1]*parent->rendersize[1];

        //scale (should be last)
        rendersize[0]*=scale[0];
        rendersize[1]*=scale[1];
    }

    {//renderpos
        renderpos=pos+renderposOffset;
        if(ignoreOrigin)
        {
            renderpos+=(parent->renderpos);
        }
        else
        {
            renderpos+=(parent->renderpos+parent->rendersize*parent->origin);//find parent origin from top left corner renderpos (forward)
            renderpos+=(pos-rendersize*origin);//move yourself to origin (backward)
        }

        //relpos
        if(relpos.size()>=1)
        {
            renderpos[0]+=relpos[0]*parent->rendersize[0];
        }
        if(relpos.size()>=2)
        {
            renderpos[1]+=relpos[1]*parent->rendersize[1];
        }
        if(relpos.size()>=3)
        {
            renderpos[0]+=relpos[2]*parent->rendersize[1];
        }
        if(relpos.size()>=4)
        {
            renderpos[1]+=relpos[3]*parent->rendersize[0];
        }

        //rp
        renderpos[0]+=rp[0]*parent->rendersize[0];
        renderpos[1]+=rp[1]*parent->rendersize[1];

        //relposme
        renderpos[0]+=relposme[0]*rendersize[0];
        renderpos[1]+=relposme[1]*rendersize[1];
    }

    if(inherit){//angle
        renderangle=angle;
        renderangle+=parent->renderangle;

        Vec2 rorigin;
        Vec2 prorigin;
        if(ignoreOrigin)
        {
            rorigin=getrenderpos();
            prorigin=parent->getrenderpos();
        }
        else
        {
            rorigin=toorigin(this);
            prorigin=toorigin(parent);
        }

        auto delta=rorigin-prorigin;
        auto len=delta.len();
        auto ang=delta.toangle();
        Vec2 dest=angleToVec2(parent->getrenderangle()+ang).scale(len);

        renderpos=prorigin+dest;
        if(!ignoreOrigin)
        {
            renderpos=fromorigin(this);
        }
    }

    dirty=false;
}

bool Body::getdirty()
{
    return dirty;
}
void Body::setdirty(bool dirty_p)//@bug should this be a bool or just true?
{
    if(dirty_p&&dirty)//if you're already dirty dont sort
    {
        return;
    }

    dirty=dirty_p;
    if(dirty)
    {
        for(Body *b:bodyChildren)
        {
            b->setdirty(true);
        }
    }
}

int Body::getuid()
{
    return uid;
}

void Body::setangle(double angle_p)
{
    angle=angle_p;
    setdirty(true);
}
void Body::setpos(Vec2 const &pos_p)
{
    pos=pos_p;
    setdirty(true);
}
void Body::setsize(Vec2 const &size_p)
{
    size=size_p;
    setdirty(true);
}
void Body::setrelpos(std::vector<double> relpos_p)//@perf
{
    relpos=relpos_p;
    setdirty(true);
}
void Body::setrelsize(std::vector<double> relsize_p)//@perf
{
    relsize=relsize_p;
    setdirty(true);
}
void Body::setscale(Vec2 const &scale_p)
{
    scale=scale_p;
    setdirty(true);
}
void Body::setscale(float scale_p)
{
    scale=Vec2(scale_p,scale_p);
    setdirty(true);
}
void Body::setrelposme(Vec2 const &relposme_p)
{
    relposme=relposme_p;
    setdirty(true);
}
void Body::setorigin(Vec2 const &origin_p)
{
    origin=origin_p;
    setdirty(true);
}
void Body::setrp(Vec2 const &rp_p)
{
    rp=rp_p;
    setdirty(true);
}
void Body::setrs(Vec2 const &rs_p)
{
    rs=rs_p;
    setdirty(true);
}

void Body::setrenderpos(Vec2 const &rp)//do not use
{
    renderpos=rp;
}
void Body::setrendersize(Vec2 const &rs)//do not use
{
    rendersize=rs;
}
void Body::setrenderangle(double renderangle_p)
{
    renderangle=renderangle_p;
}

//all getters return copies, have to be set

double Body::getangle()
{
    return angle;
}
Vec2 Body::getpos()
{
    return pos;
}
Vec2 Body::getsize()
{
    return size;
}
std::vector<double> Body::getrelpos()
{
    return relpos;
}
std::vector<double> Body::getrelsize()
{
    return relsize;
}
Vec2 Body::getscale()
{
    return scale;
}
Vec2 Body::getrelposme()
{
    return relposme;
}
Vec2 Body::getorigin()
{
    return origin;
}
Vec2 Body::getrp()
{
    return rp;
}
Vec2 Body::getrs()
{
    return rs;
}

Vec2 Body::getrenderpos()
{
    return renderpos;
}
Vec2 Body::getrendersize()
{
    return rendersize;
}
double Body::getrenderangle()
{
    return renderangle;
}
//</body>

//<renderer/ing>
//shared between animation and renderer
void drawImageFromBody(Image *img_p,byte renderalpha_p,Body *b_p,SDL_Rect &dest_p,Vec2 &rs)
{
    SDL_SetTextureAlphaMod(img_p->texture->rawTexture, renderalpha_p);

    img_p->dest=dest_p;
    img_p->angle=b_p->getrenderangle();
    img_p->center.x=(b_p->getorigin().x*rs.x);
    img_p->center.y=(b_p->getorigin().y*rs.y);

    img_p->draw();
}
Animation::Animation():imgv(0)
{}
Animation::Animation(std::vector<Image *> *imgv_p,bool loop_p,int FPS_p):loop(loop_p),FPS(FPS_p),imgv(imgv_p),ranOnce(false),index(0),timer(0)
{

}
void Animation::draw(long long deltaMilli,byte renderalpha,Body *b,SDL_Rect &dest,Vec2 &rs)
{
    timer+=deltaMilli;
    //@TODO animate at FPS
    if(imgv&&imgv->size()&&index<imgv->size())
    {
        drawImageFromBody((*imgv)[index],renderalpha,b,dest,rs);
        //if you dont reset index to zero it wont loop, since it checks index is less than size
        if(loop&&index>=imgv->size()-1)
        {
            index=0;
        }
        if(1000/FPS<timer)
        {
            ++index;
            timer=0;
        }
    }
    else
    {
        ranOnce=true;
    }
}

Renderer::Renderer():alwaysdraw(false),dirty(true),maskSurface(0),alpha(255),renderalpha(255),img(0),st(ShapeType::NONE),text(""),shapeScale({1,1}),dest({}),shapeCenter({}),shapeDest({}),color(),shape(NULL)
{}
Renderer::Renderer(Animation anim_p):Renderer()
{
    anim=anim_p;
}
Renderer::Renderer(Image **img_p):Renderer()
{
    img=img_p;
}
//@todo implement text and shape renderer
Renderer::Renderer(std::string text_p, Color color_p, bool fastRender):Renderer()//@note if you want to use text on an entity you have to call the text constructor, then add shapes and images after
{
    text=text_p;
    color=color_p;
    if(!curFontPath)
    {
        std::cout<<"Error: creating text entity without setting SG2D::curFontPath"<<std::endl;
    }
    textImage=Text(curFontPath,color,fastRender);
}
//note: shapes cant be rotated
Renderer::Renderer(ShapeType st_p, Color color_p):Renderer()
{
    st=st_p;
    color=color_p;
}

void Renderer::draw(long long deltaMilli,Body *b,int alphaOffset,Vec2 origin,Vec2 positionOffset)
{
    auto rp=b->getrenderpos()+positionOffset;
    auto rs=b->getrendersize();

    //perfcheck
    auto rprs=rp+rs;
    auto ssize=SG2D::getScreenSize();
    if( (rprs[0]<0 || rprs[1]<0|| rp[1]>ssize[1] || rp[0]>ssize[0]) && !alwaysdraw)
    {
        withinScreen=false;
        return;
    }
    else
    {
        withinScreen=true;
    }

    if(dirty)
    {
        shape=0;//recreates if null
        textImage.dirty=true;
        dirty=false;
    }

    //@todo *jittering problem* find solution for imprecision? this resolves 99.99999 and 100.0000 interchanging when casting to int and 90.5 90.4999999 interchanging when rounding
    rp.x+=.0001;//help it cast it to a reliable integer
    rp.y+=.0001;//apparently cant cast correctly to int when you have an "exact" floating point
    rs.x+=.0001;//??
    rs.y+=.0001;//@bug

    dest.x=round(rp.x);
    dest.y=round(rp.y);
    dest.w=round(rs.x);
    dest.h=round(rs.y);

    renderalpha=alpha+alphaOffset;
    if(alpha+alphaOffset>255)renderalpha=255;//no overflow
    if(alpha+alphaOffset<0)renderalpha=0;
    //@perf if alpha==0 return?

    color[3]=renderalpha;
    if(st!=ShapeType::NONE)//@TODO refactor shape, remove duplicated code
    {
        if(!shape)
        {
            if(st==ShapeType::CIRCLE)
            {
                if(maskSurface)
                {//if masksurface the shape doesnt matter
                    shape=std::make_shared<Texture>(ALLOC_rect(color,maskSurface));
                }
                else
                {
                    shape=std::make_shared<Texture>(ALLOC_circle(dest.w/2,color));
                }
            }
            if(st==ShapeType::RECT)
            {
                shape=std::make_shared<Texture>(ALLOC_rect(color,maskSurface));
            }

            SDL_SetTextureAlphaMod(shape.get()->rawTexture,color[3]);
            SDL_SetTextureBlendMode(shape.get()->rawTexture, SDL_BLENDMODE_BLEND);
        }

        shapeDest=dest;//@perf this could be optimized away
        shapeDest.w*=shapeScale.x;
        shapeDest.h*=shapeScale.y;

        shapeDest.x+=(dest.w-shapeDest.w)/2;
        shapeDest.y+=(dest.h-shapeDest.h)/2;//

        shapeCenter.x=(b->getorigin().x*rs.x);
        shapeCenter.y=(b->getorigin().y*rs.y);
        //NULL for shapeSrc, always draw the whole shape
        SDL_RenderCopyEx( getRenderer(), shape.get()->rawTexture, 0, &shapeDest, b->getrenderangle(), &shapeCenter, SDL_FLIP_NONE );

    }

    if(img)
    {
        drawImageFromBody(*img,renderalpha,b,dest,rs);
    }
    anim.draw(deltaMilli,renderalpha,b,dest,rs);

    if(text.length())
    {
        color[3]=renderalpha;
        textImage.color.a=color[3];

        textImage.settext(text);
        textImage.dest=dest;
        textImage.angle=b->getrenderangle();
        textImage.center.x=(b->getorigin().x*rs.x);
        textImage.center.y=(b->getorigin().y*rs.y);
        textImage.draw();
    }
}

void Renderer::settext(std::string text_p)
{
    if(text_p==text)return;
    text=text_p;
}
std::string Renderer::gettext()
{
    return text;
}

void Renderer::setcolor(Color color_p)
{
    color=color_p;

    shape=0;//will recreate if null
}
Color Renderer::getcolor()
{
    return color;
}
SDL_Rect Renderer::getdest()
{
    return dest;
}
void Renderer::setalpha(byte alpha_p)
{
    alpha=alpha_p;
    if(st!=ShapeType::NONE)
    {
        shape=0;
    }
}
byte Renderer::getalpha()
{
    return alpha;
}

byte Renderer::getrenderalpha()
{
    return renderalpha;
}
//</renderer>

//<entity>
Entity::Entity(){}
Entity::Entity(Renderer r_p,Entity *parent_p,bool listed,bool isPlayer) : Body(parent_p,listed), r(r_p)
{//order of zero would be same order as parent!
    hoverCursor=nullptr;
    visible=true;
    myvisible=true;
    myenabled=true;
    enabled=true;
    solid=true;
    hasCollided=false;
    offsetByCamera=false;
    parent=parent_p;
    relparent=true;
    relchildren=true;
    positionMotion={};
    spin=0;
    resizeFactorDirty=1.1;
    lastResize={};
    defaultPos=PosType::RP;
    uid_e=getuid();

    setdirty(true);

    if(isPlayer)
    {
        SG2D::player=this;
    }

    if(listed)
    {
        SG2D::ent.push_back(this);//these are just references now, everything is done recursively from parents
    }

    if(parent)
    {//push the player here as well but he wont get updated with the rest. he gets updated first. hes ignored in the recursive update
        parent_p->children.push_back(this);
        setorder(parent->children.size());
        SG2D::sortEnts(parent);//@perf sort here? or insert in the right place?
    }
}
Entity::~Entity()
{
    using namespace SG2D;
    if(!parent)return;
    //if(!listed)return;

    //@TODO
    //@BUG

    //the destruction of entities is TERRIBLE and needs to be carefully rewritten.
    //every reference has to be removed from an entity before the end of that entities destructor.
    //the ent array should be removed, it serves no purpose now that there are children
    //if you free a parent without first freeing a child terrible things will happen.
    //msgBox("ent free",entityName.c_str());
    for(int i=0; i<ent.size(); i++)
    {
        if(ent[i]->uid_e==uid_e)continue;

        for(int a=0; a<ent[i]->collider.size(); a++)
        {
            auto &ele=ent[i]->collider[a];
            if(ele->uid_e==uid_e)
            {
                ent[i]->collider.erase(ent[i]->collider.begin()+a);
            }
        }
    }

    auto &ch=getparent()->children;
    for(int a=0; a<ch.size(); a++)
    {
        if(ch[a]->uid_e==uid_e)
        {
            ch.erase(ch.begin()+a);
        }
    }

    for(int i=0; i<ent.size(); i++)
    {
        if(ent[i]->uid_e==uid_e)
        {
            ent.erase(ent.begin()+i);
            break;
        }
    }
}

byte Entity::getalpha()
{
    return r.getalpha();
}

void Entity::setalpha(byte alpha_p)
{
    r.setalpha(alpha_p);
}

Entity *Entity::getparent()
{
    return parent;
}
int Entity::getorder()
{
    return order;
}
int Entity::getorderRecurs()
{
    int recursOrder=getorder();
    Entity *e=this;
    while(e->parent)
    {
        recursOrder+=e->parent->getorder();
        e=e->parent;
    }
    return recursOrder;
}
void Entity::setorder(int order_p)
{//@bug this shouldnt need to setdirty right?
    //order=order_p+(parent?parent->order:0);
    order=order_p;
    SG2D::sortEnts(parent);
}

void Entity::update(int deltaMilli)
{//@todo movement based on deltaMilli
    if(!myenabled)return;

    if(this==SG2D::player)
    {//@perf
        //if you're the player you get updated out of order so you might be dirty but update before your parent
        //could do something like be dirty for two frames instead of just constantly dirty
        setdirty(true);
    }
    startrenderpos=getrenderpos();

    if(onupdate)
    {
        auto valid=onupdate();
        if(!valid)
        {
            return;
        }
    }

    //mover.update(this,deltaMilli); unused
    //if spin or vel is nonzero it will be dirty every frame

    if(spin>.00001)
    {
        auto angleDelta=spin*deltaMilli/1000;
        setangle(angleDelta+getangle());
    }

    Vec2 positionDelta=positionMotion.update(deltaMilli);
    if((std::abs(positionDelta.x)>.000001||std::abs(positionDelta.y)>.000001))
    {
        //msgBox("real delta",(std::to_string(positionDelta.x)+','+std::to_string(positionDelta.x)).c_str());
        positionDelta=(positionDelta*deltaMilli)/1000;
        lastDelta=positionDelta;
        setCurrentPos(getCurrentPos()+positionDelta);
    }
    //update physics component from parents physics component (or zero for no parent)

    Body::update(relparent&&parent->relchildren,{0,0});

    auto myrendersize=getrendersize();
    if(myrendersize.x>lastResize.x*resizeFactorDirty || myrendersize.x<lastResize.x/resizeFactorDirty
        || myrendersize.y>lastResize.y*resizeFactorDirty || myrendersize.y<lastResize.y/resizeFactorDirty)
    {
        r.dirty=true;
        lastResize=getrendersize();
    }

    if(SG2D::enableCamera&&this==SG2D::player)//this has to be done after we know the position?
    {
        SG2D::cameraOffset=-(getCurrentPos()-SG2D::cameraCenter);
    }
    Vec2 renderposOffset={};//renderpos
    if(SG2D::enableCamera&&offsetByCamera)
    {
        renderposOffset=SG2D::cameraOffset;//cant do this with player since ur assigning renderpos to a function of renderpos

        if(SG2D::player->defaultPos==PosType::RP){
            renderposOffset*=SG2D::getScreenSize();
        }

        //@perf this could be optimized, (setCameraOffset)
        setdirty(true);//if camera is enabled everything is going to be dirty all the time basically
    }

    setrenderpos(getrenderpos()+renderposOffset);
}
bool Entity::finalize()//@perf
{
    /*
    whats happening is: first i do the frame,
    then i check for collisions (here in finalize)
    if im colliding then i set myself back two frame positions and then recompute the whole frame again (since the offset will be different). (two updates, so need to set myself back delta*2 to be at zero)

    other solutions:

    factor out the code to compute the renderpos, just do that check at the start of the frame (but will have 1 frame of latency on other objects positions)
    call that at the
    */
    SDL_Rect desta;
    SDL_Rect destb;
    auto renderposme=getrenderpos();
    auto rendersizeme=getrendersize();

    for(auto &ele : collider)
    {
        desta.x=renderposme.x;
        desta.y=renderposme.y;
        desta.w=rendersizeme.x;
        desta.h=rendersizeme.y;

        auto renderposit=ele->getrenderpos();
        auto rendersizeit=ele->getrendersize();
        destb.x=renderposit.x;
        destb.y=renderposit.y;
        destb.w=rendersizeit.x;
        destb.h=rendersizeit.y;

        if(insec(desta,destb))
        {
            setCurrentPos(getCurrentPos()-lastDelta);
            setCurrentPos(getCurrentPos()-lastDelta);
            hasCollided=true;
            return true;
        }
    }
    return false;
}

void Entity::draw(long long deltaMilli)
{
    if(!myvisible)return;

    if(onDrawBegin)
    {
        onDrawBegin();
    }

    int alphaOffset=0;
    if(parent&&relparent&&parent->relchildren)
    {
        alphaOffset=parent->r.getrenderalpha()-255;
    }
    r.draw(deltaMilli,this,alphaOffset,getorigin());

    if(drawCallback)
    {
        drawCallback();
        drawCallback=nullptr;
    }
    entityWasDrawnLastFrame[uid_e]=true;
}
bool Entity::decendedFrom(Entity *e)
{
    Entity *p=parent;
    while(p)
    {
        if(p==e)
        {
            return true;
        }
        p=p->parent;
    }
    return false;
}

Vec2 Entity::getCurrentPos()
{
        if(defaultPos==PosType::RP)
        {
            return getrp();
        }
        if(defaultPos==PosType::POS)
        {
            return getpos();
        }
        return {0,0};
}
void Entity::setCurrentPos(Vec2 v)
{
        if(defaultPos==PosType::RP)
        {
            setrp(v);
        }
        if(defaultPos==PosType::POS)
        {
            setpos(v);
        }
}
//this will break easily. dont use it unless there is nothing else effecting the position (until its rewritten)
/*
void Entity::moveTo(Vec2 endpos, double acceleration, double velConst, std::function<void()> callback)
{
    mover.callback=callback;
    mover.endpos=endpos;
    mover.beginpos=getCurrentPos();
    mover.acceleration=acceleration;
    mover.velConst=velConst;
    vel=0;
    mover.moving=true;
}
*/
//</entity>

namespace
{
    SDL_mutex *SG2D_mutex=SDL_CreateMutex();

    std::deque<std::string> msgs;

    Vec2 toorigin(Body *e)
    {
        return e->getrenderpos()+e->getrendersize()*e->getorigin();
    }
    Vec2 fromorigin(Body *e)
    {
        return e->getrenderpos()-e->getrendersize()*e->getorigin();
    }

    bool killThread=false;//used within SG2D_mutex locks, stops thread loops
    int getmessages(void *ptr)
    {
        using namespace std;
        std::string stream;
        bool running=true;
        while(running)
        {
            stream+=recv(server);

            SDL_LockMutex(SG2D_mutex);
            running=!killThread;
            if(stream.length())
            {
                pair<vector<string>,string> read=separate('\n',stream,true);
                for(string &ele : read.first)
                {
                    msgs.push_back(ele);//does not include newline because of third separate argument
                    //log0("\"MSG RECV: \""+ele);
                }
                stream=read.second;//if it goes beyond end of string substr just returns the rest of the string
            }
            else
            {
                //@todo handle disconnections
                //std::cout<<std::endl<<"Disconnected from server: "<<SDL_GetError()<<std::endl;
                close(server);
                SDL_UnlockMutex(SG2D_mutex);
                return -1;
            }
            SDL_UnlockMutex(SG2D_mutex);
            SDL_Delay(0);
        }
        return 0;
    }
}

namespace SG2D
{
    TCPsocket serverIP=0;
    TCPsocket server=0;
    std::string userName;

    Entity root(Renderer(),0,false);
    World world;

    SDL_Thread *serverThread=0;

    void SG2D_cleanup()
    {
        SDL_LockMutex(SG2D_mutex);
        killThread=true;
        if(server)close(server);
        SDL_UnlockMutex(SG2D_mutex);
        if(serverThread)
        {
            int threadStatus;
            SDL_WaitThread(serverThread,&threadStatus);
        }
    }

    void windowEventHandler(SDL_Event *event)
    {
        if(event->window.event==SDL_WINDOWEVENT_RESIZED)
        {
            if(onResize)
            {
                onResize({(double)event->window.data1,(double)event->window.data2});
            }
            initWH.x=event->window.data1;
            initWH.y=event->window.data2;
            root.setrendersize(initWH);
            for(Body *b:root.bodyChildren)
            {
                b->setdirty(true);
            }
        }
    }
    bool initServer(char const *serverIP)
    {
        server=connect(serverIP);//@todo check timeout
        if(server)
        {
            serverThread=SDL_CreateThread(getmessages,"getmessages",(void*)0);
            return true;
        }
        else
        {
            printf("server error: %s",SDL_GetError());
            return false;
        }
    }
    Vec2 init(char const *title, int width, int height, int flags, char const *userName_p,char const *serverIP)
    {
        log0("SG2D v2.02\n");
        Vec2 wh=::init(title,width,height,flags);
        initWH=wh;
        root.myvisible=false;//must disable both update and draw, root is getting updated called!
        root.myenabled=false;
        root.setorigin({0,0});
        root.setrendersize(wh);
        root.setrenderpos({0,0});

        onWindowEvent=(void*)windowEventHandler;//inside input.cpp

        if(serverIP&&userName_p)
        {
            initServer(serverIP);
            userName=userName_p;
        }
        //else multiplayer is disable (if nullptr is passed to init), server is null
        #ifdef SG2D_DOCLEANUP
        atexit(SG2D_cleanup);
        #endif
        return wh;
    }
    void sortEnts(Entity *parent)
    {
        std::sort(parent->children.begin(), parent->children.end(), [](Entity *a, Entity *b)
        {
            return b->getorder() > a->getorder();
        });
    }
    void updateRecurs(Entity *e,long long deltaMilli)
    {
        if(!e->enabled)return;
        if(e!=SG2D::player)e->update(deltaMilli);//@perf - how expensive is this check?
        for(int i=0; i<e->children.size(); i++)
        {
            updateRecurs(e->children[i],deltaMilli);
        }
    }
    void finalizeRecurs(Entity *e)
    {
        if(!e->enabled)return;
        if(e!=SG2D::player)e->finalize();//@perf - how expensive is this check?
        for(int i=0; i<e->children.size(); i++)
        {
            finalizeRecurs(e->children[i]);
        }
    }
    void drawRecurs(Entity *e,long long deltaMilli)
    {
        if(!e->visible)return;
        e->draw(deltaMilli);
        for(int i=0; i<e->children.size(); i++)
        {
            drawRecurs(e->children[i],deltaMilli);
        }
    }


    World::World()
    {
        last_MB_RIGHT=false;
        last_MB_LEFT=false;
        last_MB_MIDDLE=false;
    }
    void World::update(Clock *gameClock)
    {

        using namespace std;

        bool MB_LEFT=mouse(MB::LEFT);
        bool MB_RIGHT=mouse(MB::RIGHT);
        bool MB_MIDDLE=mouse(MB::MIDDLE);

        Vec2 *mpos=mouse();

        Vec2 relmpos=*mpos/getScreenSize();

        string header="<"+userName+"><"+to_string(gameClock->getNow())+">";

        if(server)//parse multiplayer messages into events
        {

            std::string recvMsg;

            SDL_LockMutex(SG2D_mutex);
            if(msgs.size())
            {
                recvMsg=msgs[0];
                msgs.pop_front();
            }
            SDL_UnlockMutex(SG2D_mutex);
            SDL_Delay(0);

            auto tags=tagsToVector(recvMsg);

            if(tags.size())
            {
                string otherName=tags[0];
                if(otherName.size()&&otherName!=userName)
                {//ignore your own broadcasts
                    bool handled=false;
                    if(tags.size()>=3)//internal messages have at least 3 tags
                    {
                        long long stamp=stoll(tags[1]);
                        string type=tags[2];

                        if(type=="down")
                        {
                            handled=true;

                            int key=tags[3][0];//SDLK or 'w' etc.
                            if(onKeyDown[key])
                            {
                                onKeyDown[key](otherName,stamp);
                            }
                        }
                        if(type=="up")
                        {
                            handled=true;

                            int key=tags[3][0];//SDLK or 'w' etc.
                            if(onKeyUp[key])
                            {
                                onKeyUp[key](otherName,stamp);
                            }
                        }
                        if(type=="click")
                        {
                            handled=true;

                            string &key=tags[3];//"lmb" "rmb" "mmb"
                            if(onMB_click[key])
                            {
                                onMB_click[key](otherName, {std::stod(tags[4]),std::stod(tags[5])}, stamp);
                                handled=true;
                            }
                        }
                        if(type=="release")
                        {
                            handled=true;

                            string &key=tags[3];//"lmb" "rmb" "mmb"
                            if(onMB_release[key])
                            {
                                onMB_release[key](otherName, {std::stod(tags[4]),std::stod(tags[5])}, stamp);
                                handled=true;
                            }
                        }
                    }

                    if(!handled && handleMessage)//THIS DOES NOT NEED AN ANALOG FOR CLIENT SIDE. sent from main
                    {//custom message
                        handleMessage(tags);
                    }
                }
            }
        }

        long long now=gameClock->getNow();

        {//tophover
            Entity *highest=nullptr;
            SDL_Rect mrect={(int)mpos->x,(int)mpos->y,1,1};
            for(Entity *e : ent)
            {
                if(e->visible&&e->solid)//@bug should this include myvisible?
                {
                    //log("mrect",mrect);
                    if(insec(mrect,e->r.getdest()))//@TODO this should take into account visible, myvisible, enabled, myenabled, solid
                    {
                        if( highest==nullptr || (e->getorderRecurs()>highest->getorderRecurs()&&entityWasDrawnLastFrame[e->getuid()]) )
                        {
                            highest=e;
                        }
                    }
                }
            }
            topHover=highest;

            if(topHover&&topHover->onclick)
            {
                SDL_SetCursor(topHover->hoverCursor?topHover->hoverCursor:SG2D::hand);
            }
            else
            {
                SDL_SetCursor(arrow);
            }

            {//handle mouse events
                if(last_MB_LEFT&&!MB_LEFT)
                {//on end click
                    if(onMB_click["lmb"])//if handler lambda exists
                    {
                        onMB_click["lmb"](userName,relmpos,now);
                        if(server)
                        {
                            std::string message=header+"<click>"+"<lmb>"+"<"+std::to_string(relmpos.x)+">"+"<"+std::to_string(relmpos.y)+">";
                            sendq(server,message);
                        }
                    }

                    if(topHover&&topHover->onclick)//if handler lambda exists
                    {
                        topHover->onclick();
                    }
                }
                if(!last_MB_LEFT&&MB_LEFT)
                {//on begin click
                    if(onMB_release["lmb"])//if handler lambda exists
                    {
                        onMB_release["lmb"](userName,relmpos,now);
                        if(server)
                        {
                            std::string message=header+"<release>"+"<lmb>"+"<"+std::to_string(relmpos.x)+">"+"<"+std::to_string(relmpos.y)+">";
                            sendq(server,message);
                        }
                    }

                    //@todo add onmousebut, other handlers
                }

                if(last_MB_MIDDLE&&!MB_MIDDLE)
                {//on end click
                    if(onMB_click["mmb"])//if handler lambda exists
                    {
                        onMB_click["mmb"](userName,relmpos,now);
                        if(server)
                        {
                            std::string message=header+"<click>"+"<mmb>"+"<"+std::to_string(relmpos.x)+">"+"<"+std::to_string(relmpos.y)+">";
                            sendq(server,message);
                        }
                    }

                }
                if(!last_MB_MIDDLE&&MB_MIDDLE)
                {//on begin click
                    if(onMB_release["mmb"])//if handler lambda exists
                    {
                        onMB_release["mmb"](userName,relmpos,now);
                        if(server)
                        {
                            std::string message=header+"<release>"+"<mmb>"+"<"+std::to_string(relmpos.x)+">"+"<"+std::to_string(relmpos.y)+">";
                            sendq(server,message);
                        }
                    }
                }

                if(last_MB_RIGHT&&!MB_RIGHT)
                {//on end click
                    if(onMB_click["rmb"])//if handler lambda exists
                    {
                        onMB_click["rmb"](userName,relmpos,now);
                        if(server)
                        {
                            std::string message=header+"<click>"+"<rmb>"+"<"+std::to_string(relmpos.x)+">"+"<"+std::to_string(relmpos.y)+">";
                            sendq(server,message);
                        }
                    }
                }
                if(!last_MB_RIGHT&&MB_RIGHT)
                {//on begin click
                    if(onMB_release["rmb"])//if handler lambda exists
                    {
                        onMB_release["rmb"](userName,relmpos,now);
                        if(server)
                        {
                            std::string message=header+"<release>"+"<rmb>"+"<"+std::to_string(relmpos.x)+">"+"<"+std::to_string(relmpos.y)+">";
                            sendq(server,message);
                        }
                    }
                }
            }//</handle mouse events>
        }//</tophover

        {//input events
            /*
            if the game is multiplayer (non-null server) you should have a handler for every other player
            key event that the client has a handler for
            */
            for(int i=SDL_lowScancode; i<=SDL_highScancode; i++)//change this loop to handle all keys
            {
                if(lastKey[(char)i]&&!key((char)i))
                {//on key up
                    //std::cout<<(char)i<<std::endl;
                    if(onKeyUp[i])
                    {//IF THERE IS NO HANDLER WILL NOT GET SENT TO SERVER!
                        if(player)
                        {
                            onKeyUp[i](userName,now);
                            if(server)
                            {
                                std::string message=header+"<up>"+"<"+std::string(1,(char)i)+">";
                                sendq(server,message);
                            }
                        }
                        else
                        {
                            log("Error",std::string("player is null but a handler is set."));
                        }


                    }
                }
                if(!lastKey[(char)i]&&key((char)i))
                {//on key down
                    if(onKeyDown[i])
                    {//IF THERE IS NO HANDLER WILL NOT GET SENT TO SERVER!
                        if(player)
                        {

                            onKeyDown[i](userName,now);
                            if(server)
                            {
                                std::string message=header+"<down>"+"<"+std::string(1,(char)i)+">";
                                sendq(server,message);
                            }
                        }
                        else
                        {
                            log("Error",std::string("player is null but a handler is set."));
                        }
                    }
                }
            }
        }

        {//end frame
            last_MB_RIGHT=MB_RIGHT;
            last_MB_LEFT=MB_LEFT;
            last_MB_MIDDLE=MB_MIDDLE;

            for(int i=SDL_lowScancode; i<=SDL_highScancode; i++)//change this loop to handle all keys
            {
                lastKey[(char)i]=key((char)i);
            }
        }//</end frame>
    }

    void frame(Clock *gameClock)
    {
        poll();
        world.update(gameClock);

        if(player)
        {//happens at start of frame, other objects rely on his state (and being consistent in the frame)
        //hes in the children array like all the others that get updated recursively but he is ignored in the updateRecurs function
            player->update(gameClock->deltaMilli);
        }
        updateRecurs(&root,gameClock->deltaMilli);
        if(player)
        {//checks collisions on new positions
            bool recompute=player->finalize();
            if(recompute)//@PERF PERF PERF
            {
                player->update(gameClock->deltaMilli);
                updateRecurs(&root,gameClock->deltaMilli);
            }
        }
        finalizeRecurs(&root);

        clear();
        if(preDraw)
        {
            preDraw();
        }
        entityWasDrawnLastFrame.clear();//entities set this inside their Entity::draw if they are drawn (does not take into account if they were outside the bounds of the screen) used for mouse events
        drawRecurs(&root,gameClock->deltaMilli);
        if(postDraw)
        {
            postDraw();
        }
        flip();

        for(auto &who : payload)//could have multiple servers
        {
            if(!who.second.size())continue;
            std::string buff;
            for(auto &data : who.second)
            {
                buff+=data+"\n";//newline separates event messages
            }
            send(who.first,buff);
            payload[who.first].clear();
        }
    }
}//</SG2D>
