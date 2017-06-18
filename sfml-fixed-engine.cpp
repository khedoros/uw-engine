#include <SFML/Graphics.hpp>
#include<SFML/OpenGL.hpp>
//#include <GL/gl.h>
#include <GL/glu.h>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <iostream>
#include "simple_map.h"
#include "texfile.h"
#include "UwText.h"
#include "critfile.h"
#include "uw_model.h"

using namespace std;

bool keys[sf::Keyboard::KeyCount];
bool mouse_buttons[sf::Mouse::ButtonCount];
int winW = 800;
int winH = 600;
sf::Vector2i mousePos, mouseDelt, mouseCent(winW/2, winH/2), realCent(winW/2, winH/2);
float theta = 180.0;
float phi   = 0.0;
float thetaR = M_PI;
float phiR = 0.0;
float camXPos = 64.0;
float camYPos = 7.5;
float camZPos = 4.0;

#ifndef DEVBUILD
float old_camXPos = 64.0;
float old_camYPos = 7.5;
float old_camZPos = 4.0;
#endif

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
texfile doors;

int cur_lev = 0;

#ifdef DEVBUILD
std::vector<float> tris(0); //Triangular "top" caps
#endif
std::vector<std::vector<float>> quads(512);
std::vector<std::vector<float>> tex(512);

std::vector<uw_model> model;
std::vector<std::vector<float>> mod_vertex;
std::vector<std::vector<float>> mod_texmap;
std::vector<std::vector<float>> mod_color;

/*  Useful for some debug things, but I don't want them generally active
int modnum = 1;
int moddir = 0;
int ptcnt = 1000;
*/

bool map_needs_update = true;

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
            xloc = float(x) * 2.0 + (2.0 - float(sm.levels[cur_lev].npcs[index].info.xpos) / 4.0);
            yloc = float(y) * 2.0 + (float(sm.levels[cur_lev].npcs[index].info.ypos) / 4.0);
            zloc = float(sm.levels[cur_lev].npcs[index].info.zpos) / 16.0;
        }
        else { //"Static objects", aka items
            xloc = float(x) * 2.0 + (2.0 - float(sm.levels[cur_lev].items[index - 256].xpos) / 4.0);
            yloc = float(y) * 2.0 + (float(sm.levels[cur_lev].items[index - 256].ypos) / 4.0);
            zloc = float(sm.levels[cur_lev].items[index - 256].zpos) / 16.0;
       }
       distance = sqrt(((xloc - camXPos) * (xloc - camXPos)) + ((yloc - camZPos) * (yloc - camZPos)) + ((zloc - camYPos) * (zloc - camYPos)));
    }

    float distance;
    uint8_t x;
    uint8_t y;
    uint16_t index;
};

typedef enum {
    TITLE, //Opening developer logos, opening screen
    CUTSCENE, //Any cutscene, including the first
    BEGIN, //Starting screen where you start, load, etc
    CHAR_CREATION, //Character creation screen
    INGAME, //Walking around the Abyss
    CONVERSATION, //In conversation/trade
    MENU //In the option menu
} game_mode;

bool info_out = true;
game_mode gmode = TITLE;

