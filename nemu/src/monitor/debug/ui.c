#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"
#include <dirent.h> // for c language, file directory operations (use 'man opendir' for more information)
#include <unistd.h> // for c language, get work path (use 'man getcwd' for more information)
#include <stdlib.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

/* define functions */
static int cmd_pwd(char *);
static int cmd_echo(char *);
static int cmd_si(char *);
static int cmd_ls(char *);
static int cmd_info(char *);
static int cmd_p(char *);
static int cmd_x(char *);
static int cmd_w(char *);
static int cmd_d(char *);

void cpu_exec(uint64_t);
void isa_reg_display();
/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  /* TODO: Add more commands *
   * you should add more commands as described in our manual
   */
  { "info", "Print information of register and watchpoints", cmd_info },
  { "si", "Execute for N steps, if N is not given, exec_once", cmd_si },
  { "x", "Scan Memory from start, from total N bytes", cmd_x },
  { "p", "Compute the value of an expression", cmd_p },
  { "echo", "Print the characters given by user", cmd_echo }, // add by wuran
  { "pwd", "Print current work path", cmd_pwd }, // add by wuran
  { "ls", "List all files in give path", cmd_ls },
  { "w", "Add a new watchpoint", cmd_w },
  { "d", "Delete a watchpoint or all watchpoints", cmd_d },
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0])) // number of commands

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("\033[1m\033[33m [%s]\033[0m - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("\033[1m\033[33m [%s]\033[0m - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_echo(char *args){
  // char * arg = strtok(args, " ");
  if(args != NULL)
    printf("%s\n", args);
  else printf("\n");
  return 0;
}

static int cmd_pwd(char * args){
  char buf[256];
  if (getcwd(buf, 256) != 0)
    printf("%s\n", buf);
  return 0;
}

static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");

  int n;
  if (arg == NULL)
    cpu_exec(n = 1);
  else {
    if (!sscanf(arg, "%d", &n) || n <= 0)
      printf("Please enter positive interger!\n");
    else
      cpu_exec(n);
  }
  printf("execute for %d steps finished.\n", n);

  return 0;
}

static int cmd_ls(char *args) {
  char *arg = strtok(NULL, " ");

  char cwd[256];
  if (arg == NULL) {
    if (!getcwd(cwd, 256)) return 0;
  }
  else
    sscanf(arg, "%s", cwd);

  DIR *dir_name = opendir(cwd);
  if (dir_name == NULL) {
    printf("ls: cannot access '%s': No such file or directory\n", cwd);
    return 0;
  }
  struct dirent *dir = readdir(dir_name);
  while (dir) {
    struct stat st;
    char name[256];
    strcpy(name, cwd);
    strcat(name, "/");
    strcat(name, dir->d_name);
   	stat(name, &st);
    if (S_ISDIR(st.st_mode))
      printf("\33[1;34m%s\t\33[0m", dir->d_name);
    else
      printf("%s\t", dir->d_name);
    dir = readdir(dir_name);
  }
  closedir(dir_name);
  putchar('\n');

  return 0;
}

static int cmd_info(char *args) {
  char *arg = strtok(NULL, " ");

	if(arg == NULL) {
	  printf("Please enter \"r\" or \"pc\" or \"w\" after \"info\"\n");
	  return 0;
	}

	if (!strcmp(arg, "r")) {
    isa_reg_display();
  }
  else if (!strcmp(arg, "pc"))
    printf("\033[1m\033[33m[pc]:\033[0m , address: 0x%x, value: 0x%x\n", cpu.pc, isa_vaddr_read(cpu.pc, 8));
  else if (!strcmp(arg, "w"))
    print_wp();
  else
    printf("Please enter \"r\" or \"pc\" or \"w\" after \"info\"\n");
  return 0;
}
static int cmd_p(char *args) {
	bool succ = true, *psucc = &succ;
	uint32_t result = expr(args, psucc);
  if (succ)
  	printf("Result is in hex: 0x%xH, is %d in dec\n", result, result);
  return 0;
}

static int cmd_x(char *args) {
  int n;
  char arg[256];
  if (!sscanf(args, "%d %[^\n]", &n, arg)) {
    printf("Pleas enter valid input\n");
    return 0;
  }
	bool succ = true, *psucc = &succ;
	uint32_t result = expr(arg, psucc);
  if (n <= 0 || !succ) {
    printf("Pleas enter valid input\n");
    return 0;
  }

  int i;
  uint8_t *p = pmem + result;
  for (i = 0; i < n; i++) {
    printf("0x%08x:", result + (i << 2));
    for (int j = 0; j < 4; j++)
      printf("\t0x%02x", *(p++));
    putchar('\n');
  }
  return 0;
}

static int cmd_w(char *args) {
  bool success = true;
  int val = expr(args, &success);
  if (success) new_wp(args, val);
  return 0;
}

static int cmd_d(char *args) {
  char *arg = strtok(NULL, " ");

  if(arg == NULL) delete_all_wp();
  else {
    int n;
    if (!sscanf(arg, "%d", &n) || n <= 0) {
      printf("Please enter positive interger!\n");
      return 0;
    }
    free_wp(n);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}