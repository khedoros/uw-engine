#include<vector>
#include<cstdint>
#include "vocfile.h"
#include<iostream>
#include<fstream>
#include<string>
#include<algorithm>
using namespace std;

bool vocfile::print_info(const std::string& filename) {
    ifstream a(filename.c_str());
    a.seekg(0,ios::end);
    size_t size = a.tellg();
    a.seekg(0,ios::beg);
    vector<uint8_t> filedata(size,10);
    a.read((char *)(&(filedata[0])), size);
    string desc(filedata.begin(),filedata.begin()+19);
    cout<<"Description: "<<desc<<" Version: "<<dec<<int(filedata[23])<<"."<<int(filedata[22])<<endl;
    size_t offset = ((size_t(filedata[21])) << (8)) + (size_t(filedata[20]));
    cout<<"offset of data start: "<<offset<<endl;
    bool bof = true;
    while(bof) {
        uint8_t tag = filedata[offset];
        switch(tag) {
          case 0: cout<<"Terminator"<<endl; bof = false; break;
          case 1: cout<<"Sound data"<<" Freq: "<<1000000 / (256 - int32_t(filedata[offset+4]))<<" Codec ID: "<<uint32_t(filedata[offset+5]); break;
          case 2: cout<<"Sound data continuation"; break;
          case 3: cout<<"Silence"; break;
          case 4: cout<<"Marker"; break;
          case 5: cout<<"Text"; break;
          case 6: cout<<"Repeat start"; break;
          case 7: cout<<"Repeat end"; break;
          case 8: cout<<"Extra info"; break;
          case 9: cout<<"Sound data (new format)"; break;
          default: cout<<"Dunno. "<<hex<<int(tag)<<"."<<endl; bof = false;
        }
        uint32_t blocksize = 0;
        if(bof) {
            blocksize = uint32_t(filedata[offset+1]) + (uint32_t(filedata[offset+2]) << (8)) + (uint32_t(filedata[offset+3]) << (16));
            cout<<"\tBlock size: "<<blocksize<<endl;
            offset += (4+blocksize);
        }
    }
    a.close();
}

bool vocfile::check(char * filename) {
    ifstream a(filename);
    a.seekg(0,ios::end);
    size_t size = a.tellg();
    a.seekg(0,ios::beg);
    if(size>200000) { a.close(); return false;}
    vector<uint8_t> filedata(size,10);
    a.read((char *)(&(filedata[0])), size);
    a.close();
    string desc(filedata.begin(),filedata.begin()+19);
    uint16_t version = (uint16_t(filedata[23]) <<(8)) + uint16_t(filedata[22]);
    if(version != 0x10a) return false;
    size_t offset = ((size_t(filedata[21])) << (8)) + (size_t(filedata[20]));
    if(offset != 0x1a) return false;
    return true;
}


tuple<size_t,size_t> vocfile::get_data_loc(const std::string& filename) {
    ifstream a(filename.c_str());
    if(!a.is_open()) {
        std::cerr<<"Couldn't open file "<<filename<<std::endl;
        return make_tuple(0,0);
    }
    a.seekg(0,ios::end);
    size_t filesize = a.tellg();
    a.seekg(0,ios::beg);
    if(filesize > 200000) {
        cerr<<"Filesize larger than expected "<<filesize<<" bytes). Bailing."<<endl;
        return make_tuple(0,0);
    }
    vector<uint8_t> filedata(filesize,10); //10, because it's not a valid block tag
    a.read((char *)(&(filedata[0])), filesize);
    string desc(filedata.begin(),filedata.begin()+19);
    size_t offset = ((size_t(filedata[21])) << (8)) + (size_t(filedata[20]));
    bool bof = true;
    while(bof) {
        uint8_t tag = filedata[offset];
        switch(tag) {
          case 0: /*cout<<"Terminator"<<endl;*/ bof = false; break;
          case 1: /*cout<<"Sound data";*/
                  return make_tuple(uint32_t(offset+4), uint32_t(filedata[offset+1]) + (uint32_t(filedata[offset+2]) << (8)) + (uint32_t(filedata[offset+3]) << (16)));
          case 2: /*cout<<"Sound data continuation";*/ break;
          case 3: /*cout<<"Silence";*/ break;
          case 4: /*cout<<"Marker";*/ break;
          case 5: /*cout<<"Text";*/ break;
          case 6: /*cout<<"Repeat start";*/ break;
          case 7: /*cout<<"Repeat end";*/ break;
          case 8: /*cout<<"Extra info";*/ break;
          case 9: /*cout<<"Sound data (new format)";*/ break;
          default: cout<<"Dunno. "<<hex<<int(tag)<<"."<<endl; bof = false;
        }
        uint32_t blocksize = 0;
        if(bof) {
            blocksize = uint32_t(filedata[offset+1]) + (uint32_t(filedata[offset+2]) << (8)) + (uint32_t(filedata[offset+3]) << (16));
            offset += (4+blocksize);
        }
    }
    a.close();
}

vector<int16_t> vocfile::get_file_dat(const std::string& filename) {
    auto data_loc = vocfile::get_data_loc(filename);
    vector<uint8_t> audio;
    audio.resize(get<1>(data_loc));
    ifstream a(filename.c_str());
    a.seekg(get<0>(data_loc));
    a.read((char *)(&(audio[0])), get<1>(data_loc));
    vector<int16_t> ret_buff;
    ret_buff.resize(get<1>(data_loc));
    copy(audio.begin(),audio.end(),ret_buff.begin());
    transform(audio.begin(),audio.end(),ret_buff.begin(), [](int16_t s) { return (256 * (int16_t(s) - 128) ); });
    return ret_buff;
}

#ifdef STAND_ALONE_VOC
int main(int argc, char *argv[]) {
    string filename(argv[1]);
    vocfile::print_info(filename);
    auto dataloc = vocfile::get_data_loc(filename);
    cout<<"Offset: "<<get<0>(dataloc)<<" Length: "<<get<1>(dataloc)<<endl;
    vocfile::get_file_dat(filename);
    return 0;
}
#endif
