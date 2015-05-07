#include "util.h"
#include "mapparser.h"
#include<iostream>

Map_Parser::Map_Parser() : valid(false), block_count(0), block_offsets(0), file_id(uw_map::unknown),
                           field_width(0), lev_dat(0)
{

}

bool Map_Parser::load(std::string filename) {
	binifstream map;
	map.open(filename.c_str(),std::ios::binary|std::ios::in);
	if(map.is_open()) {
		std::cout<<"Opened the file."<<std::endl;
	}
	else {
		std::cout<<"Failed to open the file."<<std::endl;
		return false;
	}
	map.seekg(0,std::ios::end);
	size_t filesize = map.tellg();
	map.seekg(0,std::ios::beg);
	map>>block_count;
	std::cout<<block_count<<" blocks in the file (total size: "<<filesize<<")"<<std::endl;

	switch(block_count) {
		case 0: std::cout<<"Looks like the demo's texture mappings. I'll treat it appropriately."<<std::endl;
            file_id = uw_map::uw1demo_tex;
            return process_tex_mappings(0);
		case 240: std::cout<<"Looks like the demo's tilemap/object list. I'll treat it appropriately."<<std::endl;
            file_id = uw_map::uw1demo_tilemap;
            return process_tilemap(0);
		case 42624: std::cout<<"Looks like the demo's animation overlay info. I'll treat it appropriately."<<std::endl;
            file_id = uw_map::uw1demo_anim_overlay;
            return process_anim_overlay(0);
		case 135: std::cout<<"Looks like UW1. I'll treat it appropriately."<<std::endl;
            file_id = uw_map::uw1map;
            field_width=9;
			break;
		case 320: std::cout<<"Looks like UW2. I'll treat it appropriately."<<std::endl;
            std::cout<<"TODO: Implement UW2 decompression. Sorry."<<std::endl;
            file_id = uw_map::uw2map;
            field_width=80;
            return false;
            break;
        default: std::cout<<"No idea what this file is. I choose to bail in a cowardly fashion."<<std::endl;
            return false;
	}
	block_offsets.resize(block_count,std::pair<uint32_t,uint32_t>(0,0));
	lev_dat.resize(field_width);
	std::vector<uw_map::level_section> sects;
	sects.resize(block_count, uw_map::ls_none);

	size_t last_index=0;
	size_t real_count = 0;
	for(int i=0;i<block_count;++i) {
        sects[i]=get_section_type(i);
		map>>block_offsets[i].first;
		if(block_offsets[i].first != 0) {
			if(real_count != 0) {
                block_offsets[i-1].second = block_offsets[i].first - block_offsets[last_index].first;
				std::cout<<" Size: "<<block_offsets[i-1].second<<std::endl;
			}
			last_index=i;
			std::cout<<"Block #"<<real_count<<" ("<<i<<") ("<<ls_strings[sects[i]]<<") Offset: "<<block_offsets[i].first;
			real_count++;
		} else {
            //std::cout<<"Empty block #"<<i<<std::endl;
		}
	}
	block_offsets[last_index].second = filesize - block_offsets[last_index].first;
	std::cout<<" Size: "<<block_offsets[last_index].second<<std::endl;

    lev_dat.resize(field_width);

	for(int i=0;i<block_count;++i) {
        if(block_offsets[i].first != 0) {
            switch(sects[i]) {
            case uw_map::level_section::tile_map:
                break;
            case uw_map::level_section::anim_overlay:
                break;
            case uw_map::level_section::tex_map:
                break;
            case uw_map::level_section::map_info:
                break;
            case uw_map::level_section::map_notes:
                break;
            case uw_map::level_section::ls_none:
                return false;
            }
            //Go through all valid blocks, and parse them into the appropriate structures
        }
	}

	return true;

}

uw_map::level_section Map_Parser::get_section_type(size_t index) {
        if(file_id == uw_map::uw1map) {
            switch(index/field_width) {
            case 0: return uw_map::tile_map;
            case 1: return uw_map::anim_overlay;
            case 2: return uw_map::tex_map;
            case 3: return uw_map::map_info;
            case 4: return uw_map::map_notes;
            default: return uw_map::ls_none;
            }
        } else if(file_id == uw_map::uw2map) {
            switch(index/field_width) {
            case 0: return uw_map::tile_map;
            case 1: return uw_map::tex_map;
            case 2: return uw_map::map_info;
            case 3: return uw_map::map_notes;
            default: return uw_map::ls_none;
            }
        }
        return uw_map::ls_none;
}

bool Map_Parser::process_tex_mappings(size_t offset) {
    std::cout<<"TODO: Implement parsing of texture mappings."<<std::endl;
    return false;
}

bool Map_Parser::process_tilemap(size_t offset) {
    std::cout<<"TODO: Implement parsing of the tilemaps."<<std::endl;
    return false;
}

bool Map_Parser::process_anim_overlay(size_t offset) {
    std::cout<<"TODO: Implement parsing of the animation overlays."<<std::endl;
    return false;
}

int main(int argc, char *argv[]) {
    std::cout<<"Using the map_parser class."<<std::endl;
    Map_Parser mp;
    if(argc>=2) {
        std::cout<<"Reading "<<argc-1<<" files."<<std::endl;
        for(int i=1;i<argc;++i)
            if(!mp.load(argv[i])) {
                break;
            } else {
                std::cout<<"Loaded "<<argv[i]<<"."<<std::endl;
            }
    }
    else {
        std::cout<<"You need to provide at least one file as an argument";
    }
    return 0;
}