void draw_model(float xloc, float yloc, float zloc, float heading, int model_num, simple_map::static_obj& obj) {
    bool adjust_tmap = false;

    if(mod_vertex[model_num].size() < 3) {
	std::cerr<<"Model #"<<model_num<<" doesn't have enough vertices."<<std::endl;
        return;
    }

    if(obj.obj_id >= 0x140 && obj.obj_id < 0x150) { //Door frames
        int x = 63 - (xloc / 2 - 1);
        int y = yloc / 2;
        int wti = sm.levels[cur_lev].d[x][y].wall_tex;

        sf::Texture::bind(&(walls.tex[sm.levels[cur_lev].wall_tex_index[wti]]));
    }
    else if(obj.obj_id == 0x160) { //Pillars
        sf::Texture::bind(&(tmobj.tex[obj.flags]));
    }
    else if(obj.obj_id == 0x161) { //A lever
        adjust_tmap = true;
        sf::Texture::bind(&(tmobj.tex[(obj.flags & 7) + 4]));
    }
    else if(obj.obj_id == 0x162) { //A switch
        adjust_tmap = true;
        sf::Texture::bind(&(tmobj.tex[(obj.flags & 7) + 12]));
    }
    else if(obj.obj_id == 0x164) { //Bridges
        if(obj.flags < 2) {
            sf::Texture::bind(&(tmobj.tex[(obj.flags) + 30]));
        }
        else if(obj.flags >= 2) {
            uint16_t fti = sm.levels[cur_lev].floor_tex_index[obj.flags - 2];
            sf::Texture::bind(&(floors.tex[fti]));
        }
    }
    else if(obj.obj_id == 0x165) { //Gravestone
        sf::Texture::bind(&(tmobj.tex[obj.flags + 28]));
    }
    else if(obj.obj_id == 0x166) { //Some writing
        adjust_tmap = true;
        sf::Texture::bind(&(tmobj.tex[(obj.flags & 7) + 20]));
    }
    else if(obj.obj_id == 0x16e) { //special tmap
        adjust_tmap = true;
        sf::Texture::bind(&(walls.tex[sm.levels[cur_lev].wall_tex_index[obj.owner]]));
    }
    else {
        sf::Texture::bind(NULL);
    }

    //TODO: Remove DEV
    sf::Texture::bind(NULL);
    glColor3f(1.0,0.5,0.0); //walls +Z Axis (bright blue)

    heading += 180.0f;
    if(heading >= 360.0f)
        heading -= 360.0f;

    glPushMatrix();

    if(adjust_tmap) {
        float trig_heading = (heading * M_PI) / 180.0;
        float x_inc = -0.01*sin(heading);
        float z_inc = -0.01*cos(heading);
        //glRotatef(180,0,1,0);
        glTranslatef(x_inc, 0.0, z_inc);
    }

    /*
    {
        int h_index = (int(heading / 45) + anim_framecount) % 8;
        heading = h_index * 45;
        GLfloat colors[] = { 1.0, 0.0, 0.0, //red
                             1.0, 0.25,0.0, //red-orange
                             1.0, 0.5, 0.0, //orange
                             1.0, 1.0, 0.0, //yellow
                             0.5,1.0, 0.0, //yellow-green
                             0.0,1.0,0.0,  //green
                             0.0,0.0,1.0,  //blue
                             0.75,0.0,0.75}; //purple
        glColor3fv(&colors[h_index*3]);
        sf::Texture::bind(NULL);
    }
    */

    //Draw the model!
    glTranslatef(xloc, zloc, yloc);
    glRotatef(heading, 0.0, 1.0, 0.0); //Rotate around y to face appropriate heading
    glScalef(2.0,2.0,2.0);             //My coordinates are 2x the ones of the original game
    assert(mod_vertex[model_num].size() % 9 == 0);
    assert(mod_vertex[model_num].size() == mod_color[model_num].size());

    //if(model_num == 0x0a) {
        //assert(obj.obj_id == 320 + 0x20);
        //std::cout<<"Draw "<<mod_vertex[model_num].size()/3<<"vertices at ("<<xloc<<", "<<yloc<<", "<<zloc<<")"<<std::endl;
    //}

    glVertexPointer(3, GL_FLOAT, 0, &mod_vertex[model_num][0]);
    glEnableClientState(GL_VERTEX_ARRAY);
    //glTexCoordPointer(2, GL_FLOAT, 0, &mod_texmap[model_num][0]);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    //glColorPointer(3, GL_FLOAT, 0, &mod_color[model_num][0]);
    //glEnableClientState(GL_COLOR_ARRAY);

    assert(mod_color.size() == mod_vertex.size());
    //int verts_to_draw = (mod_vertex[model_num].size() / 3 > ptcnt) ? ptcnt : mod_vertex[model_num].size() / 3;
    int verts_to_draw = mod_vertex[model_num].size() / 3;
    glDrawArrays(GL_TRIANGLES, 0, verts_to_draw); //3 = number of coordinates per vertex
    //for(int vert = 1; vert <= verts_to_draw; ++vert) {
    //    float color = 1.0 - float(vert % 32) / 32.0f;
    //    glColor3f(color,color,color);
    //    glDrawArrays(GL_POINTS, (vert-1)*3, verts_to_draw); //3 = number of coordinates per vertex
    //}

    //for(int faces = 1; faces <= verts_to_draw / 3; ++faces) {
    //    float color = float(faces-1) / float(mod_vertex[model_num].size() / 9);
    //    glColor3f(color,color,color);
    //    glDrawArrays(GL_TRIANGLES, 0, faces * 3); //3 = number of coordinates per vertex
    //}

    //glDisableClientState(GL_COLOR_ARRAY);
    //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glColor3f(0.0,0.0,1.0); //walls +Z Axis (bright blue)
    glBegin(GL_LINES);
      glVertex3f(model[model_num].cent_x, 0.0,model[model_num].cent_y);
      glVertex3f(model[model_num].cent_x,40.0,model[model_num].cent_y);
    glEnd();

    glColor3f(1.0,0.0,0.0); //walls +Z Axis (bright blue)
    glBegin(GL_LINES);
      glVertex3f(model[model_num].cent_x1, 0.0,model[model_num].cent_y1);
      glVertex3f(model[model_num].cent_x1,40.0,model[model_num].cent_y1);
    glEnd();

    glColor3f(1.0,1.0,1.0); //walls +Z Axis (bright blue)
    glBegin(GL_LINES);
      glVertex3f(0.0, 0.0,0.0);
      glVertex3f(0.0,40.0,0.0);
    glEnd();

    glPopMatrix();
}


