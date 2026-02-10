#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <stdbool.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap);

char *itoa(long long val, char *str, int base) {
  if (base < 2 || base > 36) {
    *str = '\0';
    return str;

  }
  char *ptr = str;
  char *ptr1 = str;
  char tmp_char;
  long long  tmp_val;
  bool is_neg = false;
  
  if (val < 0 && base == 10) {
    is_neg = true;
    tmp_val = -val;
  } else {
    tmp_val = val;
  }

  do {
    long long  remainder = tmp_val % base;
    *ptr = (remainder < 10) ? (remainder + '0') : (remainder - 10 + 'a');
    ptr++;
    tmp_val /= base;
  } while (tmp_val > 0);

  if (is_neg) {
    *ptr = '-';
    ptr++;
  }

  *ptr = '\0';
  ptr--;

  // reverse
  while(ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr = *ptr1;
    *ptr1 = tmp_char;
    ptr--;
    ptr1++;
  }
  return str;
}

int printf(const char *fmt, ...) {
  char buf[8192];
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  for (int i = 0; i < len; i++) {
    putch(buf[i]);
  }
  return len;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, -1, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(out, -1, fmt, ap);
  va_end(ap);
  return len;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return len;
}

// 核心函数，将全部调用这个函数来实现其他功能
int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  size_t count = 0;
  const char *p = fmt;

  #define EMIT(c) do { \
    if (count < n - 1) { out[count] = c; } \
    count++; \
  } while(0)

  while(*p) {
    if (*p != '%') {
      EMIT(*p);
      p++;
    } else {
      p++;
      switch (*p) {
        case 'd':
        case 'u':
        {
          int val = va_arg(ap, int);
          char buf[64];
          char *ptr = itoa(val, buf, 10);
          while (*ptr) {
            EMIT(*ptr++);
          }
          break;
        }
        case 'x':
        case 'p':
        {
          unsigned int val = va_arg(ap, unsigned int);
          char buf[64];
          char *ptr = itoa(val, buf, 16);
          while (*ptr) {
            EMIT(*ptr++);
          }
          break;
        }
        case 's':
        {
          char *val = va_arg(ap, char *);
          if (val == NULL) val = "(null)";
          while (*val) {
            EMIT(*val++);
          }
          break;
        }
        default:
          panic("Unsupport type in vsnprint");
      }
      p++;
    }
  }
  if (n > 0) {
    if (count < n) out[count] = '\0';
    else out[n - 1] = '\0';
  }
  return count;
}

#endif
