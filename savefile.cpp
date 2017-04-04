#include<iostream>
#include<fstream>
#include<vector>
#include<stdint.h>
using namespace std;

int main(int argc, char **argv) {
    ifstream in("PLAYER.DAT");
    if(!in.is_open()) {
        cerr<<"Bleh, couldn't open PLAYER.DAT"<<endl;
        return 1;
    }

    in.seekg(0,ios::end);
    size_t filesize = in.tellg();
    in.seekg(0,ios::beg);
    vector<uint8_t> data(filesize,0);
    in.read(reinterpret_cast<char *>(&data[0]), filesize);
    in.close();
    uint8_t key = data[0]+3;
    for(int i=1;i<221;++i) {
        data[i] ^= key;
        key+=3;
    }
    cout<<"Name: ";
    int i=1;
    for(;i<15;++i) cout<<data[i];
    cout<<endl;
    i=0x1f;
    cout<<"Strength: "<<int(data[i++])<<endl;
    cout<<"Dexterity: "<<int(data[i++])<<endl;
    cout<<"Intelligence: "<<int(data[i++])<<endl;
    cout<<"Attack: "<<int(data[i++])<<endl;
    cout<<"Defense: "<<int(data[i++])<<endl;
    cout<<"Unarmed: "<<int(data[i++])<<endl;
    cout<<"Sword: "<<int(data[i++])<<endl;
    cout<<"Axe: "<<int(data[i++])<<endl;
    cout<<"Mace: "<<int(data[i++])<<endl;
    cout<<"Missile: "<<int(data[i++])<<endl;
    cout<<"Mana: "<<int(data[i++])<<endl;
    cout<<"Lore: "<<int(data[i++])<<endl;
    cout<<"Casting: "<<int(data[i++])<<endl;
    cout<<"Traps: "<<int(data[i++])<<endl;
    cout<<"Search: "<<int(data[i++])<<endl;
    cout<<"Track: "<<int(data[i++])<<endl;
    cout<<"Sneak: "<<int(data[i++])<<endl;
    cout<<"Repair: "<<int(data[i++])<<endl;
    cout<<"Charm: "<<int(data[i++])<<endl;
    cout<<"Picklock: "<<int(data[i++])<<endl;
    cout<<"Acrobat: "<<int(data[i++])<<endl;
    cout<<"Appraise: "<<int(data[i++])<<endl;
    cout<<"Swimming: "<<int(data[i++])<<endl;
    cout<<"Current health: "<<int(data[i++])<<endl;
    cout<<"Max Health: "<<int(data[i++])<<endl;
    cout<<"Current Mana: "<<int(data[i++])<<endl;
    cout<<"Max Mana: "<<int(data[i++])<<endl;
    cout<<"Hunger: "<<int(data[i++])<<endl;
    cout<<"Unknown 1: "<<int(data[i++])<<endl;
    cout<<"Unknown 2: "<<int(data[i++])<<endl;
    cout<<"Unknown 3: "<<int(data[i++])<<endl;
    cout<<"Character Lvl: "<<int(data[i++])<<endl;
    cout<<"Unknown 4: "<<int(data[i++])<<endl;
    cout<<"Unknown 5: "<<int(data[i++])<<endl;
    cout<<"Unknown 6: "<<int(data[i++])<<endl;
    cout<<"Unknown 7: "<<int(data[i++])<<endl;
    cout<<"Unknown 8: "<<int(data[i++])<<endl;
    cout<<"Unknown 9: "<<int(data[i++])<<endl;
    cout<<"Runes H->A: "<<int(data[i++])<<endl;
    cout<<"Runes P->I: "<<int(data[i++])<<endl;
    cout<<"Runes Y->Q: "<<int(data[i++])<<endl;
    cout<<"Unknown 10: "<<int(data[i++])<<endl;
    cout<<"Unknown 11: "<<int(data[i++])<<endl;
    cout<<"Unknown 12: "<<int(data[i++])<<endl;
    cout<<"Unknown 13: "<<int(data[i++])<<endl;
    cout<<"Unknown 14: "<<int(data[i++])<<endl;
    cout<<"Wt avail. (in 1/10 stones): "<<(int(data[i++]) + int(data[i++])*256)<<endl;
    cout<<int(data[i])<<" "<<int(data[i+1])<<" "<<int(data[i+2])<<" "<<int(data[i+3])<<endl;
    cout<<"XP (in 1/10 points): "<<int(data[i++])+int(data[i++])*256+int(data[i++])*65536+int(data[i++])*16777216<<endl;
    cout<<"Unknown 15: "<<int(data[i++])<<endl;
    cout<<"Unknown 16: "<<int(data[i++])<<endl;
    cout<<"X Pos: "<<int(data[i++])+int(data[i++])*256<<endl;
    cout<<"Y Pos: "<<int(data[i++])+int(data[i++])*256<<endl;
    cout<<"Z Pos: "<<int(data[i++])+int(data[i++])*256<<endl;
    cout<<"Heading: "<<int(data[i++])+int(data[i++])*256<<endl;
    cout<<"Dungeon Level: "<<int(data[i++])+int(data[i++])*256<<endl;
    cout<<"Unknown 17: "<<int(data[i++])<<endl;
    cout<<"Player Poison (bits 2..5): "<<int(data[i++])<<endl;
    i=0xCF;
    cout<<"Game Time: "<<int(data[i++])+int(data[i++])*256+int(data[i++])*65536+int(data[i++])*16777216<<endl;
    i=0xDD;
    cout<<"Current health (??): "<<int(data[i++])<<endl;
    return 0;
}
