#pragma once

#include<iostream>
#include<fstream>
#include<vector>
#include<stdint.h>
#include<cassert>

uint8_t read8(std::ifstream& in);
uint16_t read16(std::ifstream& in);
uint32_t read32(std::ifstream& in);
uint64_t read64(std::ifstream& in);
void print_raw(std::ifstream& in, uint8_t tag, uint32_t tsize, std::vector<uint8_t>& data, int line_length = 32, bool reset = false);
void print_raw(uint32_t tsize, std::vector<uint8_t>& data, int line_length = 32);

class binifstream : public std::ifstream {
public:
    template <typename T>
    binifstream& operator>>(T& val) {
        char *temp=new char[sizeof(T)];
        this->read(temp,sizeof(T));
        val=T(*(reinterpret_cast<T *>(temp)));
        delete[] temp;
        //cout<<"file: grabbed "<<sizeof(T)<<" byte(s) at "<<int(tellg())-int(sizeof(T))<<endl;
        return *this;
    }
};

struct color {
public:
    color(uint8_t r1,uint8_t g1,uint8_t b1,uint8_t a1) : r(r1), g(g1), b(b1), a(a1) {}
    color(uint8_t r1,uint8_t g1,uint8_t b1) : r(r1), g(g1), b(b1), a(255) {}
    color() : r(0), g(0), b(0), a(255) {}
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} ;

//Constructing: Construct with data read from an ifstream, store data in a vector?
//Use: Extract with >> operator, read sizeof(argument) bytes, throw exception if it fails?
class BinBuffer {
public:
    bool is_good();
    BinBuffer();
    bool load(binifstream& stream, std::pair<uint32_t,uint32_t> offset_length);

    template <typename T>
    BinBuffer& operator>>(T & val) {
        char *temp=new char[sizeof(T)];
        for(int i=0;i<sizeof(T) && it != data.end();++i) {
            temp[i]=*it++;
        }
        if(it==data.end()) {
            good=false;
        }
        val=T(*(reinterpret_cast<T *>(temp)));
        delete[] temp;
        //cout<<"file: grabbed "<<sizeof(T)<<" byte(s) at "<<int(tellg())-int(sizeof(T))<<endl;
        return *this;
    }
private:
    std::vector<uint8_t> data;
    std::vector<uint8_t>::iterator it;
    bool good;
};

uint16_t get16(std::vector<uint8_t>& d, uint32_t i);
uint32_t get32(std::vector<uint8_t>& d, uint32_t i);
float fix2float(uint16_t d);
float frac2float(uint16_t d);

class vert {
    public:
    float x;
    float y;
    float z;
    float u;
    float v;
    uint16_t tex;
    uint16_t n;
    vert(float ix, float iy, float iz, uint16_t in) {
        x = ix; y = iy; z = iz; n = in; u = 0; v = 0;
    }
    vert(float ix, float iy, float iz, float iu, float iv, uint16_t in) {
        x = ix; y = iy; z = iz; u = iu; v = iv; n = in;
    }
    vert() {x=0; y=0; z=0; n = -1; u = 0; v = 0;}
};

