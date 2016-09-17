#include "kail.h"

void kail_startup(const uw_patch_file& gtl) {
    patches = gtl;
    opl = JavaOPLCreate(/*bool stereo =*/ true);
    reset_synth(opl);
}

void kail_shutdown() {
    if (opl) delete opl;
}

bool kail_seq::load(xmi& x, const uint16_t seqnum) {
    return false;
}

void kail_seq::start() {

}

void kail_seq::wait() {

}

bool kail_seq::onGetData(sf::SoundStream::Chunk& data) {

}

void kail_seq::onSeek(sf::Time timeoffset) {

}
