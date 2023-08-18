#include <vector>
#include <stack>
#include <cstdint>
#include <string>
//#include <variant>

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

// warning: shift left for a negative signed int is UB before c++20
// consider safe impl for signed int.
struct Symbol {
  uint64_t id, value;
};
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

union UnitValue {
 uint64_t uinteger;
 int64_t integer;
 double fp;
};

struct Unit {
  UnitValue value;
  UnitType type;
};

// INT VALue, Unsigned INT VALue, Floating-Point VALue
#define INTVAL(_unit)  (_unit).value.integer
#define UINTVAL(_unit) (_unit).value.uinteger
#define FPVAL(_unit) (_unit).value.fp

//memo: use independent pc stack?

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
