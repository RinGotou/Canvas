#include <vector>
#include <stack>
#include <cstdint>
#include <string>

//7bit inst, 57bit args
using Code = uint64_t;
// |--------args--------|--inst--|

// Maybe we can use better container design?
using Program = std::vector<Code>;

//for runtime stack
using std::stack;
using std::vector;

constexpr int64_t kMaxJumpAddrRange = 0x1FFFFFFFFFFFFFFF;

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
  Decimal //double
};

union UnitValue {
 uint64_t uinteger;
 int64_t integer;
 double decimal;
};

struct Unit {
  UnitValue value;
  UnitType type;
};

// INT VALue, Unsigned INT VALue, DECImal VALue
#define INTVAL(_unit)  (_unit).value.integer
#define UINTVAL(_unit) (_unit).value.uinteger
#define DECIVAL(_unit) (_unit).value.decimal

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
