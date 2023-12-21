.globl memzero
memzero:
	str xzr, [x0], #8	// Post-index: store 0 to the unmodified address in x0 first, then update x0 (to x0 + #8)
	subs x1, x1, #8
	b.gt memzero
	ret