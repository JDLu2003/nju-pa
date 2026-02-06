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

word_t vaddr_read(vaddr_t addr, int len);

// enum {
//   TK_NOTYPE = 256, 
//   TK_NUM, 
//   TK_MULTI, TK_DIVI, TK_EQ, TK_PLUS, TK_SUB,
//   TK_LP, TK_RP,

//   /* TODO: Add more token types */

// };
#define TOKEN_LIST(X) \
    X(TK_NUM,     "NUM",      0,  0, "Number") \
    X(TK_MULTI,   "MUL",      10, 1, "Multiplication") \
    X(TK_DIVI,    "DIV",      10, 1, "Division") \
    X(TK_ADD,     "ADD",      5,  1, "Addition") \
    X(TK_SUB,     "SUB",      5,  1, "Subtraction") \
    X(TK_EQ,      "EQ",       3,  1, "Equality") \
    X(TK_LP,      "LP",       20, 0, "Left Parenthesis") \
    X(TK_RP,      "RP",       20, 0, "Right Parenthesis") \
    X(TK_HEX,     "HEX",      0,  0, "HEX Number") \
    X(TK_NOEQ,    "NOEQ",     3,  1, "No Equality") \
    X(TK_AND,     "AND",      2,  1, "AND") \
    X(TK_DER,     "DER",      15, 1, "Dereference") \
    X(TK_REG,     "REG",      0,  0, "Register")

#define NEXT_SHOULD_BE_TK_DER(x) (x != TK_NUM && x != TK_HEX && x != TK_RP && x != TK_REG)

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
  {"0[xX][0-9a-fA-F]+", TK_HEX}, // hexadecimal number
  {"\\$(x[0-9]|x[1-2][0-9]|x3[0-1]|zero|ra|sp|gp|tp|t[0-6]|s[0-1][0-1]?|a[0-7]|fp|pc|0)", TK_REG}, // register
  {"[0-9]+", TK_NUM},     // decimal number
  {"\\*", TK_MULTI},      // multiplication
  {"\\/", TK_DIVI},       // division
  {"\\-", TK_SUB},        // subtraction
  {"\\(", TK_LP},         // left parenthesis
  {"\\)", TK_RP},         // right parenthesis
  {"!=", TK_NOEQ},        // not equal
  {"&&", TK_AND},         // logical AND
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

#define TOKEN_MAX 256

static Token tokens[TOKEN_MAX] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static __attribute__((unused)) void tokens_display() {
  for (int i = 0; i < nr_token; i++) {
    printf("Token %d: type=%s", i, get_token_name(tokens[i].type));
    if (tokens[i].type == TK_NUM) {
      printf(", str=%s", tokens[i].str);
    } else if (tokens[i].type == TK_HEX) {
      printf(", str=%s", tokens[i].str);
    } else if (tokens[i].type == TK_REG) {
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
      
        if (nr_token >= TOKEN_MAX) {
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
        } else if (rules[i].token_type == TK_HEX) {
            if (substr_len >= 32) { // 32 是 Token 结构体中 str 的大小
                printf("Error: hexadecimal number too long\n");
                return false;
            }
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
        } else if (rules[i].token_type == TK_REG) {
            if (substr_len >= 32) { // 32 是 Token 结构体中 str 的大小
                printf("Error: register name too long\n");
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

  // tokens_display();
  return true;
}

/// @brief 检查是否被括号包围
/// @param p 
/// @param q 
/// @return 
static bool check_parentheses(int p, int q) {
  if (tokens[p].type != TK_LP 
    || tokens[q].type != TK_RP) {
    return false;
  }
  int count = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == TK_LP)
    {
      count += 1;
    } else if (tokens[i].type == TK_RP)
    {
      count -= 1;
    }
    if (count == 0 && i < q) {
      return false;
    }
  }
  return count == 0;
};


/// @brief 求主操作符在 tokens[p..q] 中的位置
/// @param p 
/// @param q 
/// @return 
static int main_operator(int p, int q) {
  int res = -1;
  int min_priority = INT32_MAX;
  int pair_count = 0;

  for (int i = p; i <= q; i++) {
    if (tokens[i].type == TK_LP) {
      pair_count++;
      continue;
    }
    if (tokens[i].type == TK_RP) {
      pair_count--;
      continue;
    }
    if (pair_count != 0)
    {
      continue;
    }
    if (!is_operator(tokens[i].type)) {
      continue;
    }
    int cur_priority = get_operator_priority(tokens[i].type);
    if (cur_priority <= min_priority) {
      min_priority = cur_priority;
      res = i;
    }
  }
  return res;
};

uint32_t eval(int p, int q) {
  Log("eval called with p=%d, q=%d", p, q);
  int res;
  if (p > q) {
    printf("Bad expression\n");
    assert(0);
  } 
  else if (p == q) {
    if (tokens[p].type == TK_NUM) {
      res =  (uint32_t)atoi(tokens[p].str);
    } else if (tokens[p].type == TK_HEX) {
      res =  (uint32_t)strtoul(tokens[p].str, NULL, 16);
    } else if (tokens[p].type == TK_REG) {
      bool success;
      res = isa_reg_str2val(tokens[p].str + 1, &success); 
      if (!success) {
        printf("Unknown register %s\n", tokens[p].str);
        assert(0);
      }
    } else {
      printf("Unexpected token type %d\n", tokens[p].type);
      assert(0);
    }
  } 
  else if (check_parentheses(p, q)) {
    return eval(p + 1, q - 1);
  } 
  else {
    if (tokens[p].type == TK_DER) {
      uint32_t addr = eval(p + 1, q);
      res = vaddr_read(addr, 4);
      return res;
    }
    int op = main_operator(p, q);
    Log("main operator at position %d, type=%s", op, get_token_name(tokens[op].type));
    uint32_t val1 = eval(p, op - 1);
    uint32_t val2 = eval(op + 1, q);
    switch (tokens[op].type) {
      case TK_ADD: res = val1 + val2; break;
      case TK_SUB: res =  val1 - val2; break;
      case TK_MULTI: res =  val1 * val2; break;
      case TK_EQ: res =  val1 == val2; break;
      case TK_DIVI: 
        if (val2 == 0) {
          printf("Division by zero\n");
          // assert(0);
        }
        res =  val1 / val2;
        break;
      case TK_NOEQ: res =  val1 != val2; break;
      case TK_AND: res =  val1 && val2; break;
      default:
        printf("Unexpected operator %d\n", tokens[op].type);
        assert(0);
    }
  }
  Log("eval returning %d for p=%d, q=%d", res, p, q);
  return res;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  // TODO();
  
  *success = true;
  // TODO(); // 错误处理
  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == TK_MULTI && 
      (i == 0 || NEXT_SHOULD_BE_TK_DER(tokens[i - 1].type))
    ) {
      tokens[i].type = TK_DER;
    }
  }
  // tokens_display();
  Log("Evaluating expression: %s, nr=%d\n", e, nr_token);

  return eval(0, nr_token-1);
}

void expr_test() {
  FILE *f = fopen("./tools/gen-expr/input", "r");
  if (f == NULL) {
    printf("cannot open file\n");
    assert(0);
  }

  char line[65536];
  uint32_t num;
  char str[65536];
  int index = 0;
  while (fgets(line, sizeof(line), f) != NULL)
  {
    if (fscanf(f, "%u %[^\n]", &num, str) == 2){
      printf("fuzz[%d]: %u==%s\n", index, num, str);

      bool success;
      uint32_t res = expr(str, &success);
      assert(num==res);
    } else {
      printf("error line[%d]: %s\n", index, line);
    }
    index++;
  }
  

}