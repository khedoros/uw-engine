#include<cstdint>
#include<string>
#include<utility>
#include<vector>
#include<fstream>

typedef struct {
	char magic[2];
	uint16_t last_page_bytes;
	uint16_t total_pages;
	uint16_t reloc_entries;
	uint16_t header_para_size;
	uint16_t min_paras_after_code;
	uint16_t max_paras_after_code;
	uint16_t stack_seg_offset;
	uint16_t initial_sp;
	uint16_t chksum;
	uint16_t init_ip;
	uint16_t init_cs;
	uint16_t reloc_offset;
	uint16_t overlay_num;
	uint8_t poss_id[4];
} mz_header;

typedef struct {
	uint8_t int_code[2];		// Overlay manager interrupt
	uint16_t memswap;		   // Runtime memory swap address
	uint32_t fileoff;		   // Offset in the file to the code
	uint16_t codesize;		  // Code size
	uint16_t relsize;		   // Relocation area size
	uint16_t nentries;		  // Number of overlay entries
	uint16_t prevstuboff;	   // Previous stub
	uint16_t prevstubseg;
	uint16_t nextstuboff;
	uint16_t nextstubseg;
} overlay_description;

typedef struct {
	int overlayIndex;
	overlay_description ovrDesc;
	std::vector<uint8_t> overlayBinary;
    std::vector<uint16_t> overlayFuncs;
	std::vector<uint16_t> overlayRelocs;
} overlay_data;

class mzBinary {
	public:
	mzBinary(const std::string& filename);
	void printHeaderInfo(bool printRelocs = false);
	void clearRelocs();
	void clearOvrRelocs(int overlay);
	void dumpBinaryToFile(int index, std::string& filename, bool clearReloc);
	void dumpOverlayToFile(int index, std::string& filename, bool clearReloc);
	private:
	std::string fileName;
	size_t fileSize;
	mz_header header;
	std::vector<std::pair<uint16_t, uint16_t>> relocs;
	std::vector<uint8_t> main_binary;
	std::vector<overlay_data> overlays;
	size_t findOverlayTable();
	size_t findOverlayDataBase(std::ifstream& in);
	size_t findOverlayBinary(int overlay);
	size_t findOverlayRelocs(int overlay);
};
