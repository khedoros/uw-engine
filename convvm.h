#pragma once
#include<string>
#include "convfile.h"
#include "globfile.h"
#include "savefile.h"
#include "UwText.h"
#include<vector>

class convvm {
public:
    convvm(int, convfile&, globfile&, savefile&, UwText&);
    void start();
    std::string& step();
    std::string op_str(int op);
    int who;
    convfile conv;
    globfile glob;
    savefile save;
    UwText text;
private:
    int pc;
    int sp;
    int bp;
    int16_t result;
    std::vector<int16_t> data_stack;
    std::vector<int16_t> code_stack;

    std::string op_strs[42];

    void push(int16_t val);
    int16_t pop();

    void op_nop();
    void op_add();
    void op_mul();
    void op_sub();
    void op_div();
    void op_mod();
    void op_or();
    void op_and();
    void op_not();
    void op_tstgt();
    void op_tstge();
    void op_tstlt();
    void op_tstle();
    void op_tsteq();
    void op_tstne();
    void op_jmp();
    void op_beq();
    void op_bne();
    void op_bra();
    void op_call();
    void op_calli();
    void op_ret();
    void op_pushi();
    void op_pushi_eff();
    void op_pop();
    void op_swap();
    void op_pushbp();
    void op_popbp();
    void op_sptobp();
    void op_bptosp();
    void op_addsp();
    void op_fetchm();
    void op_sto();
    void op_offset();
    void op_start();
    void op_save_reg();
    void op_push_reg();
    void op_strcmp();
    void op_exit_op();
    void op_say_op();
    void op_respond_op();
    void op_neg();
};
