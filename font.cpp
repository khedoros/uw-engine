#include<iostream>
#include<fstream>
#include<string>
#include<stdint.h>
#include<vector>

using namespace std;

uint8_t read8(ifstream& in) {
    uint8_t ret = 0;
    in.read(reinterpret_cast<char *>(&ret), 1);
    return ret;
}

uint16_t read16(ifstream& in) {
    uint16_t ret = 0;
    in.read(reinterpret_cast<char *>(&ret), 2);
    return ret;
}

uint32_t read32(ifstream& in) {
    uint32_t ret = 0;
    in.read(reinterpret_cast<char *>(&ret), 4);
    return ret;
}

class fontfile {
    public:
    vector<vector<vector<uint8_t>>> font;
    vector<uint8_t> widths;

    bool load(string fn) {
        ifstream in;
        in.open(fn.c_str(), ios::binary|ios::in);
        if(!in.is_open()) {
            cerr<<"Failed to open "<<fn<<endl;
            return false;
        }
        in.seekg(0, ios::end);
        uint32_t filesize = in.tellg();
        in.seekg(0, ios::beg);
        uint16_t pad1 = read16(in);
        uint16_t bytes_per_char = read16(in); //in bytes
        uint16_t space_width = read16(in); //in pixels
        uint16_t font_height = read16(in); //in pixels
        uint16_t font_width = read16(in); //in bytes
        uint16_t max_char_width = read16(in); //in pixels
        uint32_t char_count = (filesize - 12) / (bytes_per_char + 1);
        if(pad1 != 1 || filesize != char_count * (bytes_per_char + 1) + 12) {
            cerr<<"File doesn't look right. Bailing."<<endl;
            return false;
        }
        cout<<font_height<<"x"<<max_char_width<<" font with "<<char_count<<" defined glyphs."<<endl;

        font.resize(char_count);
        widths.resize(char_count);

        for(size_t glyph = 0; glyph < char_count; ++glyph) {
            font[glyph].resize(font_height);
            //cout<<"Doing glyph "<<glyph<<endl;
            for(size_t row = 0; row < font_height; ++row) {
                font[glyph][row].resize(font_width);
                //cout<<"Doing row "<<row<<endl;
                for(size_t col = 0; col < font_width; ++col) {
                    //cout<<"Doing column "<<col<<endl;
                    font[glyph][row][col] = read8(in);
                    //cout<<"Read column "<<col<<endl;
                }
                //cout<<"Did row "<<row<<endl;
            }
            //cout<<"Did glyph "<<glyph<<endl;
            //cout<<"Reading the width of glyph "<<glyph<<" (vector has size "<<widths.size()<<")"<<endl;
            widths[glyph] = read8(in);
            //cout<<"This char has width "<<widths[glyph]<<endl;
        }

        cout<<"Ready to print out my results."<<endl;
        for(size_t g = 0; g < char_count; ++g) {
            if(widths[g] == 0) continue;
            cout<<"Char "<<g<<":"<<endl;
            for(size_t r = 0; r < font_height; ++r) {
                for(size_t c = 0; c < font_width; ++c) {
                    for(int i=128;i>0;i/=2)
                        if((font[g][r][c] & i) > 0) cout<<"##";
                        else cout<<"  ";
                }
                cout<<endl;
            }
            cout<<endl;
        }
            
        return true;
    }
};
int main(int argc, char* argv[]) {
    fontfile in;
    if(argc == 2) {
        if(!in.load(string(argv[1]))) {
            cerr<<"Couldn't open the file "<<argv[1]<<endl;
            return 1;
        }
    }
    else {
        cerr<<"Provide the path to the font file to open."<<endl;
        return 1;
    }
    cout<<"Loaded the file successfully."<<endl;
    return 0;
}
