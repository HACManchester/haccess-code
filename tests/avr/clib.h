#ifdef __cplusplus
extern "C" {
#endif
  
extern  char * itoa (int val, char *s, int radix);
extern  char * ltoa (long val, char *s, int radix);
extern  char * utoa (unsigned int val, char *s, int radix);
extern  char * ultoa (unsigned long val, char *s, int radix);

extern  char * dtostrf (double __val, signed char __width, unsigned char __prec, char *__s);

#ifdef __cplusplus
}
#endif
