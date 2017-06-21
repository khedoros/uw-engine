#include "util.h"
#include<iostream>
#include<fstream>
using namespace std;

//Consumes the tag, but also reads it into "data"
void print_raw(ifstream& in, uint8_t tag, uint16_t tsize, vector<uint8_t>& data) {
    data.resize(tsize);
    size_t offset = in.tellg();
    printf("%02x (%04x bytes):\n", tag, tsize);
    in.read(reinterpret_cast<char *>(&data[0]), tsize);
    for(int line=0;line<tsize/32;line++) {
        for(int c=0;c<32;c++) {
            printf("%02x ", data[line*32+c]);
        }
        printf(" |");
        for(int c=0;c<32;c++) {
            if(data[line*32+c] > ' ' && data[line*32+c] < 127) {
                printf("%c", data[line*32+c]);
            }
            else {
                printf(".");
            }
        }
        printf("| \n");
    }
    if(tsize%32) {
        for(int c=0;c<tsize%32;c++) {
            printf("%02x ", data[c+tsize-tsize%32]);
        }
        printf(" |");
        for(int c=0;c<tsize%32;c++) {
            if(data[c+tsize-tsize%32] > ' ' && data[c+tsize-tsize%32] < 127) {
                printf("%c", data[c+tsize-tsize%32]);
            }
            else {
                printf(".");
            }
        }
        printf("| \n");
    }
}

int main(int argc, char *argv[]) {
    if(argc != 2) {
        cerr<<"Provide a library name."<<endl;
        return 1;
    }
    ifstream in(argv[1]);
    if(!in.is_open()) {
        cerr<<"Couldn't open \""<<argv[1]<<"\"."<<endl;
        return 1;
    }

    uint8_t tag = 0;
    bool tag_types[256] = {0};
    uint16_t tsize = 0;
    vector<uint8_t> data(0);
    size_t offset = 0;
    while(tag != 0xf1 && !in.eof()) {
        tag = read8(in);
        tag_types[tag] = true;
        data.resize(0);
        switch(tag) {
            case 0x80:
                tsize = read16(in);
                print_raw(in,tag,tsize,data);
                cout<<"THEADR: \"";
                for(int i=0;i<data[0];i++) cout<<data[1+i];
                cout<<"\"\n"<<endl;
                break;
            case 0x88:
                tsize = read16(in);
                print_raw(in,tag,tsize,data);
                printf("COMENT: type %02x, class %02x", data[0],data[1]);
                if(data[1] == 0x00) {
                    printf(" data: \"");
                    for(int i=0;i<data[2];i++) cout<<data[3+i];
                    cout<<"\"\n"<<endl;
                }
                else if(data[1] == 0xe9) {
                    printf(" data: \"", data[0],data[1]);
                    for(int i=0;i<data[6];i++) cout<<data[7+i];
                    cout<<"\"\n"<<endl;
                }
                else {
                    cout<<"\n"<<endl;
                }
                break;
            case 0xf0:
                cerr<<"Library; not handled yet."<<endl;
                in.close();
                return 1;
                break;
            default:
                tsize = read16(in);
                print_raw(in,tag,tsize,data);
                cout<<endl;
                break;
        }
    }
    for(int i=0;i<256;i++) if(tag_types[i]) printf("Saw tag %02x\n", i);
    in.close();
}
