#include<iostream>
#include "util.h"
#include <string>
#include<vector>
#include "UwText.h"
#include<cstdio>

UwText::UwText() {}

size_t UwText::get_block_count() const {
    return blocks.size();
}

size_t UwText::get_block_num(size_t blocknum) const {
    if(blocknum < blocks.size())
        return blocks[blocknum].block_num;
    else return 0;
}

std::string UwText::get_block_name(size_t blocknum) const {
    if(blocknum < blocks.size())
        return blocks[blocknum].name;
    else return std::string("");
}

size_t UwText::get_string_count(size_t blocknum) const {
    if(blocknum < blocks.size())
        return blocks[blocknum].strings.size();
    else
        return 0;
}

std::string UwText::get_string(size_t blocknum, size_t stringnum) const {
    if(blocknum < blocks.size() && stringnum < blocks[blocknum].strings.size())
        return blocks[blocknum].strings[stringnum];
    else return std::string("");
}

bool UwText::load(std::string filename) {
    binifstream in;
    in.open(filename.c_str(),std::ios::in|std::ios::binary);
    if(!in.is_open()) {
        std::cerr<<"Couldn't open the file "<<filename<<std::endl;
        return false;
    }
    //std::cout<<"UwText reading strings from "<<filename<<std::endl;
    in.seekg(0, std::ios::end);
    size_t filesize=in.tellg();
    in.seekg(0, std::ios::beg);
    if(filesize < 180*4 + 2 + 2) {
        std::cerr<<"This file won't even hold the expected file headers."<<std::endl;
        return false;
    }
    uint16_t node_count;
    in>>node_count;
    //std::cout<<"The file has "<<node_count<<" nodes."<<std::endl;
    nodes.resize(node_count);
    for(size_t i=0;i<nodes.size();++i) {
        in>>nodes[i];
        //std::cout<<"Node: "<<i<<" Char: "<<int(nodes[i].c)<<" Parent: "<<int(nodes[i].p)<<" Left: "<<int(nodes[i].l)<<" Right: "<<int(nodes[i].r)<<std::endl;
    }

    uint16_t block_count;
    in>>block_count;
    //std::cout<<block_count<<" text blocks in the file."<<std::endl;
    blocks.resize(block_count);
    for(int i=0;i<block_count;++i) {
        std::string n;
        in>>blocks[i].block_num;
        in>>blocks[i].block_off;
        switch(blocks[i].block_num) {
        case 1:     n="general UI strings"; break;
        case 2:     n="character creation strings, mantras (?)"; break;
        case 3:     n="wall text/scroll/book/book title strings (*)"; break;
        case 4:     n="object descriptions (*)"; break;
        case 5:     n="object \"look\" descriptions, object quality states"; break;
        case 6:     n="spell names"; break;
        case 7:     n="conversation partner names, starting at string 17 for conv 1"; break;
        case 8:     n="text on walls, signs"; break;
        case 9:     n="text trap messages"; break;
        case 0xa:   n="wall/floor description text"; break;
        case 0x18:  n="debugging strings (not used ingame)"; break;
        case 0xc00: n="intro cutscene text"; break;
        case 0xc01: n="ending cutscene text"; break;
        case 0xc02: n="tyball cutscene text"; break;
        case 0xc03: n="arial cuscene text (?)"; break;
        case 0xc18: n="dream cutscene 1 text \"arrived\""; break;
        case 0xc19: n="dream cutscene 2 text \"talismans\""; break;
        default:    n="dunno";
        }
        if(blocks[i].block_num >= 0xc1a && blocks[i].block_num <= 0xc21)
            n="garamon cutscene texts";
        else if(blocks[i].block_num >= 0xe01 && blocks[i].block_num <= 0xf3a)
            n="conversation strings";
            blocks[i].name=n;
            //std::cout<<"Number: "<<blocks[i].block_num<<" Offset: "<<blocks[i].block_off<<" Description: "<<blocks[i].name<<std::endl;
    }
    if(filesize<=blocks[block_count-1].block_off) {
        std::cerr<<"File seems to be truncated before the end."<<std::endl;
        return false;
    }
    for(int block=0;block<blocks.size();++block) {
        bool retval=process_block(in, block);
        if(!retval) {
            std::cerr<<"Problem processing block #"<<block<<". Aborting."<<std::endl;
            return false;
        }
    }
    return true;
}

