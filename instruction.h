// Canvas Bytecode Inst Definitions.
//TODO: Decimal command
//TODO: branch command
// U - Unsigned
// SL - Shift left
// AH - Assemble Highpart
// F - Float

#ifdef INIT_INSTID
#define DEF_INST(_id, _str) _id,
#endif

#ifdef INIT_INSTSTR
#define DEF_INST(_id, _str) _str,
#endif

DEF_INST(Add, "add")
DEF_INST(Sub, "sub")
DEF_INST(Mul, "mul")
DEF_INST(Div, "div")
DEF_INST(Mod, "mod")
DEF_INST(AddU, "addu")
DEF_INST(SubU, "subu")
DEF_INST(MulU, "mulu")
DEF_INST(DivU, "divu")
DEF_INST(ModU, "modu")
DEF_INST(AddF, "addf")
DEF_INST(SubF, "subf")
DEF_INST(MulF, "mulf")
DEF_INST(DivF, "divf")

DEF_INST(PushUInt, "pushuint")
DEF_INST(PushUIntSL, "pushuintsl")
DEF_INST(PushIntAH, "pushintah")
DEF_INST(PushInt, "pushint")
DEF_INST(PushDecLoAH, "pushdecloah")

DEF_INST(Jump, "jmp")
DEF_INST(Branch, "branch")

DEF_INST(Pop, "pop")
DEF_INST(PrintStackTop, "print")

DEF_INST(ShiftLeft, "sl")
DEF_INST(ShiftLeftImm, "sli")
DEF_INST(LogicShiftRight, "lsr")
DEF_INST(ArithShiftRight, "asr")
DEF_INST(LogicShiftRightImm, "lsri")
DEF_INST(ArithShiftRightImm, "asri")

DEF_INST(And, "and")
DEF_INST(Or, "or")
DEF_INST(Not, "not")
DEF_INST(XOr, "xor")
DEF_INST(LogicAnd, "land")
DEF_INST(LogicOr, "lor")
DEF_INST(LogicNot, "lnot")

DEF_INST(RotateLeft, "rl")
DEF_INST(RotateRight, "rr")
DEF_INST(RotateLeftImm, "rli")
DEF_INST(RotateRightImm, "rri")

DEF_INST(Doze, "doze")

#undef DEF_INST