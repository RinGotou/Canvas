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

constexpr uint32_t kMaxJumpAddrRange = 0x1FFFFFF;

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

bool GetInst(uint32_t &dest, string_view str) {
  bool result = false;  
  
  for (size_t idx = 0, size = kInstStrings.size(); 
      idx < size; idx += 1) {
    if (strcmp(str.data(), kInstStrings[idx]) == 0) {
      dest = static_cast<uint32_t>(idx);
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
    || inst == Inst::PushHalfWordImm
    || inst == Inst::PushHalfWordImmSL16
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
  if (IS_MACRO("pushimm")) {
    base = GetIntLiteralBase(assembly[1]);
    if (base == 2) {
      assembly[1] = assembly[1].substr(2, assembly[1].size() - 2);
    }
    int64_t value = stoll(assembly[1], nullptr, base);
    auto view = reinterpret_cast<uint64_t *>(&value);

    if (*view > UINT16_MAX && *view <= UINT32_MAX) {
      uint32_t hi = *view >> 16;
      uint32_t lo = *view & UINT16_MAX;
      prog.push_back((hi << 7) + Code(Inst::PushHalfWordImmSL16));
      prog.push_back((lo << 7) + Code(Inst::PushHalfWordImm));
      prog.push_back(Code(Inst::AddU));
      prog.push_back(Code(Inst::SpawnSignedInt));
    }
    else if (*view > UINT32_MAX) {
      uint32_t hi32 = *view >> 32;
      uint32_t lo32 = *view & UINT32_MAX;
      // load hi32
      uint32_t hi = hi32 >> 16;
      uint32_t lo = hi32 & UINT16_MAX;
      prog.push_back((hi << 7) + Code(Inst::PushHalfWordImmSL16));
      prog.push_back((lo << 7) + Code(Inst::PushHalfWordImm));
      prog.push_back(Code(Inst::AddU));
      prog.push_back((32u << 7) + Code(Inst::ShiftLeftImm));
      // load lo32
      hi = lo32 >> 16;
      lo = lo32 & UINT16_MAX;
      prog.push_back((hi << 7) + Code(Inst::PushHalfWordImmSL16));
      prog.push_back((lo << 7) + Code(Inst::PushHalfWordImm));
      prog.push_back(Code(Inst::AddU));
      // final combination
      prog.push_back(Code(Inst::AddU));
    }
    else {
      Code arg = (*view) << 7;
      prog.push_back(arg + Code(Inst::PushHalfWordImm));
    }
    prog.push_back(Code(Inst::SpawnSignedInt));
  }
  else if (IS_MACRO("pushuimm")) {
    base = GetIntLiteralBase(assembly[1]);
    if (base == 2) {
      assembly[1] = assembly[1].substr(2, assembly[1].size() - 2);
    }
    uint64_t value = stoull(assembly[1], nullptr, base);
    if (value > UINT16_MAX && value <= UINT32_MAX) {
      uint32_t hi = value >> 16;
      uint32_t lo = value & UINT16_MAX;
      prog.push_back((hi << 7) + Code(Inst::PushHalfWordImmSL16));
      prog.push_back((lo << 7) + Code(Inst::PushHalfWordImm));
      prog.push_back(Code(Inst::AddU));
      prog.push_back(Code(Inst::SpawnSignedInt));
    }
    else if (value > UINT32_MAX) {
      uint32_t hi32 = value >> 32;
      uint32_t lo32 = value & UINT32_MAX;
      // load hi32
      uint32_t hi = hi32 >> 16;
      uint32_t lo = hi32 & UINT16_MAX;
      prog.push_back((hi << 7) + Code(Inst::PushHalfWordImmSL16));
      prog.push_back((lo << 7) + Code(Inst::PushHalfWordImm));
      prog.push_back(Code(Inst::AddU));
      prog.push_back((32u << 7) + Code(Inst::ShiftLeftImm));
      // load lo32
      hi = lo32 >> 16;
      lo = lo32 & UINT16_MAX;
      prog.push_back((hi << 7) + Code(Inst::PushHalfWordImmSL16));
      prog.push_back((lo << 7) + Code(Inst::PushHalfWordImm));
      prog.push_back(Code(Inst::AddU));
      // final combination
      prog.push_back(Code(Inst::AddU));
    }
    else {
      Code arg = value << 7;
      prog.push_back((arg << 7) + Code(Inst::PushHalfWordImm));
    }
  }
  else if (IS_MACRO("pushfp")) {
    double value = stod(assembly[1]);
    auto view = reinterpret_cast<uint64_t *>(&value);
    uint64_t hi32 = *view >> 32;
    uint64_t lo32 = *view & UINT32_MAX;
    // load hi32
    uint32_t hi = hi32 >> 16;
    uint32_t lo = hi32 & UINT16_MAX;
    prog.push_back((hi << 7) + Code(Inst::PushHalfWordImmSL16));
    prog.push_back((lo << 7) + Code(Inst::PushHalfWordImm));
    prog.push_back(Code(Inst::AddU));
    prog.push_back((32u << 7) + Code(Inst::ShiftLeftImm));
    // load lo32
    hi = lo32 >> 16;
    lo = lo32 & UINT16_MAX;
    prog.push_back((hi << 7) + Code(Inst::PushHalfWordImmSL16));
    prog.push_back((lo << 7) + Code(Inst::PushHalfWordImm));
    prog.push_back(Code(Inst::AddU));
    // final combination
    prog.push_back(Code(Inst::AddU));
    prog.push_back(Code(Inst::SpawnFP));
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
  uint32_t inst, args;
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

      Code output = args << 7;
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
            fwrite(&code, sizeof(Code), 1, fp);
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
