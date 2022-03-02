#include <x86intrin.h>
//#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define NB_MEASURE 128

static void outb(uint16_t port, uint8_t value) {
	asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}

//size_t strlen(const char *str) {
//    const char *s;
//
//    for (s = str; *s; ++s)
//        ;
//    return (s - str);
//}
//
///* reverse:  reverse string s in place */
//void reverse(char s[])
//{
//     int i, j;
//     char c;
//
//     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
//         c = s[i];
//         s[i] = s[j];
//         s[j] = c;
//     }
//}
///* itoa:  convert n to characters in s */
//void itoa(int n, char s[])
//{
//    int i, sign;
//
//    if ((sign = n) < 0)  /* record sign */
//        n = -n;          /* make n positive */
//    i = 0;
//    do {       /* generate digits in reverse order */
//        s[i++] = n % 10 + '0';   /* get next digit */
//    } while ((n /= 10) > 0);     /* delete it */
//    if (sign < 0)
//        s[i++] = '-';
//    s[i++] = '\n';
//    s[i] = '\0';
//    reverse(s);
//}

void print_measures(){
//    char *p;
//    char buff[256];

//    for(uint i = 1; i < n; i++){
//        itoa(measures[i], buff);
//        for (p = buff; *p; ++p){
//            outb(0xE9, *p);}
//        outb(0xE9, '\t');
//        itoa(measures[i]-measures[i-1], buff);
//        for (p = buff; *p; ++p){
//            outb(0xE9, *p);}
    outb(0xBE, 0);

//}

}

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {
//	const char *p;
    uint64_t *measures = (uint64_t *)0x10000+(NB_MEASURE*448)+1;

//    for (p = "Hello, world!\n"; *p; ++p){
//		outb(0xE9, *p);}

//    char buff[256];
//    unsigned long long test = __rdtsc();
//    itoa(test, buff);
//    for (p = buff; *p; ++p){
//        outb(0xE9, *p);}
//    outb(0xE9, '\n');
//    for(uint i = 1; i < n; i++){
//        itoa(test, buff);
//        for (p = buff; *p; ++p){
//            outb(0xE9, *p);}
//    outb(0xE9, '\n');}

    for(int i=0; i< NB_MEASURE;i++){
        *(measures++) = __rdtsc();
//        outb(0xE9, ' ');
    }


    print_measures();

	*(long *) 0x400 = 42;

	for (;;)
		asm("hlt" : /* empty */ : "a" (42) : "memory");
}

/* rdtsc */
extern __inline uint64_t
__attribute__((__gnu_inline__, __always_inline__, __artificial__))
__rdtsc ()
{
    return __builtin_ia32_rdtsc ();
}
