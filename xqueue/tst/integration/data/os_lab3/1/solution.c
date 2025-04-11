diff --git a/kern/env.c b/kern/env.c
index fbb6bed..cf72984 100644
--- a/kern/env.c
+++ b/kern/env.c
@@ -114,8 +114,13 @@ envid2env(envid_t envid, struct Env **env_store, bool checkperm)
 void
 env_init(void)
 {
-	// Set up envs array
-	// LAB 3: Your code here.
+	int i;
+	for (i = NENV - 1; i >= 0; i--) {
+		envs[i].env_status = ENV_FREE;
+		envs[i].env_id = 0;
+		envs[i].env_link = env_free_list;
+		env_free_list = envs + i;
+	}
 
 	// Per-CPU part of the initialization
 	env_init_percpu();
@@ -177,8 +182,13 @@ env_setup_vm(struct Env *e)
 	//	is an exception -- you need to increment env_pgdir's
 	//	pp_ref for env_free to work correctly.
 	//    - The functions in kern/pmap.h are handy.
+	e->env_pgdir = (pde_t *) page2kva(p);
+
+	for (i = PDX(UTOP); i < NPDENTRIES; ++i) {
+		e->env_pgdir[i] = kern_pgdir[i];
+	}
 
-	// LAB 3: Your code here.
+	p->pp_ref++;
 
 	// UVPT maps the env's own page table read-only.
 	// Permissions: kernel R, user R
@@ -260,13 +270,21 @@ env_alloc(struct Env **newenv_store, envid_t parent_id)
 static void
 region_alloc(struct Env *e, void *va, size_t len)
 {
-	// LAB 3: Your code here.
-	// (But only if you need it for load_icode.)
-	//
-	// Hint: It is easier to use region_alloc if the caller can pass
-	//   'va' and 'len' values that are not page-aligned.
-	//   You should round va down, and round (va + len) up.
-	//   (Watch out for corner-cases!)
+	void *nva = ROUNDDOWN(va, PGSIZE);
+	size_t nlen = ROUNDUP(PGOFF(va) + len, PGSIZE);
+
+	void *i;
+	for (i = nva; i < nva + len; i += PGSIZE) {
+		struct PageInfo *p = page_alloc(0);
+		if (p == NULL) {
+			panic("Couldn't allocate a page for a region");
+		}
+
+		int ins = page_insert(e->env_pgdir, p, i, PTE_W | PTE_U);
+		if (ins) {
+			panic("Couldn't insert the page for a region");
+		}
+	}
 }
 
 //
@@ -322,12 +340,46 @@ load_icode(struct Env *e, uint8_t *binary, size_t size)
 	//  to make sure that the environment starts executing there.
 	//  What?  (See env_run() and env_pop_tf() below.)
 
-	// LAB 3: Your code here.
+	struct Elf *hdr = (struct Elf *)binary;
+	if (hdr->e_magic != ELF_MAGIC) {
+		panic("Not an ELF binary");
+	}
+
+	if (hdr->e_phoff > size) {
+		panic("Insufficient bytes in ELF binary");
+	}
+
+	struct Proghdr *ph = (struct Proghdr *) (binary + hdr->e_phoff);
+	struct Proghdr *eph = ph + hdr->e_phnum;
+
+	for (; ph < eph; ph++) {
+
+		if (ph->p_type != ELF_PROG_LOAD) {
+			continue;
+		}
+
+		if (ph->p_filesz > ph->p_memsz) {
+			panic("Malformed ELF");
+		}
+
+		if (ph->p_offset + ph->p_filesz > size) {
+			panic("Part of a segment is missing from ELF");
+		}
+
+		void *addr = (void *)ph->p_pa;
+		region_alloc(e, addr, ph->p_memsz);
+		lcr3(PADDR(e->env_pgdir));
+		memcpy(addr, binary + ph->p_offset, ph->p_filesz);
+		memset(addr + ph->p_filesz, 0x0, ph->p_memsz - ph->p_filesz);
+		lcr3(PADDR(kern_pgdir));
+	}
+
+	e->env_tf.tf_eip = hdr->e_entry;
 
 	// Now map one page for the program's initial stack
 	// at virtual address USTACKTOP - PGSIZE.
 
-	// LAB 3: Your code here.
+	region_alloc(e, (void *)USTACKTOP - PGSIZE, PGSIZE);
 }
 
 //
