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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[128];
  word_t val_old;
  bool is_used;

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].expr[0] = '\0';
    wp_pool[i].is_used = false;
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

// 分配 wp
static WP* wp_alloc() {
  if (free_ == NULL) {
    printf("Error: No free watchpoint\n");
    return NULL;
  }
  WP *wp = free_;
  free_ = free_->next;
  wp->next = head;
  head = wp;
  wp->is_used = false;
  wp->val_old = 0;
  wp->expr[0] = '\0';
  return wp;
}

// 回收wp，helper func
static void wp_reclaim(WP *wp) {
  if (wp == NULL) {
    Log("Error: wp is null\n");
    return;
  }
  if (head == wp) {
    head = wp->next;
    goto RECLAIM;
  }
  WP *p = head;
  while (p != NULL && p->next != wp) {
    p = p->next;
  }
  if (p == NULL) {
    Log("Error: cannot find the wp\n");
    return;
  }
  p->next = wp->next;
RECLAIM:
  wp->next = free_;
  free_ = wp;

  wp->val_old = 0;
  wp->expr[0] = '\0';
}

int wp_new(char *exp) {
  bool success = false;
  int res = expr(exp, &success);
  if (!success) {
    printf("Error: load illegal watchpoint expression\n");
    return -1;
  }
  WP *w = wp_alloc();
  if (w == NULL) {
    return -1;
  }

  strcpy(w->expr, exp);
  w->is_used = true;
  w->val_old = res;
  Log("new watchpoint %d: %s\n", w->NO, w->expr);
  return w->NO;
}

int wp_delete(int no) {
  WP *wp = head;
  while (wp != NULL) {
    if (wp->NO == no) {
      wp_reclaim(wp);
      return no;
    }
    wp = wp->next;
  }
  return -1;
}

void wp_exist_display() {
  WP *wp = head;
  if (wp == NULL) {
    printf("No watchpoint\n");
    return;
  }
  printf("Watchpoints:\n");
  printf("NO\tExpression\tOld Value\n");
  while (wp != NULL) {
    printf("%d\t%s\t" FMT_WORD "\n", wp->NO, wp->expr, wp->val_old);
    wp = wp->next;
  }
}

int wp_check() {
  WP *wp = head;
  bool success;
  while (wp != NULL) {
    word_t val_new = expr(wp->expr, &success);
    if (!success) {
      panic("load illegal watchpoint expression");
      return -1;
    }
    if (val_new != wp->val_old || wp->is_used == false) {
      printf("Watchpoint %d triggered: %s\n", wp->NO, wp->expr);
      printf("Old value: " FMT_WORD ", New value: " FMT_WORD "\n", wp->val_old, val_new);
      wp->val_old = val_new;
      return wp->NO;
    }
    wp = wp->next;
  }
  return -1;
}


