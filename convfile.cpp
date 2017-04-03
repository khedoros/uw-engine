#include<iostream>
#include "convfile.h"
#include "util.h"

//Implements the conversation program loader

bool convfile::load(std::string& cnvfile) {
    binifstream in;
    in.open(cnvfile.c_str());
    if(!in.is_open()) {
        std::cerr<<"Couldn't open "<<cnvfile<<". Aborting."<<std::endl;
        return false;
    }
    uint16_t cnv_count;
    in>>cnv_count;
    std::cout<<cnv_count<<" conversations in this file."<<std::endl;
    std::vector<uint32_t> offsets;
    offsets.resize(cnv_count);
    convos.resize(cnv_count);
    for(uint32_t i=0;i<cnv_count;++i) {
        offsets[i] = read32(in);
        if(offsets[i] != 0) {
            std::cout<<"Conversation #"<<i<<" offset: "<<offsets[i]<<std::endl;
            size_t bookmark = in.tellg();
            in.seekg(offsets[i], std::ios::beg);

            uint32_t dummy32=0;
            uint16_t dummy16=0;
            in>>dummy32;
            if(dummy32 != 0x00000828) {
                std::cout<<"Improper magic number at offset "<<offsets[i]<<std::endl;
                std::cout<<"Found "<<std::hex<<dummy32<<std::endl;
                return false;
            }
            uint16_t code_size = read16(in);
            std::cout<<"Code is "<<code_size<<" 16-bit words long."<<std::endl;
            convos[i].code.resize(code_size);
            in>>dummy32;
            uint16_t convo_string_block = read16(in);
            convos[i].string_block = convo_string_block;
            uint16_t var_mem_slots = read16(in);
            convos[i].var_slots = var_mem_slots;
            uint16_t imported_globals = read16(in);
            //convos[i].globals.resize(imported_globals); This needs split into variables and functions
            std::cout<<"String block: "<<convo_string_block<<" variable memory: "<<var_mem_slots<<" # of imported globals: "<<imported_globals<<std::endl;
            for(uint16_t j = 0; j < imported_globals; ++j) {
                char import_name[64] = {0};
                uint16_t name_length = read16(in);
                in.read(&(import_name[0]), ((name_length < 63)?name_length:63));
                uint16_t import_id = read16(in);
                in>>dummy16;
                if(dummy16 != 1) {
                    std::cout<<"Imported global doesn't have the right magic number"<<std::endl;
                }
                uint16_t itype = read16(in);
                uint16_t rtype = read16(in);
                if(itype == 0x10f)
                    std::cout<<"\tVar \""<<import_name<<"\" address "<<import_id<<" type "<<std::hex<<rtype<<std::dec<<std::endl;
                else if(itype == 0x111)
                    std::cout<<"\tFunct \""<<import_name<<"\" number "<<import_id<<" type "<<std::hex<<rtype<<std::dec<<std::endl;
                else
                    std::cerr<<"Unknown import type!"<<std::endl;
            }
            std::vector<uint16_t> *binary = &(convos[i].code);
            binary->resize(code_size);
            uint32_t addr = 0;
            for(auto it = binary->begin(); it != binary->end(); ++it) {
                std::cout<<std::hex<<addr<<std::dec;
                addr++;
                *it = read16(in);
                switch(*it) {
                case 0x00:
                    std::cout<<"\tNOP"<<std::endl;
                    break;
                case 0x01:
                    std::cout<<"\tOPADD"<<std::endl;
                    break;
                case 0x02:
                    std::cout<<"\tOPMUL"<<std::endl;
                    break;
                case 0x03:
                    std::cout<<"\tOPSUB"<<std::endl;
                    break;
                case 0x04:
                    std::cout<<"\tOPDIV"<<std::endl;
                    break;
                case 0x05:
                    std::cout<<"\tOPMOD"<<std::endl;
                    break;
                case 0x06:
                    std::cout<<"\tOPOR"<<std::endl;
                    break;
                case 0x07:
                    std::cout<<"\tOPAND"<<std::endl;
                    break;
                case 0x08:
                    std::cout<<"\tOPNOT"<<std::endl;
                    break;
                case 0x09:
                    std::cout<<"\tTSTGT"<<std::endl;
                    break;
                case 0x0a:
                    std::cout<<"\tTSTGE"<<std::endl;
                    break;
                case 0x0b:
                    std::cout<<"\tTSTLT"<<std::endl;
                    break;
                case 0x0c:
                    std::cout<<"\tTSTLE"<<std::endl;
                    break;
                case 0x0d:
                    std::cout<<"\tTSTEQ"<<std::endl;
                    break;
                case 0x0e:
                    std::cout<<"\tTSTNE"<<std::endl;
                    break;
                case 0x0f:
                    ++it;
                    *it = read16(in);
                    std::cout<<"\tJMP "<<std::hex<<*it<<std::dec<<std::endl<<std::endl;
                    addr++;
                    break;
                case 0x10:
                    ++it;
                    *it = read16(in);
                    std::cout<<"\tBEQ "<<std::hex<<*it<<std::dec<<std::endl;
                    addr++;
                    break;
                case 0x11:
                    ++it;
                    *it = read16(in);
                    std::cout<<"\tBNE "<<std::hex<<*it<<std::dec<<std::endl;
                    addr++;
                    break;
                case 0x12:
                    ++it;
                    *it = read16(in);
                    std::cout<<"\tBRA "<<std::hex<<*it<<std::dec<<std::endl<<std::endl;
                    addr++;
                    break;
                case 0x13:
                    ++it;
                    *it = read16(in);
                    std::cout<<"\tCALL "<<std::hex<<*it<<std::dec<<std::endl;
                    addr++;
                    break;
                case 0x14:
                    ++it;
                    *it = read16(in);
                    std::cout<<"\tCALLI "<<std::hex<<*it<<std::dec<<std::endl;
                    addr++;
                    break;
                case 0x15:
                    std::cout<<"\tRET\n"<<std::endl;
                    break;
                case 0x16:
                    ++it;
                    *it = read16(in);
                    std::cout<<"\tPUSHI "<<std::hex<<*it<<std::dec<<std::endl;
                    addr++;
                    break;
                case 0x17:
                    ++it;
                    *it = read16(in);
                    std::cout<<"\tPUSHI_EFF "<<std::hex<<*it<<std::dec<<std::endl;
                    addr++;
                    break;
                case 0x18:
                    std::cout<<"\tPOP"<<std::endl;
                    break;
                case 0x19:
                    std::cout<<"\tSWAP"<<std::endl;
                    break;
                case 0x1a:
                    std::cout<<"\tPUSHBP"<<std::endl;
                    break;
                case 0x1b:
                    std::cout<<"\tPOPBP"<<std::endl;
                    break;
                case 0x1c:
                    std::cout<<"\tSPTOBP"<<std::endl;
                    break;
                case 0x1d:
                    std::cout<<"\tBPTOSP"<<std::endl;
                    break;
                case 0x1e:
                    std::cout<<"\tADDSP"<<std::endl;
                    break;
                case 0x1f:
                    std::cout<<"\tFETCHM"<<std::endl;
                    break;
                case 0x20:
                    std::cout<<"\tSTO"<<std::endl;
                    break;
                case 0x21:
                    std::cout<<"\tOFFSET"<<std::endl;
                    break;
                case 0x22:
                    std::cout<<"\tSTART"<<std::endl;
                    break;
                case 0x23:
                    std::cout<<"\tSAVE_REG"<<std::endl;
                    break;
                case 0x24:
                    std::cout<<"\tPUSH_REG"<<std::endl;
                    break;
                case 0x25:
                    std::cout<<"\tSTRCMP"<<std::endl;
                    break;
                case 0x26:
                    std::cout<<"\tEXIT_OP\n"<<std::endl;
                    break;
                case 0x27:
                    std::cout<<"\tSAY_OP"<<std::endl;
                    break;
                case 0x29:
                    std::cout<<"\tOPNEG"<<std::endl;
                    break;
                default: std::cerr<<"Unknown opcode \""<<std::hex<<*it<<std::dec<<"\""<<std::endl;
                }
            }
            
            in.seekg(bookmark, std::ios::beg);
        }
    }
    return true;
}

#ifdef STAND_ALONE
int main(int argc, char *argv[]) {
    bool retval = false;
    std::string cnvfile;
    if(argc == 2) {
        convfile cnv;
        cnvfile = argv[1];
        retval = cnv.load(cnvfile);
    }
    if(!retval || argc != 2) {
        std::cerr<<"Provide a conversation archive in your argument."<<std::endl;
        return 1;
    }
    return 0;
}
#endif
