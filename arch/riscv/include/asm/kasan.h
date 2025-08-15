/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2019 Andes Technology Corporation */

#ifndef __ASM_KASAN_H
#define __ASM_KASAN_H

#ifndef __ASSEMBLY__

#ifdef CONFIG_KASAN

#define KASAN_SHADOW_SCALE_SHIFT	3 /* 通常这个值是 3，表示每 8B 的内核内存会映射到 1B 的影子内存 shadow memory */

#define KASAN_SHADOW_SIZE	(UL(1) << (38 - KASAN_SHADOW_SCALE_SHIFT))
#define KASAN_SHADOW_START	KERN_VIRT_START /* 2^64 - 2^38 */
#define KASAN_SHADOW_END	(KASAN_SHADOW_START + KASAN_SHADOW_SIZE)

#define KASAN_SHADOW_OFFSET	_AC(CONFIG_KASAN_SHADOW_OFFSET, UL)

void kasan_init(void);
asmlinkage void kasan_early_init(void);

#endif
#endif
#endif /* __ASM_KASAN_H */
