diff --git a/kern/env.c b/kern/env.c
index 07d6a58..630328f 100644
--- a/kern/env.c
+++ b/kern/env.c
@@ -117,8 +117,13 @@ envid2env(envid_t envid, struct Env **env_store, bool checkperm)
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
@@ -180,8 +185,13 @@ env_setup_vm(struct Env *e)
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
@@ -246,7 +256,7 @@ env_alloc(struct Env **newenv_store, envid_t parent_id)
 	// You will set e->env_tf.tf_eip later.
 
 	// Enable interrupts while in user mode.
-	// LAB 4: Your code here.
+	e->env_tf.tf_eflags |= FL_IF;
 
 	// Clear the page fault handler until user installs one.
 	e->env_pgfault_upcall = 0;
@@ -272,13 +282,21 @@ env_alloc(struct Env **newenv_store, envid_t parent_id)
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
@@ -334,12 +352,46 @@ load_icode(struct Env *e, uint8_t *binary, size_t size)
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
@@ -352,7 +404,13 @@ load_icode(struct Env *e, uint8_t *binary, size_t size)
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
@@ -464,6 +522,18 @@ env_pop_tf(struct Trapframe *tf)
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
+	unlock_kernel();
+	env_pop_tf(&e->env_tf);
 	// Step 1: If this is a context switch (a new environment is running):
 	//	   1. Set the current environment (if any) back to
 	//	      ENV_RUNNABLE if it is ENV_RUNNING (think about
@@ -481,8 +551,6 @@ env_run(struct Env *e)
 	//	and make sure you have set the relevant parts of
 	//	e->env_tf to sensible values.
 
-	// LAB 3: Your code here.
-
 	panic("env_run not yet implemented");
 }
 
diff --git a/kern/init.c b/kern/init.c
index 607e5ca..c045c4f 100644
--- a/kern/init.c
+++ b/kern/init.c
@@ -49,7 +49,7 @@ i386_init(void)
 	pic_init();
 
 	// Acquire the big kernel lock before waking up APs
-	// Your code here:
+	lock_kernel();
 
 	// Starting non-boot CPUs
 	boot_aps();
@@ -59,7 +59,9 @@ i386_init(void)
 	ENV_CREATE(TEST, ENV_TYPE_USER);
 #else
 	// Touch all you want.
-	ENV_CREATE(user_primes, ENV_TYPE_USER);
+	ENV_CREATE(user_yield, ENV_TYPE_USER);
+	ENV_CREATE(user_yield, ENV_TYPE_USER);
+	ENV_CREATE(user_yield, ENV_TYPE_USER);
 #endif // TEST*
 
 	// Schedule and run the first user environment!
@@ -114,11 +116,8 @@ mp_main(void)
 	// Now that we have finished some basic setup, call sched_yield()
 	// to start running processes on this CPU.  But make sure that
 	// only one CPU can enter the scheduler at a time!
