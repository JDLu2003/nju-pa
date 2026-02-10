#include <common.h>
void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);

#define IRINGBUF_SIZE 16

typedef struct {
  word_t pc;
  uint32_t inst;
  char log[128];
} IRingBufEntry;

static IRingBufEntry iringbuf[IRINGBUF_SIZE];
static int p_current = 0;
static bool full = false;

void iringbuf_write(word_t pc, uint32_t inst) {
  iringbuf[p_current].pc = pc;
  iringbuf[p_current].inst = inst;
  char *log = iringbuf[p_current].log;
  uint64_t pc_64 = (uint64_t)pc;

  disassemble(log, 128, pc_64, (uint8_t *)&iringbuf[p_current].inst, 4);
  p_current = (p_current + 1) % IRINGBUF_SIZE;
  if (p_current == 0) full = true;
}

void iringbuf_display() {
  if (!full && p_current == 0) return;

  int end = p_current;
  int i = full ? p_current : 0;
  
  printf("Most recent instructions:\n");
  do {
    printf(FMT_WORD " " FMT_WORD " %s\n", iringbuf[i].pc, iringbuf[i].inst, iringbuf[i].log);
    i = (i + 1) % IRINGBUF_SIZE;
  } while (i != end);
}

