/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/difftest.h>
#include "../local-include/reg.h"

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  // printf("\033[1;33mDIFTEST: Checking state at PC = " FMT_WORD "\033[0m\n", pc); // ANSI yellow for visibility

  if (ref_r->pc != pc) {
    printf("\033[1;31mDIFTEST FAILED: PC mismatch!\033[0m REF = " FMT_WORD ", NEMU = " FMT_WORD "\n", ref_r->pc, pc);
    Log("PC mismatch: REF = " FMT_WORD ", NEMU = " FMT_WORD, ref_r->pc, pc);
    return false;
  }
  for (int i = 0; i < 32; i++) {
    if (ref_r->gpr[i] != cpu.gpr[i]) {
      printf("\033[1;31mDIFTEST FAILED: Reg[%d] (%s) mismatch!\033[0m REF = " FMT_WORD ", NEMU = " FMT_WORD "\n",
             i, reg_name(i), ref_r->gpr[i], cpu.gpr[i]);
      Log("Reg[%d] (%s) mismatch: REF = " FMT_WORD ", NEMU = " FMT_WORD, // Keep for log file
          i, reg_name(i), ref_r->gpr[i], cpu.gpr[i]);
      return false;
    }
  }
  // printf("\033[1;32mDIFTEST: PC and GPRs match.\033[0m\n");
  return true;
}

void isa_difftest_attach() {
}
