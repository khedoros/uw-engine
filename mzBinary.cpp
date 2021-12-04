#include<iostream>
#include<fstream>
#include<stdint.h>
#include<assert.h>
#include<vector>
#include<string>
#include "mzBinary.h"
using namespace std;

void mzBinary::printHeaderInfo() {
	cout<<"Filename: "<<fileName<<endl<<"Filesize: "<<fileSize<<endl;
	printf("Bytes on last page: %04x\n"
		   "Total pages: %04x\n"
		   "Relocation entries: %d\n"
		   "Header size in paragraphs: %04x\n"
		   "Min mem needed after exe: %04x\n"
		   "Max mem needed after exe: %04x\n"
		   "SS offset: %04x\n"
		   "Initial SP: %04x\n"
		   "Checksum: %04x\n"
		   "Entry point: %04x:%04x\n"
		   "Offset of relocation table: %04x\n"
		   "Overlay number (expect 0): %04x\n"
		   "Could be ID: %02x %02x %02x %02x\n",header.last_page_bytes, header.total_pages, header.reloc_entries,
												header.header_para_size, header.min_paras_after_code, header.max_paras_after_code,
												header.stack_seg_offset, header.initial_sp,header.chksum, header.init_cs,header.init_ip,
												header.reloc_offset, header.overlay_num,
												header.poss_id[0], header.poss_id[1], header.poss_id[2], header.poss_id[3]);
	if(header.poss_id[0] == 0x01 && header.poss_id[1] == 0x00 && header.poss_id[2] == 0xfb) {
		std::cout<<"Probably Borland TLink, version "<<header.poss_id[3]/16<<endl;
	}
	cout<<"Relocation table: "<<header.reloc_entries<<" entries"<<endl;
	bool odd=false;
	uint16_t offset=0;
	for(auto off:relocs) {
		printf("%04x:%04x\n",off.first,off.second);
	}
}

mzBinary::mzBinary(const std::string& filename) : fileName(filename) {
	assert(sizeof(mz_header) == 32);
	ifstream in;
	in.open(filename.c_str());
	in.seekg(0,ios::end);
	size_t size = in.tellg();
	fileSize = size;
	in.seekg(0,ios::beg);
	in.read(reinterpret_cast<char *>(&header), sizeof(mz_header));
	in.seekg(header.reloc_offset,ios::beg);
	uint16_t reloc_entry[2];
	for(int i=0;i<header.reloc_entries;i++) {
		in.read(reinterpret_cast<char *>(&(reloc_entry[0])), 2 * sizeof(uint16_t));
		relocs.emplace(relocs.end(), std::make_pair(reloc_entry[0],reloc_entry[1]));
	}

	size_t binarySize = (header.total_pages - 1) * 512 + header.last_page_bytes;
	in.seekg(16 * header.header_para_size, ios::beg);
	main_binary.resize(binarySize);
	in.read(reinterpret_cast<char *>(main_binary.data()), binarySize);

	// findOverlayTable
	// read overlay descriptions into overlays, into ovrDesc
	// findOverlayDataBase
	// Use the table and base address to iterate through the overlays, reading them into "overlays" "overlayBinary" and "overlayRelocs" fields
	in.close(); 
}

size_t mzBinary::findOverlayTable() {
	size_t location = 0;
	for(int i = 0; i < main_binary.size(); i+=0x10) {
		
	}
	return location;
}

size_t mzBinary::findOverlayDataBase() {
	return 0;
}

size_t mzBinary::findOverlayBinary(int overlay) {
	return 0;
}

size_t mzBinary::findOverlayRelocs(int overlay) {
	return 0;
}

void mzBinary::dumpBinaryToFile(int index, std::string& filename, bool clearReloc) {}
void mzBinary::dumpOverlayToFile(int index, std::string& filename, bool clearReloc) {}

void mzBinary::clearRelocs();
void mzBinary::clearOvrRelocs(int overlay);
