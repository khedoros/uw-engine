#include <SFML/Graphics.hpp>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <iostream>
#include "simple_map.h"
#include "texfile.h"
#include "UwText.h"
#include "critfile.h"

using namespace std;

bool keys[sf::Keyboard::KeyCount];
bool mouse_buttons[sf::Mouse::ButtonCount];
int winW = 512;
int winH = 512;
sf::Vector2i mousePos, mouseDelt, mouseCent(winW/2, winH/2), realCent(winW/2, winH/2);
float theta = 180.0;
float phi   = 0.0;
float thetaR = M_PI;
float phiR = 0.0;
float camXPos = 64.0;
float camYPos = 7.5;
float camZPos = 4.0;
wchar_t textChar;
bool mouseCap = false;
bool inFocus = true;
int anim_framecount = 0;
int anim_div = 0;
int hi_obj_id = 0;

simple_map sm;
texfile walls;
texfile floors;
UwText strings;
critfile cf;
texfile objs;
texfile switches;
texfile tmobj;

int cur_lev = 0;

class sprite_info;

class sprite_info {
    public:
    static bool comp(const sprite_info& a, const sprite_info& b) {
        return a.distance >= b.distance;
    }

    sprite_info() : x(255), y(255), index(65535) {}
    sprite_info(uint8_t xa, uint8_t ya, uint16_t indexa) : x(xa), y(ya), index(indexa) {
        float xloc = 0.0, yloc = 0.0, zloc = 0.0;
        bool is_npc = (index < 256);
        if(is_npc) { //"Mobile objects", aka NPCs
            xloc = float(x) * 2.0 + (2.0 - float(sm.levels[cur_lev].npcs[index].info.xpos) / 3.5);
            yloc = float(y) * 2.0 + (float(sm.levels[cur_lev].npcs[index].info.ypos) / 3.5);
            zloc = float(sm.levels[cur_lev].npcs[index].info.zpos) / 16.0;
        }
        else { //"Static objects", aka items
            xloc = float(x) * 2.0 + (2.0 - float(sm.levels[cur_lev].items[index - 256].xpos) / 3.5);
            yloc = float(y) * 2.0 + (float(sm.levels[cur_lev].items[index - 256].ypos) / 3.5);
            zloc = float(sm.levels[cur_lev].items[index - 256].zpos) / 16.0;
       }
       distance = sqrt(((xloc - camXPos) * (xloc - camXPos)) + ((yloc - camZPos) * (yloc - camZPos)) + ((zloc - camYPos) * (zloc - camYPos)));
    }

    float distance;
    uint8_t x;
    uint8_t y;
    uint16_t index;
};

//Stencil definitions
const int ST_NOTHING = 0; //Nothing rendered there
const int ST_FLOOR = 1;   //Looking at a floor poly
const int ST_WALL = 2;    //Looking at a wall poly
const int ST_CEIL = 3;    //The Ceiling
const int ST_NPC = 4;     //Some NPC Guy
const int ST_ITEM = 5;    //Some item
const int ST_UI = 6;      //Looking at some part of the UI

typedef struct {
    uint8_t st_type; //Uses the stencil definitions
    uint16_t id; //Definition-specific id
    float xpos;
    float ypos;
    float zpos;
} ST_INFO_STRUCT;

//Determine information about what the player is currently pointing at
ST_INFO_STRUCT get_type() {
    ST_INFO_STRUCT a;
    return a;
}

typedef enum {
    TITLE, //Opening developer logos, opening screen
    CUTSCENE, //Any cutscene, including the first
    BEGIN, //Starting screen where you start, load, etc
    CHAR_CREATION, //Character creation screen
    INGAME, //Walking around the Abyss
    CONVERSATION, //In conversation/trade
    MENU //In the option menu
} game_mode;

game_mode gmode = TITLE;

