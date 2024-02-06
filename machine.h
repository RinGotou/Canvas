#include <unordered_map>
#include <vector>
#include <stack>
#include <cstdint>
#include <string>
#include <cstdlib>
#include "memory-utils.h"

//7bit inst, 25bit args
using Code = uint32_t;
// |--------args--------|--inst--|

// 1 Word = 32bits
// HalfWord, DoubleWord, Byte

// Maybe we can use better container design?
using Program = std::vector<Code>;

//for runtime stack
using std::stack;
using std::vector;
using std::pair;

// Base class for Memory Interface that will need to be
// inherited by any exact implementation.
class MemoryInterface {
protected:
public:
  MemoryInterface() { /* Placebo */ }
  virtual void *Alloc(size_t num, size_t size) = 0;
  virtual void Delete(void *) = 0;
  virtual void Collect() = 0;
};

// Barebone wrapper of calloc/free.
class SimpleMemoryInterface : virtual public MemoryInterface {
protected:
public:
  SimpleMemoryInterface() {}
  void *Alloc(size_t num, size_t size) override {
    return calloc(num, size);
  }

  void Delete(void *ptr) override {
    free(ptr);
  }

  void Collect() {}
};

// warning: shift left for a negative signed int is UB before c++20
// consider safe impl for signed int.
//struct Symbol {
//  uint64_t id, value;
//};
//TODO: strtab?
//TODO: Magic string?

#define INIT_INSTID
enum class Inst : uint8_t {
#include "instruction.h"
};
#undef INIT_INSTID

//TODO: string operation inst
#define INIT_INSTSTR
const vector<const char*> kInstStrings = {
#include "instruction.h"
};
#undef INIT_INSTSTR

enum class UnitType {
  Int, //int64_t 
  UInt, //uint64_t
  FP //double
};

//TODO: For string features, we must add symbol table.
union UnitValue {
 uint64_t uinteger;
 int64_t integer;
 double fp;
 void *ptr;
};

struct Unit {
  UnitValue value;
  UnitType type;
};

// INT VALue, Unsigned INT VALue, Floating-Point VALue
#define INTVAL(_unit)  (_unit).value.integer
#define UINTVAL(_unit) (_unit).value.uinteger
#define FPVAL(_unit) (_unit).value.fp

class Machine {
  protected:
  stack<Unit> stack_;
  uint64_t pc_;
  
  public:
  Machine() : stack_(), pc_(0) {}
  ~Machine() {}

  //TODO: accept symbol table
  bool Run(Program &prog);  
};
