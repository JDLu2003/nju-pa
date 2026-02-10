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

#include <common.h>
#include <elf.h>

#ifdef CONFIG_FTRACE

typedef struct {
  char name[64];
  vaddr_t start_addr;
  vaddr_t end_addr;
} FuncSymbol;

static FuncSymbol *func_table = NULL;
static int func_count = 0;
static int call_depth = 0;

// 根据地址查找函数名
static const char* find_func_name(vaddr_t addr) {
  for (int i = 0; i < func_count; i++) {
    if (addr >= func_table[i].start_addr && addr < func_table[i].end_addr) {
      return func_table[i].name;
    }
  }
  return "???";
}

// 根据函数名查找地址（用于表达式求值）
bool ftrace_find_symbol(const char *name, vaddr_t *addr) {
  for (int i = 0; i < func_count; i++) {
    if (strcmp(func_table[i].name, name) == 0) {
      *addr = func_table[i].start_addr;
      return true;
    }
  }
  return false;
}

// 打印函数表内容
void display_ftrace_table() {
  Log("ftrace: Function Symbol Table");
  Log("%-30s %-15s %-15s", "Name", "Start Addr", "End Addr");
  for (int i = 0; i < func_count; i++) {
    Log("%-30s " FMT_WORD "      " FMT_WORD,
        func_table[i].name, func_table[i].start_addr, func_table[i].end_addr);
  }
}

// 初始化 ftrace，解析 ELF 文件
void init_ftrace(const char *elf_file) {
  if (elf_file == NULL) {
    Log("No ELF file provided, ftrace disabled");
    return;
  }

  FILE *fp = fopen(elf_file, "rb");
  if (fp == NULL) {
    Log("Failed to open ELF file: %s", elf_file);
    return;
  }

  // 读取 ELF header
  Elf32_Ehdr ehdr;
  if (fread(&ehdr, sizeof(ehdr), 1, fp) != 1) {
    Log("Failed to read ELF header");
    fclose(fp);
    return;
  }

  // 验证 ELF magic number
  if (ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
      ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
      ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
      ehdr.e_ident[EI_MAG3] != ELFMAG3) {
    Log("Invalid ELF file");
    fclose(fp);
    return;
  }

  // 读取 section headers
  Elf32_Shdr *shdr = malloc(sizeof(Elf32_Shdr) * ehdr.e_shnum);
  // 磁头偏移到 section header table
  fseek(fp, ehdr.e_shoff, SEEK_SET);
  // 读取 section header table
  if (fread(shdr, sizeof(Elf32_Shdr), ehdr.e_shnum, fp) != ehdr.e_shnum) {
    Log("Failed to read section headers");
    free(shdr);
    fclose(fp);
    return;
  }

  // 查找符号表和字符串表
  Elf32_Shdr *symtab_shdr = NULL;
  Elf32_Shdr *strtab_shdr = NULL;

  // 遍历 section header table
  // 去找到符号表对应的 header
  for (int i = 0; i < ehdr.e_shnum; i++) {
    if (shdr[i].sh_type == SHT_SYMTAB) {
      symtab_shdr = &shdr[i];
      strtab_shdr = &shdr[shdr[i].sh_link];
      break;
    }
  }

  if (symtab_shdr == NULL || strtab_shdr == NULL) {
    Log("Symbol table or string table not found");
    free(shdr);
    fclose(fp);
    return;
  }

  // 读取完整符号表
  int sym_count = symtab_shdr->sh_size / sizeof(Elf32_Sym);
  Elf32_Sym *symtab = malloc(symtab_shdr->sh_size);
  fseek(fp, symtab_shdr->sh_offset, SEEK_SET);
  if (fread(symtab, symtab_shdr->sh_size, 1, fp) != 1) {
    Log("Failed to read symbol table");
    free(symtab);
    free(shdr);
    fclose(fp);
    return;
  }

  // 读取完整字符串表
  char *strtab = malloc(strtab_shdr->sh_size);
  fseek(fp, strtab_shdr->sh_offset, SEEK_SET);
  if (fread(strtab, strtab_shdr->sh_size, 1, fp) != 1) {
    Log("Failed to read string table");
    free(strtab);
    free(symtab);
    free(shdr);
    fclose(fp);
    return;
  }

  // 统计函数符号数量
  func_count = 0;
  for (int i = 0; i < sym_count; i++) {
    if (ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC) {
      func_count++;
    }
  }

  // 分配函数表
  func_table = malloc(sizeof(FuncSymbol) * func_count);
  int func_idx = 0;

  // 填充函数表
  for (int i = 0; i < sym_count; i++) {
    if (ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC) {
      const char *name = strtab + symtab[i].st_name;
      strncpy(func_table[func_idx].name, name, 63);
      func_table[func_idx].name[63] = '\0';
      func_table[func_idx].start_addr = symtab[i].st_value;
      func_table[func_idx].end_addr = symtab[i].st_value + symtab[i].st_size;

      // 调试输出：显示前几个函数符号的信息
      if (func_idx < 5) {
        Log("ftrace: [%d] %s @ [" FMT_WORD ", " FMT_WORD ")",
            func_idx, func_table[func_idx].name,
            func_table[func_idx].start_addr, func_table[func_idx].end_addr);
      }
      func_idx++;
    }
  }

  Log("ftrace: loaded %d function symbols from %s", func_count, elf_file);

  free(strtab);
  free(symtab);
  free(shdr);
  fclose(fp);
}

// 记录函数调用
void ftrace_call(vaddr_t pc, vaddr_t target) {
  const char *func_name = find_func_name(target);
  if (strcmp(func_name, "???") == 0) {
    // 找不到符号时 panic，便于调试
    panic("ftrace: Cannot find symbol for address " FMT_WORD, target);
  }
  _Log(FMT_WORD ": %*scall [%s@" FMT_WORD "]\n",
       pc, call_depth * 2, "", func_name, target);
  call_depth++;
}

// 记录函数返回
void ftrace_ret(vaddr_t pc) {
  call_depth--;
  if (call_depth < 0) call_depth = 0;
  const char *func_name = find_func_name(pc);
  if (strcmp(func_name, "???") == 0) {
    // 找不到符号时 panic，便于调试
    panic("ftrace: Cannot find symbol for address " FMT_WORD, pc);
  }
  _Log(FMT_WORD ": %*sret  [%s]\n",
       pc, call_depth * 2, "", func_name);
}

#endif