void draw_objs(const std::vector<sprite_info>& info) {
    uint8_t x = 0, y = 0;
    uint16_t first_obj = 0;
    //cout<<"X: "<<camXPos<<" Y: "<<camYPos<<" Z: "<<camZPos<<endl;
    for(sprite_info i: info) {
        //cout<<"Processing object "<<first_obj<<" distance: "<<i.distance<<endl;
        x = i.x;
        y = i.y;
        first_obj = i.index;

#ifndef DEVBUILD
        if(i.distance >= 10) continue;
#endif

        if(first_obj == 0) return;
        float xloc = 0.0, yloc = 0.0, zloc = 0.0, w = 0.0, h = 0.0, hsx = 0.0, hsy = 0.0, scale = 32.0;
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
            xloc = float(x) * 2.0 + (2.0 - float(obj.info.xpos) / 4.0);
            yloc = float(y) * 2.0 + (float(obj.info.ypos) / 4.0);
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
            heading = 180 - heading;
            xloc = float(x) * 2.0 + (2.0 - float(obj.xpos) / 4.0) - 1.0/8.0;
            yloc = float(y) * 2.0 + (float(obj.ypos) / 4.0) + 1.0/8.0;
            zloc = float(obj.zpos) / 16.0;

            if(obj_id >= 0x140 && obj_id < 0x150) {
                xloc-=2.0/8.0;
                yloc+=2.0/8.0;
            }
            else if(obj_id == 0x164) {
                xloc-=2.0/8.0;
            }
            if(obj_id >= 232 && obj_id < 256) { //replace runestones with the generic image
                tex = &(objs.tex[224]);
            }
            else if(obj_id >= 320 && obj_id < 368) { //3d objects
                std::array<int, 48> model_num = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                                  0x03, 0x08, 0x08, 0x07, 0x07, 0x06, 0x05, 0x0b, 0x18, 0x09, 0x17, 0x1b, 0x1c, 0x19, 0x1a, 0x04,
                                                  0x0a, 0x10, 0x11, 0x0d, 0x02, 0x13, 0x12, 0x1d, 0x1e, 0x1f, 0x07, 0x07, 0x07, 0x07, 0x16, 0x07 };
                draw_model(xloc, yloc, zloc, heading, model_num[obj_id - 320], obj);

                continue;
            }
            else if(obj_id >= 368 && obj_id < 384) { //replace switches with the right pics from the other texture file
                tex = &(switches.tex[obj_id - 368]);
                obj_phi = 0;
                obj_theta = heading;
                heading = (heading * M_PI) / 180.0;
                xloc += 0.01*sin(heading);
                yloc += 0.01*cos(heading);
            }
            else {
                tex = &(objs.tex[obj_id]);
            }
            wh = tex->getSize();
            w = wh.x;
            h = wh.y;
            w /= scale;
            h /= scale;
        }

        //Draw the object
        glPushMatrix();
          if(obj_id == hi_obj_id) {
              glColor3f(1.0,1.0,0.0);
          } else {
              glColor3f(1.0,1.0,1.0);
          }

          glTranslatef(xloc,zloc,yloc); //Move object where you want it to show up
          glRotatef(obj_theta, 0.0, 1.0, 0.0); //Rotate around y to face camera
          glRotatef(obj_phi,   1.0, 0.0, 0.0); //Rotate around model's X axis to face camera
          //Define quad in modelspace
          const float tex_coords[] = {
              0.0, 1.0,
              1.0, 1.0,
              1.0, 0,0,
              0.0, 0.0,
          };
          const float vert_coords[] = {
              0.0f-(w/2.0f), 0.0f,   0.0f,
              0.0f+(w/2.0f), 0.0f,   0.0f,
              0.0f+(w/2.0f), 0.0f+h, 0.0f,
              0.0f-(w/2.0f), 0.0f+h, 0.0f,
          };

          sf::Texture::bind(tex);

          glVertexPointer(3, GL_FLOAT, 0, vert_coords);
          glEnableClientState(GL_VERTEX_ARRAY);

          glTexCoordPointer(2, GL_FLOAT, 0, tex_coords);
          glEnableClientState(GL_TEXTURE_COORD_ARRAY);

          glDrawArrays(GL_QUADS, 0, 4);

          glDisableClientState(GL_TEXTURE_COORD_ARRAY);
          glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
    }

}

float slope_mod(float sub_x, float sub_z) {
    int tile_x = 63 - (sub_x / 2 - 1);
    int tile_z = sub_z / 2;
    auto type = sm.levels[cur_lev].d[tile_x][tile_z].type;
    float slopemod = 0.0;
    sub_x = sub_x - 2.0 * float(63 - tile_x);
    sub_z = sub_z - 2.0 * float(tile_z);
    //cout<<"subx: "<<sub_x<<", subz: "<<sub_z<<endl;
    if(type == simple_map::SLOPE_N) {
        slopemod = sub_z / 4.0;
    }
    else if(type == simple_map::SLOPE_S) {
        slopemod = 0.5 - (sub_z / 4.0);
    }
    //else if(type == simple_map::SLOPE_E) {
    //    slopemod = 0.5 - (sub_x / 2.0);
    //}
    //else if(type == simple_map::SLOPE_W) {
    //    slopemod = sub_x / 2.0;
    //}
    if(slopemod != 0.0) {
        cout<<tile_z<<"\t"<<sub_z<<endl;
    }
    return slopemod;
}

