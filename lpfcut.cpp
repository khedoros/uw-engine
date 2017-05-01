#include "lpfcut.h"

Lpfcut::Lpfcut(bool rep/*= true*/) : repeat(rep) {
    file_id[4]=0;
    content_type[4]=0;
    //repeat=true;
}

const uint8_t * Lpfcut::getNextFrame() {
    if(frame.size() == 0)
        return NULL;

    auto buffer_it = frame.begin();
    size_t page_index = lp_order[frame_page_index];
    std::vector<unsigned char> framestream = descs[page_index].rec_data[frame_record_index];
    if(framestream.size()>0) {
        //std::cout<<"Showing LP "<<page_index<<", record "<<frame_record_index<<std::endl;
    }
    else {
        //std::cout<<"I'm skipping over LP "<<page_index<<", record "<<frame_record_index<<", because it's empty."<<std::endl;
        if(iterate_frame()) {
            return &(frame[0]);
        }
        else {
            return NULL;
        }
    }

    auto it = framestream.begin();

    ++it; //Skip first byte; I don't know what it does
    uint8_t flag = *it++;
    int16_t offset = *it++;
    int16_t offset_2 = *it++;
    offset_2 <<=(8);
    offset |= offset_2;

    if(flag == 0) {
        //std::cout<<"Flag is "<<(unsigned int)flag<<", and the extra offset is "<<offset<<", but I don't know what that means."<<std::endl;
/*
        for(int i=0;i<offset;++i)
            ++it;*/
    }

    for(/*auto it = framestream.begin()*/; it != framestream.end(); /*++it*/) {
        int8_t val = *it++; //Flag that is a command, a length, or a color value.
        if(val > 0) { //short pixel "dump"
            //std::cout<<"Short_dump(Length: "<<(unsigned int)val<<")."<<std::endl;
            for(int i = 0; i < val; ++i) {
                *buffer_it++ = pal[*it].r;
                *buffer_it++ = pal[*it].g;
                *buffer_it++ = pal[*it].b;
                ++buffer_it;
                ++it;
            }
        }
        else if(val == 0) { //short pixel "run"
            uint8_t count = *it++;
            uint8_t data = *it++;
            //std::cout<<"Short_run(Length: "<<(unsigned int)count<<", Value: "<<(unsigned int)data<<")."<<std::endl;
            for(int i = 0; i < count; ++i) {
                *buffer_it++ = pal[data].r;
                *buffer_it++ = pal[data].g;
                *buffer_it++ = pal[data].b;
                ++buffer_it;
            }
        }
        else { //short skip, or one of the long runs
            val &= 0x7f;
            if(val != 0) { //short pixel "skip"
                //std::cout<<"Short_skip(Length: "<<(unsigned int)val<<")."<<std::endl;
                for(int i = 0; i < val; ++i) {
                    ++buffer_it;
                    ++buffer_it;
                    ++buffer_it;
                    ++buffer_it;
                }
            }
            else { //long runs
                int16_t val16 = *it++;
                int16_t val16_2 = *it++;
                val16_2 <<=(8);
                val16 |= val16_2;
                if(val16 > 0) {//long pixel "skip"
                    //std::cout<<"Long_skip(Length: "<<(unsigned int)val16<<")."<<std::endl;
                    for(int i = 0; i < val16; ++i) {
                        ++buffer_it;
                        ++buffer_it;
                        ++buffer_it;
                        ++buffer_it;
                    }
                }
                else if(val16 == 0) {
                    //std::cout<<"Break out of the for loop."<<std::endl;
                    break;
                }
                else if(val16 < 0) {
                    val16 &= 0x7fff;
                    if(val16 >= 0x4000) { //long pixel "run"
                        uint8_t data = *it++;
                        //std::cout<<"Long_run(Length: "<<(unsigned int)val16<<", Value: "<<(unsigned int)data<<")."<<std::endl;
                        for(int i = 0; i < val16 - 0x4000; ++i) {
                            *buffer_it++ = pal[data].r;
                            *buffer_it++ = pal[data].g;
                            *buffer_it++ = pal[data].b;
                            ++buffer_it;
                        }
                    }
                    else {
                        //std::cout<<"Long_dump(Length: "<<(unsigned int)val16<<")."<<std::endl;
                        for(int i = 0; i < val16; ++i) { //long pixel dump
                            *buffer_it++ = pal[*it].r;
                            *buffer_it++ = pal[*it].g;
                            *buffer_it++ = pal[*it].b;
                            ++buffer_it;
                            ++it;
                        }
                    }
                }
            }
        }
        //std::cout<<"Index after iteration: "<<index<<std::endl;
    }
/*
    buffer_it = frame.begin();
    for(size_t y = 0; y < height; ++y) {
        for(size_t x = 0; x < width; ++x) {
            if(x<256) {
                *buffer_it++ = pal[x].r;
                *buffer_it++ = pal[x].g;
                *buffer_it++ = pal[x].b;
                ++buffer_it;
            }
            else {
                *buffer_it++ = pal[0].r;
                *buffer_it++ = pal[0].g;
                *buffer_it++ = pal[0].b;
                ++buffer_it;
            }
        }
    }
  */  
    if(iterate_frame()) {
        return &(frame[0]);
    }
    else {
        return NULL;
    }

}

