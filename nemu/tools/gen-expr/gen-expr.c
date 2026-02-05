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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
#define BUF_SIZE 65536
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static uint32_t choose(int n) {
  return rand() % n;
}

static int length;

static int expr_push(const char *c) {
  size_t len = strlen(c);
  if (length + len >= BUF_SIZE) return -1;
  
  memcpy(buf + length, c, len);
  length += len;
  buf[length] = '\0';
  return 0;
}

static int gen_rand_space() {
  if (choose(10) < 3) {
    char local[5] = "";
    int count = choose(3) + 1;
    for (int i = 0; i < count; i++) {
      local[i] = ' ';
    }
    local[count] = '\0';
    return expr_push(local);
  }
  return 0;
}

static int gen_num() {
  if (gen_rand_space() != 0) return -1;
  char local[20] = {};
  uint32_t num = rand() % 2000;
  sprintf(local, "%uU", num);
  return expr_push(local);
}

static int gen_char(char c) {
  if (gen_rand_space() != 0) return -1;
  char local[2] = {c, '\0'};
  return expr_push(local);
}

static int gen_op() {
  if (gen_rand_space() != 0) return -1;
  switch (choose(4)) {
    case 0: return gen_char('+');
    case 1: return gen_char('-');
    case 2: return gen_char('*');
    case 3: return gen_char('/');
    default: return -1; 
  }
}

static int gen_rand_expr_inner(int depth) {
  int ret = 0;
  if (depth > 10) {
    return gen_num();
  }
  switch (choose(3)) {
    case 0:
      ret = gen_num();
      break;
    case 1:
      ret = gen_char('(');
      if (ret == 0) ret = gen_rand_expr_inner(depth + 1);
      if (ret == 0) ret = gen_char(')');
      break;
    case 2:
      ret = gen_rand_expr_inner(depth + 1);
      if (ret == 0) ret = gen_op();
      if (ret == 0) ret = gen_rand_expr_inner(depth + 1);
      break;
    default:
      ret = 0;
      break;
  }
  return ret;
}

static void gen_rand_expr() {
  buf[0] = '\0';
  length = 0;
  while (gen_rand_expr_inner(0) != 0 && length > 20) {
    buf[0] = '\0';
    length = 0;
    fprintf(stderr, "Error generating random expression\n");
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    for (int i = 0; buf[i] != '\0'; i++)
    {
      if (buf[i] == 'U') {
        buf[i] = ' ';
      }
    }
    

    printf("%u %s\n", result, buf);
  }
  return 0;
}
