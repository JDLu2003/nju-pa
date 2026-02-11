/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
* ... (版权声明略)
***************************************************************************************/

#include <device/map.h>
#include <utils.h>

// 定义按键按下的掩码。如果该位为1，表示按键按下；为0表示按键弹起。
// 0x8000 即第15位（从0开始计）
#define KEYDOWN_MASK 0x8000

// 如果不是在 AM (Abstract Machine) 环境下运行（即作为独立的模拟器运行）
#ifndef CONFIG_TARGET_AM
#include <SDL2/SDL.h>

// 使用 X-Macro 技巧定义 NEMU 支持的所有按键列表
// 这种方式方便同时生成枚举值、字符串映射等
#define NEMU_KEYS(f) \
  f(ESCAPE) f(F1) f(F2) f(F3) f(F4) f(F5) f(F6) f(F7) f(F8) f(F9) f(F10) f(F11) f(F12) \
f(GRAVE) f(1) f(2) f(3) f(4) f(5) f(6) f(7) f(8) f(9) f(0) f(MINUS) f(EQUALS) f(BACKSPACE) \
f(TAB) f(Q) f(W) f(E) f(R) f(T) f(Y) f(U) f(I) f(O) f(P) f(LEFTBRACKET) f(RIGHTBRACKET) f(BACKSLASH) \
f(CAPSLOCK) f(A) f(S) f(D) f(F) f(G) f(H) f(J) f(K) f(L) f(SEMICOLON) f(APOSTROPHE) f(RETURN) \
f(LSHIFT) f(Z) f(X) f(C) f(V) f(B) f(N) f(M) f(COMMA) f(PERIOD) f(SLASH) f(RSHIFT) \
f(LCTRL) f(APPLICATION) f(LALT) f(SPACE) f(RALT) f(RCTRL) \
f(UP) f(DOWN) f(LEFT) f(RIGHT) f(INSERT) f(DELETE) f(HOME) f(END) f(PAGEUP) f(PAGEDOWN)

// 定义一个宏，用于生成枚举成员名，例如 NEMU_KEY_ESCAPE
#define NEMU_KEY_NAME(k) NEMU_KEY_ ## k,

// 定义按键的枚举值
enum {
  NEMU_KEY_NONE = 0,
  MAP(NEMU_KEYS, NEMU_KEY_NAME) // 展开宏列表，生成所有按键的枚举
};

// 定义一个宏，用于建立 SDL 扫描码到 NEMU 内部键值的映射
#define SDL_KEYMAP(k) keymap[SDL_SCANCODE_ ## k] = NEMU_KEY_ ## k;
static uint32_t keymap[256] = {}; // 存储映射关系的数组

// 初始化映射表，将 SDL 的按键常量映射到 NEMU 内部定义的常量
static void init_keymap() {
  MAP(NEMU_KEYS, SDL_KEYMAP)
}

// 定义一个环形队列来存储按键事件，防止按键丢失
#define KEY_QUEUE_LEN 1024
static int key_queue[KEY_QUEUE_LEN] = {};
static int key_f = 0, key_r = 0; // 队头(f)和队尾(r)指针

// 将按键码放入队列
static void key_enqueue(uint32_t am_scancode) {
  key_queue[key_r] = am_scancode;
  key_r = (key_r + 1) % KEY_QUEUE_LEN;
  // 如果队列满了，触发断言（在模拟器中通常不希望丢失输入）
  Assert(key_r != key_f, "key queue overflow!");
}

// 从队列中取出一个按键码
static uint32_t key_dequeue() {
  uint32_t key = NEMU_KEY_NONE;
  if (key_f != key_r) {
    key = key_queue[key_f];
    key_f = (key_f + 1) % KEY_QUEUE_LEN;
  }
  return key;
}

// 外部接口：由 SDL 事件循环调用，将捕获到的物理按键发送到模拟器
void send_key(uint8_t scancode, bool is_keydown) {
  // 仅在模拟器运行中且该按键在映射表内时处理
  if (nemu_state.state == NEMU_RUNNING && keymap[scancode] != NEMU_KEY_NONE) {
    // 组合按键码：低位是键值，高位（KEYDOWN_MASK）表示按下还是弹起
    uint32_t am_scancode = keymap[scancode] | (is_keydown ? KEYDOWN_MASK : 0);
    key_enqueue(am_scancode);
  }
}

#else // 如果是在 AM 环境下运行（例如 NEMU 跑在另一个 NEMU 上）

#define NEMU_KEY_NONE 0

// 直接通过 AM 的 IO 读取接口获取按键
static uint32_t key_dequeue() {
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  uint32_t am_scancode = ev.keycode | (ev.keydown ? KEYDOWN_MASK : 0);
  return am_scancode;
}
#endif

// 模拟 i8042 的数据寄存器在内存中的映射位置
static uint32_t *i8042_data_port_base = NULL;

// 键盘 I/O 处理函数：当 CPU 读取键盘数据端口时被触发
static void i8042_data_io_handler(uint32_t offset, int len, bool is_write) {
  // 键盘端口通常是只读的（对于获取按键而言）
  assert(!is_write);
  // 偏移量必须为0（因为该设备只映射了4字节）
  assert(offset == 0);
  // 当 CPU 读取该端口时，从队列中取出一个按键码放入数据寄存器供 CPU 读取
  i8042_data_port_base[0] = key_dequeue();
}

// 初始化 i8042 键盘设备
void init_i8042() {
  // 申请 4 字节的空间作为数据寄存器
  i8042_data_port_base = (uint32_t *)new_space(4);
  i8042_data_port_base[0] = NEMU_KEY_NONE;

#ifdef CONFIG_HAS_PORT_IO
  // 如果支持端口 I/O (x86 风格)，将设备注册到特定的端口地址
  add_pio_map ("keyboard", CONFIG_I8042_DATA_PORT, i8042_data_port_base, 4, i8042_data_io_handler);
#else
  // 如果使用内存映射 I/O (RISC-V/MIPS 风格)，将设备注册到特定的内存地址
  add_mmio_map("keyboard", CONFIG_I8042_DATA_MMIO, i8042_data_port_base, 4, i8042_data_io_handler);
#endif

  // 如果不是 AM 环境，初始化 SDL 按键映射表
  IFNDEF(CONFIG_TARGET_AM, init_keymap());
}
