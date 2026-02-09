#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  // panic("Not implemented");
  const char *p = s;
  while(*p) p++;
  return p - s;
}

char *strcpy(char *dst, const char *src) {
  // panic("Not implemented");
  char *d = dst;
  while ((*d++ = *src++));
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  // panic("Not implemented");
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dst[i] = src[i];
  }
  for ( ; i < n; i++) {
    dst[i] = '\0';
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  // panic("Not implemented");
  char *d = dst;
  const char *s = src;
  while (*d) ++d;
  while ((*d++ = *s++));
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  // panic("Not implemented");
  while (*s1 && (*s1 == *s2)) {
    s1++; s2++;
  }
  return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  // panic("Not implemented");
  if (n == 0) return 0;
  while (n > 1 && *s1 && *s1 == *s2) {
    s1++; s2++;
    n--;
  }
  return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void *memset(void *s, int c, size_t n) {
  // panic("Not implemented");
  unsigned char *p = (unsigned char *)s;
  while (n--) {
    *p++ = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  // panic("Not implemented");
  const unsigned char *s = src;
  unsigned char *d = dst;

  if (d > s && d < s + n) {
    s += n;
    d += n;
    while (n--) {
      *--d = *--s;
    }
  } else {
    while (n--) {
      *d++ = *s++;
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  // panic("Not implemented");
  return memmove(out, in, n);
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = s1, *p2 = s2; 
  while (n-- > 0) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++; p2++;
  }
  return 0;
}

#endif