std::string UwText::decode_string(binifstream& in, size_t offset) {
    size_t bookmark=in.tellg();
    in.seekg(offset,std::ios::beg);

    uint8_t cur=0;
    uint8_t buf=0;
    std::string retval("");
    size_t bits_left=0;
    huffnode n=nodes[nodes.size()-1];
    while(cur!='|') {
        if(bits_left==0) {
            in>>buf;
            bits_left=8;
            //std::cout<<"Grabbed buf: "<<uint16_t(buf)<<std::endl;
        }
        for(;bits_left>0;--bits_left) {
            if(buf&0x80) {
                n=nodes[n.r];
                //std::cout<<"Went right"<<std::endl;
            }
            else {
                n=nodes[n.l];
                //std::cout<<"Went left"<<std::endl;
            }
            //std::cout<<"Buf before shift: "<<std::hex<<int(buf)<<std::endl;
            buf&=0x7f;
            buf<<=1;
            //std::cout<<"Buf after shift: "<<int(buf)<<std::dec<<std::endl;
            if(n.l==255&&n.r==255) {
                if(n.c != '|')
                    retval+=n.c;
                    //std::cout<<"Current retval: "<<retval<<std::endl;
                    cur=n.c;
                    n=nodes[nodes.size()-1];
                }
                if(cur=='|') break;
        }
    }            
    in.seekg(bookmark,std::ios::beg);
    return retval;
}

bool UwText::process_block(binifstream& in, int num) {
    block_data* d=&(blocks[num]);
    in.seekg(d->block_off,std::ios::beg);
    uint16_t string_count;
    in>>string_count;
    //std::cout<<"Found block "<<d->block_num<<" (\""<<d->name<<"\", "<<string_count<<" strings at offset: "<<d->block_off<<")"<<std::endl;
    std::vector<uint16_t> str_off;
    str_off.resize(string_count);
    d->strings.resize(string_count);
    size_t BASE_OFFSET = d->block_off + 2 + 2 * string_count;
    for(int i=0;i<string_count;++i) {
        in>>str_off[i];
        uint8_t min[2] = {0xff};
        uint8_t max[2] = {0};
        //std::cout<<std::dec<<"String "<<i<<": Offset: "<<str_off[i] + BASE_OFFSET<<std::hex<<" Val: ";
        if(nodes.size() == 0) {
            size_t bookmark = in.tellg();
            in.seekg(BASE_OFFSET+str_off[i],std::ios::beg);
            std::string retval;
            char temp_buff[40];
            char out_buff[40];
            uint8_t buf,buf2 = 0;
            for(int j=0;j<40;j+=2) {
                in>>buf>>buf2;
                sprintf(&temp_buff[0],"%02X %02X ",buf,buf2);
                retval+=temp_buff;

                if(buf<min[0]) min[0] = buf;
                if(buf>max[0]) max[0] = buf;
                if(buf2<min[1]) min[1] = buf2;
                if(buf2>max[1]) max[1] = buf2;
                out_buff[j]=buf; out_buff[j+1]=buf2;
                //if(buf >= 33 && buf <= 94)      out_buff[j] = (buf+1)/2 + 112;
                //else if(buf >=95 && buf <= 126) out_buff[j] = (buf+1)/2 + 176;
                //else {std::cout<<"byte 1 out of expected range ("<<std::hex<<int(buf)<<")"<<std::endl; out_buff[j] = 0; }

                //if(buf % 2 == 0) out_buff[j+1] = buf2 + 126;
                //else out_buff[j+1] = buf2 + 31 + (buf2/96);
            }
            std::ofstream out_test("test.txt");
            out_test.write(&out_buff[0], 40);
            out_test.close();
            in.seekg(bookmark,std::ios::beg);
            d->strings[i] = retval;
            //std::cout<<retval<<"First byte: ["<<std::hex<<int(min[0])<<", "<<int(max[0])<<"] Second byte: ["<<int(min[1])<<", "<<int(max[1])<<"]"<<std::endl;
        }
        else {
            d->strings[i]=decode_string(in, BASE_OFFSET + str_off[i]);
        }
        //std::cout<<d.strings[i]<<std::endl;
    }

    return true;
}

#ifdef STAND_ALONE_TEXT
int main(int argc, char *argv[]) {
    if(argc!=2) {
        std::cout<<"Give me one file as an argument."<<std::endl;
        return 1;
    }
    UwText txt;
    txt.load(std::string(argv[1]));
        for(int b = 0;b<txt.get_block_count();++b) {
            std::cout<<"Block "<<txt.get_block_num(b)<<" ("<<txt.get_block_name(b)<<"), "<<txt.get_string_count(b)<<" strings:"<<std::endl;
            for(int c = 0;c<txt.get_string_count(b);++c) {
                std::cout<<"Block "<<txt.get_block_num(b)<<" String "<<c<<": "<<txt.get_string(b,c)<<std::endl;
            }
            std::cout<<std::endl;
        }
        
            
    return 0;
}
#endif
