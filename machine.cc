#include "machine.h"
#include <cstdio>
#include <bit>

// useful macros
//TODO: add more arg option to op to make full use of 64-bit inst
#define GET_INST(_code) static_cast<uint8_t>(_code % uint64_t(0x80))
#define GET_ARGS(_code) (_code >> 7)
#define HAS_ARGS(_code) ((_code >> 7) != 0x0)

#define POP_VALUE_TO(_tmp) \
  _tmp = stack_.top();     \
  stack_.pop();

bool Machine::Run(Program &prog) {
  bool result = true;

  //reset state
  pc_ = 0;
  stack<Unit>().swap(stack_);

  auto prog_size = prog.size();
  Unit tmp0, tmp1;
  Code current;

  while (pc_ < prog_size) {
    current = prog[pc_];
    switch (static_cast<Inst>(GET_INST(current))) {
    case Inst::Add:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Int});
      INTVAL(stack_.top()) = INTVAL(tmp1) + INTVAL(tmp0); 
      break;
    case Inst::Sub:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Int});
      INTVAL(stack_.top()) = INTVAL(tmp0) - INTVAL(tmp1);
      break;
    case Inst::Mul:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Int});
      INTVAL(stack_.top()) = INTVAL(tmp1) * INTVAL(tmp0);
      break;
    case Inst::Div:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Int});
      INTVAL(stack_.top()) = INTVAL(tmp0) / INTVAL(tmp1);
      break;
    case Inst::Mod:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Int});
      INTVAL(stack_.top()) = INTVAL(tmp0) % INTVAL(tmp1);
      break;
    case Inst::AddU:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = UINTVAL(tmp1) + UINTVAL(tmp0);
      break;
    case Inst::SubU:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = UINTVAL(tmp0) - UINTVAL(tmp1);
      break;
    case Inst::MulU:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = UINTVAL(tmp1) * UINTVAL(tmp0);
      break;
    case Inst::DivU:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = UINTVAL(tmp0) / UINTVAL(tmp1);
      break;
    case Inst::ModU:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = UINTVAL(tmp0) % UINTVAL(tmp1);
      break;
    case Inst::AddF:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Decimal});
      DECIVAL(stack_.top()) = DECIVAL(tmp0) + DECIVAL(tmp1);
      break;
    case Inst::SubF:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Decimal});
      DECIVAL(stack_.top()) = DECIVAL(tmp0) - DECIVAL(tmp1);
      break;
    case Inst::MulF:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Decimal});
      DECIVAL(stack_.top()) = DECIVAL(tmp0) * DECIVAL(tmp1);
      break;
    case Inst::DivF:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Decimal});
      DECIVAL(stack_.top()) = DECIVAL(tmp0) / DECIVAL(tmp1);
      break;
    //load 32-bit unsigned int
    case Inst::PushUInt:
      stack_.push(Unit{GET_ARGS(current), UnitType::UInt});
      break;
    //load 32-bit unsigned int and shift left 32-bit
    case Inst::PushUIntSL:
      UINTVAL(tmp0) = GET_ARGS(current);
      UINTVAL(tmp0) <<= 32;
      stack_.push(Unit{tmp0.value, UnitType::UInt});
      break;
    //assemble signed int hi/lo part into single value
    case Inst::PushIntAH:
      POP_VALUE_TO(tmp0);
      UINTVAL(tmp1) = GET_ARGS(current);
      stack_.push(Unit{0, UnitType::Int});
      UINTVAL(stack_.top()) = UINTVAL(tmp0) + UINTVAL(tmp1);
      break;
    //load 32-bit signed int
    case Inst::PushInt:
      //cast to int32_t after shift, then assign to int64_t
      INTVAL(tmp0) = static_cast<int32_t>(GET_ARGS(current));
      stack_.push(Unit{tmp0.value, UnitType::Int});
      break;
    case Inst::PushDecLoAH:
      POP_VALUE_TO(tmp0);
      UINTVAL(tmp1) = GET_ARGS(current);
      stack_.push(Unit{0, UnitType::Decimal});
      UINTVAL(stack_.top()) = UINTVAL(tmp0) + UINTVAL(tmp1);
      break;
    case Inst::Jump:
      pc_ = GET_ARGS(current);
      continue;
    //jump if top value is (equals to) true
    case Inst::Branch:
      if (!stack_.empty()) {
        if (UINTVAL(stack_.top()) != 0ull) {
          pc_ = GET_ARGS(current);
          continue;
        }
      }
      break;
    case Inst::Pop:
      if (!stack_.empty()) {
        stack_.pop();
      }
      break;
    case Inst::PrintStackTop:
      if (!stack_.empty()) {
        switch (stack_.top().type) {
        case UnitType::Int:
          printf("%s: %ld\n", "Int", INTVAL(stack_.top()));
          break;
        case UnitType::UInt:
          printf("%s: %lu\n", "UInt", UINTVAL(stack_.top()));
          break;
        case UnitType::Decimal:
          printf("%s: %f\n", "Decimal", DECIVAL(stack_.top()));
          break;
        }
      }
      else {
        //TODO: interrupt
        std::puts("(!)Empty stack");
      }
      break;

    case Inst::ShiftLeft:
      POP_VALUE_TO(tmp0); //shift amount
      //target is on stack top.
      INTVAL(stack_.top()) << UINTVAL(tmp0);
      break;

    case Inst::ShiftLeftImm:
      UINTVAL(tmp0) = GET_ARGS(current);
      INTVAL(stack_.top()) << UINTVAL(tmp0);
      break;

    case Inst::LogicShiftRight:
      POP_VALUE_TO(tmp0);
      UINTVAL(stack_.top()) >> UINTVAL(tmp0);
      break;

    case Inst::ArithShiftRight:
      POP_VALUE_TO(tmp0);
      INTVAL(stack_.top()) >> UINTVAL(tmp0);
      break;

    case Inst::LogicShiftRightImm:
      UINTVAL(tmp0) = GET_ARGS(current);
      UINTVAL(stack_.top()) >> UINTVAL(tmp0);
      break;

    case Inst::ArithShiftRightImm:
      UINTVAL(tmp0) = GET_ARGS(current);
      INTVAL(stack_.top()) >> UINTVAL(tmp0);
      break;

    case Inst::And:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = UINTVAL(tmp0) & UINTVAL(tmp1);
      break;

    case Inst::Or:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = UINTVAL(tmp0) | UINTVAL(tmp1);
      break;

    case Inst::Not:
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = ~UINTVAL(tmp0);
      break;

    case Inst::XOr:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = UINTVAL(tmp0) ^ UINTVAL(tmp1);
      break;

    //Any non-zero value is converted to true.
    case Inst::LogicAnd:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = UINTVAL(tmp0) && UINTVAL(tmp1);
      break;

    case Inst::LogicOr:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = UINTVAL(tmp0) || UINTVAL(tmp1);
      break;

    case Inst::LogicNot:
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = !UINTVAL(tmp0);
      break;

    //Use C++20 directly for UB-free impl of rotate shift.
    case Inst::RotateLeft:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = std::rotl(UINTVAL(tmp0), UINTVAL(tmp1));
      break;

    case Inst::RotateRight:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = std::rotr(UINTVAL(tmp0), UINTVAL(tmp1));
      break;

    case Inst::RotateLeftImm:
      UINTVAL(tmp1) = GET_ARGS(current);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = std::rotl(UINTVAL(tmp0), UINTVAL(tmp1));
      break;

    case Inst::RotateRightImm:
      UINTVAL(tmp1) = GET_ARGS(current);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      UINTVAL(stack_.top()) = std::rotr(UINTVAL(tmp0), UINTVAL(tmp1));
      break;

    case Inst::Doze:
    default:
      break;
    }

    pc_ += 1;
  }  

  return result;
}
