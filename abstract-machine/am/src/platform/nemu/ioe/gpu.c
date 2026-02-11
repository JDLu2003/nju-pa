#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

static int screen_width = 0;
static int screen_height = 0;

void __am_gpu_config(AM_GPU_CONFIG_T *cfg);

void __am_gpu_init() {
  AM_GPU_CONFIG_T cfg;
  __am_gpu_config(&cfg);
  screen_width = cfg.width;
  screen_height = cfg.height;
  // int i;
  // uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  // for (i = 0; i < screen_width * screen_height; i ++) fb[i] = i;
  // outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t screen_info = inl(VGACTL_ADDR);
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = (int)(screen_info>>16), .height = (int)screen_info & 0xFFF,
    .vmemsz = 0
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  uint32_t *pixels = (uint32_t *)ctl->pixels;

  for (int j = 0; j < ctl->h; j++) {
    for (int i = 0; i < ctl->w; i++) {
      int dest_x = ctl->x + i;
      int dest_y = ctl->y + j;

      if (dest_x >= 0 && dest_x < screen_width &&
          dest_y >= 0 && dest_y < screen_height) {
        fb[dest_y * screen_width + dest_x] = pixels[j * ctl->w + i];
      }
    }
  }

  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
