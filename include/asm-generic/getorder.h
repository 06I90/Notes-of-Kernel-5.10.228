/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_GETORDER_H
#define __ASM_GENERIC_GETORDER_H

#ifndef __ASSEMBLY__

#include <linux/compiler.h>
#include <linux/log2.h>

/**
 * get_order - Determine the allocation order of a memory size
 * @size: The size for which to get the order
 *
 * Determine the allocation order of a particular sized block of memory.  This
 * is on a logarithmic scale对数的刻度, where:
 *
 *	0 -> 2^0 * PAGE_SIZE and below
 *	1 -> 2^1 * PAGE_SIZE to 2^0 * PAGE_SIZE + 1
 *	2 -> 2^2 * PAGE_SIZE to 2^1 * PAGE_SIZE + 1
 *	3 -> 2^3 * PAGE_SIZE to 2^2 * PAGE_SIZE + 1
 *	4 -> 2^4 * PAGE_SIZE to 2^3 * PAGE_SIZE + 1
 *	...
 *
 * The order returned is used to find the smallest allocation granule颗粒 required
 * to hold an object of the specified size.
 *
 * The result is undefined if the size is 0.
 * 根据size返回阶数order
 * __always_inline强制内联可以提高函数调用的效率，减少函数调用的开销
 * __attribute_const__告诉编译器该函数是纯函数，即在相同的输入下总是返回相同的输出，并且没有副作用
 * PAGE_SHIFT是系统页面大小的对数值，通常为12，表示页面大小为2的12次方字节，即4096字节
 */
static inline __attribute_const__ int get_order(unsigned long size)
{
	if (__builtin_constant_p(size)) {/* 如果 size 是编译时常量 */
		if (!size)
			return BITS_PER_LONG - PAGE_SHIFT;/* 如果 size 为 0，返回最大可能的 order 值 */

		if (size < (1UL << PAGE_SHIFT))/* 如果 size 小于页面大小，说明是0阶 */
			return 0;

		return ilog2((size) - 1) - PAGE_SHIFT + 1;/* ilog2(8)返回 3，因为二进制表示为 1000，返回最高位的 1 的位置（从 0 开始计数） */
	}/* size是4097时，返回1 */

	size--;
	size >>= PAGE_SHIFT;/* 右移 PAGE_SHIFT，相当于 size / PAGE_SIZE */
#if BITS_PER_LONG == 32
	return fls(size);/* find last set bit，返回最高位的 1 的位置（从 1 开始计数），fls(4)返回 3，因为二进制表示为 100 */
#else
	return fls64(size);
#endif
}

#endif	/* __ASSEMBLY__ */

#endif	/* __ASM_GENERIC_GETORDER_H */
