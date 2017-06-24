#include<iostream>
#include<fstream>
#include<vector>
#include<stdint.h>
#include "util.h"

using namespace std;

#ifdef STANDALONE_SAVE
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
    uint8_t key = 0;
    in.read(reinterpret_cast<char *>(&key), 1);
    in.read(reinterpret_cast<char *>(&data[0]), filesize-1);
    in.close();
    for(int i=0;i<216;++i) {
        key+=3;
        data[i] ^= key;
    }

    cout<<"Raw dump: ";
    print_raw(data.size(), data, 16);
    cout<<"\nName: ";
    int i=0;
    for(;i<14;++i) cout<<data[i];
    cout<<hex<<'	'<<i<<dec<<endl;
    i=0x1e;
    cout<<"Strength: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Dexterity: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Intelligence: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Attack: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Defense: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unarmed: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Sword: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Axe: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Mace: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Missile: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Mana: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Lore: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Casting: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Traps: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Search: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Track: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Sneak: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Repair: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Charm: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Picklock: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Acrobat: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Appraise: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Swimming: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Current health: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Max Health: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Current Mana: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Max Mana: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Hunger: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 1: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 2: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 3: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Character Lvl: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 4: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 5: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 6: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 7: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 8: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 9: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Runes H->A: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Runes P->I: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Runes Y->Q: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 10: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 11: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 12: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 13: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 14: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Wt avail. (in 1/10 stones): "<<(int(data[i++]) + int(data[i++])*256)<<hex<<'	'<<i<<dec<<endl;
    cout<<int(data[i])<<" "<<int(data[i+1])<<" "<<int(data[i+2])<<" "<<int(data[i+3])<<hex<<'	'<<i<<dec<<endl;
    cout<<"XP (in 1/10 points): "<<int(data[i++])+int(data[i++])*256+int(data[i++])*65536+int(data[i++])*16777216<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 15: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 16: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"X Pos: "<<int(data[i++])+int(data[i++])*256<<hex<<'	'<<i<<dec<<endl;
    cout<<"Y Pos: "<<int(data[i++])+int(data[i++])*256<<hex<<'	'<<i<<dec<<endl;
    cout<<"Z Pos: "<<int(data[i++])+int(data[i++])*256<<hex<<'	'<<i<<dec<<endl;
    cout<<"Heading: "<<int(data[i++])+int(data[i++])*256<<hex<<'	'<<i<<dec<<endl;
    cout<<"Dungeon Level: "<<int(data[i++])+int(data[i++])*256<<hex<<'	'<<i<<dec<<endl;
    cout<<"Unknown 17: "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    cout<<"Player Poison (bits 2..5): "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    i=0xCF;
    cout<<"Game Time: "<<int(data[i++])+int(data[i++])*256+int(data[i++])*65536+int(data[i++])*16777216<<hex<<'	'<<i<<dec<<endl;
    i=0xDD;
    cout<<"Current health (??): "<<int(data[i++])<<hex<<'	'<<i<<dec<<endl;
    return 0;
}
#endif
