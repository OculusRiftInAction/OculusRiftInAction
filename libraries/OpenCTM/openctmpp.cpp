#include "openctmpp.h"

CTMuint CTMimporter::StreamLoaderFn(void * aBuf, CTMuint aCount, void * aUserData) {
    std::istream & instream = *(std::istream *) aUserData;
    return (CTMuint)instream.readsome((char*) aBuf, aCount);
}