void draw_objs(const std::vector<sprite_info>& info) {
    uint8_t x = 0, y = 0;
    uint16_t first_obj = 0;
    //cout<<"X: "<<camXPos<<" Y: "<<camYPos<<" Z: "<<camZPos<<endl;
    for(sprite_info i: info) {
        //cout<<"Processing object "<<first_obj<<" distance: "<<i.distance<<endl;
        x = i.x;
        y = i.y;
        first_obj = i.index;

        if(first_obj == 0) return;
        float xloc = 0.0, yloc = 0.0, zloc = 0.0, w = 0.0, h = 0.0, hsx = 0.0, hsy = 0.0, scale = 24.0;
        float obj_theta = -1 * theta;
        float obj_phi = -1 * phi;
        float heading = 0;
        uint16_t obj_id = 0;
        bool is_npc = (first_obj < 256);
        sf::Vector2u wh = sf::Vector2u(0,0);
        sf::Texture * tex;
    
        //X and Y are between 0 and 63, but modelspace coords are between 0.0 and 127.0 in the x+y axes
        //In-tile positions are on a 2x2 area, and the in-tile coords have a range 0-7, so we divide by 4 to make them fit in the tile
        //Object z-pos values are 0-127, but modelspace height is 0.0 to 8.0, so divide by 16 to make it fit
        if(is_npc) { //"Mobile objects", aka NPCs
            simple_map::mobile_obj obj = sm.levels[cur_lev].npcs[first_obj];
            obj_id = obj.info.obj_id;
            uint8_t whoami = (obj_id & 0x3f); //sm.levels[cur_lev].npcs[first_obj].info.owner - 1;
            xloc = float(x) * 2.0 + (2.0 - float(obj.info.xpos) / 3.5);
            yloc = float(y) * 2.0 + (float(obj.info.ypos) / 3.5);
            zloc = float(obj.info.zpos) / 16.0;
            heading = obj.info.heading;
            heading *= 45.0;
            
            float cam_heading = atan((yloc - camZPos) / (xloc - camXPos) ) * 180.0 / M_PI; //camera heading to the npc
            if(xloc - camXPos < 0) cam_heading += 180; //tan^-1's output doesn't distinguish which side of the circle the angle's on
            heading = heading - cam_heading; //Find the difference between where the npc is facing and the direction to the camera
            if(heading > 360.0) heading -= 360.0; //Keep the heading within the expected range.
            if(heading < 0.0) heading += 360.0;
            critfile::slots direction = critfile::IDLE_180;
            if((heading >= -22.5 && heading < 22.5) || (heading >= 337.5 && heading < 360.0)) direction = critfile::IDLE_90;
            else if(heading >= 22.5 && heading < 67.5) direction = critfile::IDLE_135;
            else if(heading >= 67.5 && heading < 112.5) direction = critfile::IDLE_180;
            else if(heading >= 112.5 && heading < 157.5) direction = critfile::IDLE_225;
            else if(heading >= 157.5 && heading < 202.5) direction = critfile::IDLE_270;
            else if(heading >= 202.5 && heading < 247.5) direction = critfile::IDLE_315;
            else if(heading >= 247.5 && heading < 292.5) direction = critfile::IDLE_0;
            else direction = critfile::IDLE_45;
            uint8_t frame_count = cf.crits[whoami].frames[direction].size();
            tex = &(cf.crits[whoami].frames[direction][anim_framecount%frame_count].tex);
            hsx = cf.crits[whoami].frames[direction][anim_framecount%frame_count].hotspot_x;
            hsy = cf.crits[whoami].frames[direction][anim_framecount%frame_count].hotspot_y;
            hsx /= scale;
            hsy /= scale;
            wh = tex->getSize();
            w = wh.x;
            h = wh.y;
            w /= scale;
            h /= scale;
        }
        else { //"Static objects", aka items
            simple_map::static_obj obj = sm.levels[cur_lev].items[first_obj - 256];
            obj_id = obj.obj_id;
            heading = obj.heading;
            heading *= 45.0;
            xloc = float(x) * 2.0 + (2.0 - float(obj.xpos) / 4.0);
            yloc = float(y) * 2.0 + (float(obj.ypos) / 4.0);
            zloc = float(obj.zpos) / 16.0;
            if(obj_id >= 232 && obj_id < 256) //replace runestones with the generic image
                tex = &(objs.tex[224]);
            else if(obj_id >= 368 && obj_id < 384) { //replace switches with the right pics from the other texture file
                tex = &(switches.tex[obj_id - 368]);
                obj_phi = 0;
                heading = 360 - heading;
                obj_theta = heading;
                heading = (heading * M_PI) / 180.0;
                xloc -= 0.01*sin(heading);
                yloc -= 0.01*cos(heading);
            }
                    //a lever       a switch          some writing
            else if(obj_id == 353 || obj_id == 354 || obj_id == 358) {
                //FIXME: dunno how the state of the levers/switches are stored.
                //       also dunno how to figure out which strings go to which writing
                if(obj_id == 353) tex = &(tmobj.tex[4]);
                else if(obj_id == 354) tex = &(tmobj.tex[12]);
                else if(obj_id == 358) tex = &(tmobj.tex[20]);
                obj_phi = 0;
                heading = 360 - heading;
                obj_theta = heading;
                heading = (heading * M_PI) / 180.0;
                xloc -= 0.01*sin(heading);
                yloc -= 0.01*cos(heading);
                //cout<<"index: "<<first_obj - 256<<" id: "<<obj_id<<"("<<strings.get_string(3,obj_id)<<") is_quant: "<<obj.quantitied<<" quant: "<<obj.quantity<<" quality: "<<obj.quality<<" owner: "<<obj.owner<<endl;
            }
            else
                tex = &(objs.tex[obj_id]);
            wh = tex->getSize();
            w = wh.x;
            h = wh.y;
            w /= scale;
            h /= scale;
        }

        //Draw the object
        if(obj_id == hi_obj_id) {
            glColor3f(1.0,1.0,0.0);
        } else {
            glColor3f(1.0,1.0,1.0);
        }
        sf::Texture::bind(tex);
        glPushMatrix();
          glTranslatef(xloc,zloc,yloc); //Move object where you want it to show up
          glRotatef(obj_theta, 0.0, 1.0, 0.0); //Rotate around y to face camera
          glRotatef(obj_phi,   1.0, 0.0, 0.0); //Rotate around model's X axis to face camera
          //Define quad in modelspace
          glBegin(GL_QUADS);
            glTexCoord2f(0.0,1.0); glVertex3f(0.0-(w/2.0), 0.0, 0.0);
            glTexCoord2f(1.0,1.0); glVertex3f(0.0+(w/2.0), 0.0, 0.0);
            glTexCoord2f(1.0,0.0); glVertex3f(0.0+(w/2.0), 0.0+h, 0.0);
            glTexCoord2f(0.0,0.0); glVertex3f(0.0-(w/2.0), 0.0+h, 0.0);

            glTexCoord2f(1.0,1.0); glVertex3f(0.0+(w/2.0), 0.0, 0.0);
            glTexCoord2f(0.0,1.0); glVertex3f(0.0-(w/2.0), 0.0, 0.0);
            glTexCoord2f(0.0,0.0); glVertex3f(0.0-(w/2.0), 0.0+h, 0.0);
            glTexCoord2f(1.0,0.0); glVertex3f(0.0+(w/2.0), 0.0+h, 0.0);
          glEnd();
        glPopMatrix();
    }
}