-	//
-	// Your code here:
-
-	// Remove this after you finish Exercise 4
-	for (;;);
+	lock_kernel();
+	sched_yield();
 }
 
 /*
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
index 76ac628..b49e057 100644
--- a/kern/pmap.c
+++ b/kern/pmap.c
@@ -99,10 +99,25 @@ boot_alloc(uint32_t n)
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
@@ -123,9 +138,6 @@ mem_init(void)
 	// Find out how much memory the machine has (npages & npages_basemem).
 	i386_detect_memory();
 
-	// Remove this line when you're ready to test this function.
-	panic("mem_init: This function is not finished\n");
-
 	//////////////////////////////////////////////////////////////////////
 	// create initial page directory.
 	kern_pgdir = (pde_t *) boot_alloc(PGSIZE);
@@ -146,11 +158,12 @@ mem_init(void)
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
@@ -174,6 +187,8 @@ mem_init(void)
 	//      (ie. perm = PTE_U | PTE_P)
 	//    - pages itself -- kernel RW, user NONE
 	// Your code goes here:
+	size_t page_blocks = ROUNDUP(npages * sizeof(struct PageInfo), PGSIZE);
+	boot_map_region(kern_pgdir, UPAGES, page_blocks, PADDR(pages), PTE_U);
 
 	//////////////////////////////////////////////////////////////////////
 	// Map the 'envs' array read-only by the user at linear address UENVS
@@ -181,7 +196,8 @@ mem_init(void)
 	// Permissions:
 	//    - the new image at UENVS  -- kernel R, user R
 	//    - envs itself -- kernel RW, user NONE
-	// LAB 3: Your code here.
+	size_t env_blocks = ROUNDUP(NENV * sizeof(struct Env), PGSIZE);
+	boot_map_region(kern_pgdir, UENVS, env_blocks, PADDR(envs), PTE_U);
 
 	//////////////////////////////////////////////////////////////////////
 	// Use the physical memory that 'bootstack' refers to as the kernel
@@ -193,7 +209,8 @@ mem_init(void)
 	//       the kernel overflows its stack, it will fault rather than
 	//       overwrite memory.  Known as a "guard page".
 	//     Permissions: kernel RW, user NONE
-	// Your code goes here:
+	// boot_map_region(kern_pgdir, KSTACKTOP - KSTKSIZE, KSTKSIZE,
+	// PADDR(bootstack), PTE_W);
 
 	//////////////////////////////////////////////////////////////////////
 	// Map all of physical memory at KERNBASE.
@@ -202,7 +219,8 @@ mem_init(void)
 	// We might not have 2^32 - KERNBASE bytes of physical memory, but
 	// we just set up the mapping anyway.
 	// Permissions: kernel RW, user NONE
-	// Your code goes here:
+	size_t kernbase_blocks = 1+~KERNBASE;
+	boot_map_region(kern_pgdir, KERNBASE, kernbase_blocks, 0, PTE_W);
 
 	// Initialize the SMP-related parts of the memory map
 	mem_init_mp();
@@ -254,6 +272,12 @@ mem_init_mp(void)
 	//     Permissions: kernel RW, user NONE
 	//
 	// LAB 4: Your code here:
+	size_t i;
+	for (i = 0; i < NCPU; ++i) {
+		uintptr_t kstacktop_i = KSTACKTOP - i * (KSTKSIZE + KSTKGAP);
+		boot_map_region(kern_pgdir, kstacktop_i - KSTKSIZE, KSTKSIZE,
+				PADDR(percpu_kstacks[i]), PTE_W);
+	}
 
 }
 
@@ -275,6 +299,9 @@ page_init(void)
 	// LAB 4:
 	// Change your code to mark the physical page at MPENTRY_PADDR
 	// as in use
+	extern unsigned char mpentry_start[], mpentry_end[];
+	size_t mpentry_pages = (mpentry_end - mpentry_start + PGSIZE - 1) /
+		PGSIZE;
 
 	// The example code here marks all physical pages as free.
 	// However this is not truly the case.  What memory is free?
@@ -294,7 +321,24 @@ page_init(void)
 	// NB: DO NOT actually touch the physical memory corresponding to
 	// free pages!
 	size_t i;
-	for (i = 0; i < npages; i++) {
+	for (i = PGNUM(PGSIZE); i < PGNUM(MPENTRY_PADDR); i++) {
+		pages[i].pp_ref = 0;
+		pages[i].pp_link = page_free_list;
+		page_free_list = &pages[i];
+	}
+
+	for (; i < PGNUM(MPENTRY_PADDR) + mpentry_pages; ++i) {
+		pages[i].pp_ref = 1;
+	}
+
+	for (; i < PGNUM(npages_basemem * PGSIZE); i++) {
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
@@ -313,8 +357,20 @@ page_init(void)
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
@@ -324,7 +380,8 @@ page_alloc(int alloc_flags)
 void
 page_free(struct PageInfo *pp)
 {
-	// Fill this function in
+	pp->pp_link = page_free_list;
+	page_free_list = pp;
 }
 
 //
@@ -363,8 +420,21 @@ page_decref(struct PageInfo* pp)
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
@@ -380,7 +450,19 @@ pgdir_walk(pde_t *pgdir, const void *va, int create)
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
@@ -411,7 +493,15 @@ boot_map_region(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm
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
 
@@ -429,8 +519,16 @@ page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm)
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
@@ -451,7 +549,14 @@ page_lookup(pde_t *pgdir, void *va, pte_t **pte_store)
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
@@ -494,11 +599,16 @@ mmio_map_region(physaddr_t pa, size_t size)
 	// Be sure to round size up to a multiple of PGSIZE and to
 	// handle if this reservation would overflow MMIOLIM (it's
 	// okay to simply panic if this happens).
-	//
-	// Hint: The staff solution uses boot_map_region.
-	//
-	// Your code here:
-	panic("mmio_map_region not implemented");
+	size = ROUNDUP(size, PGSIZE);
+	if (size >= MMIOLIM - base) {
+		panic("Too big a reservation");
+	}
+	boot_map_region(kern_pgdir, base, size, pa, PTE_W | PTE_PCD | PTE_PWT);
+
+	uintptr_t res = base;
+	base += size;
+
+	return (void *)res;
 }
 
 static uintptr_t user_mem_check_addr;
@@ -524,8 +634,16 @@ static uintptr_t user_mem_check_addr;
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
 
diff --git a/kern/sched.c b/kern/sched.c
index 93f2d34..6ba6c3d 100644
--- a/kern/sched.c
+++ b/kern/sched.c
@@ -28,7 +28,18 @@ sched_yield(void)
 	// no runnable environments, simply drop through to the code
 	// below to halt the cpu.
 
-	// LAB 4: Your code here.
+	struct Env *e = (curenv ? curenv : envs - 1);
+	size_t i;
+	for (i = 0; i < NENV; ++i) {
+		e = envs + ((e - envs + 1) % NENV);
+		if (e->env_status == ENV_RUNNABLE) {
+			env_run(e);
+		} else if (e == curenv) {
+			if (e->env_status == ENV_RUNNING) {
+				env_run(e);
+			}
+		}
+	}
 
 	// sched_halt never returns
 	sched_halt();
diff --git a/kern/syscall.c b/kern/syscall.c
index f784fdd..e3770dd 100644
--- a/kern/syscall.c
+++ b/kern/syscall.c
@@ -20,8 +20,7 @@ sys_cputs(const char *s, size_t len)
 {
 	// Check that the user has permission to read memory [s, s+len).
 	// Destroy the environment if not.
-
-	// LAB 3: Your code here.
+	user_mem_assert(curenv, s, len, PTE_U);
 
 	// Print the string supplied by the user.
 	cprintf("%.*s", len, s);
@@ -82,11 +81,24 @@ sys_exofork(void)
 	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
 	// from the current environment -- but tweaked so sys_exofork
 	// will appear to return 0.
+	struct Env *e;
+	int ret = 0;
+	if ((ret = env_alloc(&e, curenv->env_id))) {
+		return ret;
+	}
 
-	// LAB 4: Your code here.
-	panic("sys_exofork not implemented");
+	e->env_status = ENV_NOT_RUNNABLE;
+	memcpy(&e->env_tf, &curenv->env_tf, sizeof (struct Trapframe));
+	e->env_tf.tf_regs.reg_eax = 0;
+
+	return e->env_id;
 }
 
+#define ENVID2ENV(name, id) \
+	struct Env *name; \
+	{ int ret = envid2env(id, &name, true); \
+	if (ret) { return ret; } }
+
 // Set envid's env_status to status, which must be ENV_RUNNABLE
 // or ENV_NOT_RUNNABLE.
 //
@@ -102,9 +114,9 @@ sys_env_set_status(envid_t envid, int status)
 	// You should set envid2env's third argument to 1, which will
 	// check whether the current environment has permission to set
 	// envid's status.
-
-	// LAB 4: Your code here.
-	panic("sys_env_set_status not implemented");
+	ENVID2ENV(e, envid);
+	e->env_status = status;
+	return 0;
 }
 
 // Set the page fault upcall for 'envid' by modifying the corresponding struct
@@ -118,10 +130,19 @@ sys_env_set_status(envid_t envid, int status)
 static int
 sys_env_set_pgfault_upcall(envid_t envid, void *func)
 {
-	// LAB 4: Your code here.
-	panic("sys_env_set_pgfault_upcall not implemented");
+	ENVID2ENV(e, envid);
+	e->env_pgfault_upcall = func;
+	return 0;
 }
 
+#define CHECK_USPACE_VA(va) \
+	{ if (((uint32_t)va % PGSIZE) || ((uintptr_t)va >= UTOP)) \
+		return -E_INVAL; }
+
+#define CHECK_USPACE_PERM(perm) \
+	(!(perm & PTE_U) || !(perm & PTE_P) || \
+			(perm & ~(PTE_P | PTE_U | PTE_W | PTE_AVAIL)))
+
 // Allocate a page of memory and map it at 'va' with permission
 // 'perm' in the address space of 'envid'.
 // The page's contents are set to 0.
@@ -147,11 +168,35 @@ sys_page_alloc(envid_t envid, void *va, int perm)
 	//   parameters for correctness.
 	//   If page_insert() fails, remember to free the page you
 	//   allocated!
+	ENVID2ENV(e, envid);
+
+	CHECK_USPACE_VA(va);
+	CHECK_USPACE_PERM(perm);
+
+	struct PageInfo *p = page_alloc(ALLOC_ZERO);
+	if (!p) {
+		return -E_NO_MEM;
+	}
 
-	// LAB 4: Your code here.
-	panic("sys_page_alloc not implemented");
+	int ret = page_insert(e->env_pgdir, p, va, perm);
+	if (ret) {
+		page_free(p);
+	}
+
+	return ret;
 }
 
+#define SHARED_PAGE(p, pgdir, va, perm) \
+	struct PageInfo *p; \
+	{ pte_t *t; \
+	p = page_lookup(pgdir, va, &t); \
+	if (!p) { \
+		return -E_INVAL; \
+	} \
+	if ((perm & PTE_W) && !(*t & PTE_W)) { \
+		return -E_INVAL; \
+	} }
+
 // Map the page of memory at 'srcva' in srcenvid's address space
 // at 'dstva' in dstenvid's address space with permission 'perm'.
 // Perm has the same restrictions as in sys_page_alloc, except
@@ -178,9 +223,17 @@ sys_page_map(envid_t srcenvid, void *srcva,
 	//   parameters for correctness.
 	//   Use the third argument to page_lookup() to
 	//   check the current permissions on the page.
+	ENVID2ENV(src, srcenvid);
+	ENVID2ENV(dst, dstenvid);
+
+	CHECK_USPACE_VA(srcva);
+	CHECK_USPACE_VA(dstva);
 
-	// LAB 4: Your code here.
-	panic("sys_page_map not implemented");
+	CHECK_USPACE_PERM(perm);
+
+	SHARED_PAGE(p, src->env_pgdir, srcva, perm);
+
+	return page_insert(dst->env_pgdir, p, dstva, perm);
 }
 
 // Unmap the page of memory at 'va' in the address space of 'envid'.
@@ -194,9 +247,10 @@ static int
 sys_page_unmap(envid_t envid, void *va)
 {
 	// Hint: This function is a wrapper around page_remove().
-
-	// LAB 4: Your code here.
-	panic("sys_page_unmap not implemented");
+	ENVID2ENV(e, envid);
+	CHECK_USPACE_VA(va);
+	page_remove(e->env_pgdir, va);
+	return 0;
 }
 
 // Try to send 'value' to the target env 'envid'.
@@ -240,8 +294,43 @@ sys_page_unmap(envid_t envid, void *va)
 static int
 sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
 {
-	// LAB 4: Your code here.
-	panic("sys_ipc_try_send not implemented");
+	struct Env *e;
+	int ret = envid2env(envid, &e, false);
+	if (ret) {
+		return ret;
+	}
+
+	if (!(e->env_ipc_recving) || (e->env_status != ENV_NOT_RUNNABLE)) {
+		return -E_IPC_NOT_RECV;
+	}
+
+	bool inserted_page = false;
+
+	if (srcva < (void *)UTOP) {
+		CHECK_USPACE_PERM(perm);
+		SHARED_PAGE(p, curenv->env_pgdir, srcva, perm);
+		if ((uintptr_t)srcva % PGSIZE) {
+			return -E_INVAL;
+		}
+
+		if (e->env_ipc_dstva < (void *)UTOP) {
+			int ret = page_insert(e->env_pgdir, p,
+					e->env_ipc_dstva, perm);
+			if (ret) {
+				return ret;
+			}
+			inserted_page = true;
+		}
+	}
+
+	e->env_ipc_recving = 0;
+	e->env_ipc_from = sys_getenvid();
+	e->env_ipc_value = value;
+	e->env_ipc_perm = inserted_page ? perm : 0;
+	e->env_status = ENV_RUNNABLE;
+	e->env_tf.tf_regs.reg_eax = 0;
+
+	return 0;
 }
 
 // Block until a value is ready.  Record that you want to receive
@@ -258,8 +347,13 @@ sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
 static int
 sys_ipc_recv(void *dstva)
 {
-	// LAB 4: Your code here.
-	panic("sys_ipc_recv not implemented");
+	if (((uintptr_t)dstva % PGSIZE) && ((uintptr_t)dstva < UTOP)) {
+		return -E_INVAL;
+	}
+	curenv->env_ipc_dstva = dstva;
+	curenv->env_ipc_recving = true;
+	curenv->env_status = ENV_NOT_RUNNABLE;
+	sys_yield();
 	return 0;
 }
 
@@ -269,8 +363,37 @@ syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4,
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
+		case SYS_yield:
+			sys_yield();
+			return 0;
+		case SYS_exofork:
+			return sys_exofork();
+		case SYS_env_set_status:
+			return sys_env_set_status(a1, a2);
+		case SYS_env_set_pgfault_upcall:
+			return sys_env_set_pgfault_upcall(a1, (void *)a2);
+		case SYS_page_alloc:
+			return sys_page_alloc(a1, (void *)a2, a3);
+		case SYS_page_map:
+			return sys_page_map(a1, (void *)a2, a3, (void *)a4, a5);
+		case SYS_page_unmap:
+			return sys_page_unmap(a1, (void *)a2);
+		case SYS_ipc_try_send:
+			return sys_ipc_try_send(a1, a2, (void *)a3, a4);
+		case SYS_ipc_recv:
+			return sys_ipc_recv((void *)a1);
+		default:
+			return -E_INVAL;
+	}
 }
 
diff --git a/kern/trap.c b/kern/trap.c
index 94386b4..070c990 100644
--- a/kern/trap.c
+++ b/kern/trap.c
@@ -71,7 +71,79 @@ trap_init(void)
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
+	extern unsigned thdl_irq0;
+	extern unsigned thdl_irq1;
+	extern unsigned thdl_irq2;
+	extern unsigned thdl_irq3;
+	extern unsigned thdl_irq4;
+	extern unsigned thdl_irq5;
+	extern unsigned thdl_irq6;
+	extern unsigned thdl_irq7;
+	extern unsigned thdl_irq8;
+	extern unsigned thdl_irq9;
+	extern unsigned thdl_irq10;
+	extern unsigned thdl_irq11;
+	extern unsigned thdl_irq12;
+	extern unsigned thdl_irq13;
+	extern unsigned thdl_irq14;
+	extern unsigned thdl_irq15;
+	extern unsigned thdl_syscall;
+
+	SETGATE(idt[ 0], 0, GD_KT, &thdl_divide,  0);
+	SETGATE(idt[ 1], 0, GD_KT, &thdl_debug,   0);
+	SETGATE(idt[ 2], 0, GD_KT, &thdl_nmi,     0);
+	SETGATE(idt[ 3], 0, GD_KT, &thdl_brkpt,   3);
+	SETGATE(idt[ 4], 0, GD_KT, &thdl_oflow,   0);
+	SETGATE(idt[ 5], 0, GD_KT, &thdl_bound,   0);
+	SETGATE(idt[ 6], 0, GD_KT, &thdl_illop,   0);
+	SETGATE(idt[ 7], 0, GD_KT, &thdl_device,  0);
+	SETGATE(idt[ 8], 0, GD_KT, &thdl_dblflt,  0);
+	SETGATE(idt[10], 0, GD_KT, &thdl_tss  ,   0);
+	SETGATE(idt[11], 0, GD_KT, &thdl_segnp,   0);
+	SETGATE(idt[12], 0, GD_KT, &thdl_stack,   0);
+	SETGATE(idt[13], 0, GD_KT, &thdl_gpflt,   0);
+	SETGATE(idt[14], 0, GD_KT, &thdl_pgflt,   0);
+	SETGATE(idt[16], 0, GD_KT, &thdl_fperr,   0);
+	SETGATE(idt[17], 0, GD_KT, &thdl_align,   0);
+	SETGATE(idt[18], 0, GD_KT, &thdl_mchk,    0);
+	SETGATE(idt[19], 0, GD_KT, &thdl_simderr, 0);
+
+	SETGATE(idt[32], 0, GD_KT, &thdl_irq0,    0);
+	SETGATE(idt[33], 0, GD_KT, &thdl_irq1,    0);
+	SETGATE(idt[34], 0, GD_KT, &thdl_irq2,    0);
+	SETGATE(idt[35], 0, GD_KT, &thdl_irq3,    0);
+	SETGATE(idt[36], 0, GD_KT, &thdl_irq4,    0);
+	SETGATE(idt[37], 0, GD_KT, &thdl_irq5,    0);
+	SETGATE(idt[38], 0, GD_KT, &thdl_irq6,    0);
+	SETGATE(idt[39], 0, GD_KT, &thdl_irq7,    0);
+	SETGATE(idt[40], 0, GD_KT, &thdl_irq8,    0);
+	SETGATE(idt[41], 0, GD_KT, &thdl_irq9,    0);
+	SETGATE(idt[42], 0, GD_KT, &thdl_irq10,   0);
+	SETGATE(idt[43], 0, GD_KT, &thdl_irq11,   0);
+	SETGATE(idt[44], 0, GD_KT, &thdl_irq12,   0);
+	SETGATE(idt[45], 0, GD_KT, &thdl_irq13,   0);
+	SETGATE(idt[46], 0, GD_KT, &thdl_irq14,   0);
+	SETGATE(idt[47], 0, GD_KT, &thdl_irq15,   0);
+
+	SETGATE(idt[48], 0, GD_KT, &thdl_syscall, 3);
 
 	// Per-CPU setup 
 	trap_init_percpu();
@@ -103,20 +175,21 @@ trap_init_percpu(void)
 	// user space on that CPU.
 	//
 	// LAB 4: Your code here:
+	size_t i = cpunum();
 
 	// Setup a TSS so that we get the right stack
 	// when we trap to the kernel.
-	ts.ts_esp0 = KSTACKTOP;
-	ts.ts_ss0 = GD_KD;
+	thiscpu->cpu_ts.ts_esp0 = KSTACKTOP - i * (KSTKSIZE + KSTKGAP);
+	thiscpu->cpu_ts.ts_ss0 = GD_KD;
 
 	// Initialize the TSS slot of the gdt.
-	gdt[GD_TSS0 >> 3] = SEG16(STS_T32A, (uint32_t) (&ts),
+	gdt[(GD_TSS0 >> 3) + i] = SEG16(STS_T32A, (uint32_t) (&thiscpu->cpu_ts),
 					sizeof(struct Taskstate), 0);
-	gdt[GD_TSS0 >> 3].sd_s = 0;
+	gdt[(GD_TSS0 >> 3) + i].sd_s = 0;
 
 	// Load the TSS selector (like other segment selectors, the
 	// bottom three bits are special; we leave them 0)
-	ltr(GD_TSS0);
+	ltr(((GD_TSS0 >> 3) + i) << 3);
 
 	// Load the IDT
 	lidt(&idt_pd);
@@ -172,7 +245,24 @@ static void
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
 
 	// Handle spurious interrupts
 	// The hardware sometimes raises these because of noise on the
@@ -185,7 +275,10 @@ trap_dispatch(struct Trapframe *tf)
 
 	// Handle clock interrupts. Don't forget to acknowledge the
 	// interrupt using lapic_eoi() before calling the scheduler!
-	// LAB 4: Your code here.
+	if (tf->tf_trapno == IRQ_OFFSET + IRQ_TIMER) {
+		lapic_eoi();
+		sched_yield();
+	}
 
 	// Unexpected trap: The user process or the kernel has a bug.
 	print_trapframe(tf);
@@ -216,6 +309,9 @@ trap(struct Trapframe *tf)
 	// Check that interrupts are disabled.  If this assertion
 	// fails, DO NOT be tempted to fix it by inserting a "cli" in
 	// the interrupt path.
+	if (read_eflags() & FL_IF) {
+		print_trapframe(tf);
+	}
 	assert(!(read_eflags() & FL_IF));
 
 	if ((tf->tf_cs & 3) == 3) {
@@ -223,6 +319,7 @@ trap(struct Trapframe *tf)
 		// Acquire the big kernel lock before doing any
 		// serious kernel work.
 		// LAB 4: Your code here.
+		lock_kernel();
 		assert(curenv);
 
 		// Garbage collect if current enviroment is a zombie
@@ -267,7 +364,10 @@ page_fault_handler(struct Trapframe *tf)
 
 	// Handle kernel-mode page faults.
 
-	// LAB 3: Your code here.
+	if ((tf->tf_cs & 3) != 3) {
+		print_trapframe(tf);
+		panic("Page fault in kernel!");
+	}
 
 	// We've already handled kernel-mode exceptions, so if we get here,
 	// the page fault happened in user mode.
@@ -299,8 +399,29 @@ page_fault_handler(struct Trapframe *tf)
 	//   user_mem_assert() and env_run() are useful here.
 	//   To change what the user environment runs, modify 'curenv->env_tf'
 	//   (the 'tf' variable points at 'curenv->env_tf').
+	if (curenv->env_pgfault_upcall) {
+
+		void *frame = (void *)UXSTACKTOP;
+		if ((tf->tf_esp >= UXSTACKTOP - PGSIZE) &&
+				(tf->tf_esp < UXSTACKTOP)) {
+			frame = (void *)(tf->tf_esp - sizeof(uint32_t));
+		}
+
+		struct UTrapframe *uf = frame;
+		--uf;
+		user_mem_assert(curenv, uf, sizeof(struct UTrapframe), PTE_W);
 
-	// LAB 4: Your code here.
+		uf->utf_esp    = tf->tf_esp;
+		uf->utf_eflags = tf->tf_eflags;
+		uf->utf_eip    = tf->tf_eip;
+		uf->utf_regs   = tf->tf_regs;
+		uf->utf_err    = tf->tf_err;
+		uf->utf_fault_va = tf->tf_trapno == T_PGFLT ? fault_va : 0;
+
+		curenv->env_tf.tf_eip = (uintptr_t)(curenv->env_pgfault_upcall);
+		curenv->env_tf.tf_esp = (uintptr_t)uf;
+		env_run(curenv);
+	}
 
 	// Destroy the environment that caused the fault.
 	cprintf("[%08x] user fault va %08x ip %08x\n",
diff --git a/kern/trapentry.S b/kern/trapentry.S
index 2dbeeca..4d2abeb 100644
--- a/kern/trapentry.S
+++ b/kern/trapentry.S
@@ -44,13 +44,50 @@
 
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
 
+TRAPHANDLER_NOEC(thdl_irq0,    32)
+TRAPHANDLER_NOEC(thdl_irq1,    33)
+TRAPHANDLER_NOEC(thdl_irq2,    34)
+TRAPHANDLER_NOEC(thdl_irq3,    35)
+TRAPHANDLER_NOEC(thdl_irq4,    36)
+TRAPHANDLER_NOEC(thdl_irq5,    37)
+TRAPHANDLER_NOEC(thdl_irq6,    38)
+TRAPHANDLER_NOEC(thdl_irq7,    39)
+TRAPHANDLER_NOEC(thdl_irq8,    40)
+TRAPHANDLER_NOEC(thdl_irq9,    41)
+TRAPHANDLER_NOEC(thdl_irq10,   42)
+TRAPHANDLER_NOEC(thdl_irq11,   43)
+TRAPHANDLER_NOEC(thdl_irq12,   44)
+TRAPHANDLER_NOEC(thdl_irq13,   45)
+TRAPHANDLER_NOEC(thdl_irq14,   46)
+TRAPHANDLER_NOEC(thdl_irq15,   47)
 
-/*
- * Lab 3: Your code here for _alltraps
- */
+TRAPHANDLER_NOEC(thdl_syscall, 48)
 