void constrain_movement(float new_xpos, float new_ypos, float new_zpos) {
#ifdef DEVBUILD
    camXPos = new_xpos;
    camYPos = new_ypos;
    camZPos = new_zpos;
    cout<<"("<<camXPos<<", "<<camYPos<<", "<<camZPos<<")"<<endl;
#else

    //TODO: 
    // - Adjust heights for slope
    // - Work out bridges
    // - Work out open vs closed doors
    // - For bridges+doors, maybe add something to simple_map to query if they exist for a block, find their state, etc


    //Calculate which tiles I'm in, for comparison
    int cur_tile_x = 63 - (camXPos / 2 - 1);
    int cur_tile_z = camZPos / 2;
    int new_tile_x = 63 - (new_xpos / 2 - 1);
    int new_tile_z = new_zpos / 2;

    simple_map::map_data tiledat = sm.levels[cur_lev].d[cur_tile_x][cur_tile_z];
    simple_map::map_data new_tiledat = sm.levels[cur_lev].d[new_tile_x][new_tile_z];

    //If not staying in the current tile...
    if(cur_tile_x != new_tile_x || cur_tile_z != new_tile_z) {
        //Make the data more easily-visible

        //Don't move onto solid/invalid tiles, unless you're already on one (somehow)
        if((new_tiledat.type == simple_map::SOLID_TILE || new_tiledat.type == simple_map::INVALID) 
                && tiledat.type != simple_map::SOLID_TILE 
                && tiledat.type != simple_map::INVALID) {
            return;
        }

        //Can't go up things that are too high
        if(new_tiledat.height > tiledat.height + 1) {
            return;
        }
    }

    if(new_tiledat.type == simple_map::DIAG_SE) {

    }
    else if(new_tiledat.type == simple_map::DIAG_SW) {

    }
    else if(new_tiledat.type == simple_map::DIAG_NE) {

    }
    else if(new_tiledat.type == simple_map::DIAG_NW) {

    }

    //cout<<"New tile: ("<<new_tile_x<<", "<<new_tile_z<<") height "<<new_tiledat.height<<", old tile: ("<<cur_tile_x<<", "<<cur_tile_z<<") height "<<tiledat.height<<endl;
    //Apply the decisions that were made, regarding the camera
    camXPos = (new_xpos + 4.0 * camXPos) / 5.0;
    camZPos = (new_zpos + 4.0 * camZPos) / 5.0;
    camYPos = new_tiledat.height / 2.0 + slope_mod(camXPos, camZPos) + 1.5;
    float base_y = new_tiledat.height / 2.0 + 1.5;
    if(camYPos != base_y) {
        cout<<"("<<camXPos<<", "<<camYPos<<" ("<<base_y<<"), "<<camZPos<<")"<<endl;
    }
#endif
}

void constrain_angles(float new_theta, float new_phi) {
#ifdef DEVBUILD    
    if(new_phi > 90.0) phi = 90.0;
    else if(new_phi < -90.0) phi = -90.0;
    else phi = new_phi;
#else
    if(new_phi > 30.0) phi = 30.0;
    else if(new_phi < -30.0) phi = -30.0;
    else phi = new_phi;
#endif

    if(new_theta > 360.0) theta -= 360.0;
    else if(new_theta < -360.0) theta += 360.0;
    else theta = new_theta;
}

void update_state(sf::RenderWindow &window) {

    float prop_theta = theta, prop_phi = phi, prop_camXPos = camXPos, prop_camYPos = camYPos, prop_camZPos = camZPos;

    //Keyboard control of view
    if(keys[sf::Keyboard::J])
        prop_theta = theta - 5.0;
    if(keys[sf::Keyboard::L])
        prop_theta = theta + 5.0;
    if(keys[sf::Keyboard::I])
        prop_phi = phi - 5.0;
    if(keys[sf::Keyboard::K])
        prop_phi = phi + 5.0;
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
        
        prop_theta += float(mouseDelt.x) * 0.05;
        prop_phi   += float(mouseDelt.y) * 0.05;
    }

    /*   Debug-related code (useful for playing with model rendering)
    if(keys[sf::Keyboard::LBracket]) {
        keys[sf::Keyboard::LBracket] = false;
        moddir = -1;
        ptcnt--;
    }
    else if(keys[sf::Keyboard::RBracket]) {
        keys[sf::Keyboard::RBracket] = false;
        moddir = 1;
        ptcnt++;
    }
    modnum += moddir;

    while(moddir) {
        if(modnum < 0) modnum = 31;
        if(modnum > 31) modnum = 0;
        if(mod_vertex[modnum].size() > 3) {
            moddir = 0;
        }
        else {
            modnum += moddir;
        }
    }

    if(ptcnt == 0) ptcnt = 1;
    if(ptcnt == 501) ptcnt = 500;
    */    //End of debug-related code

    constrain_angles(prop_theta, prop_phi);

    thetaR = theta * M_PI / 180.0;
    phiR = phi * M_PI / 180.0;
    //Keyboard movement control
    if(keys[sf::Keyboard::A]) {
        prop_camXPos += -1.0 * cos(thetaR) * cos(phiR);
        prop_camYPos += 0;
        prop_camZPos += -1.0 * sin(thetaR) * cos(phiR);
    }
    if(keys[sf::Keyboard::D]) {
        prop_camXPos += cos(thetaR) * cos(phiR);
        prop_camYPos += 0;
        prop_camZPos += sin(thetaR) * cos(phiR);
    }
    if(keys[sf::Keyboard::S]) {
        prop_camXPos += -1.0 * sin(thetaR) * cos(phiR);
        prop_camYPos += sin(phiR);
        prop_camZPos += cos(thetaR) * cos(phiR);
    }
    if(keys[sf::Keyboard::W]) {
        prop_camXPos += sin(thetaR) * cos(phiR);
        prop_camYPos += -1.0 * sin(phiR);
        prop_camZPos += -1.0 * cos(thetaR) * cos(phiR);
    }
    if(keys[sf::Keyboard::Space]) {
        prop_camYPos += 0.5;
    }
    if(keys[sf::Keyboard::LShift]) {
        prop_camYPos -= 0.5;
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
        map_needs_update = true;
    }
    if(keys[sf::Keyboard::Dash]  && cur_lev > 0) {
        keys[sf::Keyboard::Dash] = false;
        cur_lev--;
        map_needs_update = true;
    }

    constrain_movement(prop_camXPos, prop_camYPos, prop_camZPos);

    #ifndef DEVBUILD
    float diffx = camXPos - old_camXPos;
    float diffy = camYPos - old_camYPos;
    float diffz = camZPos - old_camZPos;
    if(diffx*diffx + diffy*diffy + diffz*diffz > 9) {
        map_needs_update = true;
        old_camXPos = camXPos;
        old_camYPos = camYPos;
        old_camZPos = camZPos;
    }
    #endif
}

