.globl memzero
memzero:
	str xzr, [x10], #8
	subs x11, x11, #8
	b.gt memzero
	ret