+_alltraps:
+	pushl %ds
+	pushl %es
+	pushal
+	mov $GD_KD, %eax
+	mov %ax, %ds
+	mov %ax, %es
+	pushl %esp
+	call trap
diff --git a/lib/fork.c b/lib/fork.c
index 56c0714..8d45043 100644
--- a/lib/fork.c
+++ b/lib/fork.c
@@ -23,8 +23,14 @@ pgfault(struct UTrapframe *utf)
 	// Hint:
 	//   Use the read-only page table mappings at uvpt
 	//   (see <inc/memlayout.h>).
+	if (!uvpd[PDX(addr)] || !(uvpt[PGNUM(addr)] & PTE_P ||
+			uvpt[PGNUM(addr)] & PTE_COW)) {
+		panic("The page does not exist");
+	}
 
-	// LAB 4: Your code here.
+	if (!(err & FEC_WR)) {
+		panic("The request was not for writing");
+	}
 
 	// Allocate a new page, map it at a temporary location (PFTEMP),
 	// copy the data from the old page to the new page, then move the new
@@ -32,10 +38,24 @@ pgfault(struct UTrapframe *utf)
 	// Hint:
 	//   You should make three system calls.
 	//   No need to explicitly delete the old page's mapping.
+	int perm = PTE_P | PTE_W | PTE_U;
 
