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

using Label = pair<string, size_t>;
using Labels = std::unordered_map<string, size_t>;

constexpr uint32_t kMaxJumpArg = 0x3FFFFFF;

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
    || inst == Inst::Branch
    || inst == Inst::FarJump
    || inst == Inst::FarBranch;
}

inline bool IsAlpha(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

inline bool IsNumber(char c) {
  return (c >= '0' && c <= '9');
}

inline bool IsLabelString(string_view src) {
  if (src.size() < 2) {
    return false;
  }
  
  if (src[src.size() - 1] != ':') {
    return false;
  }

  for (size_t i = 0; i < src.size() - 1; i += 1) {
    if (!IsAlpha(src[i]) && !IsNumber(src[i])) {
      return false;
    }
  }

  return true;
}

inline bool IsSegmentUnit(string_view src) {
  if (src.size() < 2) {
    return false;
  }

  if (src[0] != '.') {
    return false;
  }

  for (size_t i = 1; i < src.size(); i += 1) {
    if (!IsAlpha(src[i]) && !IsNumber(src[i])) {
      return false;
    }
  }

  return true;
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

bool TryExpandJumpInsn(vector<string> &assembly, Program &prog, Labels &labels) {
  if (assembly.size() < 2) {
    return false;
  }

  bool result = true;

  auto it = labels.find(assembly[1]);

  if (it == labels.end()) {
    result = false;
  }
  else {
    size_t offset = it->second;

#define IS_INSN(_str) (strcmp(assembly[0].data(), _str) == 0)
    // if offset's binary length large than 25bit, use FarJump/FarBranch instead.
    if (offset > kMaxJumpArg) {
      if (offset <= UINT32_MAX) {
        uint32_t hi = offset >> 16;
        uint32_t lo = offset & UINT16_MAX;
        prog.push_back((hi << 7) + Code(Inst::PushHalfWordImmSL16));
        prog.push_back((lo << 7) + Code(Inst::PushHalfWordImm));
        prog.push_back(Code(Inst::AddU));
      }
      else if (offset > UINT32_MAX) {
        uint32_t hi32 = offset >> 32;
        uint32_t lo32 = offset & UINT32_MAX;
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

      if (IS_INSN("jmp") || IS_INSN("farjmp")) {
        prog.push_back(Code(Inst::FarJump));
      }
      else if (IS_INSN("branch") || IS_INSN("farbranch")) {
        prog.push_back(Code(Inst::FarBranch));
      }
    }
    else {
      Code insn = offset << 7;

      if (IS_INSN("jmp") || IS_INSN("farjmp")) {
        insn += Code(Inst::Jump);
      }
      else if (IS_INSN("branch") || IS_INSN("farbranch")) {
        insn += Code(Inst::Branch);
      }

      prog.push_back(insn);
    }
#undef IS_INSN
  }

  return result;
}

bool ExpandSegementContents(vector<string> &assembly, Program &prog, Labels &labels) {
  bool result = true;


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
  Labels labels;
  uint32_t inst, args;
  int base;
  // Record offset without label line
  size_t asm_offset = 0;
  bool fine = true;

  if (fp != nullptr) {
    vector<string> assembly;

    while (ReadInst(assembly, fp)) {
      if (assembly.empty()) {
        continue;
      }

      if (IsLabelString(assembly[0])) {
        auto substr = assembly[0].substr(0, assembly[0].size() - 1);
        //printf("Found label %s\n", substr.data()); 
        labels.insert(Label(substr.data(), asm_offset));
        continue;
      }

      asm_offset += 1;

      if (TryExpandMacro(assembly, prog)) {
        continue;
      }

      if (!GetInst(inst, assembly[0])) {
        printf("Invalid instruction: %s %lu\n", assembly[0].data(), assembly.size());
        fine = false;

        break;
      }

      if (IsJumpInst(static_cast<Inst>(inst))) {
        if (!TryExpandJumpInsn(assembly, prog, labels)) {
          puts("Warning: failed to expand jump insn"); 
        }
        continue;
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
              assembly[1] = assembly[1].substr(2, assembly[1].size() - 2);
            }
            args = stoull(assembly[1], nullptr, base);
          }
        }
        else {
          base = GetIntLiteralBase(assembly[1]);
          if (base == 2) {
            assembly[1] = assembly[1].substr(2, assembly[1].size() - 2);
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