void draw_level_bounds() {
    float plane[4][3] = {{0,0,1}, //plane, normal pointing along the +Y axis
                         {1,0,1},
                         {1,0,0},
                         {0,0,0}};
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

    glColor3f(0.4,0.4,0.4); // object alignment grids
    for(float x = 0; x < 128.0; x+=0.25) {
        if(int(x*4)%8 == 0)
            glLineWidth(4);
        else 
            glLineWidth(1);
        glBegin(GL_LINES);
          glVertex3f(x,8.01,0.0);
          glVertex3f(x,8.01,128.0);
          glVertex3f(0.0,8.01,x);
          glVertex3f(128.0,8.01,x);
        glEnd();
    }
}

void push_vert(std::vector<float>& t, std::vector<float>& vert, float u, float v, float x, float y, float z) {
    t.push_back(u); t.push_back(v);
    vert.push_back(x); vert.push_back(y); vert.push_back(z);
}

void update_level_map() {
    if(!map_needs_update) {
        return;
    }

#ifdef DEVBUILD
    tris.resize(0);
#endif
    for(int texnum=0;texnum < 512; texnum++) {
        quads[texnum].resize(0);
        tex[texnum].resize(0);
    }

    //Build geometry for the actual map tiles
    for(int x = 0;x<64;++x) {
        for(int z = 0;z<64;++z) {
            int x_index = 63 - x; //I want x to go in the other direction

#ifndef DEVBUILD
            float xdiff = x * 2 - camXPos;
            float zdiff = z * 2 - camZPos;

            if(xdiff*xdiff + zdiff*zdiff >= 100) continue;
#endif

            simple_map::level level = sm.levels[cur_lev];
            simple_map::map_data tiledat = level.d[x_index][z];
            uint16_t fti = level.floor_tex_index[tiledat.floor_tex];
            uint16_t wti = level.wall_tex_index[tiledat.wall_tex];
            uint16_t cti = level.ceil_tex_index;
            
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

            if(t == simple_map::SOLID_TILE) {
                glColor3f(0.0,0.0,0.0);
            } else {
                glColor3f(1.0,1.0,1.0);
            }

            int index = 0;
            if(t == simple_map::SOLID_TILE) {
                //sf::Texture::bind(NULL);
                index = floors.tex.size();
            }
            //else if(floors.animtex[fti].size() == 0)
            //    sf::Texture::bind(&(floors.tex[fti]));
            else {
                //sf::Texture::bind(&(floors.animtex[fti][anim_framecount % floors.animtex[fti].size()]));
                //cout<<"Tex "<<fti<<" using frame "<<(anim_framecount%floors.animtex[fti].size())<<endl;
                index = fti;
            }

#ifndef DEVBUILD
            if(t == simple_map::SOLID_TILE) continue;
#endif

            //Draw the floor
            push_vert(tex[index], quads[index], 1.0, 1.0, 2.0f*x,      neb, 2.0f*z+2.0f);
            push_vert(tex[index], quads[index], 0.0, 1.0, 2.0f*x+2.0f, nwb, 2.0f*z+2.0f);
            push_vert(tex[index], quads[index], 0.0, 0.0, 2.0f*x+2.0f, swb, 2.0f*z     );
            push_vert(tex[index], quads[index], 1.0, 0.0, 2.0f*x,      seb, 2.0f*z     );

            //If no animated texture wall exists, bind the static one. Otherwise, bind the appropriate frame of the animated texture.
            //if(walls.animtex[wti].size() == 0)
            //    sf::Texture::bind(&(walls.tex[wti]));
            //else
            //    sf::Texture::bind(&(walls.animtex[wti][anim_framecount % walls.animtex[wti].size()]));

            //Indexes 0-255 are for the floors, 256-511 are for the walls.
            index = wti + 256;

            //Draw the walls
              if(t == simple_map::OPEN_TILE || t == simple_map::SLOPE_N || t == simple_map::SLOPE_S || t == simple_map::SLOPE_E || t == simple_map::SLOPE_W ||
                 t == simple_map::DIAG_SE || t == simple_map::DIAG_SW || t == simple_map::DIAG_NE || t == simple_map::DIAG_NW) {

                //North wall
                if(nnet > neb || nnwt > nwb) {
                    push_vert(tex[index], quads[index], 1.0,  (8.0 - nnet)/2.0,      2*x,   nnet, 2*z+2); 
                    push_vert(tex[index], quads[index], 0.0,  (8.0 - nnwt)/2.0,      2*x+2, nnwt, 2*z+2);
                    push_vert(tex[index], quads[index], 0.0,  (8.0 - nwb )/2.0,  2*x+2, nwb,  2*z+2);
                    push_vert(tex[index], quads[index], 1.0,  (8.0 - neb )/2.0,  2*x,   neb,  2*z+2);
                }
                //South wall
                if(sset > seb || sswt > swb) {
                    push_vert(tex[index], quads[index], 1.0,  (8.0-sswt)/2.0,       2*x+2,sswt,2*z);
                    push_vert(tex[index], quads[index], 0.0,  (8.0-sset)/2.0,       2*x,  sset,2*z);
                    push_vert(tex[index], quads[index], 0.0,  (8.0-seb)/2.0, 2*x,  seb, 2*z);
                    push_vert(tex[index], quads[index], 1.0,  (8.0-swb)/2.0, 2*x+2,swb, 2*z);
                }
                //East wall
                if(enet > neb || eset > seb) {
                    push_vert(tex[index], quads[index], 1.0,  (8.0 - eset)/2.0,     2*x,eset,2*z);
                    push_vert(tex[index], quads[index], 0.0,  (8.0 - enet)/2.0,     2*x,enet,2*z+2);
                    push_vert(tex[index], quads[index], 0.0,  (8.0-neb)/2.0, 2*x,neb, 2*z+2);
                    push_vert(tex[index], quads[index], 1.0,  (8.0-seb)/2.0, 2*x,seb, 2*z);
                }
                //West wall
                if(wnwt > nwb || wswt > swb) {
                    push_vert(tex[index], quads[index], 1.0,  (8.0-wnwt)/2.0,    2*x+2,wnwt,2*z+2);
                    push_vert(tex[index], quads[index], 0.0,  (8.0-wswt)/2.0,    2*x+2,wswt,2*z);
                    push_vert(tex[index], quads[index], 0.0,  (8.0-swb)/2.0, 2*x+2,swb,2*z);
                    push_vert(tex[index], quads[index], 1.0,  (8.0-nwb)/2.0, 2*x+2,nwb,2*z+2);
                }
            }
            if(t == simple_map::DIAG_SE) {
            //Draw SE top cap
#ifdef DEVBUILD
                tris.push_back(2*x+2); tris.push_back(8.0); tris.push_back(2*z);
                tris.push_back(2*x);   tris.push_back(8.0); tris.push_back(2*z+2);
                tris.push_back(2*x+2); tris.push_back(8.0); tris.push_back(2*z+2);
#endif

                push_vert(tex[index], quads[index], 1.0,0.0,2*x,8.0,2*z+2);
                push_vert(tex[index], quads[index], 0.0,0.0,2*x+2,8.0,2*z);
                push_vert(tex[index], quads[index], 0.0,(8.0-h)/2.0,2*x+2,h,2*z);
                push_vert(tex[index], quads[index], 1.0,(8.0-h)/2.0,2*x,h,2*z+2);
            }
            else if(t == simple_map::DIAG_NW) {
            //Draw NW top cap
#ifdef DEVBUILD
                tris.push_back(2*x);   tris.push_back(8.0); tris.push_back(2*z+2);
                tris.push_back(2*x+2); tris.push_back(8.0); tris.push_back(2*z);
                tris.push_back(2*x);   tris.push_back(8.0); tris.push_back(2*z);
#endif

                push_vert(tex[index], quads[index], 1.0,0.0,2*x+2,8.0,2*z);
                push_vert(tex[index], quads[index], 0.0,0.0,2*x,8.0,2*z+2);
                push_vert(tex[index], quads[index], 0.0,(8.0-h)/2.0,2*x,h,2*z+2);
                push_vert(tex[index], quads[index], 1.0,(8.0-h)/2.0,2*x+2,h,2*z);
            }
            else if(t == simple_map::DIAG_SW) {
            //Draw SW top cap
#ifdef DEVBUILD
                tris.push_back(2*x+2); tris.push_back(8.0); tris.push_back(2*z+2);
                tris.push_back(2*x);   tris.push_back(8.0); tris.push_back(2*z);
                tris.push_back(2*x);   tris.push_back(8.0); tris.push_back(2*z+2);
#endif

                push_vert(tex[index], quads[index], 1.0,0.0,2*x,8.0,2*z);
                push_vert(tex[index], quads[index], 0.0,0.0,2*x+2,8.0,2*z+2);
                push_vert(tex[index], quads[index], 0.0,(8.0-h)/2.0,2*x+2,h,2*z+2);
                push_vert(tex[index], quads[index], 1.0,(8.0-h)/2.0,2*x,h,2*z);
            }
            else if(t == simple_map::DIAG_NE) {
            //Draw NE top cap
#ifdef DEVBUILD
                tris.push_back(2*x+2); tris.push_back(8.0); tris.push_back(2*z+2);
                tris.push_back(2*x+2); tris.push_back(8.0); tris.push_back(2*z);
                tris.push_back(2*x);   tris.push_back(8.0); tris.push_back(2*z);
#endif

                push_vert(tex[index], quads[index], 1.0,0.0,2*x+2,8.0,2*z+2);
                push_vert(tex[index], quads[index], 0.0,0.0,2*x,8.0,2*z);
                push_vert(tex[index], quads[index], 0.0,(8.0-h)/2.0,2*x,h,2*z);
                push_vert(tex[index], quads[index], 1.0,(8.0-h)/2.0,2*x+2,h,2*z+2);
            }
            //South wall
            if(st != simple_map::SOLID_TILE && (t == simple_map::DIAG_NW || t == simple_map::DIAG_NE)) {
                push_vert(tex[index], quads[index], 1.0,0.0,2*x,8.0,2*z);
                push_vert(tex[index], quads[index], 0.0,0.0,2*x+2,8.0,2*z);
                push_vert(tex[index], quads[index], 0.0,(8.0-h)/2.0,2*x+2,swb,2*z);
                push_vert(tex[index], quads[index], 1.0,(8.0-h)/2.0,2*x,seb,2*z);
            }
            //North wall
            if(nt != simple_map::SOLID_TILE && (t == simple_map::DIAG_SW || t == simple_map::DIAG_SE)) {
                push_vert(tex[index], quads[index], 0.0,(8.0-h)/2.0,2*x,neb,2*z+2);
                push_vert(tex[index], quads[index], 1.0,(8.0-h)/2.0,2*x+2,nwb,2*z+2);
                push_vert(tex[index], quads[index], 1.0,0.0,2*x+2,8.0,2*z+2);
                push_vert(tex[index], quads[index], 0.0,0.0,2*x,8.0,2*z+2);
            }
            //East wall
            if(et != simple_map::SOLID_TILE && (t == simple_map::DIAG_NW || t == simple_map::DIAG_SW)) {
                push_vert(tex[index], quads[index], 0.0,(8.0-h)/2.0,2*x,seb,2*z);
                push_vert(tex[index], quads[index], 1.0,(8.0-h)/2.0,2*x,neb,2*z+2);
                push_vert(tex[index], quads[index], 1.0,0.0,2*x,8.0,2*z+2);
                push_vert(tex[index], quads[index], 0.0,0.0,2*x,8.0,2*z);
            }
            //West wall
            if(wt != simple_map::SOLID_TILE && (t == simple_map::DIAG_NE || t == simple_map::DIAG_SE)) {
                push_vert(tex[index], quads[index], 0.0,(8.0-h)/2.0,2*x+2,nwb,2*z+2);
                push_vert(tex[index], quads[index], 1.0,(8.0-h)/2.0,2*x+2,swb,2*z);
                push_vert(tex[index], quads[index], 1.0,0.0,2*x+2,8.0,2*z);
                push_vert(tex[index], quads[index], 0.0,0.0,2*x+2,8.0,2*z+2);
            }

        }
    }
    map_needs_update = false;
}