@@ -340,7 +392,13 @@ load_icode(struct Env *e, uint8_t *binary, size_t size)
 void
 env_create(uint8_t *binary, size_t size, enum EnvType type)
 {
-	// LAB 3: Your code here.
+	struct Env *e;
+	int r = env_alloc(&e, 0);
+	if (r) {
+		panic("Couldn't allocate the environment");
+	}
+	e->env_type = type;
+	load_icode(e, binary, size);
 }
 
 //
@@ -438,6 +496,17 @@ env_pop_tf(struct Trapframe *tf)
 void
 env_run(struct Env *e)
 {
+	if (e != curenv) {
+		if (curenv && curenv->env_status == ENV_RUNNING) {
+			curenv->env_status = ENV_RUNNABLE;
+		}
+		curenv = e;
+		e->env_status = ENV_RUNNING;
+		e->env_runs++;
+		lcr3(PADDR(e->env_pgdir));
+	}
+
+	env_pop_tf(&e->env_tf);
 	// Step 1: If this is a context switch (a new environment is running):
 	//	   1. Set the current environment (if any) back to
 	//	      ENV_RUNNABLE if it is ENV_RUNNING (think about
@@ -455,8 +524,6 @@ env_run(struct Env *e)
 	//	and make sure you have set the relevant parts of
 	//	e->env_tf to sensible values.
 
-	// LAB 3: Your code here.
-
 	panic("env_run not yet implemented");
 }
 
diff --git a/kern/kdebug.c b/kern/kdebug.c
index f4ee8ee..1b0088a 100644
--- a/kern/kdebug.c
+++ b/kern/kdebug.c
@@ -141,7 +141,10 @@ debuginfo_eip(uintptr_t addr, struct Eipdebuginfo *info)
 
 		// Make sure this memory is valid.
 		// Return -1 if it is not.  Hint: Call user_mem_check.
-		// LAB 3: Your code here.
+		if (user_mem_check(curenv, usd, sizeof(struct UserStabData),
+				PTE_U)) {
+			return -1;
+		}
 
 		stabs = usd->stabs;
 		stab_end = usd->stab_end;
@@ -149,7 +152,14 @@ debuginfo_eip(uintptr_t addr, struct Eipdebuginfo *info)
 		stabstr_end = usd->stabstr_end;
 
 		// Make sure the STABS and string table memory is valid.
-		// LAB 3: Your code here.
+		if (user_mem_check(curenv, stabs, stab_end - stabs, PTE_U)) {
+			return -1;
+		}
+
+		if (user_mem_check(curenv, stabstr, stabstr_end - stabstr,
+				PTE_U)) {
+			return -1;
+		}
 	}
 
 	// String table validity checks
diff --git a/kern/pmap.c b/kern/pmap.c
index 1277e70..75d6680 100644
--- a/kern/pmap.c
+++ b/kern/pmap.c
@@ -97,10 +97,25 @@ boot_alloc(uint32_t n)
 	// Allocate a chunk large enough to hold 'n' bytes, then update
 	// nextfree.  Make sure nextfree is kept aligned
 	// to a multiple of PGSIZE.
-	//
-	// LAB 2: Your code here.
 
-	return NULL;
+	result = nextfree;
+
+	if (n) {
+		extern char end[];
+
+		uint32_t last_page = PGNUM(PADDR(nextfree));
+		uint32_t pages_needed = (n / PGSIZE) + !!(n % PGSIZE);
+
+		size_t max_pages = PGNUM(PADDR((void*)0xf0400000));
+
+		if (pages_needed > max_pages - last_page) {
+			panic("Out of memory");
+		}
+
+		nextfree += pages_needed * PGSIZE;
+	}
+
+	return result;
 }
 
 // Set up a two-level page table:
@@ -121,9 +136,6 @@ mem_init(void)
 	// Find out how much memory the machine has (npages & npages_basemem).
 	i386_detect_memory();
 
