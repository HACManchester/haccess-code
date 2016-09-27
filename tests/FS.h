/* FS.h
 *
 * Implementation of FS.h to provide File class for IniFile, etc.
 */

#include <fstream>
#include <iostream>

#ifndef __FS_FS_H
#define __FS_FS_H

namespace fs {
  
enum SeekMode {
    SeekSet = 0,
    SeekCur = 1,
    SeekEnd = 2
};

class File {
  //std::fstream stream;
  
 public:
  File() { }
  ~File() { }

  size_t read(uint8_t *buf, size_t size);
  bool seek(uint32_t pos, SeekMode mode);
  int available();
  size_t position();
  size_t size();
  void close();

  String readStringUntil(char ch);
  
  operator bool();
};

}

using fs::File;

using fs::SeekMode;
using fs::SeekSet;
using fs::SeekCur;
using fs::SeekEnd;

#endif