void update_state(sf::RenderWindow &window) {
    //Keyboard control of view
    if(keys[sf::Keyboard::J])
        theta -= 5.0;
    if(keys[sf::Keyboard::L])
        theta += 5.0;
    if(keys[sf::Keyboard::I])
        phi -= 5.0;
    if(keys[sf::Keyboard::K])
        phi += 5.0;
    //Mouse control of view
    if(mouseCap && inFocus && mousePos != mouseCent) { //detect mouse movement
        mouseDelt = mousePos - mouseCent;
        /*
        cout<<"Cent: ("<<mouseCent.x<<", "<<mouseCent.y<<")"<<endl;
        cout<<"Pos : ("<<mousePos.x<<", "<<mousePos.y<<")"<<endl;
        cout<<"Diff: ("<<mouseDelt.x<<", "<<mouseDelt.y<<")"<<endl;
        cout<<"Direction: ";
        */
        sf::Mouse::setPosition(mouseCent,window);
        //while(window.isOpen() && sf::Mouse::getPosition(window) != mouseCent) {}
        
        theta += float(mouseDelt.x) * 0.05;
        phi   += float(mouseDelt.y) * 0.05;
    }

    if(phi > 90.0) phi = 90.0;
    if(phi < -90.0) phi = -90.0;
    if(theta > 360.0) theta -= 360.0;
    if(theta < -360.0) theta += 360.0;
    thetaR = theta * M_PI / 180.0;
    phiR = phi * M_PI / 180.0;
    //Keyboard movement control
    if(keys[sf::Keyboard::A]) {
        camXPos += -1.0 * cos(thetaR) * cos(phiR);
        camYPos += 0;
        camZPos += -1.0 * sin(thetaR) * cos(phiR);
    }
    if(keys[sf::Keyboard::D]) {
        camXPos += cos(thetaR) * cos(phiR);
        camYPos += 0;
        camZPos += sin(thetaR) * cos(phiR);
    }
    if(keys[sf::Keyboard::S]) {
        camXPos += -1.0 * sin(thetaR) * cos(phiR);
        camYPos += sin(phiR);
        camZPos += cos(thetaR) * cos(phiR);
    }
    if(keys[sf::Keyboard::W]) {
        camXPos += sin(thetaR) * cos(phiR);
        camYPos += -1.0 * sin(phiR);
        camZPos += -1.0 * cos(thetaR) * cos(phiR);
    }
    if(keys[sf::Keyboard::Space]) {
        camYPos += 0.5;
    }
    if(keys[sf::Keyboard::LShift]) {
        camYPos -= 0.5;
    }
    if(keys[sf::Keyboard::Num1]) {
        hi_obj_id--;
        keys[sf::Keyboard::Num1] = false;
        if(hi_obj_id < 0) hi_obj_id = 511;
        cout<<"Hilighting obj: "<<hi_obj_id<<" ("<<strings.get_string(3, hi_obj_id)<<")"<<endl;
    }
    if(keys[sf::Keyboard::Num2]) {
       hi_obj_id++;
       keys[sf::Keyboard::Num2] = false;
       if(hi_obj_id >= 512) hi_obj_id = 0;
       cout<<"Hilighting obj: "<<hi_obj_id<<" ("<<strings.get_string(3, hi_obj_id)<<")"<<endl;
    }

    //Switching levels up and down
    if(keys[sf::Keyboard::Equal] && cur_lev < sm.levels.size() - 1) {
        keys[sf::Keyboard::Equal] = false;
        cur_lev++;
    }
    if(keys[sf::Keyboard::Dash]  && cur_lev > 0) {
        keys[sf::Keyboard::Dash] = false;
        cur_lev--;
    }
}

void draw_level_bounds() {
    float plane[4][3] = {{0,0,1}, //plane, normal pointing along the +Y axis
                         {1,0,1},
                         {1,0,0},
                         {0,0,0}};
    //glPushMatrix();
    //glBegin(GL_QUADS);
    for(int i=0;i<6;++i) {
          glPushMatrix();
          switch(i) {
          case 0:
              glColor3f(0.0,0.3,0.0); //floor Y Axis (dark green)
              glTranslatef(0.0,-0.01,0.0);
              glScalef(128.0,0.0,128.0);
              break;
          case 1:
              glColor3f(0.0,1.0,0.0); //ceiling Y Axis (bright green)
              glTranslatef(0.0,32.0,0.0);
              glScalef(128.0,0.0,128.0);
              break;
          case 2:
              glColor3f(0.3,0.0,0.0); //walls -X Axis (dark red)
              glTranslatef(0.0,0.0,0.0);
              glScalef(0.0,32.0,128.0);
              glRotatef(90.0,0.0,0.0,1.0);
              break;
          case 3:
              glColor3f(1.0,0.0,0.0); //walls +X Axis (bright red)
              glTranslatef(128.0,0.0,0.0);
              glScalef(0.0,32.0,128.0);
              glRotatef(90.0,0.0,0.0,1.0);
              break;
          case 4:
              glColor3f(0.0,0.0,0.3); //walls -Z Axis (dark blue)
              glTranslatef(0.0,32.0,0.0);
              glScalef(128.0,32.0,0.0);
              glRotatef(90.0,1.0,0.0,0.0);
              break;
          case 5:
              glColor3f(0.0,0.0,1.0); //walls +Z Axis (bright blue)
              glTranslatef(0.0,32.0,128.0);
              glScalef(128.0,32.0,0.0);
              glRotatef(90.0,1.0,0.0,0.0);
              break;
          }
          glBegin(GL_QUADS);
            glVertex3fv(plane[0]);
            glVertex3fv(plane[1]);
            glVertex3fv(plane[2]);
            glVertex3fv(plane[3]);

            glVertex3fv(plane[0]);
            glVertex3fv(plane[3]);
            glVertex3fv(plane[2]);
            glVertex3fv(plane[1]);
          glEnd();
        glPopMatrix();
    }
    //glEnd();
    //glPopMatrix();
}