-	// LAB 4: Your code here.
+	int ret = sys_page_alloc(0, PFTEMP, perm);
+	if (ret) {
+		panic("Couldn't allocate a page");
+	}
 
-	panic("pgfault not implemented");
+	void *a = (void *)ROUNDDOWN(addr, PGSIZE);
+	memcpy((void *)PFTEMP, a, PGSIZE);
+
+	ret = sys_page_map(0, (void *)PFTEMP, 0, a, perm);
+	if (ret) {
+		panic("Mapping failed");
+	}
+	ret = sys_page_unmap(0, (void *)PFTEMP);
+	if (ret) {
+		panic("Unmapping failed");
+	}
 }
 
 //
@@ -52,10 +72,21 @@ pgfault(struct UTrapframe *utf)
 static int
 duppage(envid_t envid, unsigned pn)
 {
-	int r;
+	int r = 0;
+
+	int perm = PTE_P | PTE_U | PTE_COW;
+	void *addr = (void *)(pn * PGSIZE);
 
-	// LAB 4: Your code here.
-	panic("duppage not implemented");
+	if (uvpt[pn] & (PTE_COW | PTE_W)) {
+		r |= sys_page_map(0, addr, envid, addr, perm);
+		r |= sys_page_map(0, addr, 0,     addr, perm);
+	} else {
+		r |= sys_page_map(0, addr, envid, addr, PTE_U | PTE_P);
+	}
+
+	if (r) {
+		panic("Failed to provide the mapping");
+	}
 	return 0;
 }
 
