#include<iostream>
#include<sstream>
#include<iomanip>
#include<fstream>
#include<string>
#include<stdint.h>
#include<vector>
#include "util.h"
#include "fontfile.h"

#ifdef STAND_ALONE
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

using namespace std;
#endif

bool fontfile::load(std::string fn) {
    std::ifstream in;
    in.open(fn.c_str(), std::ios::binary|std::ios::in);
    if(!in.is_open()) {
        std::cerr<<"Failed to open "<<fn<<std::endl;
        return false;
    }
    filename = fn;
    in.seekg(0, std::ios::end);
    uint32_t filesize = in.tellg();
    in.seekg(0, std::ios::beg);
    uint16_t pad1 = read16(in);
    uint16_t bytes_per_char = read16(in); //in bytes
    /*uint16_t*/ space_width = read16(in); //in pixels
    /*uint16_t*/ font_height = read16(in); //in pixels
    uint16_t font_width = read16(in); //in bytes
    /*uint16_t*/ max_char_width = read16(in); //in pixels
    uint32_t char_count = (filesize - 12) / (bytes_per_char + 1);
    if(pad1 != 1 || filesize != char_count * (bytes_per_char + 1) + 12) {
        std::cerr<<"File doesn't look right. Bailing."<<std::endl;
        return false;
    }
    std::cout<<font_height<<"x"<<max_char_width<<" font with "<<char_count<<" defined glyphs."<<std::endl;

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
        std::cout<<"This char has width "<<int(widths[glyph])<<std::endl;
    }

    return true;
}

void fontfile::print() {
    uint16_t char_count = font.size();
    for(size_t g = 0; g < char_count; ++g) {
        if(widths[g] == 0) continue;
        cout<<"Char "<<g<<"("<<int(widths[g])<<" wide) :"<<endl;
        uint16_t font_width = widths[g];
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
}

std::string fontfile::to_bdf() {
    std::ostringstream bdf_repr;
    bdf_repr << std::dec;
    bdf_repr << "STARTFONT 2.2\n";
    bdf_repr << "FONT UWFONT\n";
    bdf_repr << "SIZE 10 72 72" << "\n"; //10 point font, 75 dpi x, 62.5 dpi y (320x200 -> 4:3 aspect ratio)
    bdf_repr << "FONTBOUNDINGBOX " << max_char_width << " " << font_height << " 0 0\n";
    bdf_repr << "STARTPROPERTIES 4\nFONT_ASCENT "<<font_height<<"\nFONT_DESCENT 0\nCHARSET_REGISTRY \"ISO8859\"\nCHARSET_ENCODING \"1\"\nENDPROPERTIES\n";
    bdf_repr << "CHARS " << font.size() << "\n";
    for(int glyph = 0; glyph < font.size() ; ++glyph) {
        bdf_repr << "STARTCHAR U+00"<<std::hex<<((glyph < 16)?"0":"")<<glyph<<"\n"<<std::dec;
        bdf_repr << "ENCODING "<<glyph<<"\n";
        int width_temp = ((widths[glyph])?widths[glyph]:max_char_width);
        int swidthx = /*500;*/(width_temp * 1000);
        //cout<< (float(swidthx)/1000.0) * (75.0/72.0) << "\t"<<swidthx <<"\t"<<width_temp<<endl;
        //bdf_repr << "SWIDTH "<<swidthx<<" 0" <<"\n";
        bdf_repr << "DWIDTH "<<width_temp<<" 0\n";
        bdf_repr << "BBX "<<width_temp<<" "<<font_height<<" 0 0\n";
        bdf_repr << "BITMAP\n"<<std::hex;
        for(int row = 0; row < font[glyph].size();++row) {
            for(int byte = 0; byte < font[glyph][row].size(); ++byte) {
                bdf_repr << std::hex << std::setfill('0') << std::setw(2) << int(font[glyph][row][byte]);
            }
            bdf_repr << "\n";
        }
        bdf_repr << "ENDCHAR\n";
    }
    /*
    for(int glyph = font.size(); glyph < 256; ++glyph) {
        bdf_repr <<"STARTCHAR U+00"<<std::hex<<((glyph < 16)?"0":"")<<glyph<<"\n"<<std::dec;
        bdf_repr << "ENCODING "<<glyph<<"\n";
        int width_temp = max_char_width;
        int swidthx = 500;
        //bdf_repr << "SWIDTH "<<swidthx<<" 0" <<"\n";
        //bdf_repr << "DWIDTH "<<width_temp<<" 0\n";
        bdf_repr << "BBX "<<width_temp<<" "<<font_height<<" 0 0\n";
        bdf_repr << "BITMAP\n"<<std::hex;
        for(int row = 0; row < font_height;++row) {
            for(int byte = 0; byte < (max_char_width / 8 + 1); ++byte) {
                bdf_repr << hex << setfill('0') << setw(2) << 0;
            }
            bdf_repr << "\n";
        }
        bdf_repr << "ENDCHAR\n";
    }
    */
    bdf_repr<<"ENDFONT\n";
    return bdf_repr.str();
}

#ifdef STAND_ALONE
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
    in.print();
    ofstream out("font.bdf");
    out<<in.to_bdf();
    out.close();
    cout<<"Loaded the file successfully. Output to font.bdf."<<endl;

    // Create the main window
    sf::RenderWindow window(sf::VideoMode(800, 600), "SFML window");
    // Load a sprite to display
    // Create a graphical text to display
    sf::Font font;
    if (!font.loadFromFile("font.bdf")) {
        
        return EXIT_FAILURE;
    }
    sf::Text text("A QUICK BROWN FOX JUMPED OVER THE LAZY DOG\na quick brown fox jumped over the lazy dog\n1234567890`~!@#$%^&*()-_[{}]\\|;:'\",<>./?", font, 10);
    text.setPosition(sf::Vector2f(0,10));
    // Start the game loop
    while (window.isOpen())
    {
        // Process events
        sf::Event event;
        while (window.pollEvent(event))
        {
            // Close window: exit
            if (event.type == sf::Event::Closed)
                window.close();
        }
        // Clear screen
        window.clear();
        // Draw the text
        window.draw(text);
        // Update the window
        window.display();
    }
    return EXIT_SUCCESS;
}
#endif
