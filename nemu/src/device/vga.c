/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
* ... (省略许可证信息)
***************************************************************************************/

#include <common.h>
#include <device/map.h>

/* 
 * 定义默认的屏幕宽度和高度。
 * MUXDEF 是一个宏，如果定义了 CONFIG_VGA_SIZE_800x600，则使用 800x600，否则使用 400x300。
 */
#define SCREEN_W (MUXDEF(CONFIG_VGA_SIZE_800x600, 800, 400))
#define SCREEN_H (MUXDEF(CONFIG_VGA_SIZE_800x600, 600, 300))

/* 获取当前屏幕宽度 */
static uint32_t screen_width() {
  // 如果是运行在 AM (Abstract Machine) 平台上，通过 io_read 读取硬件配置；
  // 否则返回上面定义的宏 SCREEN_W。
  return MUXDEF(CONFIG_TARGET_AM, io_read(AM_GPU_CONFIG).width, SCREEN_W);
}

/* 获取当前屏幕高度 */
static uint32_t screen_height() {
  return MUXDEF(CONFIG_TARGET_AM, io_read(AM_GPU_CONFIG).height, SCREEN_H);
}

/* 计算显存大小：宽 * 高 * 每个像素占用的字节数 (uint32_t 即 4 字节，ARGB) */
static uint32_t screen_size() {
  return screen_width() * screen_height() * sizeof(uint32_t);
}

static void *vmem = NULL;          // 指向显存（Frame Buffer）的指针
static uint32_t *vgactl_port_base = NULL; // 指向 VGA 控制寄存器基地址的指针

#ifdef CONFIG_VGA_SHOW_SCREEN
#ifndef CONFIG_TARGET_AM
/* 
 * 如果是在宿主机上直接运行 NEMU（非 AM 模式），使用 SDL2 库来创建窗口并显示图像 
 */
#include <SDL2/SDL.h>

static SDL_Renderer *renderer = NULL; // SDL 渲染器
static SDL_Texture *texture = NULL;   // SDL 纹理（对应显存内容）

static void init_screen() {
  SDL_Window *window = NULL;
  char title[128];
  // 设置窗口标题，包含当前模拟的指令集架构名称
  sprintf(title, "%s-NEMU", str(__GUEST_ISA__));
  SDL_Init(SDL_INIT_VIDEO);
  // 创建窗口和渲染器。如果是 400x300 模式，通常会放大 2 倍显示以方便观察。
  SDL_CreateWindowAndRenderer(
      SCREEN_W * (MUXDEF(CONFIG_VGA_SIZE_400x300, 2, 1)),
      SCREEN_H * (MUXDEF(CONFIG_VGA_SIZE_400x300, 2, 1)),
      0, &window, &renderer);
  SDL_SetWindowTitle(window, title);
  // 创建一个 32 位 ARGB 格式的纹理
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STATIC, SCREEN_W, SCREEN_H);
  SDL_RenderPresent(renderer);
}

/* 将显存 (vmem) 中的数据同步到 SDL 窗口中 */
static inline void update_screen() {
  // 更新纹理数据
  SDL_UpdateTexture(texture, NULL, vmem, SCREEN_W * sizeof(uint32_t));
  SDL_RenderClear(renderer);
  // 将纹理拷贝到渲染器
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  // 刷新屏幕显示
  SDL_RenderPresent(renderer);
}
#else
/* 如果是在 AM 平台上运行，初始化为空，更新操作调用 AM 的 IO 接口 */
static void init_screen() {}

static inline void update_screen() {
  // 调用 AM 的绘图接口将 vmem 内容画到屏幕上
  io_write(AM_GPU_FBDRAW, 0, 0, vmem, screen_width(), screen_height(), true);
}
#endif
#endif

/* 
 * 该函数由 NEMU 的主循环或设备更新逻辑调用。
 * 作用：检查同步寄存器，决定是否刷新屏幕。
 */
void vga_update_screen() {
  // TODO: 当同步寄存器 (sync register) 为非零时，调用 `update_screen()`，
  // 然后将同步寄存器清零。
  // 同步寄存器通常位于 vgactl_port_base[1]。
}

/* 初始化 VGA 设备 */
void init_vga() {
  // 1. 为 VGA 控制寄存器分配空间（8 字节：4字节用于分辨率，4字节用于同步信号）
  vgactl_port_base = (uint32_t *)new_space(8);
  
  // 2. 在控制寄存器的第一个 32 位中存储屏幕的高和宽
  // 高 16 位存宽度，低 16 位存高度
  vgactl_port_base[0] = (screen_width() << 16) | screen_height();

  // 3. 注册 I/O 映射
#ifdef CONFIG_HAS_PORT_IO
  // 使用端口 I/O (Port I/O) 映射
  add_pio_map ("vgactl", CONFIG_VGA_CTL_PORT, vgactl_port_base, 8, NULL);
#else
  // 使用内存映射 I/O (MMIO) 映射
  add_mmio_map("vgactl", CONFIG_VGA_CTL_MMIO, vgactl_port_base, 8, NULL);
#endif

  // 4. 为显存分配空间并映射到指定的物理地址 (CONFIG_FB_ADDR)
  vmem = new_space(screen_size());
  add_mmio_map("vmem", CONFIG_FB_ADDR, vmem, screen_size(), NULL);

  // 5. 如果配置了显示屏幕，则初始化 SDL 窗口并将显存清零（黑屏）
  IFDEF(CONFIG_VGA_SHOW_SCREEN, init_screen());
  IFDEF(CONFIG_VGA_SHOW_SCREEN, memset(vmem, 0, screen_size()));
}
