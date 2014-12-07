#include "Common.h"

namespace oria {


  std::string readFile(const std::string & filename) {
    using namespace std;
    ifstream ins(filename.c_str(), ios::binary);
    if (!ins) {
      throw runtime_error("Failed to load file " + filename);
    }
    assert(ins);
    stringstream sstr;
    sstr << ins.rdbuf();
    return sstr.str();
  }
}