-	// Remove this line when you're ready to test this function.
-	panic("mem_init: This function is not finished\n");
-
 	//////////////////////////////////////////////////////////////////////
 	// create initial page directory.
 	kern_pgdir = (pde_t *) boot_alloc(PGSIZE);
@@ -144,11 +156,12 @@ mem_init(void)
 	// each physical page, there is a corresponding struct PageInfo in this
 	// array.  'npages' is the number of physical pages in memory.
 	// Your code goes here:
-
+	pages = boot_alloc(sizeof(struct PageInfo) * npages);
 
 	//////////////////////////////////////////////////////////////////////
 	// Make 'envs' point to an array of size 'NENV' of 'struct Env'.
 	// LAB 3: Your code here.
+	envs = boot_alloc(sizeof(struct Env) * NENV);
 
 	//////////////////////////////////////////////////////////////////////
 	// Now that we've allocated the initial kernel data structures, we set
@@ -172,6 +185,8 @@ mem_init(void)
 	//      (ie. perm = PTE_U | PTE_P)
 	//    - pages itself -- kernel RW, user NONE
 	// Your code goes here:
+	size_t page_blocks = ROUNDUP(npages * sizeof(struct PageInfo), PGSIZE);
+	boot_map_region(kern_pgdir, UPAGES, page_blocks, PADDR(pages), PTE_U);
 
 	//////////////////////////////////////////////////////////////////////
 	// Map the 'envs' array read-only by the user at linear address UENVS
@@ -179,7 +194,8 @@ mem_init(void)
 	// Permissions:
 	//    - the new image at UENVS  -- kernel R, user R
 	//    - envs itself -- kernel RW, user NONE
-	// LAB 3: Your code here.
+	size_t env_blocks = ROUNDUP(NENV * sizeof(struct Env), PGSIZE);
+	boot_map_region(kern_pgdir, UENVS, env_blocks, PADDR(envs), PTE_U);
 
 	//////////////////////////////////////////////////////////////////////
 	// Use the physical memory that 'bootstack' refers to as the kernel
@@ -191,7 +207,8 @@ mem_init(void)
 	//       the kernel overflows its stack, it will fault rather than
 	//       overwrite memory.  Known as a "guard page".
 	//     Permissions: kernel RW, user NONE
-	// Your code goes here:
+	boot_map_region(kern_pgdir, KSTACKTOP - KSTKSIZE, KSTKSIZE, PADDR(bootstack),
+		PTE_W);
 
 	//////////////////////////////////////////////////////////////////////
 	// Map all of physical memory at KERNBASE.
@@ -200,7 +217,8 @@ mem_init(void)
 	// We might not have 2^32 - KERNBASE bytes of physical memory, but
 	// we just set up the mapping anyway.
 	// Permissions: kernel RW, user NONE
-	// Your code goes here:
+	size_t kernbase_blocks = 1+~KERNBASE;
+	boot_map_region(kern_pgdir, KERNBASE, kernbase_blocks, 0, PTE_W);
 
 	// Check that the initial page directory has been set up correctly.
 	check_kern_pgdir();
@@ -260,7 +278,14 @@ page_init(void)
 	// NB: DO NOT actually touch the physical memory corresponding to
 	// free pages!
 	size_t i;
