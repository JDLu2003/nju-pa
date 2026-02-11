#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t kbd_code = inl(KBD_ADDR);

  kbd->keydown = (kbd_code & KEYDOWN_MASK) != 0;
  kbd->keycode = kbd_code & (KEYDOWN_MASK - 1);
}
