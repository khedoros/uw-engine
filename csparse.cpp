#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<stdint.h>
#include "UwText.h"

using namespace std;
uint16_t get_short (ifstream &);

int main (int argc, char *argv[]) {
    string filename = "";
    if (argc == 2)
        filename = argv[1];
    cout << "Trying to open " << filename << endl;
    ifstream infile (filename.c_str ());
    if (!infile.is_open ()) {
        cerr << "File didn't open. Fix it." << endl;
        return 1;
    }
    int filesize = 0;
    infile.seekg (0, ios::end);
    filesize = infile.tellg ();
    infile.seekg (0, ios::beg);
    cout << "Filesize: " << filesize << endl;
    if (filesize % 2 != 0 || filesize > 2048) {
        cerr << "Not likely to be a cutscene description. Closing." << endl;
        infile.close ();
        return 2;
    }
    bool rec_val = true;
    int seen[32] = { 0 };
    while (!infile.eof ()) {
        uint16_t val = get_short (infile);
        if (rec_val)
            cout << hex << "At frame " << val << ": ";
        else {
            int arg_counts[] = { 2, 0, 2, 1,
                2, 1, 0, 1,
                2, 1, 1, 1,
                1, 3, 2, 0, //UW1 functions end at 0xf

                0, 0, 0, 0,
                0, 0, 0, 0,
                1, 1, 0, 1,
                0, 0, 0, 0 };
            cout<<hex<<val;
            if (val < 32) {
                vector<uint16_t> args(0,0);
                for (int i = 0; i < arg_counts[val]; ++i) {
                    args.push_back(get_short(infile));
                    cout << " " << args.back();
                }
                cout << "\t";
                seen[val]++;
                switch (val) {
                    case 0:
                        cout<<"Display text arg[1] with palette color arg[0]";
                        break;
                    case 1:
                        break;
                    case 2:
                        cout<<"No-op; great way to waste 6 bytes";
                        break;
                    case 3:
                        cout<<"Pause for arg[0] seconds";
                        break;
                    case 4:
                        cout<<"Probably plays up to frame arg[0] (at rate arg[1]? playing at 5fps seems pretty close to correct)";
                        break;
                    case 5:
                        cout<<"Probably has to do with setting the frame to show for the static version of the cutscenes";
                        break;
                    case 6:
                        cout<<"End of cutscene"; val = get_short(infile);
                        break;
                    case 7:
                        cout<<"Repeat segment (not sure of start) arg[0] number of times";
                        break;
                    case 8:
                        cout<<"Switch current animation file to cs{arg[0]}.n{arg[1]}";
                        break;
                    case 9:
                        cout<<"Fade-out at arg[0] rate (higher is slower)";
                        break;
                    case 0xa:
                        cout<<"Fade-in at arg[0] rate (higher is slower)";
                        break;
                    case 0xb:
                        break;
                    case 0xc:
                        break;
                    case 0xd:
                        cout<<"Display text arg[1] with palette color arg[0] and play audio arg[2]";
                        break;
                    case 0xe:
                        cout<<"Wait for arg[0]/2 seconds if audio is on, arg[1] seconds if not";
                        break;
                    case 0xf:
                        cout<<"Play 'klang' sound effect";
                        break;
                    default: cout<<"UW2?";
                }
                cout << endl << dec;
            }
            else {
                cout<<"Val is currently "<<val<<", which seems bad."<<endl;
            }
        }
        rec_val = !rec_val;
    }
    for (int i = 0; i < 16; ++i)
        if (seen[i])
            cout << hex << "inst: " << i << ": " << dec << seen[i] << endl;
    return 0;
}

uint16_t get_short (ifstream & in) {
    uint16_t buffer;
    in.read (reinterpret_cast < char *>(&buffer), 2);
    return buffer;
}
