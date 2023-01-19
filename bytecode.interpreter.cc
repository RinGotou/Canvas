#include "machine.h"
#include <cstdio>
#include <cstring>

using std::FILE;
using std::fopen;
using std::fread;
using std::feof;
using std::ferror;

int main(int argc, char **argv) {
  if (argc != 2) {
    puts("Provide a valid bytecode binary file!");
    return 0;
  }

  Program prog;
  auto fp = fopen(argv[1], "rb");
  Code code;
  bool fine = true;

  if (fp != nullptr) {
    while (!feof(fp) && !ferror(fp)) {
      if (ferror(fp)) {
        puts("Error while read file");
        fine = false;
        break;
      }

      fread(&code, sizeof(Code), 1, fp);
      prog.push_back(code);
    }
  }

  if (fine && !prog.empty()) {
    Machine machine;
    machine.Run(prog);
  } 

  fclose(fp);

  return 0;
}
