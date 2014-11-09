/************************************************************************************

 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ************************************************************************************/

#include "Common.h"
#include "Files.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <stdexcept>
#include <chrono>

using namespace std;

string Files::read(const string & filename) {
  ifstream ins(filename.c_str(), ios::binary);
  if (!ins) {
    throw runtime_error("Failed to load file " + filename);
  }
  assert(ins);
  stringstream sstr;
  sstr << ins.rdbuf();
  return sstr.str();
}

time_t Files::modified(const string & filename) {
  return 0;
//  return tr2::sys::last_write_time(tr2::sys::path(filename));
}

bool Files::exists(const string & filename) {
  return false;
//  return tr2::sys::exists(tr2::sys::path(filename));
}
