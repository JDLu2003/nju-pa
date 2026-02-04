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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

// enum {
//   TK_NOTYPE = 256, 
//   TK_NUM, 
//   TK_MULTI, TK_DIVI, TK_EQ, TK_PLUS, TK_SUB,
//   TK_LP, TK_RP,

//   /* TODO: Add more token types */

// };
#define TOKEN_LIST(X) \
    X(TK_NUM,    "NUM",    0,  0, "Number") \
    X(TK_MULTI,  "MUL",    10, 1, "Multiplication") \
    X(TK_DIVI,   "DIV",    10, 1, "Division") \
    X(TK_ADD,    "ADD",    5,  1, "Addition") \
    X(TK_SUB,    "SUB",    5,  1, "Subtraction") \
    X(TK_EQ,     "EQ",     3,  1, "Equality") \
    X(TK_LP,     "LP",     20, 0, "Left Parenthesis") \
    X(TK_RP,     "RP",     20, 0, "Right Parenthesis")

enum {
  TK_NOTYPE = 256,
#define GENERATE_ENUM(id, str, prec, is_op, desc) id,
    TOKEN_LIST(GENERATE_ENUM)
#undef GENERATE_ENUM
};

const char* get_token_name(int type) {
  switch (type) {
#define GENERATE_CASE(id, str, prec, is_op, desc)case id: return str;
      TOKEN_LIST(GENERATE_CASE)
#undef GENERATE_CASE
      default: return "UNKNOWN";
  }
}

bool is_operator(int type) {
  switch (type) {
#define GENERATE_IS_OP(id, str, prec, is_op, desc) case id: return is_op;
    TOKEN_LIST(GENERATE_IS_OP)
#undef GENERATE_IS_OP
    default: return 0;
  }
}

int get_operator_priority(int type) {
  switch (type) {
#define GENERATE_GET_PRIORITY(id, str, prec, is_op, desc) case id: return prec;
    TOKEN_LIST(GENERATE_GET_PRIORITY)
#undef GENERATE_GET_PRIORITY
    default: return -1;
  }
}


static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_ADD},         // plus
  {"==", TK_EQ},        // equal
  {"[0-9]+", TK_NUM},     // number
  {"\\*", TK_MULTI},
  {"\\/", TK_DIVI},
  {"\\-", TK_SUB},
  {"\\(", TK_LP},         // left parenthesis
  {"\\)", TK_RP},         // right parenthesis
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static void tokens_display() {
  for (int i = 0; i < nr_token; i++) {
    printf("Token %d: type=%s", i, get_token_name(tokens[i].type));
    if (tokens[i].type == TK_NUM) {
      printf(", str=%s", tokens[i].str);
    }
    printf("\n");
  }
}

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        // switch (rules[i].token_type) {
        //   default: TODO();
        // }
        if (rules[i].token_type == TK_NOTYPE) {
          break; 
       }
      
        if (nr_token >= 32) {
            printf("Error: Too many tokens.\n");
            return false;
        }
        tokens[nr_token].type = rules[i].token_type;
        if (rules[i].token_type == TK_NUM) {
            if (substr_len >= 32) { // 32 是 Token 结构体中 str 的大小
                printf("Error: number too long\n");
                return false;
            }
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
        }
        nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}
