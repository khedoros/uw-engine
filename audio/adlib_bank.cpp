//Parses Miles Sound System .bnk files

#include<string>
#include<iomanip>
#include "util.h"
using namespace std;

bool load(std::string fn) {
    binifstream infile;
    infile.open(fn.c_str(),std::ios::binary);
    infile.seekg(0,ios::end);
    size_t filesize = infile.tellg();
    infile.seekg(0,ios::beg);
    uint8_t maj_ver, min_ver, zero;
    char sig[7];
    sig[6]=0;
    infile>>maj_ver>>min_ver>>sig[0]>>sig[1]>>sig[2]>>sig[3]>>sig[4]>>sig[5];
    if(string(sig) != "ADLIB-") {
        cout<<"Suspected error. File signature doesn't match \"ADLIB-\"."<<endl;
        return false;
    }
    cout<<"Version: "<<int(maj_ver)<<"."<<int(min_ver)<<" signature: "<<sig<<endl;
    uint16_t records_used, records_total;
    uint32_t name_offset, data_offset;
    infile>>records_used>>records_total>>name_offset>>data_offset;
    const size_t HEADER_SIZE = 28, NAME_SIZE = 12, DATA_SIZE = 30;
    size_t records_to_parse = records_total;
    cout<<records_used<<"/"<<records_total<<" records used. Name offset: "<<name_offset<<" Data offset: "<<data_offset<<endl;
    if(filesize != HEADER_SIZE + records_total * (NAME_SIZE + DATA_SIZE)) {
        if(filesize != HEADER_SIZE + records_used * (NAME_SIZE + DATA_SIZE) && filesize != data_offset + records_used * DATA_SIZE) {
            cout<<"Suspected error. Expected filesize of "<<HEADER_SIZE + records_total * (NAME_SIZE + DATA_SIZE)<<" or "<<HEADER_SIZE + records_used * (NAME_SIZE + DATA_SIZE)<<", found "<<filesize<<endl;
            float guessed_records = filesize - HEADER_SIZE;
            guessed_records = guessed_records / float(NAME_SIZE + DATA_SIZE);
            cout<<"I'd guess that there are actually "<<guessed_records<<" present in the file (that's what it looks like I have data for)"<<endl;
            cout<<"I'll go ahead and try anyhow."<<endl;
        }
        else {
            cout<<"Slightly weird filesize (doesn't match the spec I read), but I think this is one of the alternate formats of the file. I'll go with it."<<endl;
            records_to_parse = records_used;
        }
    }
    
    for(int ii=0;ii<8;++ii) {
        infile>>zero;
        if(zero != 0) {
            cout<<"Suspected error. Expected 0, found "<<int(zero)<<" in the middle of what's supposed to be 0-padded data."<<endl;
            return false;
        }
    }

    infile.seekg(name_offset,ios::beg);
    for(size_t ii=0;ii<records_to_parse;++ii) {
        uint16_t data_index;
        uint8_t used; //0 is "not in use", contrary to some documentation I found 
        uint8_t name[10]; //null-terminated already
        name[9]=0;
        infile>>data_index>>used;
        infile.read(reinterpret_cast<char *>(&(name[0])),9);
        cout<<"Record #"<<dec<<ii<<" Index in data: "<<hex<<setw(4)<<data_index<<"   In use: "<<int(used)<<"  Name: "<<name;
        size_t bookmark = infile.tellg();
        infile.seekg(data_offset + DATA_SIZE * data_index, ios::beg);

        uint8_t type, voice_no, op0_params[13], op1_params[13], op0_wav, op1_wav;
        infile>>type>>voice_no;
        for(int jj=0;jj<13;++jj) infile>>op0_params[jj];
        for(int jj=0;jj<13;++jj) infile>>op1_params[jj];
        infile>>op0_wav>>op1_wav;
/*    Operator Wave types (0-3 are OPL2, 4-7 added in OPL3)
 *    0: Sine
 *    1: Half-Sine
 *    2: Absolute Value Sine
 *    3: Pulse Sine
 *    4: Sine, Even periods only
 *    5: Abs-Sine, Even periods only
 *    6: Square
 *    7: Derived Square
 *   
 */

/* Panning? Left, Right, Center. Don't see the entry. Listed in "carrier" slot only
 * Fine-Tune? Not a hardware property. Listed in "carrier" slot only
 *
 */

//3 bits + 2 bits + 37 bits = 42 bits per operator, sounds like

/*    Operator param names (in order)
 *    1       byte    Key scale level (0-3, 2 bits)
 *    1       byte    Frequency multiplier (0-15, 4 bits)
 *    1       byte    Feed back (0-7, 4 bits) (carrier slot only)
 *    1       byte    Attack rate (0-15, 4 bits)
 *    1       byte    Sustain level (0-15, 4 bits)
 *    1       byte    Sustaining sound (aka envelope gain. 0/1, 1 bit)
 *    1       byte    Decay rate (0-15, 4 bits)
 *    1       byte    Release rate (0-15, 4 bits)
 *    1       byte    Output level (0-63, 6 bits )
 *    1       byte    Amplitude vibrato (aka Tremolo. 0/1, 1 bit)
 *    1       byte    Frequency vibrato (Vibrato. 0/1, 1 bit)
 *    1       byte    Envelope scaling (aka Key Scale Rate? 0/1, 1 bit)
 *    1       byte    0=FM sound  1=Additive sound (0/1, 1 bit)
 *                    (last field for operator 0 only, unused for operator 1)
 */
      string type_str="melody";
      if(type == 1) type_str = "percussion";
      cout<<" Type: "<<setw(2)<<type_str<<" Voice #"<<int(voice_no)<<"\n\tOperator 0: Waveform: "<<int(op0_wav)<<" Params:";
      for(int jj=0;jj<13;++jj) cout<<" "<<hex<<setw(2)<<int(op0_params[jj]);
      cout<<"\n\tOperator 1: Waveform: "<<int(op1_wav)<<" Params:";
      for(int jj=0;jj<13;++jj) cout<<" "<<setw(2)<<int(op1_params[jj]);
      cout<<endl<<endl;
      infile.seekg(bookmark,ios::beg);
    }
    return true;
}

int main(int argc, char *argv[]) {
    load(std::string(argv[1]));
}
