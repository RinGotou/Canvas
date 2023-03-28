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

//TODO:symbol table(for string, etc.)
// Symbol type: const string, 

//TODO: Decimal command
//TODO: branch command
// U - Unsigned
// SL - Shift left
// AH - Assemble Highpart
// F - Float
enum class Inst : uint8_t {
  Doze = 0, Add, Sub, Mul, Div, Mod,
  AddU, SubU, MulU, DivU, ModU,
  PushInt, PushUInt, PushUIntSL, PushIntAH, 
  Jump, Branch, Pop, PrintStackTop,
  PushDecLoAH, AddF, SubF, MulF, DivF,
  Branch, 
};

//TODO: string operation inst
// warning: keep element ordering according to Inst
const vector<const char*> kInstStrings = {
  "doze",
  "add",
  "sub",
  "mul",
  "div",
  "mod",
  "addu",
  "subu",
  "mulu",
  "divu",
  "modu",
  "pushint",
  "pushuint",
  "pushuintsl",
  "pushintah",
  "j",
  "b",
  "pop",
  "print",
  "pushdecloah",
  "addf",
  "subf",
  "mulf",
  "divf",
};

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