@@ -78,8 +109,39 @@ duppage(envid_t envid, unsigned pn)
 envid_t
 fork(void)
 {
-	// LAB 4: Your code here.
-	panic("fork not implemented");
+	set_pgfault_handler(pgfault);
+	envid_t cpid = sys_exofork();
+	if (!cpid) {
+		thisenv = envs + ENVX(sys_getenvid());
+		return 0;
+	}
+
+	if (cpid < 0) {
+		panic("Failed to copy environments");
+	}
+
+	size_t i;
+	for (i = 0; i < USTACKTOP; i += PGSIZE) {
+		if (i == UXSTACKTOP - 1) {
+			continue;
+		} else if ((uvpd[PDX(i)] & PTE_P) &&
+				(uvpt[PGNUM(i)] & PTE_U) &&
+				(uvpt[PGNUM(i)] & PTE_P)) {
+			duppage(cpid, PGNUM(i));
+		}
+	}
+
+	int ret = sys_page_alloc(cpid, (void *)(UXSTACKTOP-PGSIZE),
+			PTE_P | PTE_W | PTE_U);
+	if (ret) {
+		return ret;
+	}
+
+	extern void _pgfault_upcall();
+	sys_env_set_pgfault_upcall(cpid, _pgfault_upcall);
+	sys_env_set_status(cpid, ENV_RUNNABLE);
+
+	return cpid;
 }
 
 // Challenge!