void render_3d() {
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity ();

    glRotatef(theta, 0.0, 1.0, 0.0); //Rotate perspective around Y axis
    glRotatef(phi, cos(thetaR), 0.0, sin(thetaR)); //Rotate pespective around camera's X axis

    glTranslatef(-1.0*camXPos, -1.0*camYPos, -1.0*camZPos); //Translate the world to where the camera expects it to be

    //Draw a bounding/orientation box around the level
    draw_level_bounds();

    simple_map::level level = sm.levels[cur_lev];
    uint16_t cti = level.ceil_tex_index;

    //Draw the ceiling
    glColor3f(1.0,1.0,1.0);
    sf::Texture::bind(&(floors.tex[cti]));
    glStencilFunc(GL_ALWAYS, ST_CEIL, -1);
    glBegin(GL_QUADS);
      glTexCoord2f(64.0,64.0); glVertex3f(0.0,8.0,0.0);
      glTexCoord2f(0.0,64.0); glVertex3f(128.0,8.0,0.0);
      glTexCoord2f(0.0,0.0); glVertex3f(128.0,8.0,128.0);
      glTexCoord2f(64.0,0.0); glVertex3f(0.0,8.0,128.0);
    glEnd();

    std::vector<sprite_info> sprite_sorter;
    sprite_sorter.reserve(1024);

    //Render the actual map tiles
    for(int x = 0;x<64;++x) {
        for(int z = 0;z<64;++z) {
            int x_index = 63 - x; //I want x to go in the other direction
            simple_map::map_data tiledat = level.d[x_index][z];
            uint16_t fti = level.floor_tex_index[tiledat.floor_tex];
            uint16_t wti = level.wall_tex_index[tiledat.wall_tex];
            uint16_t cti = level.ceil_tex_index;
            
            bool door = tiledat.is_door;

            //Set type of current tile and its neighbors
            simple_map::tile_type t = tiledat.type;
            simple_map::tile_type nt = (z<63)?       level.d[x_index][z+1].type : simple_map::INVALID;
            simple_map::tile_type st = (z>0)?        level.d[x_index][z-1].type : simple_map::INVALID;
            simple_map::tile_type et = (x_index<63)? level.d[x_index+1][z].type : simple_map::INVALID;
            simple_map::tile_type wt = (x_index>0)?  level.d[x_index-1][z].type : simple_map::INVALID;

            //Find base height of current tile and its neighbors
            float h = float(tiledat.height)/2.0;
            float nh = (z<63       && nt != simple_map::SOLID_TILE)? level.d[x_index][z+1].height/2.0 : 8.0;
            float sh = (z>0        && st != simple_map::SOLID_TILE)? level.d[x_index][z-1].height/2.0 : 8.0;
            float eh = (x_index<63 && et != simple_map::SOLID_TILE)? level.d[x_index+1][z].height/2.0 : 8.0;
            float wh = (x_index>0  && wt != simple_map::SOLID_TILE)? level.d[x_index-1][z].height/2.0 : 8.0;

            //Set floor heights
            float neb=8.0,nwb=neb,seb=neb,swb=neb;
            if(t != simple_map::SOLID_TILE) {
                neb = (t == simple_map::SLOPE_N || t == simple_map::SLOPE_E)?h+0.5:h;
                nwb = (t == simple_map::SLOPE_N || t == simple_map::SLOPE_W)?h+0.5:h;
                seb = (t == simple_map::SLOPE_S || t == simple_map::SLOPE_E)?h+0.5:h;
                swb = (t == simple_map::SLOPE_S || t == simple_map::SLOPE_W)?h+0.5:h;
            }

            //Set wall heights (each corner needs to be split in 2) Example: If I have a tall wall to the north and a short one to the west, the NW corner needs to go as high as both.
            float nnet = 8.0, enet = nnet, 
                  nnwt = nnet, wnwt = nnet,
                  sset = nnet, eset = nnet,
                  sswt = nnet, wswt = nnet;
            if(nt != simple_map::INVALID && nt != simple_map::SOLID_TILE) {
                nnet = (nt == simple_map::SLOPE_S || nt == simple_map::SLOPE_E)?nh+0.5:nh;
                nnwt = (nt == simple_map::SLOPE_S || nt == simple_map::SLOPE_W)?nh+0.5:nh;
            }
            if(st != simple_map::INVALID && st != simple_map::SOLID_TILE) {
                sset = (st == simple_map::SLOPE_N || st == simple_map::SLOPE_E)?sh+0.5:sh;
                sswt = (st == simple_map::SLOPE_N || st == simple_map::SLOPE_W)?sh+0.5:sh;
            }
            if(et != simple_map::INVALID && et != simple_map::SOLID_TILE) {
                enet = (et == simple_map::SLOPE_N || et == simple_map::SLOPE_W)?eh+0.5:eh;
                eset = (et == simple_map::SLOPE_S || et == simple_map::SLOPE_W)?eh+0.5:eh;
            }
            if(wt != simple_map::INVALID && wt != simple_map::SOLID_TILE) {
                wnwt = (wt == simple_map::SLOPE_N || wt == simple_map::SLOPE_E)?wh+0.5:wh;
                wswt = (wt == simple_map::SLOPE_S || wt == simple_map::SLOPE_E)?wh+0.5:wh;
            }

            if(door) {
                glColor3f(1.0,0.0,1.0);
            } else if(t == simple_map::SOLID_TILE) {
                glColor3f(0.0,0.0,0.0);
            } else {
                glColor3f(1.0,1.0,1.0);
            }

            //If no animated floor texture exists, bind the static one. Otherwise, bind the appropriate frame of the animated texture.
            if(floors.animtex[fti].size() == 0)
                sf::Texture::bind(&(floors.tex[fti]));
            else {
                sf::Texture::bind(&(floors.animtex[fti][anim_framecount % floors.animtex[fti].size()]));
                //cout<<"Tex "<<fti<<" using frame "<<(anim_framecount%floors.animtex[fti].size())<<endl;
            }

            //Draw the floor
            glStencilFunc(GL_ALWAYS, ST_FLOOR, -1);
            glBegin(GL_QUADS);
              glTexCoord2f(1.0,1.0); glVertex3f(2*x,neb,2*z+2);
              glTexCoord2f(0.0,1.0); glVertex3f(2*x+2,nwb,2*z+2);
              glTexCoord2f(0.0,0.0); glVertex3f(2*x+2,swb,2*z);
              glTexCoord2f(1.0,0.0); glVertex3f(2*x,seb,2*z);
            glEnd();


            if(t == simple_map::SOLID_TILE) continue;

            //If no animated texture wall exists, bind the static one. Otherwise, bind the appropriate frame of the animated texture.
            if(walls.animtex[wti].size() == 0)
                sf::Texture::bind(&(walls.tex[wti]));
            else
                sf::Texture::bind(&(walls.animtex[wti][anim_framecount % walls.animtex[wti].size()]));

            //Draw the walls
            glStencilFunc(GL_ALWAYS, ST_WALL, -1);
            glBegin(GL_QUADS);
              if(t == simple_map::OPEN_TILE || t == simple_map::SLOPE_N || t == simple_map::SLOPE_S || t == simple_map::SLOPE_E || t == simple_map::SLOPE_W ||
                 t == simple_map::DIAG_SE || t == simple_map::DIAG_SW || t == simple_map::DIAG_NE || t == simple_map::DIAG_NW) {
                  //North wall
                  if(nnet > neb || nnwt > nwb) {
                      glTexCoord2f(1.0,0.0);            glVertex3f(2*x,nnet,2*z+2);
                      glTexCoord2f(0.0,0.0);            glVertex3f(2*x+2,nnwt,2*z+2);
                      glTexCoord2f(0.0,(nnwt-nwb)/2.0); glVertex3f(2*x+2,nwb,2*z+2);
                      glTexCoord2f(1.0,(nnet-neb)/2.0); glVertex3f(2*x,neb,2*z+2);
                  }
                  //South wall
                  if(sset > seb || sswt > swb) {
                      glTexCoord2f(1.0,0.0);            glVertex3f(2*x+2,sswt,2*z);
                      glTexCoord2f(0.0,0.0);            glVertex3f(2*x,sset,2*z);
                      glTexCoord2f(0.0,(sswt-swb)/2.0); glVertex3f(2*x,seb,2*z);
                      glTexCoord2f(1.0,(sset-seb)/2.0); glVertex3f(2*x+2,swb,2*z);
                  }
                  //East wall
                  if(enet > neb || eset > seb) {
                      glTexCoord2f(1.0,0.0);            glVertex3f(2*x,eset,2*z);
                      glTexCoord2f(0.0,0.0);            glVertex3f(2*x,enet,2*z+2);
                      glTexCoord2f(0.0,(enet-neb)/2.0); glVertex3f(2*x,neb,2*z+2);
                      glTexCoord2f(1.0,(eset-seb)/2.0); glVertex3f(2*x,seb,2*z);
                  }
                  //West wall
                  if(wnwt > nwb || wswt > swb) {
                      glTexCoord2f(1.0,0.0);            glVertex3f(2*x+2,wnwt,2*z+2);
                      glTexCoord2f(0.0,0.0);            glVertex3f(2*x+2,wswt,2*z);
                      glTexCoord2f(0.0,(wswt-swb)/2.0); glVertex3f(2*x+2,swb,2*z);
                      glTexCoord2f(1.0,(wnwt-nwb)/2.0); glVertex3f(2*x+2,nwb,2*z+2);
                  }
              }
              if(t == simple_map::DIAG_SE) {

                //Draw SE top cap
                glEnd();
                glColor3f(0.0,0.0,0.0);
                glBegin(GL_TRIANGLES);
                  glVertex3f(2*x+2,8.0,2*z);
                  glVertex3f(2*x,8.0,2*z+2);
                  glVertex3f(2*x+2,8.0,2*z+2);
                glEnd();
                glColor3f(1.0,1.0,1.0);
                glBegin(GL_QUADS);
                

                  glTexCoord2f(1.0,0.0);            glVertex3f(2*x,8.0,2*z+2);
                  glTexCoord2f(0.0,0.0);            glVertex3f(2*x+2,8.0,2*z);
                  glTexCoord2f(0.0,(8.0-h)/2.0); glVertex3f(2*x+2,h,2*z);
                  glTexCoord2f(1.0,(8.0-h)/2.0); glVertex3f(2*x,h,2*z+2);
              }
              else if(t == simple_map::DIAG_NW) {

                //Draw NW top cap
                glEnd();
                glColor3f(0.0,0.0,0.0);
                glBegin(GL_TRIANGLES);
                  glVertex3f(2*x,8.0,2*z+2);
                  glVertex3f(2*x+2,8.0,2*z);
                  glVertex3f(2*x,8.0,2*z);
                glEnd();
                glColor3f(1.0,1.0,1.0);
                glBegin(GL_QUADS);


                  glTexCoord2f(1.0,0.0);            glVertex3f(2*x+2,8.0,2*z);
                  glTexCoord2f(0.0,0.0);            glVertex3f(2*x,8.0,2*z+2);
                  glTexCoord2f(0.0,(8.0-h)/2.0); glVertex3f(2*x,h,2*z+2);
                  glTexCoord2f(1.0,(8.0-h)/2.0); glVertex3f(2*x+2,h,2*z);
              }
              else if(t == simple_map::DIAG_SW) {

                //Draw SW top cap
                glEnd();
                glColor3f(0.0,0.0,0.0);
                glBegin(GL_TRIANGLES);
                  glVertex3f(2*x+2,8.0,2*z+2);
                  glVertex3f(2*x,8.0,2*z);
                  glVertex3f(2*x,8.0,2*z+2);
                glEnd();
                glColor3f(1.0,1.0,1.0);
                glBegin(GL_QUADS);


                  glTexCoord2f(1.0,0.0);            glVertex3f(2*x,8.0,2*z);
                  glTexCoord2f(0.0,0.0);            glVertex3f(2*x+2,8.0,2*z+2);
                  glTexCoord2f(0.0,(8.0-h)/2.0); glVertex3f(2*x+2,h,2*z+2);
                  glTexCoord2f(1.0,(8.0-h)/2.0); glVertex3f(2*x,h,2*z);
              }
              else if(t == simple_map::DIAG_NE) {

                //Draw NE top cap
                glEnd();
                glColor3f(0.0,0.0,0.0);
                glBegin(GL_TRIANGLES);
                  glVertex3f(2*x+2,8.0,2*z+2);
                  glVertex3f(2*x+2,8.0,2*z);
                  glVertex3f(2*x,8.0,2*z);
                glEnd();
                glColor3f(1.0,1.0,1.0);
                glBegin(GL_QUADS);


                  glTexCoord2f(1.0,0.0);            glVertex3f(2*x+2,8.0,2*z+2);
                  glTexCoord2f(0.0,0.0);            glVertex3f(2*x,8.0,2*z);
                  glTexCoord2f(0.0,(8.0-h)/2.0); glVertex3f(2*x,h,2*z);
                  glTexCoord2f(1.0,(8.0-h)/2.0); glVertex3f(2*x+2,h,2*z+2);
              }
              //South wall
              if(st != simple_map::SOLID_TILE && (t == simple_map::DIAG_NW || t == simple_map::DIAG_NE)) {
                  glTexCoord2f(1.0,0.0);            glVertex3f(2*x,8.0,2*z);
                  glTexCoord2f(0.0,0.0);            glVertex3f(2*x+2,8.0,2*z);
                  glTexCoord2f(0.0,(8.0-h)/2.0); glVertex3f(2*x+2,swb,2*z);
                  glTexCoord2f(1.0,(8.0-h)/2.0); glVertex3f(2*x,seb,2*z);
              }
              //North wall
              if(nt != simple_map::SOLID_TILE && (t == simple_map::DIAG_SW || t == simple_map::DIAG_SE)) {
                  glTexCoord2f(0.0,(8.0-h)/2.0); glVertex3f(2*x,neb,2*z+2);
                  glTexCoord2f(1.0,(8.0-h)/2.0); glVertex3f(2*x+2,nwb,2*z+2);
                  glTexCoord2f(1.0,0.0);            glVertex3f(2*x+2,8.0,2*z+2);
                  glTexCoord2f(0.0,0.0);            glVertex3f(2*x,8.0,2*z+2);
              }
              //East wall
              if(et != simple_map::SOLID_TILE && (t == simple_map::DIAG_NW || t == simple_map::DIAG_SW)) {
                  glTexCoord2f(0.0,(8.0-h)/2.0); glVertex3f(2*x,seb,2*z);
                  glTexCoord2f(1.0,(8.0-h)/2.0); glVertex3f(2*x,neb,2*z+2);
                  glTexCoord2f(1.0,0.0);            glVertex3f(2*x,8.0,2*z+2);
                  glTexCoord2f(0.0,0.0);            glVertex3f(2*x,8.0,2*z);
              }
              //West wall
              if(wt != simple_map::SOLID_TILE && (t == simple_map::DIAG_NE || t == simple_map::DIAG_SE)) {
                  glTexCoord2f(0.0,(8.0-h)/2.0); glVertex3f(2*x+2,nwb,2*z+2);
                  glTexCoord2f(1.0,(8.0-h)/2.0); glVertex3f(2*x+2,swb,2*z);
                  glTexCoord2f(1.0,0.0);            glVertex3f(2*x+2,8.0,2*z);
                  glTexCoord2f(0.0,0.0);            glVertex3f(2*x+2,8.0,2*z+2);
              }
            glEnd();
            glStencilFunc(GL_ALWAYS, ST_NOTHING, -1);

            //Draw objects on this tile
            if(tiledat.first_obj != 0) {
                uint16_t index = tiledat.first_obj;
                while(index != 0) {
                    sprite_sorter.push_back(sprite_info(x,z,index));
                    if(index < 256) index = sm.levels[cur_lev].npcs[index].get_next();
                    else            index = sm.levels[cur_lev].items[index - 256].get_next();
                }
            }
        }
    }
    if(sprite_sorter.size() != 0) {
        std::sort(sprite_sorter.begin(), sprite_sorter.end(), sprite_info::comp);
        draw_objs(sprite_sorter);
        sprite_sorter.resize(0);
    }
    sf::Texture::bind(NULL);
}

