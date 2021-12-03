#include<iostream>
#include<fstream>
#include<stdint.h>
#include<assert.h>
#include<vector>
#include<string>
#include "dos_header.h"
using namespace std;

void printHeaderInfo(const string& filename) {
    assert(sizeof(mz_header) == 32);
    ifstream in;
    in.open(filename.c_str());
    in.seekg(0,ios::end);
    size_t size = in.tellg();
    in.seekg(0,ios::beg);
    cout<<"Filename: "<<filename<<endl<<"Filesize: "<<size<<endl;
    mz_header h;
    in.read(reinterpret_cast<char *>(&h), sizeof(mz_header));
    vector<uint16_t> reloc_table;
    reloc_table.resize(h.reloc_entries*2);
    in.seekg(h.reloc_offset,ios::beg);
    in.read(reinterpret_cast<char *>(&(reloc_table[0])), h.reloc_entries*4);
    printf("Bytes on last page: %04x\nTotal pages: %04x\nRelocation entries: %d\nHeader size in paragraphs: %04x\nMin mem needed after exe: %04x\nMax mem needed after exe: %04x\nSS offset: %04x\nInitial SP: %04x\nChecksum: %04x\nEntry point: %04x:%04x\nOffset of relocation table: %04x\nOverlay number (expect 0): %04x\nCould be ID: %02x %02x %02x %02x\n",h.last_page_bytes,h.total_pages,h.reloc_entries,h.header_para_size,h.min_paras_after_code,h.max_paras_after_code,h.stack_seg_offset,h.initial_sp,h.chksum,h.init_cs,h.init_ip,h.reloc_offset,h.overlay_num,h.poss_id[0],h.poss_id[1],h.poss_id[2],h.poss_id[3]);
    if(h.poss_id[0] == 0x01 && h.poss_id[1] == 0x00 && h.poss_id[2] == 0xfb) {
        std::cout<<"Probably Borland TLink, version "<<h.poss_id[3]/16<<endl;
    }
    cout<<"Relocation table: "<<h.reloc_entries<<" entries"<<endl;
    bool odd=false;
    uint16_t offset=0;
    for(uint16_t off:reloc_table) {
        if(odd)
            printf("%04x:%04x\n",off,offset);
        else
            offset = off;
        odd=!odd;
    }
}
