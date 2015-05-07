#ifndef MAPPARSER_H
#define MAPPARSER_H
#include "util.h"
#include<vector>

namespace uw_map {
	typedef enum {
        uw1demo_tilemap,
        uw1demo_anim_overlay,
		uw1demo_tex,
		uw1map,
		uw2map,
		unknown
	} filetype;

		typedef enum {
        tile_map,
        anim_overlay,
        tex_map,
        map_info,
        map_notes,
        ls_none
	} level_section;

    typedef struct {
        uint8_t tile_type;
        uint8_t height;
        uint8_t floor_tex;
        bool magic_allowed;
        bool door;
        uint8_t wall_tex;
        uint16_t obj_index;
    } tilemap;
    typedef struct {

    } overlay;
    typedef struct {
    } texmap;
    typedef struct {
    } mapinfo;
    typedef struct {
    } mapnotes;


}

class Map_Parser {
public:
	Map_Parser();
	bool load(std::string filename);
	//TODO: Provide some kind of accessor functions
private:
    bool process_tex_mappings(size_t offset);
    bool process_tilemap(size_t offset);
    bool process_anim_overlay(size_t offset);
    bool process_automap(size_t offset);
    bool process_map_notes(size_t offset);
	bool valid;
	uint16_t block_count;
	std::vector<std::pair<uint32_t/*offset*/,uint32_t/*length*/>> block_offsets;

	uw_map::filetype file_id;
	size_t field_width;

    class level_data {
        uw_map::tilemap tile;
        uw_map::overlay ol;
        uw_map::texmap tex;
        uw_map::mapinfo mi;
        uw_map::mapnotes mn;
    };

	std::vector<level_data> lev_dat;

	const std::string ls_strings[5] = {std::string("Tile Map"),
	                                   std::string("Animation Overlay"),
	                                   std::string("Texture Map"),
	                                   std::string("Automap Info"),
	                                   std::string("Map Notes")};

    uw_map::level_section get_section_type(size_t index);
};
#endif