color Lpfcut::getPalEntry(int i) {
    if(i<256&&i>0) return pal[i];
    else return color();
}

bool Lpfcut::load(std::string filename) {
    binifstream filein;
    filein.open(filename.c_str());
    if(!filein.is_open()) {
        std::cout<<"Failed to open cutscene file \""<<filename<<"\" (File could not be read.)"<<std::endl;
        frame.resize(0);
        return false;
    }
    uint8_t throw_away_byte;
    uint16_t throw_away_word;
    uint32_t throw_away_dword;

    //Read file id "magic bytes"
    filein.read(&file_id[0], 4);

    //Throw away 2 bytes of padded data
    filein>>throw_away_word;

    //Read values for full counts of long pages and records
    filein>>lp_count>>rec_count;

    //Prepare vector that will hold playback order information
    lp_order.resize(lp_count);
    filein>>throw_away_dword;

    //Read content type string and the width and height variables
    filein.read(&content_type[0],4);
    filein>>width>>height;
    std::cout<<"FileID (expecting \"LPF \"): "<<file_id<<"\nLP's in file: "<<lp_count<<"\nRecords in file: "<<rec_count<<"\nContent Type (expecting \"ANIM\"): "<<content_type<<"\nWidth: "<<width<<"\nHeight: "<<height<<std::endl;

    if(std::string(file_id) != "LPF " || std::string(content_type) != "ANIM") {
        //std::cout<<"The opening file tags don't look right. This probably isn't an LPF animation file."<<std::endl;
        frame.resize(0);
        std::cout<<"Failed to open cutscene file \""<<filename<<"\" (File contains data in an unknown format.)"<<std::endl;
        return false;
    }

    //Read palette information (skip unused header data)
    filein.seekg(PAL_OFFSET,std::ios::beg);
    for(int i=0;i<256;++i) {
        filein>>pal[i].b>>pal[i].g>>pal[i].r>>throw_away_byte;
        /*std::printf("Col#%03d:(%02x,%02x,%02x) ", i, int(pal[i].r), int(pal[i].g), int(pal[i].b));
        if((i+1)%4==0)
            std::cout<<std::endl;*/
    }

    //Read large page descriptors, which occur after the header and palette data
    filein.seekg(LPD_OFFSET,std::ios::beg);
    for(int i=0;i<256;++i) {
        filein>>descs[i].first_record>>descs[i].lp_rec_count>>descs[i].data_size;
        //If the data seems valid, then make
        if(descs[i].first_record!=0||descs[i].lp_rec_count!=0||descs[i].data_size!=0) {
            //std::printf("LPD#%03d: First: %05d Count: %05d Size: %05d\n",i,descs[i].first_record,descs[i].lp_rec_count,descs[i].data_size);
            descs[i].rec_len.resize(descs[i].lp_rec_count);
            descs[i].rec_data.resize(descs[i].lp_rec_count);
            if(descs[i].first_record==0)
                lp_order[0]=i;
        }
    }

    //Find the "in-order" traversal of the LPDs
    size_t last_found_rec=0;
    std::cout<<"Record 0: Starts at: 0 Index: "<<lp_order[0]<<std::endl;
    for(int i=1;i<lp_count;++i) {
        size_t lowest_found=65536; //arbitrarily large number that handles all the examples I have
        size_t lowest_index=0;
        for(int j=0;j<lp_count;++j) {
            if(descs[j].first_record<lowest_found&&descs[j].first_record>last_found_rec) {
                lowest_found=descs[j].first_record;
                lowest_index=j;
            }
        }
        last_found_rec=lowest_found;
        lp_order[i]=lowest_index;
        std::cout<<"Record "<<i<<": Starts at: "<<last_found_rec<<" Index: "<<lowest_index<<std::endl;
    }

    //Start loading large pages
    for(int pnum = 0; pnum < lp_count; ++pnum) {

        //Verify that the header matches The second copy
        lpdesc *curdesc = &descs[pnum];
        filein.seekg(FIRST_LP_OFFSET + pnum * LP_SIZE, std::ios::beg);

        //Just used "throw_away" because I don't intend to store these values long-term.
        filein>>throw_away_word;
        if(throw_away_word!=curdesc->first_record) {
            //std::printf("LP index doesn't match. Expected: %d Found: %d\n",curdesc->first_record,throw_away_word);
        }

        filein>>throw_away_word;
        if(throw_away_word!=curdesc->lp_rec_count) {
            //std::printf("LP record count doesn't match. Expected: %d Found: %d\n",curdesc->lp_rec_count,throw_away_word);
        }

        filein>>throw_away_word;
        if(throw_away_word!=curdesc->data_size) {
            //std::printf("LP data size doesn't match. Expected: %d Found: %d\n",curdesc->data_size,throw_away_word);
        }

        //Empty value; no known purpose
        filein>>throw_away_word;

        //location is the beginning of the data for the records (??? not positive about the extra 8 bytes)
        size_t location = FIRST_LP_OFFSET + pnum * LP_SIZE + curdesc->lp_rec_count * 2 + 8/*lp header size*/;

        //Read the record sizes and data
        for(int j=0;j<curdesc->lp_rec_count;++j) {
            filein>>curdesc->rec_len[j];
            //std::printf("Record #%d: Offset: %llu Length: %d\n", j, location, curdesc->rec_len[j]);
            curdesc->rec_data[j].resize(curdesc->rec_len[j]);

            //Save where I am in the table of data lengths
            size_t bookmark=filein.tellg();

            //Jump to read the data into RAM
            filein.seekg(location, std::ios::beg);
            //std::printf("Reading %d bytes to descs[%d]->rec_data[%d].\n",curdesc->rec_len[j], pnum, j);
            filein.read(reinterpret_cast<char *>(&(curdesc->rec_data[j][0])), curdesc->rec_len[j]);

            //Jump back to the table of record lengths
            filein.seekg(bookmark, std::ios::beg);

            //Make sure I've got the location of the next segment of data to read
            location += curdesc->rec_len[j];
        }

        if(location - (FIRST_LP_OFFSET + pnum * LP_SIZE + curdesc->lp_rec_count * 2) != curdesc->data_size) {
            //std::printf("Expected data size of %d, found data size of %llu\n",curdesc->data_size, location - (FIRST_LP_OFFSET + pnum * LP_SIZE + curdesc->lp_rec_count * 2));
        }
        else {
            //std::printf("Found data matching expected size.\n");
        }
    }
    frame.resize(height*width*4, 255);
    frame_page_index=0;
    frame_record_index=0;
    return true;
}

bool Lpfcut::iterate_frame() {
    //std::cout<<"Old page: "<<frame_page_index<<" record: "<<frame_record_index<<std::endl;
    frame_record_index++;
    if(frame_record_index >= descs[lp_order[frame_page_index]].lp_rec_count) {
        frame_record_index = 0;
        frame_page_index++;
        if(frame_page_index >= lp_count) {
            frame_page_index = 0;
            if(!repeat)
                return false;
        }
    }
    return true;
    //std::cout<<"New page: "<<frame_page_index<<" record: "<<frame_record_index<<std::endl;
}