void render_3d() {
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity ();

    glRotatef(theta, 0.0, 1.0, 0.0); //Rotate perspective around Y axis
    glRotatef(phi, cos(thetaR), 0.0, sin(thetaR)); //Rotate pespective around camera's X axis

    glTranslatef(-1.0*camXPos, -1.0*camYPos, -1.0*camZPos); //Translate the world to where the camera expects it to be

#ifdef DEVBUILD
    //Draw a bounding/orientation box around the level
    draw_level_bounds();
#endif

    simple_map::level level = sm.levels[cur_lev];
    uint16_t cti = level.ceil_tex_index;

    //Draw the ceiling
    glColor3f(1.0,1.0,1.0);
    sf::Texture::bind(&(floors.tex[cti]));
    glBegin(GL_QUADS);
      glTexCoord2f(64.0,64.0); glVertex3f(0.0,8.0,0.0);
      glTexCoord2f(0.0,64.0); glVertex3f(128.0,8.0,0.0);
      glTexCoord2f(0.0,0.0); glVertex3f(128.0,8.0,128.0);
      glTexCoord2f(64.0,0.0); glVertex3f(0.0,8.0,128.0);
    glEnd();

    std::vector<sprite_info> sprite_sorter;
    sprite_sorter.reserve(1024);

    update_level_map();
    //Draw the floors and walls. "index" is the texture number to use.
    for(int index = 0; index < 512; index++) {
        if(quads[index].size() == 0) {
            continue;
        }

        if(index < 256) { //Floors
            if(index >= floors.tex.size()) {
                //Condition only applies to tiles that don't have anything in them
                sf::Texture::bind(NULL);
                glColor3f(0.0,0.0,0.0);
            }
            else if(floors.animtex[index].size() == 0) {
                glColor3f(1.0,1.0,1.0);
                sf::Texture::bind(&(floors.tex[index]));
            }
            else {
                glColor3f(1.0,1.0,1.0);
                sf::Texture::bind(&(floors.animtex[index][anim_framecount % floors.animtex[index].size()]));
                //cout<<"Tex "<<fti<<" using frame "<<(anim_framecount%floors.animtex[fti].size())<<endl;
            }
        }
        else { //Walls
            if(index >= 256) {
                glColor3f(1.0,1.0,1.0);
            }
            //If no animated texture wall exists, bind the static one. Otherwise, bind the appropriate frame of the animated texture.
            if(index - 256 >= walls.tex.size()) {
                //Shouldn't ever be true
                sf::Texture::bind(NULL);
            }
            else if(walls.animtex[index - 256].size() == 0) {
                sf::Texture::bind(&(walls.tex[index - 256]));
            }
            else {
                sf::Texture::bind(&(walls.animtex[index - 256][anim_framecount % walls.animtex[index - 256].size()]));
            }
        }

        glVertexPointer(3, GL_FLOAT, 0, &quads[index][0]);
        glEnableClientState(GL_VERTEX_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, &tex[index][0]);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glDrawArrays(GL_QUADS, 0, quads[index].size() / 3); //3 = number of coordinates per vertex

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    for(int x = 0;x<64;++x) {
        for(int z = 0;z<64;++z) {
            int x_index = 63 - x; //I want x to go in the other direction
            simple_map::map_data tiledat = level.d[x_index][z];
            //Draw objects on this tile
            uint16_t index = tiledat.first_obj;
            while(index != 0) {
                sprite_sorter.push_back(sprite_info(x,z,index));
                if(index < 256) index = sm.levels[cur_lev].npcs[index].get_next();
                else            index = sm.levels[cur_lev].items[index - 256].get_next();
            }
        }
    }

#ifdef DEVBUILD
    //Draw the triangular top caps of the level
    sf::Texture::bind(NULL);
    glColor3f(0.0,0.0,0.0);
    glVertexPointer(3, GL_FLOAT, 0, &tris[0]);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_TRIANGLES, 0, tris.size() / 3); //3 = number of coordinates per vertex
    glDisableClientState(GL_VERTEX_ARRAY);
    glColor3f(1.0,1.0,1.0);
#endif

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
    string level, wall, floor, pal, strs, crf, obj, apals, sw, tm, exe, drs;
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
    exe = fn+"/uw_demo.exe";
    drs = fn+"/data/doors.gr";
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
        exe = fn+"/UW_DEMO.EXE";
        drs = fn+"/DATA/DOORS.GR";
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
            exe = fn+"/uw.exe";
            drs = fn+"/data/doors.gr";
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
                exe = fn+"/UW.EXE";
                drs = fn+"/DATA/DOORS.GR";
                if(!sm.load(level)) {
                    cout<<"Couldn't load any of the level files."<<endl;
                    return false;
                }
            }
        }
    }
    bool retval = walls.load(pal,wall) && floors.load(pal,floor) && strings.load(strs) && cf.load(crf,pal) && objs.load(pal,obj,apals) && switches.load(pal, sw, apals) && tmobj.load(pal,tm,apals) && doors.load(pal,drs,apals);

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

    model.resize(32);
    mod_vertex.resize(32);
    mod_texmap.resize(32);
    mod_color.resize(32);
    for(int i=0; i < 32; i++) {
        model[i].load(exe, pal, i);
        mod_vertex[i] = model[i].get_verts(uw_model::geometry);
        mod_texmap[i] = model[i].get_verts(uw_model::texcoords);
        mod_color[i]  = model[i].get_verts(uw_model::colors);
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

void set_viewport() {
    if(winW >= (4.0f/3.0f) * winH) {
        int spare = winW - ((4.0f/3.0f)*winH);
        glViewport(spare/2,0, GLsizei((4.0f/3.0f)*winH), GLsizei(winH));
    }
    else {
        int spare = winH - ((3.0f/4.0f)*winW);
        glViewport(0,spare/2, GLsizei(winW), GLsizei((3.0f/4.0f) * winW));
    }
}

void gameloop() {
    sf::RenderWindow window(sf::VideoMode(winW,winH,32), "Ultima Underworld Engine Remake Attempt", sf::Style::Default, sf::ContextSettings(24,0,0,1,5));
    window.setKeyRepeatEnabled(false);
    window.setFramerateLimit(20);

    glClearColor (0.0, 0.0, 0.0, 0.0);
    glClearDepth (1.0);
    glShadeModel (GL_FLAT);
    set_viewport();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, float(winW)/float(winH), 0.1, 200.0);
    glMatrixMode (GL_MODELVIEW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    //glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glPointSize(4);

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
                set_viewport();
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
        if(anim_div == 7) {
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
