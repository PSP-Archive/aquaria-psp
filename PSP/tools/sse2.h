/*
 * Aquaria PSP port
 * Copyright (C) 2010 Andrew Church <achurch@achurch.org>
 *
 * tools/sse2.h: Macros for using SSE2 assembly on x86 or x86-64 CPUs.
 * Define USE_SSE2 and (if compiling for x86-64) ARCH_AMD64 before
 * including this header.
 */

#ifndef SSE2_H
#define SSE2_H

#ifdef USE_SSE2  // SSE2が有効の場合のみ

/*************************************************************************/

/* SSE2命令を使う場合、コンパイラが使わないようにしておくマクロ */
#define SSE2_INIT \
    asm volatile("":::"xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7")

/*************************************************************************/

/*
 * AMD64でコンパイルした場合、ベースレジスタによるメモリアクセスは必ずRxx
 * （RAXなどの64ビットレジスタ）を使わなければならない。そういうレジスタが
 * 存在しないx86との差を吸収するため、ここで各レジスタの名前をマクロとして
 * 定義する。
 */

#ifdef ARCH_AMD64

# define EAX "%%rax"
# define EBX "%%rbx"
# define ECX "%%rcx"
# define EDX "%%rdx"
# define ESP "%%rsp"
# define EBP "%%rbp"
# define ESI "%%rsi"
# define EDI "%%rdi"
# define LABEL(l) #l"(%%rip)"
# define PTRSIZE 8
# define PTRSIZE_STR "8"

#else

# define EAX "%%eax"
# define EBX "%%ebx"
# define ECX "%%ecx"
# define EDX "%%edx"
# define ESP "%%esp"
# define EBP "%%ebp"
# define ESI "%%esi"
# define EDI "%%edi"
# define LABEL(l) #l
# define PTRSIZE 4
# define PTRSIZE_STR "4"

#endif

/*************************************************************************/

#endif  // USE_SSE2
#endif  // SSE2_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
