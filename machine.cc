#include "machine.h"
#include <cstdio>

// useful macros
#define GET_INST(_code) static_cast<uint8_t>(_code % uint64_t(0x80))
#define GET_ARGS(_code) (_code >> 7)

#define POP_VALUE_TO(_tmp) \
  _tmp = stack_.top();     \
  stack_.pop();

bool Machine::Run(Program &prog) {
  bool result = true;

  //reset state
  pc_ = 0;
  stack<Unit>().swap(stack_);
  //while (!stack_.empty()) stack_.pop();

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
      stack_.top().value.integer = 
        tmp1.value.integer + tmp0.value.integer;
      break;
    case Inst::Sub:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Int});
      stack_.top().value.integer = 
        tmp0.value.integer - tmp1.value.integer;
      break;
    case Inst::Mul:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Int});
      stack_.top().value.integer = 
        tmp1.value.integer * tmp0.value.integer;
      break;
    case Inst::Div:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Int});
      stack_.top().value.integer = 
        tmp0.value.integer / tmp1.value.integer;
      break;
    case Inst::Mod:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Int});
      stack_.top().value.integer = 
        tmp0.value.integer % tmp1.value.integer;
      break;
    case Inst::AddU:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      stack_.top().value.integer = 
        tmp1.value.integer + tmp0.value.integer;
      break;
    case Inst::SubU:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      stack_.top().value.integer = 
        tmp0.value.integer - tmp1.value.integer;
      break;
    case Inst::MulU:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      stack_.top().value.integer = 
        tmp1.value.integer * tmp0.value.integer;
      break;
    case Inst::DivU:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      stack_.top().value.integer = 
        tmp0.value.integer / tmp1.value.integer;
      break;
    case Inst::ModU:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::UInt});
      stack_.top().value.integer = 
        tmp0.value.integer % tmp1.value.integer;
      break;
    case Inst::AddF:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Decimal});
      stack_.top().value.decimal = 
        tmp0.value.decimal + tmp1.value.decimal;
      break;
    case Inst::SubF:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Decimal});
      stack_.top().value.decimal = 
        tmp0.value.decimal - tmp1.value.decimal;
      break;
    case Inst::MulF:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Decimal});
      stack_.top().value.decimal = 
        tmp0.value.decimal * tmp1.value.decimal;
      break;
    case Inst::DivF:
      POP_VALUE_TO(tmp1);
      POP_VALUE_TO(tmp0);
      stack_.push(Unit{0, UnitType::Decimal});
      stack_.top().value.decimal = 
        tmp0.value.decimal / tmp1.value.decimal;
      break;
    //load 32-bit unsigned int
    case Inst::PushUInt:
      stack_.push(Unit{GET_ARGS(current), UnitType::UInt});
      break;
    //load 32-bit unsigned int and shift left 32-bit
    case Inst::PushUIntSL:
      tmp0.value.uinteger = GET_ARGS(current);
      tmp0.value.uinteger <<= 32;
      stack_.push(Unit{tmp0.value, UnitType::UInt});
      break;
    //assemble signed int hi/lo part into single value
    case Inst::PushIntAH:
      POP_VALUE_TO(tmp0);
      tmp1.value.uinteger = GET_ARGS(current);
      stack_.push(Unit{0, UnitType::Int});
      stack_.top().value.uinteger = 
        tmp0.value.uinteger + tmp1.value.uinteger;
      break;
    //load 32-bit signed int
    case Inst::PushInt:
      //cast to int32_t after shift, then assign to int64_t
      tmp0.value.integer = static_cast<int32_t>(GET_ARGS(current));
      stack_.push(Unit{tmp0.value, UnitType::Int});
      break;
    case Inst::PushDecLoAH:
      POP_VALUE_TO(tmp0);
      tmp1.value.uinteger = GET_ARGS(current);
      stack_.push(Unit{0, UnitType::Decimal});
      stack_.top().value.uinteger = 
        tmp0.value.uinteger + tmp1.value.uinteger;
      break;
    case Inst::Jump:
      pc_ = GET_ARGS(current);
      continue;
    //jump if top value is (equals to) true
    case Inst::Branch:
      if (!stack_.empty()) {
        if (stack_.top().value.uinteger != 0ull) {
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
          printf("%s: %ld\n", "Int", stack_.top().value.integer);
          break;
        case UnitType::UInt:
          printf("%s: %lu\n", "UInt", stack_.top().value.uinteger);
          break;
        case UnitType::Decimal:
          printf("%s: %f\n", "Decimal", stack_.top().value.decimal);
          break;
        }
      }
      else {
        //TODO: interrupt
        std::puts("!Empty stack");
      }
      break;
    case Inst::Doze:
    default:
      break;
    }

    pc_ += 1;
  }  

  return result;
}