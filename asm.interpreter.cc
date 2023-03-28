#include "machine.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string_view>
#include <charconv>

using std::FILE;
using std::fopen;
using std::string;
using std::puts;
using std::fgetc;
using std::feof;
using std::ferror;
using std::vector;
using std::string_view;
using std::strcmp;
using std::stoull;
using std::fwrite;
using std::stoll;
using std::stoull;
using std::stod;

bool ReadInst(vector<string> &dest, FILE *fp) {
  bool result = true;
  
  dest.clear();
  unsigned char c;
  string buf;

  // TODO:comments
  while (true) {
    c = static_cast<unsigned char>(fgetc(fp));
    
    if (feof(fp)) {
      result = false;
      break;
    }

    if (ferror(fp)) {
      puts("Stream error occurred while reading file");
      result = false;
      break;
    }

    if (c == '\r' || c == '\n') {
      if (!buf.empty()) {
        dest.push_back(buf);
      }
      break;
    }
    else if (c == ' ' || c == '\t') {
      if (!buf.empty()) {
        dest.push_back(buf);
        buf.clear();
      }
    }
    else {
      buf.append(1, c);
    }
  }

  return result;
}

bool GetInst(uint64_t &dest, string_view str) {
  bool result = false;  
  
  for (size_t idx = 0, size = kInstStrings.size(); 
      idx < size; idx += 1) {
    if (strcmp(str.data(), kInstStrings[idx]) == 0) {
      dest = static_cast<uint64_t>(idx);
      result = true;
      break;
    }
  }

  return result;
}

inline bool IsUnsignedInst(Inst inst) {
  return inst == Inst::AddU
    || inst == Inst::SubU
    || inst == Inst::MulU
    || inst == Inst::DivU
    || inst == Inst::ModU
    || inst == Inst::PushUInt
    || inst == Inst::PushUIntSL
    || inst == Inst::Jump
    || inst == Inst::Branch;
}

inline bool IsJumpInst(Inst inst) {
  return inst == Inst::Jump
    || inst == Inst::Branch;
}

int GetIntLiteralBase(string_view str) {
  if (str.size() == 1) {
    return 10;
  }

  auto prefix = str.substr(0, 2);

  if (prefix[0] == '+' || prefix[0] == '-') {
    return 10;
  }

  if (prefix == "0x" || prefix == "0X") {
    return 16;
  }

  if (prefix == "0b" || prefix == "0B") {
    return 2;
  }

  if (prefix[0] == '0') {
    return 8;
  }

  return 10;
}

bool TryExpandMacro(vector<string> &assembly, Program &prog) {
  if (assembly.size() < 2) {
    return false;
  }

  bool result = true;
  int base = 10;

#define IS_MACRO(_str) (strcmp(assembly[0].data(), _str) == 0)
  if (IS_MACRO("pint")) {
    base = GetIntLiteralBase(assembly[1]);
    if (base == 2) {
      assembly[1] = assembly[1].substr(2, assembly[1].size() - 2);
    }
    int64_t value = stoll(assembly[1], nullptr, base);
    auto view = reinterpret_cast<uint64_t *>(&value);

    if (*view > UINT32_MAX) {
      uint64_t hi = *view >> 32;
      uint64_t lo = *view & UINT32_MAX;
      prog.push_back((hi << 7) + uint64_t(Inst::PushUIntSL));
      prog.push_back((lo << 7) + uint64_t(Inst::PushIntAH));
    }
    else {
      uint64_t arg = (*view) << 7;
      prog.push_back(arg + uint64_t(Inst::PushInt));
    }
  }
  else if (IS_MACRO("puint")) {
    base = GetIntLiteralBase(assembly[1]);
    if (base == 2) {
      assembly[1] = assembly[1].substr(2, assembly[1].size() - 2);
    }
    uint64_t value = stoull(assembly[1], nullptr, base);
    if (value > UINT32_MAX) {
      uint64_t hi = value >> 32;
      uint64_t lo = value & UINT32_MAX;
      prog.push_back((hi << 7) + uint64_t(Inst::PushUIntSL));
      prog.push_back((lo << 7) + uint64_t(Inst::PushUInt));
      prog.push_back(uint64_t(Inst::AddU));
    }
    else {
      uint64_t arg = value << 7;
      prog.push_back((arg << 7) + uint64_t(Inst::PushUInt));
    }
  }
  else if (IS_MACRO("pdec")) {
    double value = stod(assembly[1]);
    auto view = reinterpret_cast<uint64_t *>(&value);
    uint64_t hi = *view >> 32;
    uint64_t lo = *view & UINT32_MAX;
    prog.push_back((hi << 7) + uint64_t(Inst::PushUIntSL));
    prog.push_back((lo << 7) + uint64_t(Inst::PushDecLoAH));
  }
  else {
    result = false;
  }
#undef IS_MACRO

  return result;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    puts("Invalid arguments");
    return 0;
  }
  //open asm file
  auto fp = fopen(argv[1], "r");
  Program prog;
  uint64_t inst, args;
  int base;
  bool fine = true;

  if (fp != nullptr) {
    vector<string> assembly;

    while (ReadInst(assembly, fp)) {
      if (assembly.empty()) {
        continue;
      }

      if (TryExpandMacro(assembly, prog)) {
        continue;
      }

      if (!GetInst(inst, assembly[0])) {
        printf("Invalid instruction: %s %s\n", 
            assembly[0].data(), assembly[1].data());
        fine = false;
        break;
      }

      //generate actual inst 
      if (assembly.size() == 2) {
        if (IsUnsignedInst(static_cast<Inst>(inst))) {
          if (assembly[1][0] == '+' || assembly[1][0] == '-') {
            puts("Invalid literal for unsigned instruction");
            fine = false;
          }
          else {
            base = GetIntLiteralBase(assembly[1]);
            if (base == 2) {
              assembly[1] = 
                assembly[1].substr(2, assembly[1].size() - 2);
            }
            args = stoull(assembly[1], nullptr, base);

            if (IsJumpInst(static_cast<Inst>(inst)) &&
                  args > kMaxJumpAddrRange) {
              fine = false;
              puts("Jump distance out of range");
            }
            else if (args > UINT32_MAX) {
              fine = false;
              puts("value out of range");
            }
          }
        }
        else {
          base = GetIntLiteralBase(assembly[1]);
          if (base == 2) {
            assembly[1] = 
              assembly[1].substr(2, assembly[1].size() - 2);
          }

          int32_t signed_val = stoll(assembly[1], nullptr, base);
          args = *reinterpret_cast<uint32_t *>(&signed_val);
        }
      }

      if (!fine) break;

      uint64_t output = args << 7;
      output += inst;
      prog.push_back(output);
    }

    fclose(fp);
    
    if (fine && !prog.empty()) {
      if (argc == 2 || strcmp(argv[2], "run") == 0) {
        Machine machine;
        machine.Run(prog);
      }
      else if (strcmp(argv[2], "compile") == 0) {
        string out(argv[1]);
        out.append(".bc");
        fp = fopen(out.data(), "wb");
        
        if (fp != nullptr) {
          for (auto &code : prog) {
            fwrite(&code, sizeof(uint64_t), 1, fp);
            if (ferror(fp)) {
              puts("Error occurred while writing bytecodes");
              break;
            }
          }
        }

        fclose(fp);
      }
    }
  }
  else {
    puts("Invalid assembly file");
  }
  
  return 0;
}