diff --git a/lib/ipc.c b/lib/ipc.c
index 6dfb5ff..ae25ff9 100644
--- a/lib/ipc.c
+++ b/lib/ipc.c
@@ -23,8 +23,17 @@ int32_t
 ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
 {
 	// LAB 4: Your code here.
-	panic("ipc_recv not implemented");
-	return 0;
+	if (!pg) {
+		pg = (void *)UTOP;
+	}
+	int ret = sys_ipc_recv(pg);
+	if (from_env_store) {
+		*from_env_store = ret ? 0 : thisenv->env_ipc_from;
+	}
+	if (perm_store) {
+		*perm_store = ret ? 0 : thisenv->env_ipc_perm;
+	}
+	return ret ? ret : thisenv->env_ipc_value;
 }
 
 // Send 'val' (and 'pg' with 'perm', if 'pg' is nonnull) to 'toenv'.
@@ -38,8 +47,20 @@ ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
 void
 ipc_send(envid_t to_env, uint32_t val, void *pg, int perm)
 {
-	// LAB 4: Your code here.
-	panic("ipc_send not implemented");
+	if (!pg) {
+		pg = (void *)UTOP;
+	}
+
+	int ret;
+	do {
+		ret = sys_ipc_try_send(to_env, val, pg, perm);
+		sys_yield();
+	} while (ret == -E_IPC_NOT_RECV);
+
+	if (ret) {
+		panic("Failed IPC %04x -> %04x: %e",
+				sys_getenvid(), to_env, ret);
+	}
 }
 
 // Find the first environment of the given type.  We'll use this to
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
diff --git a/lib/pfentry.S b/lib/pfentry.S
index f40aeeb..5bded84 100644
--- a/lib/pfentry.S
+++ b/lib/pfentry.S
@@ -63,20 +63,24 @@ _pgfault_upcall:
 	// registers are available for intermediate calculations.  You
 	// may find that you have to rearrange your code in non-obvious
 	// ways as registers become unavailable as scratch space.