-	for (i = 0; i < npages; i++) {
+	for (i = PGNUM(PGSIZE); i < PGNUM(npages_basemem * PGSIZE); i++) {
+		pages[i].pp_ref = 0;
+		pages[i].pp_link = page_free_list;
+		page_free_list = &pages[i];
+	}
+
+	char *nextfree = boot_alloc(0);
+	for (i = PGNUM(PADDR(nextfree)); i < npages; i++) {
 		pages[i].pp_ref = 0;
 		pages[i].pp_link = page_free_list;
 		page_free_list = &pages[i];
@@ -279,8 +304,20 @@ page_init(void)
 struct PageInfo *
 page_alloc(int alloc_flags)
 {
-	// Fill this function in
-	return 0;
+	struct PageInfo *pp = page_free_list;
+
+	if (!pp) {
+		return NULL;
+	}
+
+	page_free_list = pp->pp_link;
+
+	if (alloc_flags & ALLOC_ZERO) {
+		char *mem = page2kva(pp);
+		memset(mem, 0x0, PGSIZE);
+	}
+
+	return pp;
 }
 
 //
@@ -290,7 +327,8 @@ page_alloc(int alloc_flags)
 void
 page_free(struct PageInfo *pp)
 {
-	// Fill this function in
+	pp->pp_link = page_free_list;
+	page_free_list = pp;
 }
 
 //
@@ -329,8 +367,21 @@ page_decref(struct PageInfo* pp)
 pte_t *
 pgdir_walk(pde_t *pgdir, const void *va, int create)
 {
-	// Fill this function in
-	return NULL;
+	if (!(pgdir[PDX(va)] & PTE_P) && create) {
+		struct PageInfo *pp = page_alloc(ALLOC_ZERO);
+		if (pp) {
+			pp->pp_ref++;
+			pgdir[PDX(va)] = page2pa(pp) | PTE_P | PTE_U | PTE_W;
+		}
+	}
+
+	if (!pgdir[PDX(va)] || !(pgdir[PDX(va)] & PTE_P)) {
+		return NULL;
+	}
+
+	pte_t *pgtbl = (pte_t *)(KADDR(PTE_ADDR(pgdir[PDX(va)])));
+
+	return pgtbl + PTX(va);
 }
 
 //
@@ -346,7 +397,19 @@ pgdir_walk(pde_t *pgdir, const void *va, int create)
 static void
 boot_map_region(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm)
 {
-	// Fill this function in
+	if (~(physaddr_t)0 - size + 1 < pa) {
+		panic("Physical	address	overflow when mapping a	region");
+	}
+
+	if (~(uintptr_t)0 - size + 1 < va) {
+		panic("Virtual address overflow	when mapping a region");
+	}
+
+	uintptr_t i;
+	for (i = 0; i < size; i += PGSIZE) {
+		*pgdir_walk(pgdir, (void*)(va + i), 1) =
+			(pa + i) | perm | PTE_P;
+	}
 }
 
 //
@@ -377,7 +440,15 @@ boot_map_region(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm
 int
 page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm)
 {
-	// Fill this function in
+	pte_t *t = pgdir_walk(pgdir, va, 1);
+	if (!t) {
+		return -E_NO_MEM;
+	}
+	pp->pp_ref++;
+	page_remove(pgdir, va);
+	*t = page2pa(pp) | PTE_P | perm;
+	tlb_invalidate(pgdir, va);
+
 	return 0;
 }
 
@@ -395,8 +466,16 @@ page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm)
 struct PageInfo *
 page_lookup(pde_t *pgdir, void *va, pte_t **pte_store)
 {
-	// Fill this function in
-	return NULL;
+	pte_t *t = pgdir_walk(pgdir, va, 0);
+	if (!t || !(*t & PTE_P)) {
+		return NULL;
+	}
+
+	if (pte_store) {
+		*pte_store = t;
+	}
+
+	return pa2page(PTE_ADDR(*t));
 }
 
 //
@@ -417,7 +496,14 @@ page_lookup(pde_t *pgdir, void *va, pte_t **pte_store)
 void
 page_remove(pde_t *pgdir, void *va)
 {
-	// Fill this function in
+	pte_t *t;
+	struct PageInfo *pp = page_lookup(pgdir, va, &t);
+
+	if (pp && (*t & PTE_P)) {
+		*t = 0;
+		page_decref(pp);
+		tlb_invalidate(pgdir, va);
+	}
 }
 
 //
@@ -455,8 +541,16 @@ static uintptr_t user_mem_check_addr;
 int
 user_mem_check(struct Env *env, const void *va, size_t len, int perm)
 {
-	// LAB 3: Your code here.
-
+	const void *nva;
+	for (nva = va; nva < va + len; nva += PGSIZE) {
+		const pte_t *t = pgdir_walk(env->env_pgdir, nva, 0);
+		if (!t || !(*t & PTE_P) || ((*t & perm) != perm) ||
+				(uint32_t)nva >= ULIM) {
+			user_mem_check_addr = (uint32_t)(nva == va ?
+				nva : ROUNDDOWN(nva, PGSIZE));
+			return -E_FAULT;
+		}
+	}
 	return 0;
 }
 
diff --git a/kern/syscall.c b/kern/syscall.c
index 7bd7d38..4c558e8 100644
--- a/kern/syscall.c
+++ b/kern/syscall.c
@@ -19,8 +19,7 @@ sys_cputs(const char *s, size_t len)
 {
 	// Check that the user has permission to read memory [s, s+len).
 	// Destroy the environment if not.
-
-	// LAB 3: Your code here.
+	user_mem_assert(curenv, s, len, PTE_U);
 
 	// Print the string supplied by the user.
 	cprintf("%.*s", len, s);
@@ -68,8 +67,18 @@ syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4,
 {
 	// Call the function corresponding to the 'syscallno' parameter.
 	// Return any appropriate return value.
-	// LAB 3: Your code here.
-
-	panic("syscall not implemented");
+	switch (syscallno) {
+		case SYS_cputs:
+			sys_cputs((char *)a1, a2);
+			return 0;
+		case SYS_cgetc:
+			return sys_cgetc();
+		case SYS_getenvid:
+			return sys_getenvid();
+		case SYS_env_destroy:
+			return sys_env_destroy(a1);
+		default:
+			return -E_INVAL;
+	}
 }
 
diff --git a/kern/trap.c b/kern/trap.c
index 0068d15..be7c186 100644
--- a/kern/trap.c
+++ b/kern/trap.c
@@ -64,7 +64,46 @@ trap_init(void)
 {
 	extern struct Segdesc gdt[];
 
-	// LAB 3: Your code here.
+	extern unsigned thdl_divide;
+	extern unsigned thdl_debug;
+	extern unsigned thdl_nmi;
+	extern unsigned thdl_brkpt;
+	extern unsigned thdl_oflow;
+	extern unsigned thdl_bound;
+	extern unsigned thdl_illop;
+	extern unsigned thdl_device;
+	extern unsigned thdl_dblflt;
+	extern unsigned thdl_tss;
+	extern unsigned thdl_segnp;
+	extern unsigned thdl_stack;
+	extern unsigned thdl_gpflt;
+	extern unsigned thdl_pgflt;
+	extern unsigned thdl_fperr;
+	extern unsigned thdl_align;
+	extern unsigned thdl_mchk;
+	extern unsigned thdl_simderr;
+	extern unsigned thdl_syscall;
+
+	SETGATE(idt[ 0], 1, GD_KT, &thdl_divide,  0);
+	SETGATE(idt[ 1], 1, GD_KT, &thdl_debug,   0);
+	SETGATE(idt[ 2], 1, GD_KT, &thdl_nmi,     0);
+	SETGATE(idt[ 3], 1, GD_KT, &thdl_brkpt,   3);
+	SETGATE(idt[ 4], 1, GD_KT, &thdl_oflow,   0);
+	SETGATE(idt[ 5], 1, GD_KT, &thdl_bound,   0);
+	SETGATE(idt[ 6], 1, GD_KT, &thdl_illop,   0);
+	SETGATE(idt[ 7], 1, GD_KT, &thdl_device,  0);
+	SETGATE(idt[ 8], 1, GD_KT, &thdl_dblflt,  0);
+	SETGATE(idt[10], 1, GD_KT, &thdl_tss  ,   0);
+	SETGATE(idt[11], 1, GD_KT, &thdl_segnp,   0);
+	SETGATE(idt[12], 1, GD_KT, &thdl_stack,   0);
+	SETGATE(idt[13], 1, GD_KT, &thdl_gpflt,   0);
+	SETGATE(idt[14], 1, GD_KT, &thdl_pgflt,   0);
+	SETGATE(idt[16], 1, GD_KT, &thdl_fperr,   0);
+	SETGATE(idt[17], 1, GD_KT, &thdl_align,   0);
+	SETGATE(idt[18], 1, GD_KT, &thdl_mchk,    0);
+	SETGATE(idt[19], 1, GD_KT, &thdl_simderr, 0);
+
+	SETGATE(idt[48], 1, GD_KT, &thdl_syscall, 3);
 
 	// Per-CPU setup 
 	trap_init_percpu();
@@ -142,7 +181,24 @@ static void
 trap_dispatch(struct Trapframe *tf)
 {
 	// Handle processor exceptions.
-	// LAB 3: Your code here.
+	switch (tf->tf_trapno) {
+		case T_PGFLT:
+			page_fault_handler(tf);
+			return;
+		case T_BRKPT:
+			monitor(tf);
+			return;
+		case T_SYSCALL:;
+			int res = syscall(
+				tf->tf_regs.reg_eax,
+				tf->tf_regs.reg_edx,
+				tf->tf_regs.reg_ecx,
+				tf->tf_regs.reg_ebx,
+				tf->tf_regs.reg_edi,
+				tf->tf_regs.reg_esi);
+			curenv->env_tf.tf_regs.reg_eax = res;
+			return;
+	}
 
 	// Unexpected trap: The user process or the kernel has a bug.
 	print_trapframe(tf);
@@ -203,7 +259,9 @@ page_fault_handler(struct Trapframe *tf)
 
 	// Handle kernel-mode page faults.
 
-	// LAB 3: Your code here.
+	if ((tf->tf_cs & 3) != 3) {
+		panic("Page fault in kernel!");
+	}
 
 	// We've already handled kernel-mode exceptions, so if we get here,
 	// the page fault happened in user mode.
diff --git a/kern/trapentry.S b/kern/trapentry.S
index 22fc640..571005c 100644
--- a/kern/trapentry.S
+++ b/kern/trapentry.S
@@ -43,13 +43,33 @@
 
 .text
 
-/*
- * Lab 3: Your code here for generating entry points for the different traps.
- */
-
+TRAPHANDLER_NOEC(thdl_divide,   0)
+TRAPHANDLER_NOEC(thdl_debug,    1)
+TRAPHANDLER_NOEC(thdl_nmi,      2)
+TRAPHANDLER_NOEC(thdl_brkpt,    3)
+TRAPHANDLER_NOEC(thdl_oflow,    4)
+TRAPHANDLER_NOEC(thdl_bound,    5)
+TRAPHANDLER_NOEC(thdl_illop,    6)
+TRAPHANDLER_NOEC(thdl_device,   7)
+TRAPHANDLER     (thdl_dblflt,   8)
+TRAPHANDLER     (thdl_tss  ,   10)
+TRAPHANDLER     (thdl_segnp,   11)
+TRAPHANDLER     (thdl_stack,   12)
+TRAPHANDLER     (thdl_gpflt,   13)
+TRAPHANDLER     (thdl_pgflt,   14)
+TRAPHANDLER_NOEC(thdl_fperr,   16)
+TRAPHANDLER_NOEC(thdl_align,   17)
+TRAPHANDLER_NOEC(thdl_mchk,    18)
+TRAPHANDLER_NOEC(thdl_simderr, 19)
 
+TRAPHANDLER_NOEC(thdl_syscall, 48)
 
-/*
- * Lab 3: Your code here for _alltraps
- */
-
+_alltraps:
+	pushl %ds
+	pushl %es
+	pushal
+	mov $GD_KD, %eax
+	mov %ax, %ds
+	mov %ax, %es
+	pushl %esp
+	call trap
diff --git a/lib/libmain.c b/lib/libmain.c
index 8a14b29..354fa84 100644
--- a/lib/libmain.c
+++ b/lib/libmain.c
@@ -8,12 +8,14 @@ extern void umain(int argc, char **argv);
 const volatile struct Env *thisenv;
 const char *binaryname = "<unknown>";
 
+extern struct Env *curenv;
+
 void
 libmain(int argc, char **argv)
 {
 	// set thisenv to point at our Env structure in envs[].
 	// LAB 3: Your code here.
-	thisenv = 0;
+	thisenv = envs + ENVX(sys_getenvid());
 
 	// save the name of the program so that panic() can use it
 	if (argc > 0)