void render() {
    switch(gmode) {
    case TITLE:
        cout<<"Insert title screen here"<<endl;
        gmode = CUTSCENE;
        break;
    case CUTSCENE:
        cout<<"Insert cutscene playback here"<<endl;
        gmode = BEGIN;
        break;
    case BEGIN:
        cout<<"Insert beginning game screen here"<<endl;
        gmode = INGAME;
        break;
    case CHAR_CREATION:
        cout<<"Insert character creation screen here"<<endl;
        break;
    case INGAME:
        render_3d();
        //cout<<"Insert in-game play here"<<endl;
        break;
    case CONVERSATION:
        cout<<"Insert conversation screen drawing in here"<<endl;
        break;
    case MENU:
        cout<<"Handle the options menu"<<endl;
    }
}

//Handle loading files that the engine already pays attention to
bool load_data(string& fn) {
    string level, wall, floor, pal, strs, crf, obj, apals, sw, tm;
    level = fn+"/data/level13.st";
    wall = fn+"/data/dw64.tr";
    floor = fn+"/data/df32.tr";
    pal = fn+"/data/pals.dat";
    strs = fn+"/data/strings.pak";
    crf = fn+"/crit";
    obj = fn+"/data/objects.gr";
    apals = fn+"/data/allpals.dat";
    sw = fn+"/data/tmflat.gr";
    tm = fn+"/data/tmobj.gr";
    if(!sm.load(level)) {
        level = fn+"/DATA/LEVEL13.ST";
        wall = fn+"/DATA/DW64.TR";
        floor = fn+"/DATA/DF32.TR";
        pal = fn+"/DATA/PALS.DAT";
        strs = fn+"/DATA/STRINGS.PAK";
        crf = fn+"/CRIT";
        obj = fn+"/DATA/OBJECTS.GR";
        apals = fn+"/DATA/ALLPALS.DAT";
        sw = fn+"/DATA/TMFLAT.GR";
        tm = fn+"/DATA/TMOBJ.GR";
        if(!sm.load(level)) {
            level = fn+"/data/lev.ark";
            wall = fn+"/data/w64.tr";
            floor = fn+"/data/f32.tr";
            pal = fn+"/data/pals.dat";
            strs = fn+"/data/strings.pak";
            crf = fn+"/crit";
            obj = fn+"/data/objects.gr";
            apals = fn+"/data/allpals.dat";
            sw = fn+"/data/tmflat.gr";
            tm = fn+"/data/tmobj.gr";
            if(!sm.load(level)) {
                level = fn+"/DATA/LEV.ARK";
                wall = fn+"/DATA/W64.TR";
                floor = fn+"/DATA/F32.TR";
                pal = fn+"/DATA/PALS.DAT";
                strs = fn+"/DATA/STRINGS.PAK";
                crf = fn+"/CRIT";
                obj = fn+"/DATA/OBJECTS.GR";
                apals = fn+"/DATA/ALLPALS.DAT";
                sw = fn+"/DATA/TMFLAT.GR";
                tm = fn+"/DATA/TMOBJ.GR";
                if(!sm.load(level)) {
                    cout<<"Couldn't load any of the level files."<<endl;
                    return false;
                }
            }
        }
    }
    bool retval = walls.load(pal,wall) && floors.load(pal,floor) && strings.load(strs) && cf.load(crf,pal) && objs.load(pal,obj,apals) && switches.load(pal, sw, apals) && tmobj.load(pal,tm,apals);

    //Use the already-loaded textures to generate color-cycled texture animations
    for(size_t i=0;i<walls.tex.size();i++) {
        std::string test = strings.get_string(9,i);
        if(test == "a waterfall" || test == "a drain") {
            //cout<<"Adding "<<51-48+1<<" frames to wall index "<<i<<" \""<<test<<"\"."<<endl;
            walls.populate_animtex(i, 48, 51);
        }
        else if(test == "a lavafall") {
            //cout<<"Adding "<<23-16+1<<" frames to wall index "<<i<<" \""<<test<<"\"."<<endl;
            walls.populate_animtex(i, 16, 23);
        }
    }
    for(size_t i=0;i<floors.tex.size();i++) {
        std::string test = strings.get_string(9,510-i);
        //Some dirt floors have water on the edges, and it doesn't hurt the others.
        //Also, oddly enough, puddles don't hit the right color indices to animate.
        if(test == "water" || test == "a dirt floor") {
            //cout<<"Adding "<<51-48+1<<" frames to floor index "<<i<<" \""<<test<<"\"."<<endl;
            floors.populate_animtex(i, 48, 51);
        }
        else if(test == "lava" || test == "rivulets of lava") {
            //cout<<"Adding "<<23-16+1<<" frames to floor index "<<i<<" \""<<test<<"\"."<<endl;
            floors.populate_animtex(i, 16, 23);
        }
    }
    return retval;
}