-	//
-	// LAB 4: Your code here.
+	movl 40(%esp), %eax
+	subl $4, 48(%esp)
+	movl 48(%esp), %ebx
+	movl %eax, (%ebx)
 
 	// Restore the trap-time registers.  After you do this, you
 	// can no longer modify any general-purpose registers.
-	// LAB 4: Your code here.
+	addl $8, %esp
+	popal
 
 	// Restore eflags from the stack.  After you do this, you can
 	// no longer use arithmetic operations or anything else that
 	// modifies eflags.
-	// LAB 4: Your code here.
+	addl $4, %esp
+	popfl
 
 	// Switch back to the adjusted trap-time stack.
-	// LAB 4: Your code here.
+	popl %esp
 
 	// Return to re-execute the instruction that faulted.
-	// LAB 4: Your code here.
+	ret
diff --git a/lib/pgfault.c b/lib/pgfault.c
index a975518..a06afd6 100644
--- a/lib/pgfault.c
+++ b/lib/pgfault.c
@@ -28,8 +28,13 @@ set_pgfault_handler(void (*handler)(struct UTrapframe *utf))
 
 	if (_pgfault_handler == 0) {
 		// First time through!
-		// LAB 4: Your code here.
-		panic("set_pgfault_handler not implemented");
+		envid_t curenv = sys_getenvid();
+		int ret = sys_page_alloc(curenv, (void *)(UXSTACKTOP-PGSIZE),
+				PTE_P | PTE_U | PTE_W);
+		if (ret) {
+			panic("Couldn't allocate user exception stack!");
+		}
+		sys_env_set_pgfault_upcall(curenv, _pgfault_upcall);
 	}
 
 	// Save handler pointer for assembly to call.
