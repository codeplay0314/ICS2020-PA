#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static uint32_t wp_num = 0;

// TODO: try to re-organize, you can abort head and free_ pointer while just use static int index
static WP *head, *free_ = NULL;

void init_wp_pool() {
  int i;
  wp_num = 0;
  for (i = 0; i < NR_WP - 1; i++) {
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;
  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
void new_wp(char* msg, int val) {
  if (!wp_num) init_wp_pool();
  if (free_ == NULL) {
    printf("Failed! Too many watchpoint.\n");
    return;
  }
  WP* wp = free_;
  free_ = free_->next;
  wp->NO = ++wp_num, wp->val = val, wp->next = head;
  strcpy(wp->msg, msg);
  head = wp;
  printf("Watchpoint %d\texpr: %s val: 0x%x\n", wp->NO, wp->msg, wp->val);
  return;
}

void free_wp(int no) {
  WP* wp = head, * pre = NULL;
  while (wp) {
    if (wp->NO == no) {
      if (pre == NULL) head = head->next;
      else pre->next = wp->next;
      wp->next = free_;
      free_ = wp;
      printf("Delete watchpoint %d\n", no);
      return;
    }
    pre = wp, wp = wp->next;
  }
  printf("No such watchpoint, numbered as %d\n", no);
  return;
}

void print_wp() {
  int cnt = 0;
  WP* wp_write[NR_WP], * wp = head;
  while (wp) {
    wp_write[cnt++] = wp;
    wp = wp->next;
  }
  if (!cnt)
    printf("No watchpoints\n");
  for (int i = cnt - 1; i >= 0; i--)
    printf("%d\twatchpoint\texpr: %s val: 0x%x\n", wp_write[i]->NO, wp_write[i]->msg, wp_write[i]->val);
  return;
}

void delete_all_wp() {
  if (free_)  {
    while (free_->next) free_ = free_->next;
    free_->next = head, head = NULL;
  }
  else
    free_ = head, head = NULL;
  printf("All watchpoints cleared\n");
  return;
}

static bool changed(WP* wp) {
  bool success = true;
  uint32_t val = expr(wp->msg, &success);
  if (val == wp->val) return false;
  wp->val = val;
  return true;
}

bool check_all_wp() {
  int cnt = 0;
  WP* wp_write[NR_WP], * wp = head;
  uint32_t old_val[NR_WP];
  while (wp) {
    old_val[cnt] = wp->val;
    if (changed(wp))
      wp_write[cnt++] = wp;
    wp = wp->next;
  }
  if (!cnt) return false;
  for (int i = cnt - 1; i >= 0; i--)
    printf("Watchpoint %d: %s\n\nOld value = 0x%x\nNew value = 0x%x\n", wp_write[i]->NO, wp_write[i]->msg, old_val[i], wp_write[i]->val);
  return true;
}