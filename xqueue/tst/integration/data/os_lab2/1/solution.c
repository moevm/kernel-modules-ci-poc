diff --git a/kern/pmap.c b/kern/pmap.c
index d90bd1e..eb945cf 100644
--- a/kern/pmap.c
+++ b/kern/pmap.c
@@ -99,7 +99,17 @@ boot_alloc(uint32_t n)
 	//
 	// LAB 2: Your code here.
 
-	return NULL;
+	result = nextfree;
+
+	if (n) {
+		extern char end[];
+		nextfree = ROUNDUP(nextfree + n, PGSIZE);
+		if (nextfree - end > npages * PGSIZE) {
+			panic("Out of memory");
+		}
+	}
+
+	return result;
 }
 
 // Set up a two-level page table:
@@ -120,9 +130,6 @@ mem_init(void)
 	// Find out how much memory the machine has (npages & npages_basemem).
 	i386_detect_memory();
 
-	// Remove this line when you're ready to test this function.
-	panic("mem_init: This function is not finished\n");
-
 	//////////////////////////////////////////////////////////////////////
 	// create initial page directory.
 	kern_pgdir = (pde_t *) boot_alloc(PGSIZE);
@@ -142,8 +149,7 @@ mem_init(void)
 	// The kernel uses this array to keep track of physical pages: for
 	// each physical page, there is a corresponding struct PageInfo in this
 	// array.  'npages' is the number of physical pages in memory.
-	// Your code goes here:
-
+	pages = boot_alloc(sizeof(struct PageInfo) * npages);
 
 	//////////////////////////////////////////////////////////////////////
 	// Now that we've allocated the initial kernel data structures, we set
@@ -166,7 +172,12 @@ mem_init(void)
 	//    - the new image at UPAGES -- kernel R, user R
 	//      (ie. perm = PTE_U | PTE_P)
 	//    - pages itself -- kernel RW, user NONE
-	// Your code goes here:
+	size_t page_blocks = ROUNDUP(npages * sizeof(struct PageInfo), PGSIZE);
+	size_t i;
+	for (i = 0; i < page_blocks; i += PGSIZE) {
+		*pgdir_walk(kern_pgdir, (void*)(UPAGES + i), 1) =
+			(PADDR(pages) + i) | PTE_U | PTE_P;
+	}
 
 	//////////////////////////////////////////////////////////////////////
 	// Use the physical memory that 'bootstack' refers to as the kernel
@@ -178,7 +189,10 @@ mem_init(void)
 	//       the kernel overflows its stack, it will fault rather than
 	//       overwrite memory.  Known as a "guard page".
 	//     Permissions: kernel RW, user NONE
-	// Your code goes here:
+	for (i = 0; i < KSTKSIZE; i += PGSIZE) {
+		*pgdir_walk(kern_pgdir, (void*)(KSTACKTOP - KSTKSIZE + i), 1) =
+			(PADDR(bootstack) + i) | PTE_W | PTE_P;
+	}
 
 	//////////////////////////////////////////////////////////////////////
 	// Map all of physical memory at KERNBASE.
@@ -187,7 +201,11 @@ mem_init(void)
 	// We might not have 2^32 - KERNBASE bytes of physical memory, but
 	// we just set up the mapping anyway.
 	// Permissions: kernel RW, user NONE
-	// Your code goes here:
+	size_t kernbase_blocks = 1+~KERNBASE;
+	for (i = 0; i < kernbase_blocks; i += PGSIZE) {
+		*pgdir_walk(kern_pgdir, (void*)(KERNBASE + i), 1) =
+			i | PTE_W | PTE_P;
+	}
 
 	// Check that the initial page directory has been set up correctly.
 	check_kern_pgdir();
@@ -247,7 +265,14 @@ page_init(void)
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
@@ -266,8 +291,20 @@ page_init(void)
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
@@ -277,7 +314,8 @@ page_alloc(int alloc_flags)
 void
 page_free(struct PageInfo *pp)
 {
-	// Fill this function in
+	pp->pp_link = page_free_list;
+	page_free_list = pp;
 }
 
 //
@@ -316,8 +354,21 @@ page_decref(struct PageInfo* pp)
 pte_t *
 pgdir_walk(pde_t *pgdir, const void *va, int create)
 {
-	// Fill this function in
-	return NULL;
+	if (!pgdir[PDX(va)] && create) {
+		struct PageInfo *pp = page_alloc(ALLOC_ZERO);
+		if (pp) {
+			pp->pp_ref++;
+			pgdir[PDX(va)] = page2pa(pp) | PTE_P | PTE_U | PTE_W;
+		}
+	}
+
+	if (!pgdir[PDX(va)]) {
+		return NULL;
+	}
+
+	pte_t *pgtbl = (pte_t *)(KADDR(PTE_ADDR(pgdir[PDX(va)])));
+
+	return pgtbl + PTX(va);
 }
 
 //
@@ -333,7 +384,11 @@ pgdir_walk(pde_t *pgdir, const void *va, int create)
 static void
 boot_map_region(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm)
 {
-	// Fill this function in
+	uintptr_t i;
+	for (i = va; i < va + size; i += PGSIZE) {
+		*pgdir_walk(pgdir, (void*)i, 1) =
+			PTE_ADDR(PTX(i)) | perm | PTE_P;
+	}
 }
 
 //
@@ -364,7 +419,16 @@ boot_map_region(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm
 int
 page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm)
 {
-	// Fill this function in
+	pp->pp_ref++;
+	page_remove(pgdir, va);
+	pp->pp_ref--;
+	pte_t *t = pgdir_walk(pgdir, va, 1);
+	if (!t) {
+		return -E_NO_MEM;
+	}
+
+	*t = page2pa(pp) | PTE_P | perm;
+	pp->pp_ref++;
 	return 0;
 }
 
@@ -382,8 +446,16 @@ page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm)
 struct PageInfo *
 page_lookup(pde_t *pgdir, void *va, pte_t **pte_store)
 {
-	// Fill this function in
-	return NULL;
+	pte_t *t = pgdir_walk(pgdir, va, 0);
+	if (!t) {
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
@@ -404,7 +476,14 @@ page_lookup(pde_t *pgdir, void *va, pte_t **pte_store)
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
