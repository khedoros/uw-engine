#include "util.h"

uint8_t read8(std::ifstream& in) {
    char buf;
    in.read(&buf,1);
    return *(reinterpret_cast<uint8_t *>(&buf));
}

uint16_t read16(std::ifstream& in) {
    char buf[2];
    in.read(&buf[0],2);
    return *(reinterpret_cast<uint16_t *>(&buf));
}

uint32_t read32(std::ifstream& in) {
    char buf[4];
    in.read(&buf[0],4);
    return *(reinterpret_cast<uint32_t *>(&buf));
}

uint64_t read64(std::ifstream& in) {
    char buf[8];
    in.read(&buf[0],8);
    return *(reinterpret_cast<uint64_t *>(&buf));
}


//Constructing: Construct with data read from an ifstream, store data in a vector?
//Use: Extract with >> operator, read sizeof(argument) bytes, throw exception if it fails?
bool BinBuffer::is_good() {
    return good;
}

BinBuffer::BinBuffer() : data(0), it(0), good(false) {}

bool BinBuffer::load(binifstream& stream, std::pair<uint32_t,uint32_t> offset_length) {
    size_t bookmark = stream.tellg();
    stream.seekg(offset_length.first,std::ios::beg);
    data.resize(offset_length.second,0);
    it=data.begin();
    stream.read(reinterpret_cast<char *>(&(*it)), offset_length.second);
    good=!(stream.fail());
    stream.seekg(bookmark,std::ios::beg);
    return !(stream.fail());
}

//Defined in util.h:
//template <typename T>
//BinBuffer& BinBuffer::operator>>(T & val)

uint16_t get16(std::vector<uint8_t>& d, uint32_t i) {
    uint16_t one = *((uint16_t *)(&d[i]));
    uint16_t two = uint16_t(d[i]) + ((uint16_t(d[i+1]))<<(8));
    assert(one == two);
    return *((uint16_t *)(&d[i]));
}

uint32_t get32(std::vector<uint8_t>& d, uint32_t i) {
    return *((uint32_t *)(&d[i]));
}

float fix2float(uint16_t d) {
    return float(static_cast<int16_t>(d))/256.0;
    return float(d)/256.0;
}

float frac2float(uint16_t d) {
    return float(d)/65536.0;
}