void gameloop();

int main(int argc, char *argv[]) {
    for(int i = 0; i < sf::Keyboard::KeyCount; ++i)
        keys[i] = false;
    if(argc==2) {
        string fn(argv[1]);
        bool ret = load_data(fn);
        if(ret) 
            cout<<"Loaded the level and textures successfully."<<endl;
        else
            return 1;
    }
    else {
        cout<<"Provide the directory with your Underworld data."<<endl;
        return 1;
    }
    cout<<"Entering main game loop."<<endl;
    gameloop();
    return 0;
}

void gameloop() {
    sf::RenderWindow window(sf::VideoMode(winW,winH,32), "Ultima Underworld Engine Remake Attempt");
    window.setKeyRepeatEnabled(false);
    window.setFramerateLimit(20);

    glClearColor (0.0, 0.0, 0.0, 0.0);
    glClearDepth (1.0);
    glClearStencil(0);
    glShadeModel (GL_FLAT);
    glViewport (0, 0, (GLsizei) winW, (GLsizei) winH);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, float(winW)/float(winH), 0.1, 200.0);
    glMatrixMode (GL_MODELVIEW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

    bool redraw = true;
    while(window.isOpen()) {
        sf::Clock c;
        sf::Event event;
        while(window.pollEvent(event)) {
            switch(event.type) {
            case sf::Event::Closed:
                window.close();
                return;
            case sf::Event::Resized:
                winW = event.size.width;
                winH = event.size.height;
                mouseCent = sf::Vector2i(winW/2, winH/2);

                glViewport (0, 0, (GLsizei) winW, (GLsizei) winH);
                glMatrixMode (GL_PROJECTION);
                glLoadIdentity ();
                gluPerspective(95.0, float(winW)/float(winH), 0.1, 200.0);
                glMatrixMode (GL_MODELVIEW);
                //cout<<"Resizing not handled yet."<<endl;
                break;
            case sf::Event::LostFocus:
                inFocus = false;
                //cout<<"Losing Focus not handled yet."<<endl;
                break;
            case sf::Event::GainedFocus:
                inFocus = true;
                //cout<<"Gaining Focus not handled yet."<<endl;
                break;
            case sf::Event::TextEntered:
                textChar = event.text.unicode;
                //cout<<char(textChar)<<endl;
                break;
            case sf::Event::KeyPressed: 
                if(event.key.code != -1 && !keys[event.key.code]) {
                    //cout<<"You pressed key code: "<<int(event.key.code)<<endl;
                    if(event.key.code == sf::Keyboard::Q)
                        window.close();
                    else if(event.key.code == sf::Keyboard::Tab && !keys[sf::Keyboard::Tab]) {
                       mouseCap = !mouseCap; 
                       cout<<"Mouse captured: "<<mouseCap<<endl;
                       window.setMouseCursorVisible(!mouseCap);
                    }
                    keys[event.key.code] = true;
                }
                break;
            case sf::Event::KeyReleased:
                if(event.key.code != -1) {
                    //cout<<"You released key code: "<<int(event.key.code)<<endl;
                    keys[event.key.code] = false;
                }
                break;
            case sf::Event::MouseWheelMoved:
                //cout<<"Mousewheel doesn't do anything."<<endl;
                break;
            case sf::Event::MouseButtonPressed:
                cout<<"Pressed mouse button: "<<int(event.mouseButton.button)<<endl;
                mouse_buttons[event.mouseButton.button] = true;
                break;
            case sf::Event::MouseButtonReleased:
                cout<<"You released mouse button: "<<int(event.mouseButton.button)<<endl;
                mouse_buttons[event.mouseButton.button] = false;
                break;
            case sf::Event::MouseEntered:
                //cout<<"Mouse entered window"<<endl;
                break;
            case sf::Event::MouseLeft:
                //cout<<"Mouse left window."<<endl;
                break;
            case sf::Event::MouseMoved:
                //cout<<"Mouse move detected."<<endl;
                mousePos = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
                break;
            case sf::Event::JoystickButtonPressed:
            case sf::Event::JoystickButtonReleased:
            case sf::Event::JoystickMoved:
            case sf::Event::JoystickConnected:
            case sf::Event::JoystickDisconnected:
                cout<<"Joystick support not implemented."<<endl;
                break;
            default: cout<<"Unhandled event ("<<int(event.type)<<")"<<endl;
            }
        }
        update_state(window);
        window.clear();
        render();
        if(anim_div == 3) {
            ++anim_framecount;
            anim_div = 0;
        }
        ++anim_div;
        if(anim_framecount >= 8) anim_framecount = 0;
        window.display();

        
        sf::Time t; //Pause for the rest of 1/60th of a second
        if(sf::milliseconds(1000.0/20.0) - c.getElapsedTime() > sf::milliseconds(5.0))
            sf::sleep(sf::milliseconds(1000.0/20.0) - c.getElapsedTime());
        c.restart();
       
    }
}
