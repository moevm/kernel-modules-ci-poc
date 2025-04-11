diff --git a/fs/bc.c b/fs/bc.c
index 094c208..069f4d2 100644
--- a/fs/bc.c
+++ b/fs/bc.c
@@ -5,9 +5,9 @@
 void*
 diskaddr(uint32_t blockno)
 {
-	if (blockno == 0 || (super && blockno >= super->s_nblocks))
-		panic("bad block number %08x in diskaddr", blockno);
-	return (char*) (DISKMAP + blockno * BLKSIZE);
+    if (blockno == 0 || (super && blockno >= super->s_nblocks))
+        panic("bad block number %08x in diskaddr", blockno);
+    return (char*) (DISKMAP + blockno * BLKSIZE);
 }
 
 // Fault any disk block that is read in to memory by
@@ -15,35 +15,42 @@ diskaddr(uint32_t blockno)
 static void
 bc_pgfault(struct UTrapframe *utf)
 {
-	void *addr = (void *) utf->utf_fault_va;
-	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;
-	int r;
-
-	// Check that the fault was within the block cache region
-	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
-		panic("page fault in FS: eip %08x, va %08x, err %04x",
-		      utf->utf_eip, addr, utf->utf_err);
-
-	// Sanity check the block number.
-	if (super && blockno >= super->s_nblocks)
-		panic("reading non-existent block %08x\n", blockno);
-
-	// Allocate a page in the disk map region, read the contents
-	// of the block from the disk into that page.
-	// Hint: first round addr to page boundary.
-	//
-	// LAB 5: you code here:
-
+    void *addr = (void *) utf->utf_fault_va;
+    uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;
+    int r;
+
+    // Check that the fault was within the block cache region
+    if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
+        panic("page fault in FS: eip %08x, va %08x, err %04x",
+              utf->utf_eip, addr, utf->utf_err);
+
+    // Sanity check the block number.
+    if (super && blockno >= super->s_nblocks)
+        panic("reading non-existent block %08x\n", blockno);
+
+    // Allocate a page in the disk map region, read the contents
+    // of the block from the disk into that page.
+    // Hint: first round addr to page boundary.
+    //
+    // LAB 5: you code here:
+    void *pgva = ROUNDDOWN(addr, BLKSIZE);
+    if (sys_page_alloc(0, pgva, PTE_U | PTE_W | PTE_P)) {
+        panic("fail in bc_pgfault");
+    }
+    uint32_t sectors = blockno * BLKSECTS;
+    if (ide_read(sectors, pgva, BLKSECTS)) {
+        panic("fail in bc_pgfault");
+    }
 }
 
 
 void
 bc_init(void)
 {
-	struct Super super;
-	set_pgfault_handler(bc_pgfault);
+    struct Super super;
+    set_pgfault_handler(bc_pgfault);
 
-	// cache the super block by reading it once
-	memmove(&super, diskaddr(1), sizeof super);
+    // cache the super block by reading it once
+    memmove(&super, diskaddr(1), sizeof super);
 }
 
diff --git a/fs/fs.c b/fs/fs.c
index 82cda56..b3383b5 100644
--- a/fs/fs.c
+++ b/fs/fs.c
@@ -10,13 +10,13 @@
 void
 check_super(void)
 {
-	if (super->s_magic != FS_MAGIC)
-		panic("bad file system magic number");
+    if (super->s_magic != FS_MAGIC)
+        panic("bad file system magic number");
 
-	if (super->s_nblocks > DISKSIZE/BLKSIZE)
-		panic("file system is too large");
+    if (super->s_nblocks > DISKSIZE/BLKSIZE)
+        panic("file system is too large");
 
-	cprintf("superblock is good\n");
+    cprintf("superblock is good\n");
 }
 
 
@@ -28,19 +28,19 @@ check_super(void)
 void
 fs_init(void)
 {
-	static_assert(sizeof(struct File) == 256);
+    static_assert(sizeof(struct File) == 256);
 
-	// Find a JOS disk.  Use the second IDE disk (number 1) if available.
-	if (ide_probe_disk1())
-		ide_set_disk(1);
-	else
-		ide_set_disk(0);
+    // Find a JOS disk.  Use the second IDE disk (number 1) if available.
+    if (ide_probe_disk1())
+        ide_set_disk(1);
+    else
+        ide_set_disk(0);
 
-	bc_init();
+    bc_init();
 
-	// Set "super" to point to the super block.
-	super = diskaddr(1);
-	check_super();
+    // Set "super" to point to the super block.
+    super = diskaddr(1);
+    check_super();
 }
 
 // Find the disk block number slot for the 'filebno'th block in file 'f'.
@@ -62,22 +62,22 @@ fs_init(void)
 static int
 file_block_walk(struct File *f, uint32_t filebno, uint32_t **ppdiskbno, bool alloc)
 {
-	int r;
-	uint32_t *ptr;
-	char *blk;
-
-	if (filebno < NDIRECT)
-		ptr = &f->f_direct[filebno];
-	else if (filebno < NDIRECT + NINDIRECT) {
-		if (f->f_indirect == 0) {
-			return -E_NOT_FOUND;
-		}
-		ptr = (uint32_t*)diskaddr(f->f_indirect) + filebno - NDIRECT;
-	} else
-		return -E_INVAL;
-
-	*ppdiskbno = ptr;
-	return 0;
+    int r;
+    uint32_t *ptr;
+    char *blk;
+
+    if (filebno < NDIRECT)
+        ptr = &f->f_direct[filebno];
+    else if (filebno < NDIRECT + NINDIRECT) {
+        if (f->f_indirect == 0) {
+            return -E_NOT_FOUND;
+        }
+        ptr = (uint32_t*)diskaddr(f->f_indirect) + filebno - NDIRECT;
+    } else
+        return -E_INVAL;
+
+    *ppdiskbno = ptr;
+    return 0;
 }
 
 // Set *blk to the address in memory where the filebno'th
@@ -90,16 +90,16 @@ file_block_walk(struct File *f, uint32_t filebno, uint32_t **ppdiskbno, bool all
 int
 file_get_block(struct File *f, uint32_t filebno, char **blk)
 {
-	int r;
-	uint32_t *ptr;
-
-	if ((r = file_block_walk(f, filebno, &ptr, 1)) < 0)
-		return r;
-	if (*ptr == 0) {
-		return -E_NOT_FOUND;
-	}
-	*blk = diskaddr(*ptr);
-	return 0;
+    int r;
+    uint32_t *ptr;
+
+    if ((r = file_block_walk(f, filebno, &ptr, 1)) < 0)
+        return r;
+    if (*ptr == 0) {
+        return -E_NOT_FOUND;
+    }
+    *blk = diskaddr(*ptr);
+    return 0;
 }
 
 // Try to find a file named "name" in dir.  If so, set *file to it.
@@ -109,27 +109,27 @@ file_get_block(struct File *f, uint32_t filebno, char **blk)
 static int
 dir_lookup(struct File *dir, const char *name, struct File **file)
 {
-	int r;
-	uint32_t i, j, nblock;
-	char *blk;
-	struct File *f;
-
-	// Search dir for name.
-	// We maintain the invariant that the size of a directory-file
-	// is always a multiple of the file system's block size.
-	assert((dir->f_size % BLKSIZE) == 0);
-	nblock = dir->f_size / BLKSIZE;
-	for (i = 0; i < nblock; i++) {
-		if ((r = file_get_block(dir, i, &blk)) < 0)
-			return r;
-		f = (struct File*) blk;
-		for (j = 0; j < BLKFILES; j++)
-			if (strcmp(f[j].f_name, name) == 0) {
-				*file = &f[j];
-				return 0;
-			}
-	}
-	return -E_NOT_FOUND;
+    int r;
+    uint32_t i, j, nblock;
+    char *blk;
+    struct File *f;
+
+    // Search dir for name.
+    // We maintain the invariant that the size of a directory-file
+    // is always a multiple of the file system's block size.
+    assert((dir->f_size % BLKSIZE) == 0);
+    nblock = dir->f_size / BLKSIZE;
+    for (i = 0; i < nblock; i++) {
+        if ((r = file_get_block(dir, i, &blk)) < 0)
+            return r;
+        f = (struct File*) blk;
+        for (j = 0; j < BLKFILES; j++)
+            if (strcmp(f[j].f_name, name) == 0) {
+                *file = &f[j];
+                return 0;
+            }
+    }
+    return -E_NOT_FOUND;
 }
 
 
@@ -137,9 +137,9 @@ dir_lookup(struct File *dir, const char *name, struct File **file)
 static const char*
 skip_slash(const char *p)
 {
-	while (*p == '/')
-		p++;
-	return p;
+    while (*p == '/')
+        p++;
+    return p;
 }
 
 // Evaluate a path name, starting at the root.
@@ -151,51 +151,51 @@ skip_slash(const char *p)
 static int
 walk_path(const char *path, struct File **pdir, struct File **pf, char *lastelem)
 {
-	const char *p;
-	char name[MAXNAMELEN];
-	struct File *dir, *f;
-	int r;
-
-	// if (*path != '/')
-	//	return -E_BAD_PATH;
-	path = skip_slash(path);
-	f = &super->s_root;
-	dir = 0;
-	name[0] = 0;
-
-	if (pdir)
-		*pdir = 0;
-	*pf = 0;
-	while (*path != '\0') {
-		dir = f;
-		p = path;
-		while (*path != '/' && *path != '\0')
-			path++;
-		if (path - p >= MAXNAMELEN)
-			return -E_BAD_PATH;
-		memmove(name, p, path - p);
-		name[path - p] = '\0';
-		path = skip_slash(path);
-
-		if (dir->f_type != FTYPE_DIR)
-			return -E_NOT_FOUND;
-
-		if ((r = dir_lookup(dir, name, &f)) < 0) {
-			if (r == -E_NOT_FOUND && *path == '\0') {
-				if (pdir)
-					*pdir = dir;
-				if (lastelem)
-					strcpy(lastelem, name);
-				*pf = 0;
-			}
-			return r;
-		}
-	}
-
-	if (pdir)
-		*pdir = dir;
-	*pf = f;
-	return 0;
+    const char *p;
+    char name[MAXNAMELEN];
+    struct File *dir, *f;
+    int r;
+
+    // if (*path != '/')
+    //	return -E_BAD_PATH;
+    path = skip_slash(path);
+    f = &super->s_root;
+    dir = 0;
+    name[0] = 0;
+
+    if (pdir)
+        *pdir = 0;
+    *pf = 0;
+    while (*path != '\0') {
+        dir = f;
+        p = path;
+        while (*path != '/' && *path != '\0')
+            path++;
+        if (path - p >= MAXNAMELEN)
+            return -E_BAD_PATH;
+        memmove(name, p, path - p);
+        name[path - p] = '\0';
+        path = skip_slash(path);
+
+        if (dir->f_type != FTYPE_DIR)
+            return -E_NOT_FOUND;
+
+        if ((r = dir_lookup(dir, name, &f)) < 0) {
+            if (r == -E_NOT_FOUND && *path == '\0') {
+                if (pdir)
+                    *pdir = dir;
+                if (lastelem)
+                    strcpy(lastelem, name);
+                *pf = 0;
+            }
+            return r;
+        }
+    }
+
+    if (pdir)
+        *pdir = dir;
+    *pf = f;
+    return 0;
 }
 
 // --------------------------------------------------------------
@@ -208,7 +208,7 @@ walk_path(const char *path, struct File **pdir, struct File **pf, char *lastelem
 int
 file_open(const char *path, struct File **pf)
 {
-	return walk_path(path, 0, pf, 0);
+    return walk_path(path, 0, pf, 0);
 }
 
 // Read count bytes from f into buf, starting from seek position
@@ -217,25 +217,25 @@ file_open(const char *path, struct File **pf)
 ssize_t
 file_read(struct File *f, void *buf, size_t count, off_t offset)
 {
-	int r, bn;
-	off_t pos;
-	char *blk;
+    int r, bn;
+    off_t pos;
+    char *blk;
 
-	if (offset >= f->f_size)
-		return 0;
+    if (offset >= f->f_size)
+        return 0;
 
-	count = MIN(count, f->f_size - offset);
+    count = MIN(count, f->f_size - offset);
 
-	for (pos = offset; pos < offset + count; ) {
-		if ((r = file_get_block(f, pos / BLKSIZE, &blk)) < 0)
-			return r;
-		bn = MIN(BLKSIZE - pos % BLKSIZE, offset + count - pos);
-		memmove(buf, blk + pos % BLKSIZE, bn);
-		pos += bn;
-		buf += bn;
-	}
+    for (pos = offset; pos < offset + count; ) {
+        if ((r = file_get_block(f, pos / BLKSIZE, &blk)) < 0)
+            return r;
+        bn = MIN(BLKSIZE - pos % BLKSIZE, offset + count - pos);
+        memmove(buf, blk + pos % BLKSIZE, bn);
+        pos += bn;
+        buf += bn;
+    }
 
-	return count;
+    return count;
 }
 
 
diff --git a/fs/fsformat.c b/fs/fsformat.c
index 216789e..fae03b3 100644
--- a/fs/fsformat.c
+++ b/fs/fsformat.c
@@ -36,9 +36,9 @@ typedef int bool;
 
 struct Dir
 {
-	struct File *f;
-	struct File *ents;
-	int n;
+    struct File *f;
+    struct File *ents;
+    int n;
 };
 
 uint32_t nblocks;
@@ -55,190 +55,190 @@ panic(const char *fmt, ...)
         vfprintf(stderr, fmt, ap);
         va_end(ap);
         fputc('\n', stderr);
-	abort();
+    abort();
 }
 
 void
 readn(int f, void *out, size_t n)
 {
-	size_t p = 0;
-	while (p < n) {
-		size_t m = read(f, out + p, n - p);
-		if (m < 0)
-			panic("read: %s", strerror(errno));
-		if (m == 0)
-			panic("read: Unexpected EOF");
-		p += m;
-	}
+    size_t p = 0;
+    while (p < n) {
+        size_t m = read(f, out + p, n - p);
+        if (m < 0)
+            panic("read: %s", strerror(errno));
+        if (m == 0)
+            panic("read: Unexpected EOF");
+        p += m;
+    }
 }
 
 uint32_t
 blockof(void *pos)
 {
-	return ((char*)pos - diskmap) / BLKSIZE;
+    return ((char*)pos - diskmap) / BLKSIZE;
 }
 
 void *
 alloc(uint32_t bytes)
 {
-	void *start = diskpos;
-	diskpos += ROUNDUP(bytes, BLKSIZE);
-	if (blockof(diskpos) >= nblocks)
-		panic("out of disk blocks");
-	return start;
+    void *start = diskpos;
+    diskpos += ROUNDUP(bytes, BLKSIZE);
+    if (blockof(diskpos) >= nblocks)
+        panic("out of disk blocks");
+    return start;
 }
 
 void
 opendisk(const char *name)
 {
-	int r, diskfd, nbitblocks;
+    int r, diskfd, nbitblocks;
 
-	if ((diskfd = open(name, O_RDWR | O_CREAT, 0666)) < 0)
-		panic("open %s: %s", name, strerror(errno));
+    if ((diskfd = open(name, O_RDWR | O_CREAT, 0666)) < 0)
+        panic("open %s: %s", name, strerror(errno));
 
-	if ((r = ftruncate(diskfd, 0)) < 0
-	    || (r = ftruncate(diskfd, nblocks * BLKSIZE)) < 0)
-		panic("truncate %s: %s", name, strerror(errno));
+    if ((r = ftruncate(diskfd, 0)) < 0
+        || (r = ftruncate(diskfd, nblocks * BLKSIZE)) < 0)
+        panic("truncate %s: %s", name, strerror(errno));
 
-	if ((diskmap = mmap(NULL, nblocks * BLKSIZE, PROT_READ|PROT_WRITE,
-			    MAP_SHARED, diskfd, 0)) == MAP_FAILED)
-		panic("mmap %s: %s", name, strerror(errno));
+    if ((diskmap = mmap(NULL, nblocks * BLKSIZE, PROT_READ|PROT_WRITE,
+                MAP_SHARED, diskfd, 0)) == MAP_FAILED)
+        panic("mmap %s: %s", name, strerror(errno));
 
-	close(diskfd);
+    close(diskfd);
 
-	diskpos = diskmap;
-	alloc(BLKSIZE);
-	super = alloc(BLKSIZE);
-	super->s_magic = FS_MAGIC;
-	super->s_nblocks = nblocks;
-	super->s_root.f_type = FTYPE_DIR;
-	strcpy(super->s_root.f_name, "/");
+    diskpos = diskmap;
+    alloc(BLKSIZE);
+    super = alloc(BLKSIZE);
+    super->s_magic = FS_MAGIC;
+    super->s_nblocks = nblocks;
+    super->s_root.f_type = FTYPE_DIR;
+    strcpy(super->s_root.f_name, "/");
 
-	nbitblocks = (nblocks + BLKBITSIZE - 1) / BLKBITSIZE;
-	bitmap = alloc(nbitblocks * BLKSIZE);
-	memset(bitmap, 0xFF, nbitblocks * BLKSIZE);
+    nbitblocks = (nblocks + BLKBITSIZE - 1) / BLKBITSIZE;
+    bitmap = alloc(nbitblocks * BLKSIZE);
+    memset(bitmap, 0xFF, nbitblocks * BLKSIZE);
 }
 
 void
 finishdisk(void)
 {
-	int r, i;
+    int r, i;
 
-	for (i = 0; i < blockof(diskpos); ++i)
-		bitmap[i/32] &= ~(1<<(i%32));
+    for (i = 0; i < blockof(diskpos); ++i)
+        bitmap[i/32] &= ~(1<<(i%32));
 
-	if ((r = msync(diskmap, nblocks * BLKSIZE, MS_SYNC)) < 0)
-		panic("msync: %s", strerror(errno));
+    if ((r = msync(diskmap, nblocks * BLKSIZE, MS_SYNC)) < 0)
+        panic("msync: %s", strerror(errno));
 }
 
 void
 finishfile(struct File *f, uint32_t start, uint32_t len)
 {
-	int i;
-	f->f_size = len;
-	len = ROUNDUP(len, BLKSIZE);
-	for (i = 0; i < len / BLKSIZE && i < NDIRECT; ++i)
-		f->f_direct[i] = start + i;
-	if (i == NDIRECT) {
-		uint32_t *ind = alloc(BLKSIZE);
-		f->f_indirect = blockof(ind);
-		for (; i < len / BLKSIZE; ++i)
-			ind[i - NDIRECT] = start + i;
-	}
+    int i;
+    f->f_size = len;
+    len = ROUNDUP(len, BLKSIZE);
+    for (i = 0; i < len / BLKSIZE && i < NDIRECT; ++i)
+        f->f_direct[i] = start + i;
+    if (i == NDIRECT) {
+        uint32_t *ind = alloc(BLKSIZE);
+        f->f_indirect = blockof(ind);
+        for (; i < len / BLKSIZE; ++i)
+            ind[i - NDIRECT] = start + i;
+    }
 }
 
 void
 startdir(struct File *f, struct Dir *dout)
 {
-	dout->f = f;
-	dout->ents = malloc(MAX_DIR_ENTS * sizeof *dout->ents);
-	dout->n = 0;
+    dout->f = f;
+    dout->ents = malloc(MAX_DIR_ENTS * sizeof *dout->ents);
+    dout->n = 0;
 }
 
 struct File *
 diradd(struct Dir *d, uint32_t type, const char *name)
 {
-	struct File *out = &d->ents[d->n++];
-	if (d->n > MAX_DIR_ENTS)
-		panic("too many directory entries");
-	strcpy(out->f_name, name);
-	out->f_type = type;
-	return out;
+    struct File *out = &d->ents[d->n++];
+    if (d->n > MAX_DIR_ENTS)
+        panic("too many directory entries");
+    strcpy(out->f_name, name);
+    out->f_type = type;
+    return out;
 }
 
 void
 finishdir(struct Dir *d)
 {
-	int size = d->n * sizeof(struct File);
-	struct File *start = alloc(size);
-	memmove(start, d->ents, size);
-	finishfile(d->f, blockof(start), ROUNDUP(size, BLKSIZE));
-	free(d->ents);
-	d->ents = NULL;
+    int size = d->n * sizeof(struct File);
+    struct File *start = alloc(size);
+    memmove(start, d->ents, size);
+    finishfile(d->f, blockof(start), ROUNDUP(size, BLKSIZE));
+    free(d->ents);
+    d->ents = NULL;
 }
 
 void
 writefile(struct Dir *dir, const char *name)
 {
-	int r, fd;
-	struct File *f;
-	struct stat st;
-	const char *last;
-	char *start;
-
-	if ((fd = open(name, O_RDONLY)) < 0)
-		panic("open %s: %s", name, strerror(errno));
-	if ((r = fstat(fd, &st)) < 0)
-		panic("stat %s: %s", name, strerror(errno));
-	if (!S_ISREG(st.st_mode))
-		panic("%s is not a regular file", name);
-	if (st.st_size >= MAXFILESIZE)
-		panic("%s too large", name);
-
-	last = strrchr(name, '/');
-	if (last)
-		last++;
-	else
-		last = name;
-
-	f = diradd(dir, FTYPE_REG, last);
-	start = alloc(st.st_size);
-	readn(fd, start, st.st_size);
-	finishfile(f, blockof(start), st.st_size);
-	close(fd);
+    int r, fd;
+    struct File *f;
+    struct stat st;
+    const char *last;
+    char *start;
+
+    if ((fd = open(name, O_RDONLY)) < 0)
+        panic("open %s: %s", name, strerror(errno));
+    if ((r = fstat(fd, &st)) < 0)
+        panic("stat %s: %s", name, strerror(errno));
+    if (!S_ISREG(st.st_mode))
+        panic("%s is not a regular file", name);
+    if (st.st_size >= MAXFILESIZE)
+        panic("%s too large", name);
+
+    last = strrchr(name, '/');
+    if (last)
+        last++;
+    else
+        last = name;
+
+    f = diradd(dir, FTYPE_REG, last);
+    start = alloc(st.st_size);
+    readn(fd, start, st.st_size);
+    finishfile(f, blockof(start), st.st_size);
+    close(fd);
 }
 
 void
 usage(void)
 {
-	fprintf(stderr, "Usage: fsformat fs.img NBLOCKS files...\n");
-	exit(2);
+    fprintf(stderr, "Usage: fsformat fs.img NBLOCKS files...\n");
+    exit(2);
 }
 
 int
 main(int argc, char **argv)
 {
-	int i;
-	char *s;
-	struct Dir root;
+    int i;
+    char *s;
+    struct Dir root;
 
-	assert(BLKSIZE % sizeof(struct File) == 0);
+    assert(BLKSIZE % sizeof(struct File) == 0);
 
-	if (argc < 3)
-		usage();
+    if (argc < 3)
+        usage();
 
-	nblocks = strtol(argv[2], &s, 0);
-	if (*s || s == argv[2] || nblocks < 2 || nblocks > 1024)
-		usage();
+    nblocks = strtol(argv[2], &s, 0);
+    if (*s || s == argv[2] || nblocks < 2 || nblocks > 1024)
+        usage();
 
-	opendisk(argv[1]);
+    opendisk(argv[1]);
 
-	startdir(&super->s_root, &root);
-	for (i = 3; i < argc; i++)
-		writefile(&root, argv[i]);
-	finishdir(&root);
+    startdir(&super->s_root, &root);
+    for (i = 3; i < argc; i++)
+        writefile(&root, argv[i]);
+    finishdir(&root);
 
-	finishdisk();
-	return 0;
+    finishdisk();
+    return 0;
 }
 
diff --git a/fs/ide.c b/fs/ide.c
index 945cfb0..f76b383 100644
--- a/fs/ide.c
+++ b/fs/ide.c
@@ -17,95 +17,95 @@ static int diskno = 1;
 static int
 ide_wait_ready(bool check_error)
 {
-	int r;
+    int r;
 
-	while (((r = inb(0x1F7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
-		/* do nothing */;
+    while (((r = inb(0x1F7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
+        /* do nothing */;
 
-	if (check_error && (r & (IDE_DF|IDE_ERR)) != 0)
-		return -1;
-	return 0;
+    if (check_error && (r & (IDE_DF|IDE_ERR)) != 0)
+        return -1;
+    return 0;
 }
 
 bool
 ide_probe_disk1(void)
 {
-	int r, x;
+    int r, x;
 
-	// wait for Device 0 to be ready
-	ide_wait_ready(0);
+    // wait for Device 0 to be ready
+    ide_wait_ready(0);
 
-	// switch to Device 1
-	outb(0x1F6, 0xE0 | (1<<4));
+    // switch to Device 1
+    outb(0x1F6, 0xE0 | (1<<4));
 
-	// check for Device 1 to be ready for a while
-	for (x = 0;
-	     x < 1000 && ((r = inb(0x1F7)) & (IDE_BSY|IDE_DF|IDE_ERR)) != 0;
-	     x++)
-		/* do nothing */;
+    // check for Device 1 to be ready for a while
+    for (x = 0;
+         x < 1000 && ((r = inb(0x1F7)) & (IDE_BSY|IDE_DF|IDE_ERR)) != 0;
+         x++)
+        /* do nothing */;
 
-	// switch back to Device 0
-	outb(0x1F6, 0xE0 | (0<<4));
+    // switch back to Device 0
+    outb(0x1F6, 0xE0 | (0<<4));
 
-	cprintf("Device 1 presence: %d\n", (x < 1000));
-	return (x < 1000);
+    cprintf("Device 1 presence: %d\n", (x < 1000));
+    return (x < 1000);
 }
 
 void
 ide_set_disk(int d)
 {
-	if (d != 0 && d != 1)
-		panic("bad disk number");
-	diskno = d;
+    if (d != 0 && d != 1)
+        panic("bad disk number");
+    diskno = d;
 }
 
 int
 ide_read(uint32_t secno, void *dst, size_t nsecs)
 {
-	int r;
+    int r;
 
-	assert(nsecs <= 256);
+    assert(nsecs <= 256);
 
-	ide_wait_ready(0);
+    ide_wait_ready(0);
 
-	outb(0x1F2, nsecs);
-	outb(0x1F3, secno & 0xFF);
-	outb(0x1F4, (secno >> 8) & 0xFF);
-	outb(0x1F5, (secno >> 16) & 0xFF);
-	outb(0x1F6, 0xE0 | ((diskno&1)<<4) | ((secno>>24)&0x0F));
-	outb(0x1F7, 0x20);	// CMD 0x20 means read sector
+    outb(0x1F2, nsecs);
+    outb(0x1F3, secno & 0xFF);
+    outb(0x1F4, (secno >> 8) & 0xFF);
+    outb(0x1F5, (secno >> 16) & 0xFF);
+    outb(0x1F6, 0xE0 | ((diskno&1)<<4) | ((secno>>24)&0x0F));
+    outb(0x1F7, 0x20);	// CMD 0x20 means read sector
 
-	for (; nsecs > 0; nsecs--, dst += SECTSIZE) {
-		if ((r = ide_wait_ready(1)) < 0)
-			return r;
-		insl(0x1F0, dst, SECTSIZE/4);
-	}
+    for (; nsecs > 0; nsecs--, dst += SECTSIZE) {
+        if ((r = ide_wait_ready(1)) < 0)
+            return r;
+        insl(0x1F0, dst, SECTSIZE/4);
+    }
 
-	return 0;
+    return 0;
 }
 
 int
 ide_write(uint32_t secno, const void *src, size_t nsecs)
 {
-	int r;
+    int r;
 
-	assert(nsecs <= 256);
+    assert(nsecs <= 256);
 
-	ide_wait_ready(0);
+    ide_wait_ready(0);
 
-	outb(0x1F2, nsecs);
-	outb(0x1F3, secno & 0xFF);
-	outb(0x1F4, (secno >> 8) & 0xFF);
-	outb(0x1F5, (secno >> 16) & 0xFF);
-	outb(0x1F6, 0xE0 | ((diskno&1)<<4) | ((secno>>24)&0x0F));
-	outb(0x1F7, 0x30);	// CMD 0x30 means write sector
+    outb(0x1F2, nsecs);
+    outb(0x1F3, secno & 0xFF);
+    outb(0x1F4, (secno >> 8) & 0xFF);
+    outb(0x1F5, (secno >> 16) & 0xFF);
+    outb(0x1F6, 0xE0 | ((diskno&1)<<4) | ((secno>>24)&0x0F));
+    outb(0x1F7, 0x30);	// CMD 0x30 means write sector
 
-	for (; nsecs > 0; nsecs--, src += SECTSIZE) {
-		if ((r = ide_wait_ready(1)) < 0)
-			return r;
-		outsl(0x1F0, src, SECTSIZE/4);
-	}
+    for (; nsecs > 0; nsecs--, src += SECTSIZE) {
+        if ((r = ide_wait_ready(1)) < 0)
+            return r;
+        outsl(0x1F0, src, SECTSIZE/4);
+    }
 
-	return 0;
+    return 0;
 }
 
diff --git a/fs/serv.c b/fs/serv.c
index 613f70e..8c6be02 100644
--- a/fs/serv.c
+++ b/fs/serv.c
@@ -30,10 +30,10 @@
 //    file IDs to struct OpenFile.
 
 struct OpenFile {
-	uint32_t o_fileid;	// file id
-	struct File *o_file;	// mapped descriptor for open file
-	int o_mode;		// open mode
-	struct Fd *o_fd;	// Fd page
+    uint32_t o_fileid;	// file id
+    struct File *o_file;	// mapped descriptor for open file
+    int o_mode;		// open mode
+    struct Fd *o_fd;	// Fd page
 };
 
 // Max number of open files in the file system at once
@@ -42,7 +42,7 @@ struct OpenFile {
 
 // initialize to force into data section
 struct OpenFile opentab[MAXOPEN] = {
-	{ 0, 0, 1, 0 }
+    { 0, 0, 1, 0 }
 };
 
 // Virtual address at which to receive page mappings containing client requests.
@@ -51,49 +51,49 @@ union Fsipc *fsreq = (union Fsipc *)0x0ffff000;
 void
 serve_init(void)
 {
-	int i;
-	uintptr_t va = FILEVA;
-	for (i = 0; i < MAXOPEN; i++) {
-		opentab[i].o_fileid = i;
-		opentab[i].o_fd = (struct Fd*) va;
-		va += PGSIZE;
-	}
+    int i;
+    uintptr_t va = FILEVA;
+    for (i = 0; i < MAXOPEN; i++) {
+        opentab[i].o_fileid = i;
+        opentab[i].o_fd = (struct Fd*) va;
+        va += PGSIZE;
+    }
 }
 
 // Allocate an open file.
 int
 openfile_alloc(struct OpenFile **o)
 {
-	int i, r;
-
-	// Find an available open-file table entry
-	for (i = 0; i < MAXOPEN; i++) {
-		switch (pageref(opentab[i].o_fd)) {
-		case 0:
-			if ((r = sys_page_alloc(0, opentab[i].o_fd, PTE_P|PTE_U|PTE_W)) < 0)
-				return r;
-			/* fall through */
-		case 1:
-			opentab[i].o_fileid += MAXOPEN;
-			*o = &opentab[i];
-			memset(opentab[i].o_fd, 0, PGSIZE);
-			return (*o)->o_fileid;
-		}
-	}
-	return -E_MAX_OPEN;
+    int i, r;
+
+    // Find an available open-file table entry
+    for (i = 0; i < MAXOPEN; i++) {
+        switch (pageref(opentab[i].o_fd)) {
+        case 0:
+            if ((r = sys_page_alloc(0, opentab[i].o_fd, PTE_P|PTE_U|PTE_W)) < 0)
+                return r;
+            /* fall through */
+        case 1:
+            opentab[i].o_fileid += MAXOPEN;
+            *o = &opentab[i];
+            memset(opentab[i].o_fd, 0, PGSIZE);
+            return (*o)->o_fileid;
+        }
+    }
+    return -E_MAX_OPEN;
 }
 
 // Look up an open file for envid.
 int
 openfile_lookup(envid_t envid, uint32_t fileid, struct OpenFile **po)
 {
-	struct OpenFile *o;
+    struct OpenFile *o;
 
-	o = &opentab[fileid % MAXOPEN];
-	if (pageref(o->o_fd) == 1 || o->o_fileid != fileid)
-		return -E_INVAL;
-	*po = o;
-	return 0;
+    o = &opentab[fileid % MAXOPEN];
+    if (pageref(o->o_fd) == 1 || o->o_fileid != fileid)
+        return -E_INVAL;
+    *po = o;
+    return 0;
 }
 
 // Open req->req_path in mode req->req_omode, storing the Fd page and
@@ -101,59 +101,59 @@ openfile_lookup(envid_t envid, uint32_t fileid, struct OpenFile **po)
 // *perm_store respectively.
 int
 serve_open(envid_t envid, struct Fsreq_open *req,
-	   void **pg_store, int *perm_store)
+       void **pg_store, int *perm_store)
 {
-	char path[MAXPATHLEN];
-	struct File *f;
-	int fileid;
-	int r;
-	struct OpenFile *o;
-
-	if (debug)
-		cprintf("serve_open %08x %s 0x%x\n", envid, req->req_path, req->req_omode);
-
-	// Copy in the path, making sure it's null-terminated
-	memmove(path, req->req_path, MAXPATHLEN);
-	path[MAXPATHLEN-1] = 0;
-
-	// Find an open file ID
-	if ((r = openfile_alloc(&o)) < 0) {
-		if (debug)
-			cprintf("openfile_alloc failed: %e", r);
-		return r;
-	}
-	fileid = r;
-
-	if (req->req_omode != 0) {
-		if (debug)
-			cprintf("file_open omode 0x%x unsupported", req->req_omode);
-		return -E_INVAL;
-	}
-
-	if ((r = file_open(path, &f)) < 0) {
-		if (debug)
-			cprintf("file_open failed: %e", r);
-		return r;
-	}
-
-	// Save the file pointer
-	o->o_file = f;
-
-	// Fill out the Fd structure
-	o->o_fd->fd_file.id = o->o_fileid;
-	o->o_fd->fd_omode = req->req_omode & O_ACCMODE;
-	o->o_fd->fd_dev_id = devfile.dev_id;
-	o->o_mode = req->req_omode;
-
-	if (debug)
-		cprintf("sending success, page %08x\n", (uintptr_t) o->o_fd);
-
-	// Share the FD page with the caller by setting *pg_store,
-	// store its permission in *perm_store
-	*pg_store = o->o_fd;
-	*perm_store = PTE_P|PTE_U|PTE_W|PTE_SHARE;
-
-	return 0;
+    char path[MAXPATHLEN];
+    struct File *f;
+    int fileid;
+    int r;
+    struct OpenFile *o;
+
+    if (debug)
+        cprintf("serve_open %08x %s 0x%x\n", envid, req->req_path, req->req_omode);
+
+    // Copy in the path, making sure it's null-terminated
+    memmove(path, req->req_path, MAXPATHLEN);
+    path[MAXPATHLEN-1] = 0;
+
+    // Find an open file ID
+    if ((r = openfile_alloc(&o)) < 0) {
+        if (debug)
+            cprintf("openfile_alloc failed: %e", r);
+        return r;
+    }
+    fileid = r;
+
+    if (req->req_omode != 0) {
+        if (debug)
+            cprintf("file_open omode 0x%x unsupported", req->req_omode);
+        return -E_INVAL;
+    }
+
+    if ((r = file_open(path, &f)) < 0) {
+        if (debug)
+            cprintf("file_open failed: %e", r);
+        return r;
+    }
+
+    // Save the file pointer
+    o->o_file = f;
+
+    // Fill out the Fd structure
+    o->o_fd->fd_file.id = o->o_fileid;
+    o->o_fd->fd_omode = req->req_omode & O_ACCMODE;
+    o->o_fd->fd_dev_id = devfile.dev_id;
+    o->o_mode = req->req_omode;
+
+    if (debug)
+        cprintf("sending success, page %08x\n", (uintptr_t) o->o_fd);
+
+    // Share the FD page with the caller by setting *pg_store,
+    // store its permission in *perm_store
+    *pg_store = o->o_fd;
+    *perm_store = PTE_P|PTE_U|PTE_W|PTE_SHARE;
+
+    return 0;
 }
 
 
@@ -164,31 +164,31 @@ serve_open(envid_t envid, struct Fsreq_open *req,
 int
 serve_read(envid_t envid, union Fsipc *ipc)
 {
-	struct Fsreq_read *req = &ipc->read;
-	struct Fsret_read *ret = &ipc->readRet;
-
-	if (debug)
-		cprintf("serve_read %08x %08x %08x\n", envid, req->req_fileid, req->req_n);
-
-	// Look up the file id, read the bytes into 'ret', and update
-	// the seek position.  Be careful if req->req_n > PGSIZE
-	// (remember that read is always allowed to return fewer bytes
-	// than requested).  Also, be careful because ipc is a union,
-	// so filling in ret will overwrite req.
-	//
-	struct OpenFile *o;
-	int r;
-
-	if ((r = openfile_lookup(envid, req->req_fileid, &o)) < 0)
-		return r;
-
-	if ((r = file_read(o->o_file, ret->ret_buf,
-			   MIN(req->req_n, sizeof ret->ret_buf),
-			   o->o_fd->fd_offset)) < 0)
-		return r;
-
-	o->o_fd->fd_offset += r;
-	return r;
+    struct Fsreq_read *req = &ipc->read;
+    struct Fsret_read *ret = &ipc->readRet;
+
+    if (debug)
+        cprintf("serve_read %08x %08x %08x\n", envid, req->req_fileid, req->req_n);
+
+    // Look up the file id, read the bytes into 'ret', and update
+    // the seek position.  Be careful if req->req_n > PGSIZE
+    // (remember that read is always allowed to return fewer bytes
+    // than requested).  Also, be careful because ipc is a union,
+    // so filling in ret will overwrite req.
+    //
+    struct OpenFile *o;
+    int r;
+
+    if ((r = openfile_lookup(envid, req->req_fileid, &o)) < 0)
+        return r;
+
+    if ((r = file_read(o->o_file, ret->ret_buf,
+               MIN(req->req_n, sizeof ret->ret_buf),
+               o->o_fd->fd_offset)) < 0)
+        return r;
+
+    o->o_fd->fd_offset += r;
+    return r;
 }
 
 
@@ -198,21 +198,21 @@ serve_read(envid_t envid, union Fsipc *ipc)
 int
 serve_stat(envid_t envid, union Fsipc *ipc)
 {
-	struct Fsreq_stat *req = &ipc->stat;
-	struct Fsret_stat *ret = &ipc->statRet;
-	struct OpenFile *o;
-	int r;
+    struct Fsreq_stat *req = &ipc->stat;
+    struct Fsret_stat *ret = &ipc->statRet;
+    struct OpenFile *o;
+    int r;
 
-	if (debug)
-		cprintf("serve_stat %08x %08x\n", envid, req->req_fileid);
+    if (debug)
+        cprintf("serve_stat %08x %08x\n", envid, req->req_fileid);
 
-	if ((r = openfile_lookup(envid, req->req_fileid, &o)) < 0)
-		return r;
+    if ((r = openfile_lookup(envid, req->req_fileid, &o)) < 0)
+        return r;
 
-	strcpy(ret->ret_name, o->o_file->f_name);
-	ret->ret_size = o->o_file->f_size;
-	ret->ret_isdir = (o->o_file->f_type == FTYPE_DIR);
-	return 0;
+    strcpy(ret->ret_name, o->o_file->f_name);
+    ret->ret_size = o->o_file->f_size;
+    ret->ret_isdir = (o->o_file->f_type == FTYPE_DIR);
+    return 0;
 }
 
 
@@ -220,68 +220,68 @@ serve_stat(envid_t envid, union Fsipc *ipc)
 int
 serve_flush(envid_t envid, struct Fsreq_flush *req)
 {
-	return 0;
+    return 0;
 }
 
 typedef int (*fshandler)(envid_t envid, union Fsipc *req);
 
 fshandler handlers[] = {
-	// Open is handled specially because it passes pages
-	/* [FSREQ_OPEN] =	(fshandler)serve_open, */
-	[FSREQ_READ] =		serve_read,
-	[FSREQ_STAT] =		serve_stat,
-	[FSREQ_FLUSH] =		(fshandler)serve_flush,
+    // Open is handled specially because it passes pages
+    /* [FSREQ_OPEN] =	(fshandler)serve_open, */
+    [FSREQ_READ] =		serve_read,
+    [FSREQ_STAT] =		serve_stat,
+    [FSREQ_FLUSH] =		(fshandler)serve_flush,
 };
 #define NHANDLERS (sizeof(handlers)/sizeof(handlers[0]))
 
 void
 serve(void)
 {
-	uint32_t req, whom;
-	int perm, r;
-	void *pg;
-
-	while (1) {
-		perm = 0;
-		req = ipc_recv((int32_t *) &whom, fsreq, &perm);
-		if (debug)
-			cprintf("fs req %d from %08x [page %08x: %s]\n",
-				req, whom, uvpt[PGNUM(fsreq)], fsreq);
-
-		// All requests must contain an argument page
-		if (!(perm & PTE_P)) {
-			cprintf("Invalid request from %08x: no argument page\n",
-				whom);
-			continue; // just leave it hanging...
-		}
-
-		pg = NULL;
-		if (req == FSREQ_OPEN) {
-			r = serve_open(whom, (struct Fsreq_open*)fsreq, &pg, &perm);
-		} else if (req < NHANDLERS && handlers[req]) {
-			r = handlers[req](whom, fsreq);
-		} else {
-			cprintf("Invalid request code %d from %08x\n", req, whom);
-			r = -E_INVAL;
-		}
-		ipc_send(whom, r, pg, perm);
-		sys_page_unmap(0, fsreq);
-	}
+    uint32_t req, whom;
+    int perm, r;
+    void *pg;
+
+    while (1) {
+        perm = 0;
+        req = ipc_recv((int32_t *) &whom, fsreq, &perm);
+        if (debug)
+            cprintf("fs req %d from %08x [page %08x: %s]\n",
+                req, whom, uvpt[PGNUM(fsreq)], fsreq);
+
+        // All requests must contain an argument page
+        if (!(perm & PTE_P)) {
+            cprintf("Invalid request from %08x: no argument page\n",
+                whom);
+            continue; // just leave it hanging...
+        }
+
+        pg = NULL;
+        if (req == FSREQ_OPEN) {
+            r = serve_open(whom, (struct Fsreq_open*)fsreq, &pg, &perm);
+        } else if (req < NHANDLERS && handlers[req]) {
+            r = handlers[req](whom, fsreq);
+        } else {
+            cprintf("Invalid request code %d from %08x\n", req, whom);
+            r = -E_INVAL;
+        }
+        ipc_send(whom, r, pg, perm);
+        sys_page_unmap(0, fsreq);
+    }
 }
 
 void
 umain(int argc, char **argv)
 {
-	static_assert(sizeof(struct File) == 256);
-	binaryname = "fs";
-	cprintf("FS is running\n");
+    static_assert(sizeof(struct File) == 256);
+    binaryname = "fs";
+    cprintf("FS is running\n");
 
-	// Check that we are able to do I/O
-	outw(0x8A00, 0x8A00);
-	cprintf("FS can do I/O\n");
+    // Check that we are able to do I/O
+    outw(0x8A00, 0x8A00);
+    cprintf("FS can do I/O\n");
 
-	serve_init();
-	fs_init();
-	serve();
+    serve_init();
+    fs_init();
+    serve();
 }
 
diff --git a/kern/env.c b/kern/env.c
index 036333a..d131ec3 100644
--- a/kern/env.c
+++ b/kern/env.c
@@ -17,7 +17,7 @@
 
 struct Env *envs = NULL;		// All environments
 static struct Env *env_free_list;	// Free environment list
-					// (linked by Env->env_link)
+                    // (linked by Env->env_link)
 
 #define ENVGENSHIFT	12		// >= LOGNENV
 
@@ -38,28 +38,28 @@ static struct Env *env_free_list;	// Free environment list
 //
 struct Segdesc gdt[NCPU + 5] =
 {
-	// 0x0 - unused (always faults -- for trapping NULL far pointers)
-	SEG_NULL,
+    // 0x0 - unused (always faults -- for trapping NULL far pointers)
+    SEG_NULL,
 
-	// 0x8 - kernel code segment
-	[GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),
+    // 0x8 - kernel code segment
+    [GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),
 
-	// 0x10 - kernel data segment
-	[GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),
+    // 0x10 - kernel data segment
+    [GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),
 
-	// 0x18 - user code segment
-	[GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),
+    // 0x18 - user code segment
+    [GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),
 
-	// 0x20 - user data segment
-	[GD_UD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 3),
+    // 0x20 - user data segment
+    [GD_UD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 3),
 
-	// Per-CPU TSS descriptors (starting from GD_TSS0) are initialized
-	// in trap_init_percpu()
-	[GD_TSS0 >> 3] = SEG_NULL
+    // Per-CPU TSS descriptors (starting from GD_TSS0) are initialized
+    // in trap_init_percpu()
+    [GD_TSS0 >> 3] = SEG_NULL
 };
 
 struct Pseudodesc gdt_pd = {
-	sizeof(gdt) - 1, (unsigned long) gdt
+    sizeof(gdt) - 1, (unsigned long) gdt
 };
 
 //
@@ -75,37 +75,37 @@ struct Pseudodesc gdt_pd = {
 int
 envid2env(envid_t envid, struct Env **env_store, bool checkperm)
 {
-	struct Env *e;
-
-	// If envid is zero, return the current environment.
-	if (envid == 0) {
-		*env_store = curenv;
-		return 0;
-	}
-
-	// Look up the Env structure via the index part of the envid,
-	// then check the env_id field in that struct Env
-	// to ensure that the envid is not stale
-	// (i.e., does not refer to a _previous_ environment
-	// that used the same slot in the envs[] array).
-	e = &envs[ENVX(envid)];
-	if (e->env_status == ENV_FREE || e->env_id != envid) {
-		*env_store = 0;
-		return -E_BAD_ENV;
-	}
-
-	// Check that the calling environment has legitimate permission
-	// to manipulate the specified environment.
-	// If checkperm is set, the specified environment
-	// must be either the current environment
-	// or an immediate child of the current environment.
-	if (checkperm && e != curenv && e->env_parent_id != curenv->env_id) {
-		*env_store = 0;
-		return -E_BAD_ENV;
-	}
-
-	*env_store = e;
-	return 0;
+    struct Env *e;
+
+    // If envid is zero, return the current environment.
+    if (envid == 0) {
+        *env_store = curenv;
+        return 0;
+    }
+
+    // Look up the Env structure via the index part of the envid,
+    // then check the env_id field in that struct Env
+    // to ensure that the envid is not stale
+    // (i.e., does not refer to a _previous_ environment
+    // that used the same slot in the envs[] array).
+    e = &envs[ENVX(envid)];
+    if (e->env_status == ENV_FREE || e->env_id != envid) {
+        *env_store = 0;
+        return -E_BAD_ENV;
+    }
+
+    // Check that the calling environment has legitimate permission
+    // to manipulate the specified environment.
+    // If checkperm is set, the specified environment
+    // must be either the current environment
+    // or an immediate child of the current environment.
+    if (checkperm && e != curenv && e->env_parent_id != curenv->env_id) {
+        *env_store = 0;
+        return -E_BAD_ENV;
+    }
+
+    *env_store = e;
+    return 0;
 }
 
 // Mark all environments in 'envs' as free, set their env_ids to 0,
@@ -117,32 +117,40 @@ envid2env(envid_t envid, struct Env **env_store, bool checkperm)
 void
 env_init(void)
 {
-	// Set up envs array
-	// LAB 3: Your code here.
-
-	// Per-CPU part of the initialization
-	env_init_percpu();
+    // Set up envs array
+    // LAB 3: Your code here.
+    for (size_t i=0; i<NENV; ++i) {
+        envs[i].env_id = 0;
+        if (i < NENV - 1) {
+            envs[i].env_link = &envs[i + 1];
+        } else {
+            envs[i].env_link = NULL;
+        }
+    }
+    env_free_list = &envs[0];
+    // Per-CPU part of the initialization
+    env_init_percpu();
 }
 
 // Load GDT and segment descriptors.
 void
 env_init_percpu(void)
 {
-	lgdt(&gdt_pd);
-	// The kernel never uses GS or FS, so we leave those set to
-	// the user data segment.
-	asm volatile("movw %%ax,%%gs" :: "a" (GD_UD|3));
-	asm volatile("movw %%ax,%%fs" :: "a" (GD_UD|3));
-	// The kernel does use ES, DS, and SS.  We'll change between
-	// the kernel and user data segments as needed.
-	asm volatile("movw %%ax,%%es" :: "a" (GD_KD));
-	asm volatile("movw %%ax,%%ds" :: "a" (GD_KD));
-	asm volatile("movw %%ax,%%ss" :: "a" (GD_KD));
-	// Load the kernel text segment into CS.
-	asm volatile("ljmp %0,$1f\n 1:\n" :: "i" (GD_KT));
-	// For good measure, clear the local descriptor table (LDT),
-	// since we don't use it.
-	lldt(0);
+    lgdt(&gdt_pd);
+    // The kernel never uses GS or FS, so we leave those set to
+    // the user data segment.
+    asm volatile("movw %%ax,%%gs" :: "a" (GD_UD|3));
+    asm volatile("movw %%ax,%%fs" :: "a" (GD_UD|3));
+    // The kernel does use ES, DS, and SS.  We'll change between
+    // the kernel and user data segments as needed.
+    asm volatile("movw %%ax,%%es" :: "a" (GD_KD));
+    asm volatile("movw %%ax,%%ds" :: "a" (GD_KD));
+    asm volatile("movw %%ax,%%ss" :: "a" (GD_KD));
+    // Load the kernel text segment into CS.
+    asm volatile("ljmp %0,$1f\n 1:\n" :: "i" (GD_KT));
+    // For good measure, clear the local descriptor table (LDT),
+    // since we don't use it.
+    lldt(0);
 }
 
 //
@@ -158,36 +166,44 @@ env_init_percpu(void)
 static int
 env_setup_vm(struct Env *e)
 {
-	int i;
-	struct PageInfo *p = NULL;
-
-	// Allocate a page for the page directory
-	if (!(p = page_alloc(ALLOC_ZERO)))
-		return -E_NO_MEM;
-
-	// Now, set e->env_pgdir and initialize the page directory.
-	//
-	// Hint:
-	//    - The VA space of all envs is identical above UTOP
-	//	(except at UVPT, which we've set below).
-	//	See inc/memlayout.h for permissions and layout.
-	//	Can you use kern_pgdir as a template?  Hint: Yes.
-	//	(Make sure you got the permissions right in Lab 2.)
-	//    - The initial VA below UTOP is empty.
-	//    - You do not need to make any more calls to page_alloc.
-	//    - Note: In general, pp_ref is not maintained for
-	//	physical pages mapped only above UTOP, but env_pgdir
-	//	is an exception -- you need to increment env_pgdir's
-	//	pp_ref for env_free to work correctly.
-	//    - The functions in kern/pmap.h are handy.
-
-	// LAB 3: Your code here.
-
-	// UVPT maps the env's own page table read-only.
-	// Permissions: kernel R, user R
-	e->env_pgdir[PDX(UVPT)] = PADDR(e->env_pgdir) | PTE_P | PTE_U;
-
-	return 0;
+    int i;
+    struct PageInfo *p = NULL;
+
+    // Allocate a page for the page directory
+    if (!(p = page_alloc(ALLOC_ZERO)))
+        return -E_NO_MEM;
+
+    // Now, set e->env_pgdir and initialize the page directory.
+    //
+    // Hint:
+    //    - The VA space of all envs is identical above UTOP
+    //	(except at UVPT, which we've set below).
+    //	See inc/memlayout.h for permissions and layout.
+    //	Can you use kern_pgdir as a template?  Hint: Yes.
+    //	(Make sure you got the permissions right in Lab 2.)
+    //    - The initial VA below UTOP is empty.
+    //    - You do not need to make any more calls to page_alloc.
+    //    - Note: In general, pp_ref is not maintained for
+    //	physical pages mapped only above UTOP, but env_pgdir
+    //	is an exception -- you need to increment env_pgdir's
+    //	pp_ref for env_free to work correctly.
+    //    - The functions in kern/pmap.h are handy.
+
+    // LAB 3: Your code here.
+    e->env_pgdir = page2kva(p);
+
+    for (i=0; i < PDX(UTOP); ++i) {
+        e->env_pgdir[i] = 0;
+    }
+    for (;i < NPDENTRIES; ++i) {
+        e->env_pgdir[i] = kern_pgdir[i];
+    }
+    p->pp_ref++;
+    // UVPT maps the env's own page table read-only.
+    // Permissions: kernel R, user R
+    e->env_pgdir[PDX(UVPT)] = PADDR(e->env_pgdir) | PTE_P | PTE_U;
+
+    return 0;
 }
 
 //
@@ -201,65 +217,66 @@ env_setup_vm(struct Env *e)
 int
 env_alloc(struct Env **newenv_store, envid_t parent_id)
 {
-	int32_t generation;
-	int r;
-	struct Env *e;
-
-	if (!(e = env_free_list))
-		return -E_NO_FREE_ENV;
-
-	// Allocate and set up the page directory for this environment.
-	if ((r = env_setup_vm(e)) < 0)
-		return r;
-
-	// Generate an env_id for this environment.
-	generation = (e->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
-	if (generation <= 0)	// Don't create a negative env_id.
-		generation = 1 << ENVGENSHIFT;
-	e->env_id = generation | (e - envs);
-
-	// Set the basic status variables.
-	e->env_parent_id = parent_id;
-	e->env_type = ENV_TYPE_USER;
-	e->env_status = ENV_RUNNABLE;
-	e->env_runs = 0;
-
-	// Clear out all the saved register state,
-	// to prevent the register values
-	// of a prior environment inhabiting this Env structure
-	// from "leaking" into our new environment.
-	memset(&e->env_tf, 0, sizeof(e->env_tf));
-
-	// Set up appropriate initial values for the segment registers.
-	// GD_UD is the user data segment selector in the GDT, and
-	// GD_UT is the user text segment selector (see inc/memlayout.h).
-	// The low 2 bits of each segment register contains the
-	// Requestor Privilege Level (RPL); 3 means user mode.  When
-	// we switch privilege levels, the hardware does various
-	// checks involving the RPL and the Descriptor Privilege Level
-	// (DPL) stored in the descriptors themselves.
-	e->env_tf.tf_ds = GD_UD | 3;
-	e->env_tf.tf_es = GD_UD | 3;
-	e->env_tf.tf_ss = GD_UD | 3;
-	e->env_tf.tf_esp = USTACKTOP;
-	e->env_tf.tf_cs = GD_UT | 3;
-	// You will set e->env_tf.tf_eip later.
-
-	// Enable interrupts while in user mode.
-	// LAB 4: Your code here.
-
-	// Clear the page fault handler until user installs one.
-	e->env_pgfault_upcall = 0;
-
-	// Also clear the IPC receiving flag.
-	e->env_ipc_recving = 0;
-
-	// commit the allocation
-	env_free_list = e->env_link;
-	*newenv_store = e;
-
-	// cprintf("[%08x] new env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
-	return 0;
+    int32_t generation;
+    int r;
+    struct Env *e;
+
+    if (!(e = env_free_list))
+        return -E_NO_FREE_ENV;
+
+    // Allocate and set up the page directory for this environment.
+    if ((r = env_setup_vm(e)) < 0)
+        return r;
+
+    // Generate an env_id for this environment.
+    generation = (e->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
+    if (generation <= 0)	// Don't create a negative env_id.
+        generation = 1 << ENVGENSHIFT;
+    e->env_id = generation | (e - envs);
+
+    // Set the basic status variables.
+    e->env_parent_id = parent_id;
+    e->env_type = ENV_TYPE_USER;
+    e->env_status = ENV_RUNNABLE;
+    e->env_runs = 0;
+
+    // Clear out all the saved register state,
+    // to prevent the register values
+    // of a prior environment inhabiting this Env structure
+    // from "leaking" into our new environment.
+    memset(&e->env_tf, 0, sizeof(e->env_tf));
+
+    // Set up appropriate initial values for the segment registers.
+    // GD_UD is the user data segment selector in the GDT, and
+    // GD_UT is the user text segment selector (see inc/memlayout.h).
+    // The low 2 bits of each segment register contains the
+    // Requestor Privilege Level (RPL); 3 means user mode.  When
+    // we switch privilege levels, the hardware does various
+    // checks involving the RPL and the Descriptor Privilege Level
+    // (DPL) stored in the descriptors themselves.
+    e->env_tf.tf_ds = GD_UD | 3;
+    e->env_tf.tf_es = GD_UD | 3;
+    e->env_tf.tf_ss = GD_UD | 3;
+    e->env_tf.tf_esp = USTACKTOP;
+    e->env_tf.tf_cs = GD_UT | 3;
+    // You will set e->env_tf.tf_eip later.
+
+    // Enable interrupts while in user mode.
+    // LAB 4: Your code here.
+    e->env_tf.tf_eflags |= FL_IF;
+
+    // Clear the page fault handler until user installs one.
+    e->env_pgfault_upcall = 0;
+
+    // Also clear the IPC receiving flag.
+    e->env_ipc_recving = 0;
+
+    // commit the allocation
+    env_free_list = e->env_link;
+    *newenv_store = e;
+
+    // cprintf("[%08x] new env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
+    return 0;
 }
 
 //
@@ -272,13 +289,26 @@ env_alloc(struct Env **newenv_store, envid_t parent_id)
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
+    // LAB 3: Your code here.
+    // (But only if you need it for load_icode.)
+    //
+    // Hint: It is easier to use region_alloc if the caller can pass
+    //   'va' and 'len' values that are not page-aligned.
+    //   You should round va down, and round (va + len) up.
+    //   (Watch out for corner-cases!)
+    uintptr_t addr = ROUNDDOWN((uintptr_t)va, PGSIZE);
+    uintptr_t end = ROUNDUP((uintptr_t)va + len, PGSIZE);
+
+    for (;addr < end; addr+= PGSIZE) {
+        struct PageInfo* page = page_alloc(0);
+        if (!page) {
+            panic("error in region_alloc");
+        }
+        int r = page_insert(e->env_pgdir, page, (void*) addr, PTE_P | PTE_U | PTE_W);
+        if (r != 0) {
+            panic("error in region_alloc");
+        }
+    }
 }
 
 //
@@ -306,40 +336,63 @@ region_alloc(struct Env *e, void *va, size_t len)
 static void
 load_icode(struct Env *e, uint8_t *binary, size_t size)
 {
-	// Hints:
-	//  Load each program segment into virtual memory
-	//  at the address specified in the ELF section header.
-	//  You should only load segments with ph->p_type == ELF_PROG_LOAD.
-	//  Each segment's virtual address can be found in ph->p_va
-	//  and its size in memory can be found in ph->p_memsz.
-	//  The ph->p_filesz bytes from the ELF binary, starting at
-	//  'binary + ph->p_offset', should be copied to virtual address
-	//  ph->p_va.  Any remaining memory bytes should be cleared to zero.
-	//  (The ELF header should have ph->p_filesz <= ph->p_memsz.)
-	//  Use functions from the previous lab to allocate and map pages.
-	//
-	//  All page protection bits should be user read/write for now.
-	//  ELF segments are not necessarily page-aligned, but you can
-	//  assume for this function that no two segments will touch
-	//  the same virtual page.
-	//
-	//  You may find a function like region_alloc useful.
-	//
-	//  Loading the segments is much simpler if you can move data
-	//  directly into the virtual addresses stored in the ELF binary.
-	//  So which page directory should be in force during
-	//  this function?
-	//
-	//  You must also do something with the program's entry point,
-	//  to make sure that the environment starts executing there.
-	//  What?  (See env_run() and env_pop_tf() below.)
-
-	// LAB 3: Your code here.
-
-	// Now map one page for the program's initial stack
-	// at virtual address USTACKTOP - PGSIZE.
-
-	// LAB 3: Your code here.
+    // Hints:
+    //  Load each program segment into virtual memory
+    //  at the address specified in the ELF section header.
+    //  You should only load segments with ph->p_type == ELF_PROG_LOAD.
+    //  Each segment's virtual address can be found in ph->p_va
+    //  and its size in memory can be found in ph->p_memsz.
+    //  The ph->p_filesz bytes from the ELF binary, starting at
+    //  'binary + ph->p_offset', should be copied to virtual address
+    //  ph->p_va.  Any remaining memory bytes should be cleared to zero.
+    //  (The ELF header should have ph->p_filesz <= ph->p_memsz.)
+    //  Use functions from the previous lab to allocate and map pages.
+    //
+    //  All page protection bits should be user read/write for now.
+    //  ELF segments are not necessarily page-aligned, but you can
+    //  assume for this function that no two segments will touch
+    //  the same virtual page.
+    //
+    //  You may find a function like region_alloc useful.
+    //
+    //  Loading the segments is much simpler if you can move data
+    //  directly into the virtual addresses stored in the ELF binary.
+    //  So which page directory should be in force during
+    //  this function?
+    //
+    //  You must also do something with the program's entry point,
+    //  to make sure that the environment starts executing there.
+    //  What?  (See env_run() and env_pop_tf() below.)
+
+    // LAB 3: Your code here.
+    struct Elf *elfhdr = (struct Elf*) binary;
+
+    if (elfhdr->e_magic != ELF_MAGIC) {
+        panic("bad elf");
+    }
+    struct Proghdr *ph = (struct Proghdr*) (binary + elfhdr->e_phoff);
+    struct Proghdr *eph = ph + elfhdr->e_phnum;
+
+    lcr3(PADDR(e->env_pgdir));
+    for(;ph < eph; ++ph) {
+        if (ph->p_type != ELF_PROG_LOAD) {
+            continue;
+        }
+        if (ph->p_filesz > ph->p_memsz) {
+            panic("size of file larger than size in memory in `load_icode`");
+        }
+        region_alloc(e, (void *) ph->p_va, ph->p_memsz);
+        memcpy((void *)ph->p_va, binary + ph->p_offset, ph->p_filesz);
+        memset((void *)(ph->p_va + ph->p_filesz), 0, ph->p_memsz - ph->p_filesz);
+    }
+    lcr3(PADDR(kern_pgdir));
+    e->env_tf.tf_eip = elfhdr->e_entry;
+
+    // Now map one page for the program's initial stack
+    // at virtual address USTACKTOP - PGSIZE.
+
+    // LAB 3: Your code here.
+    region_alloc(e, (void*) (USTACKTOP - PGSIZE), PGSIZE);
 }
 
 //
@@ -352,10 +405,19 @@ load_icode(struct Env *e, uint8_t *binary, size_t size)
 void
 env_create(uint8_t *binary, size_t size, enum EnvType type)
 {
-	// LAB 3: Your code here.
-
-	// If this is the file server (type == ENV_TYPE_FS) give it I/O privileges.
-	// LAB 5: Your code here.
+    // LAB 3: Your code here.
+    struct Env *e;
+    int r;
+    if ((r = env_alloc(&e, 0)) < 0) {
+        panic("env_create: %e", r);
+    }
+    load_icode(e, binary, size);
+    e->env_type = type;
+    // If this is the file server (type == ENV_TYPE_FS) give it I/O privileges.
+    // LAB 5: Your code here.
+    if (type == ENV_TYPE_FS) {
+        e->env_tf.tf_eflags |= FL_IOPL_3;
+    }
 }
 
 //
@@ -364,51 +426,51 @@ env_create(uint8_t *binary, size_t size, enum EnvType type)
 void
 env_free(struct Env *e)
 {
-	pte_t *pt;
-	uint32_t pdeno, pteno;
-	physaddr_t pa;
-
-	// If freeing the current environment, switch to kern_pgdir
-	// before freeing the page directory, just in case the page
-	// gets reused.
-	if (e == curenv)
-		lcr3(PADDR(kern_pgdir));
-
-	// Note the environment's demise.
-	// cprintf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
-
-	// Flush all mapped pages in the user portion of the address space
-	static_assert(UTOP % PTSIZE == 0);
-	for (pdeno = 0; pdeno < PDX(UTOP); pdeno++) {
-
-		// only look at mapped page tables
-		if (!(e->env_pgdir[pdeno] & PTE_P))
-			continue;
-
-		// find the pa and va of the page table
-		pa = PTE_ADDR(e->env_pgdir[pdeno]);
-		pt = (pte_t*) KADDR(pa);
-
-		// unmap all PTEs in this page table
-		for (pteno = 0; pteno <= PTX(~0); pteno++) {
-			if (pt[pteno] & PTE_P)
-				page_remove(e->env_pgdir, PGADDR(pdeno, pteno, 0));
-		}
-
-		// free the page table itself
-		e->env_pgdir[pdeno] = 0;
-		page_decref(pa2page(pa));
-	}
-
-	// free the page directory
-	pa = PADDR(e->env_pgdir);
-	e->env_pgdir = 0;
-	page_decref(pa2page(pa));
-
-	// return the environment to the free list
-	e->env_status = ENV_FREE;
-	e->env_link = env_free_list;
-	env_free_list = e;
+    pte_t *pt;
+    uint32_t pdeno, pteno;
+    physaddr_t pa;
+
+    // If freeing the current environment, switch to kern_pgdir
+    // before freeing the page directory, just in case the page
+    // gets reused.
+    if (e == curenv)
+        lcr3(PADDR(kern_pgdir));
+
+    // Note the environment's demise.
+    cprintf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
+
+    // Flush all mapped pages in the user portion of the address space
+    static_assert(UTOP % PTSIZE == 0);
+    for (pdeno = 0; pdeno < PDX(UTOP); pdeno++) {
+
+        // only look at mapped page tables
+        if (!(e->env_pgdir[pdeno] & PTE_P))
+            continue;
+
+        // find the pa and va of the page table
+        pa = PTE_ADDR(e->env_pgdir[pdeno]);
+        pt = (pte_t*) KADDR(pa);
+
+        // unmap all PTEs in this page table
+        for (pteno = 0; pteno <= PTX(~0); pteno++) {
+            if (pt[pteno] & PTE_P)
+                page_remove(e->env_pgdir, PGADDR(pdeno, pteno, 0));
+        }
+
+        // free the page table itself
+        e->env_pgdir[pdeno] = 0;
+        page_decref(pa2page(pa));
+    }
+
+    // free the page directory
+    pa = PADDR(e->env_pgdir);
+    e->env_pgdir = 0;
+    page_decref(pa2page(pa));
+
+    // return the environment to the free list
+    e->env_status = ENV_FREE;
+    e->env_link = env_free_list;
+    env_free_list = e;
 }
 
 //
@@ -419,20 +481,20 @@ env_free(struct Env *e)
 void
 env_destroy(struct Env *e)
 {
-	// If e is currently running on other CPUs, we change its state to
-	// ENV_DYING. A zombie environment will be freed the next time
-	// it traps to the kernel.
-	if (e->env_status == ENV_RUNNING && curenv != e) {
-		e->env_status = ENV_DYING;
-		return;
-	}
-
-	env_free(e);
-
-	if (curenv == e) {
-		curenv = NULL;
-		sched_yield();
-	}
+    // If e is currently running on other CPUs, we change its state to
+    // ENV_DYING. A zombie environment will be freed the next time
+    // it traps to the kernel.
+    if (e->env_status == ENV_RUNNING && curenv != e) {
+        e->env_status = ENV_DYING;
+        return;
+    }
+
+    env_free(e);
+
+    if (curenv == e) {
+        curenv = NULL;
+        sched_yield();
+    }
 }
 
 
@@ -445,17 +507,17 @@ env_destroy(struct Env *e)
 void
 env_pop_tf(struct Trapframe *tf)
 {
-	// Record the CPU we are running on for user-space debugging
-	curenv->env_cpunum = cpunum();
-
-	__asm __volatile("movl %0,%%esp\n"
-		"\tpopal\n"
-		"\tpopl %%es\n"
-		"\tpopl %%ds\n"
-		"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
-		"\tiret"
-		: : "g" (tf) : "memory");
-	panic("iret failed");  /* mostly to placate the compiler */
+    // Record the CPU we are running on for user-space debugging
+    curenv->env_cpunum = cpunum();
+
+    __asm __volatile("movl %0,%%esp\n"
+        "\tpopal\n"
+        "\tpopl %%es\n"
+        "\tpopl %%ds\n"
+        "\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
+        "\tiret"
+        : : "g" (tf) : "memory");
+    panic("iret failed");  /* mostly to placate the compiler */
 }
 
 //
@@ -467,25 +529,34 @@ env_pop_tf(struct Trapframe *tf)
 void
 env_run(struct Env *e)
 {
-	// Step 1: If this is a context switch (a new environment is running):
-	//	   1. Set the current environment (if any) back to
-	//	      ENV_RUNNABLE if it is ENV_RUNNING (think about
-	//	      what other states it can be in),
-	//	   2. Set 'curenv' to the new environment,
-	//	   3. Set its status to ENV_RUNNING,
-	//	   4. Update its 'env_runs' counter,
-	//	   5. Use lcr3() to switch to its address space.
-	// Step 2: Use env_pop_tf() to restore the environment's
-	//	   registers and drop into user mode in the
-	//	   environment.
-
-	// Hint: This function loads the new environment's state from
-	//	e->env_tf.  Go back through the code you wrote above
-	//	and make sure you have set the relevant parts of
-	//	e->env_tf to sensible values.
-
-	// LAB 3: Your code here.
-
-	panic("env_run not yet implemented");
+    // Step 1: If this is a context switch (a new environment is running):
+    //	   1. Set the current environment (if any) back to
+    //	      ENV_RUNNABLE if it is ENV_RUNNING (think about
+    //	      what other states it can be in),
+    //	   2. Set 'curenv' to the new environment,
+    //	   3. Set its status to ENV_RUNNING,
+    //	   4. Update its 'env_runs' counter,
+    //	   5. Use lcr3() to switch to its address space.
+    // Step 2: Use env_pop_tf() to restore the environment's
+    //	   registers and drop into user mode in the
+    //	   environment.
+
+    // Hint: This function loads the new environment's state from
+    //	e->env_tf.  Go back through the code you wrote above
+    //	and make sure you have set the relevant parts of
+    //	e->env_tf to sensible values.
+
+    // LAB 3: Your code here.
+    if (curenv && curenv->env_status == ENV_RUNNING) {
+        curenv->env_status = ENV_RUNNABLE;
+    }
+    curenv = e;
+    curenv->env_status = ENV_RUNNING;
+    curenv->env_runs++;
+    lcr3(PADDR(curenv->env_pgdir));
+
+    unlock_kernel();
+
+    env_pop_tf(&curenv->env_tf);
 }
 
diff --git a/kern/init.c b/kern/init.c
index c2817ad..ccd1005 100644
--- a/kern/init.c
+++ b/kern/init.c
@@ -21,55 +21,56 @@ static void boot_aps(void);
 void
 i386_init(void)
 {
-	extern char edata[], end[];
+    extern char edata[], end[];
 
-	// Before doing anything else, complete the ELF loading process.
-	// Clear the uninitialized global data (BSS) section of our program.
-	// This ensures that all static/global variables start out zero.
-	memset(edata, 0, end - edata);
+    // Before doing anything else, complete the ELF loading process.
+    // Clear the uninitialized global data (BSS) section of our program.
+    // This ensures that all static/global variables start out zero.
+    memset(edata, 0, end - edata);
 
-	// Initialize the console.
-	// Can't call cprintf until after we do this!
-	cons_init();
+    // Initialize the console.
+    // Can't call cprintf until after we do this!
+    cons_init();
 
-	cprintf("6828 decimal is %o octal!\n", 6828);
+    cprintf("6828 decimal is %o octal!\n", 6828);
 
-	// Lab 2 memory management initialization functions
-	mem_init();
+    // Lab 2 memory management initialization functions
+    mem_init();
 
-	// Lab 3 user environment initialization functions
-	env_init();
-	trap_init();
+    // Lab 3 user environment initialization functions
+    env_init();
+    trap_init();
 
-	// Lab 4 multiprocessor initialization functions
-	mp_init();
-	lapic_init();
+    // Lab 4 multiprocessor initialization functions
+    mp_init();
+    lapic_init();
 
-	// Lab 4 multitasking initialization functions
-	pic_init();
+    // Lab 4 multitasking initialization functions
+    pic_init();
 
-	// Acquire the big kernel lock before waking up APs
-	// Your code here:
+    // Acquire the big kernel lock before waking up APs
+    // Your code here:
+    lock_kernel();
 
-	// Starting non-boot CPUs
-	boot_aps();
+    // Starting non-boot CPUs
+    boot_aps();
 
-	// Start fs.
-	ENV_CREATE(fs_fs, ENV_TYPE_FS);
+    // Start fs.
+    ENV_CREATE(fs_fs, ENV_TYPE_FS);
 
 #if defined(TEST)
-	// Don't touch -- used by grading script!
-	ENV_CREATE(TEST, ENV_TYPE_USER);
+    // Don't touch -- used by grading script!
+    ENV_CREATE(TEST, ENV_TYPE_USER);
 #else
-	// Touch all you want.
-	ENV_CREATE(user_icode, ENV_TYPE_USER);
+    // Touch all you want.
+    ENV_CREATE(user_icode, ENV_TYPE_USER);
 #endif // TEST*
 
-	// Should not be necessary - drains keyboard because interrupt has given up.
-	kbd_intr();
+    // Should not be necessary - drains keyboard because interrupt has given up.
+    kbd_intr();
 
-	// Schedule and run the first user environment!
-	sched_yield();
+    // Schedule and run the first user environment!
+    sched_yield();
 }
 
 // While boot_aps is booting a given CPU, it communicates the per-core
@@ -81,50 +82,51 @@ void *mpentry_kstack;
 static void
 boot_aps(void)
 {
-	extern unsigned char mpentry_start[], mpentry_end[];
-	void *code;
-	struct CpuInfo *c;
-
-	// Write entry code to unused memory at MPENTRY_PADDR
-	code = KADDR(MPENTRY_PADDR);
-	memmove(code, mpentry_start, mpentry_end - mpentry_start);
-
-	// Boot each AP one at a time
-	for (c = cpus; c < cpus + ncpu; c++) {
-		if (c == cpus + cpunum())  // We've started already.
-			continue;
-
-		// Tell mpentry.S what stack to use 
-		mpentry_kstack = percpu_kstacks[c - cpus] + KSTKSIZE;
-		// Start the CPU at mpentry_start
-		lapic_startap(c->cpu_id, PADDR(code));
-		// Wait for the CPU to finish some basic setup in mp_main()
-		while(c->cpu_status != CPU_STARTED)
-			;
-	}
+    extern unsigned char mpentry_start[], mpentry_end[];
+    void *code;
+    struct CpuInfo *c;
+
+    // Write entry code to unused memory at MPENTRY_PADDR
+    code = KADDR(MPENTRY_PADDR);
+    memmove(code, mpentry_start, mpentry_end - mpentry_start);
+
+    // Boot each AP one at a time
+    for (c = cpus; c < cpus + ncpu; c++) {
+        if (c == cpus + cpunum())  // We've started already.
+            continue;
+
+        // Tell mpentry.S what stack to use
+        mpentry_kstack = percpu_kstacks[c - cpus] + KSTKSIZE;
+        // Start the CPU at mpentry_start
+        lapic_startap(c->cpu_id, PADDR(code));
+        // Wait for the CPU to finish some basic setup in mp_main()
+        while(c->cpu_status != CPU_STARTED)
+            ;
+    }
 }
 
 // Setup code for APs
 void
 mp_main(void)
 {
-	// We are in high EIP now, safe to switch to kern_pgdir 
-	lcr3(PADDR(kern_pgdir));
-	cprintf("SMP: CPU %d starting\n", cpunum());
-
-	lapic_init();
-	env_init_percpu();
-	trap_init_percpu();
-	xchg(&thiscpu->cpu_status, CPU_STARTED); // tell boot_aps() we're up
-
-	// Now that we have finished some basic setup, call sched_yield()
-	// to start running processes on this CPU.  But make sure that
-	// only one CPU can enter the scheduler at a time!
-	//
-	// Your code here:
-
-	// Remove this after you finish Exercise 4
-	for (;;);
+    // We are in high EIP now, safe to switch to kern_pgdir
+    lcr3(PADDR(kern_pgdir));
+    cprintf("SMP: CPU %d starting\n", cpunum());
+
+    lapic_init();
+    env_init_percpu();
+    trap_init_percpu();
+    xchg(&thiscpu->cpu_status, CPU_STARTED); // tell boot_aps() we're up
+
+    // Now that we have finished some basic setup, call sched_yield()
+    // to start running processes on this CPU.  But make sure that
+    // only one CPU can enter the scheduler at a time!
+    //
+    // Your code here:
+    lock_kernel();
+    sched_yield();
+    // Remove this after you finish Exercise 4
+    for (;;);
 }
 
 /*
@@ -140,36 +142,36 @@ const char *panicstr;
 void
 _panic(const char *file, int line, const char *fmt,...)
 {
-	va_list ap;
+    va_list ap;
 
-	if (panicstr)
-		goto dead;
-	panicstr = fmt;
+    if (panicstr)
+        goto dead;
+    panicstr = fmt;
 
-	// Be extra sure that the machine is in as reasonable state
-	__asm __volatile("cli; cld");
+    // Be extra sure that the machine is in as reasonable state
+    __asm __volatile("cli; cld");
 
-	va_start(ap, fmt);
-	cprintf("kernel panic on CPU %d at %s:%d: ", cpunum(), file, line);
-	vcprintf(fmt, ap);
-	cprintf("\n");
-	va_end(ap);
+    va_start(ap, fmt);
+    cprintf("kernel panic on CPU %d at %s:%d: ", cpunum(), file, line);
+    vcprintf(fmt, ap);
+    cprintf("\n");
+    va_end(ap);
 
 dead:
-	/* break into the kernel monitor */
-	while (1)
-		monitor(NULL);
+    /* break into the kernel monitor */
+    while (1)
+        monitor(NULL);
 }
 
 /* like panic, but don't */
 void
 _warn(const char *file, int line, const char *fmt,...)
 {
-	va_list ap;
+    va_list ap;
 
-	va_start(ap, fmt);
-	cprintf("kernel warning at %s:%d: ", file, line);
-	vcprintf(fmt, ap);
-	cprintf("\n");
-	va_end(ap);
+    va_start(ap, fmt);
+    cprintf("kernel warning at %s:%d: ", file, line);
+    vcprintf(fmt, ap);
+    cprintf("\n");
+    va_end(ap);
 }
diff --git a/kern/kdebug.c b/kern/kdebug.c
index f4ee8ee..ceed63f 100644
--- a/kern/kdebug.c
+++ b/kern/kdebug.c
@@ -13,10 +13,10 @@ extern const char __STABSTR_BEGIN__[];		// Beginning of string table
 extern const char __STABSTR_END__[];		// End of string table
 
 struct UserStabData {
-	const struct Stab *stabs;
-	const struct Stab *stab_end;
-	const char *stabstr;
-	const char *stabstr_end;
+    const struct Stab *stabs;
+    const struct Stab *stab_end;
+    const char *stabstr;
+    const char *stabstr_end;
 };
 
 
@@ -58,48 +58,48 @@ struct UserStabData {
 //
 static void
 stab_binsearch(const struct Stab *stabs, int *region_left, int *region_right,
-	       int type, uintptr_t addr)
+           int type, uintptr_t addr)
 {
-	int l = *region_left, r = *region_right, any_matches = 0;
-
-	while (l <= r) {
-		int true_m = (l + r) / 2, m = true_m;
-
-		// search for earliest stab with right type
-		while (m >= l && stabs[m].n_type != type)
-			m--;
-		if (m < l) {	// no match in [l, m]
-			l = true_m + 1;
-			continue;
-		}
-
-		// actual binary search
-		any_matches = 1;
-		if (stabs[m].n_value < addr) {
-			*region_left = m;
-			l = true_m + 1;
-		} else if (stabs[m].n_value > addr) {
-			*region_right = m - 1;
-			r = m - 1;
-		} else {
-			// exact match for 'addr', but continue loop to find
-			// *region_right
-			*region_left = m;
-			l = m;
-			addr++;
-		}
-	}
-
-	if (!any_matches)
-		*region_right = *region_left - 1;
-	else {
-		// find rightmost region containing 'addr'
-		for (l = *region_right;
-		     l > *region_left && stabs[l].n_type != type;
-		     l--)
-			/* do nothing */;
-		*region_left = l;
-	}
+    int l = *region_left, r = *region_right, any_matches = 0;
+
+    while (l <= r) {
+        int true_m = (l + r) / 2, m = true_m;
+
+        // search for earliest stab with right type
+        while (m >= l && stabs[m].n_type != type)
+            m--;
+        if (m < l) {	// no match in [l, m]
+            l = true_m + 1;
+            continue;
+        }
+
+        // actual binary search
+        any_matches = 1;
+        if (stabs[m].n_value < addr) {
+            *region_left = m;
+            l = true_m + 1;
+        } else if (stabs[m].n_value > addr) {
+            *region_right = m - 1;
+            r = m - 1;
+        } else {
+            // exact match for 'addr', but continue loop to find
+            // *region_right
+            *region_left = m;
+            l = m;
+            addr++;
+        }
+    }
+
+    if (!any_matches)
+        *region_right = *region_left - 1;
+    else {
+        // find rightmost region containing 'addr'
+        for (l = *region_right;
+             l > *region_left && stabs[l].n_type != type;
+             l--)
+            /* do nothing */;
+        *region_left = l;
+    }
 }
 
 
@@ -113,119 +113,128 @@ stab_binsearch(const struct Stab *stabs, int *region_left, int *region_right,
 int
 debuginfo_eip(uintptr_t addr, struct Eipdebuginfo *info)
 {
-	const struct Stab *stabs, *stab_end;
-	const char *stabstr, *stabstr_end;
-	int lfile, rfile, lfun, rfun, lline, rline;
-
-	// Initialize *info
-	info->eip_file = "<unknown>";
-	info->eip_line = 0;
-	info->eip_fn_name = "<unknown>";
-	info->eip_fn_namelen = 9;
-	info->eip_fn_addr = addr;
-	info->eip_fn_narg = 0;
-
-	// Find the relevant set of stabs
-	if (addr >= ULIM) {
-		stabs = __STAB_BEGIN__;
-		stab_end = __STAB_END__;
-		stabstr = __STABSTR_BEGIN__;
-		stabstr_end = __STABSTR_END__;
-	} else {
-		// The user-application linker script, user/user.ld,
-		// puts information about the application's stabs (equivalent
-		// to __STAB_BEGIN__, __STAB_END__, __STABSTR_BEGIN__, and
-		// __STABSTR_END__) in a structure located at virtual address
-		// USTABDATA.
-		const struct UserStabData *usd = (const struct UserStabData *) USTABDATA;
-
-		// Make sure this memory is valid.
-		// Return -1 if it is not.  Hint: Call user_mem_check.
-		// LAB 3: Your code here.
-
-		stabs = usd->stabs;
-		stab_end = usd->stab_end;
-		stabstr = usd->stabstr;
-		stabstr_end = usd->stabstr_end;
-
-		// Make sure the STABS and string table memory is valid.
-		// LAB 3: Your code here.
-	}
-
-	// String table validity checks
-	if (stabstr_end <= stabstr || stabstr_end[-1] != 0)
-		return -1;
-
-	// Now we find the right stabs that define the function containing
-	// 'eip'.  First, we find the basic source file containing 'eip'.
-	// Then, we look in that source file for the function.  Then we look
-	// for the line number.
-
-	// Search the entire set of stabs for the source file (type N_SO).
-	lfile = 0;
-	rfile = (stab_end - stabs) - 1;
-	stab_binsearch(stabs, &lfile, &rfile, N_SO, addr);
-	if (lfile == 0)
-		return -1;
-
-	// Search within that file's stabs for the function definition
-	// (N_FUN).
-	lfun = lfile;
-	rfun = rfile;
-	stab_binsearch(stabs, &lfun, &rfun, N_FUN, addr);
-
-	if (lfun <= rfun) {
-		// stabs[lfun] points to the function name
-		// in the string table, but check bounds just in case.
-		if (stabs[lfun].n_strx < stabstr_end - stabstr)
-			info->eip_fn_name = stabstr + stabs[lfun].n_strx;
-		info->eip_fn_addr = stabs[lfun].n_value;
-		addr -= info->eip_fn_addr;
-		// Search within the function definition for the line number.
-		lline = lfun;
-		rline = rfun;
-	} else {
-		// Couldn't find function stab!  Maybe we're in an assembly
-		// file.  Search the whole file for the line number.
-		info->eip_fn_addr = addr;
-		lline = lfile;
-		rline = rfile;
-	}
-	// Ignore stuff after the colon.
-	info->eip_fn_namelen = strfind(info->eip_fn_name, ':') - info->eip_fn_name;
-
-
-	// Search within [lline, rline] for the line number stab.
-	// If found, set info->eip_line to the right line number.
-	// If not found, return -1.
-	//
-	// Hint:
-	//	There's a particular stabs type used for line numbers.
-	//	Look at the STABS documentation and <inc/stab.h> to find
-	//	which one.
-	// Your code here.
-
-
-	// Search backwards from the line number for the relevant filename
-	// stab.
-	// We can't just use the "lfile" stab because inlined functions
-	// can interpolate code from a different file!
-	// Such included source files use the N_SOL stab type.
-	while (lline >= lfile
-	       && stabs[lline].n_type != N_SOL
-	       && (stabs[lline].n_type != N_SO || !stabs[lline].n_value))
-		lline--;
-	if (lline >= lfile && stabs[lline].n_strx < stabstr_end - stabstr)
-		info->eip_file = stabstr + stabs[lline].n_strx;
-
-
-	// Set eip_fn_narg to the number of arguments taken by the function,
-	// or 0 if there was no containing function.
-	if (lfun < rfun)
-		for (lline = lfun + 1;
-		     lline < rfun && stabs[lline].n_type == N_PSYM;
-		     lline++)
-			info->eip_fn_narg++;
-
-	return 0;
+    const struct Stab *stabs, *stab_end;
+    const char *stabstr, *stabstr_end;
+    int lfile, rfile, lfun, rfun, lline, rline;
+
+    // Initialize *info
+    info->eip_file = "<unknown>";
+    info->eip_line = 0;
+    info->eip_fn_name = "<unknown>";
+    info->eip_fn_namelen = 9;
+    info->eip_fn_addr = addr;
+    info->eip_fn_narg = 0;
+
+    // Find the relevant set of stabs
+    if (addr >= ULIM) {
+        stabs = __STAB_BEGIN__;
+        stab_end = __STAB_END__;
+        stabstr = __STABSTR_BEGIN__;
+        stabstr_end = __STABSTR_END__;
+    } else {
+        // The user-application linker script, user/user.ld,
+        // puts information about the application's stabs (equivalent
+        // to __STAB_BEGIN__, __STAB_END__, __STABSTR_BEGIN__, and
+        // __STABSTR_END__) in a structure located at virtual address
+        // USTABDATA.
+        const struct UserStabData *usd = (const struct UserStabData *) USTABDATA;
+
+        // Make sure this memory is valid.
+        // Return -1 if it is not.  Hint: Call user_mem_check.
+        // LAB 3: Your code here.
+        if (user_mem_check(curenv, usd, sizeof(struct UserStabData), PTE_U) != 0) {
+            return -1;
+        }
+
+        stabs = usd->stabs;
+        stab_end = usd->stab_end;
+        stabstr = usd->stabstr;
+        stabstr_end = usd->stabstr_end;
+
+        // Make sure the STABS and string table memory is valid.
+        // LAB 3: Your code here.
+        if (user_mem_check(curenv, stabs, sizeof(struct Stab), PTE_U) != 0) {
+            return -1;
+        }
+        if (user_mem_check(curenv, stabstr, stabstr_end - stabstr, PTE_U) != 0) {
+            return -1;
+        }
+    }
+
+    // String table validity checks
+    if (stabstr_end <= stabstr || stabstr_end[-1] != 0)
+        return -1;
+
+    // Now we find the right stabs that define the function containing
+    // 'eip'.  First, we find the basic source file containing 'eip'.
+    // Then, we look in that source file for the function.  Then we look
+    // for the line number.
+
+    // Search the entire set of stabs for the source file (type N_SO).
+    lfile = 0;
+    rfile = (stab_end - stabs) - 1;
+    stab_binsearch(stabs, &lfile, &rfile, N_SO, addr);
+    if (lfile == 0)
+        return -1;
+
+    // Search within that file's stabs for the function definition
+    // (N_FUN).
+    lfun = lfile;
+    rfun = rfile;
+    stab_binsearch(stabs, &lfun, &rfun, N_FUN, addr);
+
+    if (lfun <= rfun) {
+        // stabs[lfun] points to the function name
+        // in the string table, but check bounds just in case.
+        if (stabs[lfun].n_strx < stabstr_end - stabstr)
+            info->eip_fn_name = stabstr + stabs[lfun].n_strx;
+        info->eip_fn_addr = stabs[lfun].n_value;
+        addr -= info->eip_fn_addr;
+        // Search within the function definition for the line number.
+        lline = lfun;
+        rline = rfun;
+    } else {
+        // Couldn't find function stab!  Maybe we're in an assembly
+        // file.  Search the whole file for the line number.
+        info->eip_fn_addr = addr;
+        lline = lfile;
+        rline = rfile;
+    }
+    // Ignore stuff after the colon.
+    info->eip_fn_namelen = strfind(info->eip_fn_name, ':') - info->eip_fn_name;
+
+
+    // Search within [lline, rline] for the line number stab.
+    // If found, set info->eip_line to the right line number.
+    // If not found, return -1.
+    //
+    // Hint:
+    //	There's a particular stabs type used for line numbers.
+    //	Look at the STABS documentation and <inc/stab.h> to find
+    //	which one.
+    // Your code here.
+
+
+    // Search backwards from the line number for the relevant filename
+    // stab.
+    // We can't just use the "lfile" stab because inlined functions
+    // can interpolate code from a different file!
+    // Such included source files use the N_SOL stab type.
+    while (lline >= lfile
+           && stabs[lline].n_type != N_SOL
+           && (stabs[lline].n_type != N_SO || !stabs[lline].n_value))
+        lline--;
+    if (lline >= lfile && stabs[lline].n_strx < stabstr_end - stabstr)
+        info->eip_file = stabstr + stabs[lline].n_strx;
+
+
+    // Set eip_fn_narg to the number of arguments taken by the function,
+    // or 0 if there was no containing function.
+    if (lfun < rfun)
+        for (lline = lfun + 1;
+             lline < rfun && stabs[lline].n_type == N_PSYM;
+             lline++)
+            info->eip_fn_narg++;
+
+    return 0;
 }
diff --git a/kern/pmap.c b/kern/pmap.c
index 76ac628..040fb2c 100644
--- a/kern/pmap.c
+++ b/kern/pmap.c
@@ -28,30 +28,30 @@ static struct PageInfo *page_free_list;	// Free list of physical pages
 static int
 nvram_read(int r)
 {
-	return mc146818_read(r) | (mc146818_read(r + 1) << 8);
+    return mc146818_read(r) | (mc146818_read(r + 1) << 8);
 }
 
 static void
 i386_detect_memory(void)
 {
-	size_t npages_extmem;
-
-	// Use CMOS calls to measure available base & extended memory.
-	// (CMOS calls return results in kilobytes.)
-	npages_basemem = (nvram_read(NVRAM_BASELO) * 1024) / PGSIZE;
-	npages_extmem = (nvram_read(NVRAM_EXTLO) * 1024) / PGSIZE;
-
-	// Calculate the number of physical pages available in both base
-	// and extended memory.
-	if (npages_extmem)
-		npages = (EXTPHYSMEM / PGSIZE) + npages_extmem;
-	else
-		npages = npages_basemem;
-
-	cprintf("Physical memory: %uK available, base = %uK, extended = %uK\n",
-		npages * PGSIZE / 1024,
-		npages_basemem * PGSIZE / 1024,
-		npages_extmem * PGSIZE / 1024);
+    size_t npages_extmem;
+
+    // Use CMOS calls to measure available base & extended memory.
+    // (CMOS calls return results in kilobytes.)
+    npages_basemem = (nvram_read(NVRAM_BASELO) * 1024) / PGSIZE;
+    npages_extmem = (nvram_read(NVRAM_EXTLO) * 1024) / PGSIZE;
+
+    // Calculate the number of physical pages available in both base
+    // and extended memory.
+    if (npages_extmem)
+        npages = (EXTPHYSMEM / PGSIZE) + npages_extmem;
+    else
+        npages = npages_basemem;
+
+    cprintf("Physical memory: %uK available, base = %uK, extended = %uK\n",
+        npages * PGSIZE / 1024,
+        npages_basemem * PGSIZE / 1024,
+        npages_extmem * PGSIZE / 1024);
 }
 
 
@@ -83,26 +83,34 @@ static void check_page_installed_pgdir(void);
 static void *
 boot_alloc(uint32_t n)
 {
-	static char *nextfree;	// virtual address of next byte of free memory
-	char *result;
-
-	// Initialize nextfree if this is the first time.
-	// 'end' is a magic symbol automatically generated by the linker,
-	// which points to the end of the kernel's bss segment:
-	// the first virtual address that the linker did *not* assign
-	// to any kernel code or global variables.
-	if (!nextfree) {
-		extern char end[];
-		nextfree = ROUNDUP((char *) end, PGSIZE);
-	}
-
-	// Allocate a chunk large enough to hold 'n' bytes, then update
-	// nextfree.  Make sure nextfree is kept aligned
-	// to a multiple of PGSIZE.
-	//
-	// LAB 2: Your code here.
-
-	return NULL;
+    static char *nextfree;	// virtual address of next byte of free memory
+    char *result;
+
+    // Initialize nextfree if this is the first time.
+    // 'end' is a magic symbol automatically generated by the linker,
+    // which points to the end of the kernel's bss segment:
+    // the first virtual address that the linker did *not* assign
+    // to any kernel code or global variables.
+    if (!nextfree) {
+        extern char end[];
+        nextfree = ROUNDUP((char *) end, PGSIZE);
+    }
+
+    // Allocate a chunk large enough to hold 'n' bytes, then update
+    // nextfree.  Make sure nextfree is kept aligned
+    // to a multiple of PGSIZE.
+    //
+    // LAB 2: Your code here.
+    result = nextfree;
+    if (n > 0) {
+        uint32_t alloc_size = ROUNDUP(n, PGSIZE);
+        nextfree += alloc_size;
+        if ((uintptr_t)nextfree > 0xf0400000) {
+//		if (alloc_size > 0xfbffffff || (uintptr_t)nextfree > 0xf0400000) {
+            panic("boot alloc out of memory");
+        }
+    }
+    return result;
 }
 
 // Set up a two-level page table:
@@ -117,119 +125,127 @@ boot_alloc(uint32_t n)
 void
 mem_init(void)
 {
-	uint32_t cr0;
-	size_t n;
-
-	// Find out how much memory the machine has (npages & npages_basemem).
-	i386_detect_memory();
-
-	// Remove this line when you're ready to test this function.
-	panic("mem_init: This function is not finished\n");
-
-	//////////////////////////////////////////////////////////////////////
-	// create initial page directory.
-	kern_pgdir = (pde_t *) boot_alloc(PGSIZE);
-	memset(kern_pgdir, 0, PGSIZE);
-
-	//////////////////////////////////////////////////////////////////////
-	// Recursively insert PD in itself as a page table, to form
-	// a virtual page table at virtual address UVPT.
-	// (For now, you don't have understand the greater purpose of the
-	// following line.)
-
-	// Permissions: kernel R, user R
-	kern_pgdir[PDX(UVPT)] = PADDR(kern_pgdir) | PTE_U | PTE_P;
-
-	//////////////////////////////////////////////////////////////////////
-	// Allocate an array of npages 'struct PageInfo's and store it in 'pages'.
-	// The kernel uses this array to keep track of physical pages: for
-	// each physical page, there is a corresponding struct PageInfo in this
-	// array.  'npages' is the number of physical pages in memory.
-	// Your code goes here:
-
-
-	//////////////////////////////////////////////////////////////////////
-	// Make 'envs' point to an array of size 'NENV' of 'struct Env'.
-	// LAB 3: Your code here.
-
-	//////////////////////////////////////////////////////////////////////
-	// Now that we've allocated the initial kernel data structures, we set
-	// up the list of free physical pages. Once we've done so, all further
-	// memory management will go through the page_* functions. In
-	// particular, we can now map memory using boot_map_region
-	// or page_insert
-	page_init();
-
-	check_page_free_list(1);
-	check_page_alloc();
-	check_page();
-
-	//////////////////////////////////////////////////////////////////////
-	// Now we set up virtual memory
-
-	//////////////////////////////////////////////////////////////////////
-	// Map 'pages' read-only by the user at linear address UPAGES
-	// Permissions:
-	//    - the new image at UPAGES -- kernel R, user R
-	//      (ie. perm = PTE_U | PTE_P)
-	//    - pages itself -- kernel RW, user NONE
-	// Your code goes here:
-
-	//////////////////////////////////////////////////////////////////////
-	// Map the 'envs' array read-only by the user at linear address UENVS
-	// (ie. perm = PTE_U | PTE_P).
-	// Permissions:
-	//    - the new image at UENVS  -- kernel R, user R
-	//    - envs itself -- kernel RW, user NONE
-	// LAB 3: Your code here.
-
-	//////////////////////////////////////////////////////////////////////
-	// Use the physical memory that 'bootstack' refers to as the kernel
-	// stack.  The kernel stack grows down from virtual address KSTACKTOP.
-	// We consider the entire range from [KSTACKTOP-PTSIZE, KSTACKTOP)
-	// to be the kernel stack, but break this into two pieces:
-	//     * [KSTACKTOP-KSTKSIZE, KSTACKTOP) -- backed by physical memory
-	//     * [KSTACKTOP-PTSIZE, KSTACKTOP-KSTKSIZE) -- not backed; so if
-	//       the kernel overflows its stack, it will fault rather than
-	//       overwrite memory.  Known as a "guard page".
-	//     Permissions: kernel RW, user NONE
-	// Your code goes here:
-
-	//////////////////////////////////////////////////////////////////////
-	// Map all of physical memory at KERNBASE.
-	// Ie.  the VA range [KERNBASE, 2^32) should map to
-	//      the PA range [0, 2^32 - KERNBASE)
-	// We might not have 2^32 - KERNBASE bytes of physical memory, but
-	// we just set up the mapping anyway.
-	// Permissions: kernel RW, user NONE
-	// Your code goes here:
-
-	// Initialize the SMP-related parts of the memory map
-	mem_init_mp();
-
-	// Check that the initial page directory has been set up correctly.
-	check_kern_pgdir();
-
-	// Switch from the minimal entry page directory to the full kern_pgdir
-	// page table we just created.	Our instruction pointer should be
-	// somewhere between KERNBASE and KERNBASE+4MB right now, which is
-	// mapped the same way by both page tables.
-	//
-	// If the machine reboots at this point, you've probably set up your
-	// kern_pgdir wrong.
-	lcr3(PADDR(kern_pgdir));
-
-	check_page_free_list(0);
-
-	// entry.S set the really important flags in cr0 (including enabling
-	// paging).  Here we configure the rest of the flags that we care about.
-	cr0 = rcr0();
-	cr0 |= CR0_PE|CR0_PG|CR0_AM|CR0_WP|CR0_NE|CR0_MP;
-	cr0 &= ~(CR0_TS|CR0_EM);
-	lcr0(cr0);
-
-	// Some more checks, only possible after kern_pgdir is installed.
-	check_page_installed_pgdir();
+    uint32_t cr0;
+    size_t n;
+
+    // Find out how much memory the machine has (npages & npages_basemem).
+    i386_detect_memory();
+
+    // Remove this line when you're ready to test this function.
+    //panic("mem_init: This function is not finished\n");
+
+    //////////////////////////////////////////////////////////////////////
+    // create initial page directory.
+    kern_pgdir = (pde_t *) boot_alloc(PGSIZE);
+    memset(kern_pgdir, 0, PGSIZE);
+
+    //////////////////////////////////////////////////////////////////////
+    // Recursively insert PD in itself as a page table, to form
+    // a virtual page table at virtual address UVPT.
+    // (For now, you don't have understand the greater purpose of the
+    // following line.)
+
+    // Permissions: kernel R, user R
+    kern_pgdir[PDX(UVPT)] = PADDR(kern_pgdir) | PTE_U | PTE_P;
+
+    //////////////////////////////////////////////////////////////////////
+    // Allocate an array of npages 'struct PageInfo's and store it in 'pages'.
+    // The kernel uses this array to keep track of physical pages: for
+    // each physical page, there is a corresponding struct PageInfo in this
+    // array.  'npages' is the number of physical pages in memory.
+    // Your code goes here:
+    pages = (struct PageInfo*)boot_alloc(npages * sizeof(struct PageInfo));
+
+    //////////////////////////////////////////////////////////////////////
+    // Make 'envs' point to an array of size 'NENV' of 'struct Env'.
+    // LAB 3: Your code here.
+    envs = (struct Env*)boot_alloc(NENV * sizeof(struct Env));
+
+    //////////////////////////////////////////////////////////////////////
+    // Now that we've allocated the initial kernel data structures, we set
+    // up the list of free physical pages. Once we've done so, all further
+    // memory management will go through the page_* functions. In
+    // particular, we can now map memory using boot_map_region
+    // or page_insert
+    page_init();
+
+    check_page_free_list(1);
+    check_page_alloc();
+    check_page();
+
+    //////////////////////////////////////////////////////////////////////
+    // Now we set up virtual memory
+
+    //////////////////////////////////////////////////////////////////////
+    // Map 'pages' read-only by the user at linear address UPAGES
+    // Permissions:
+    //    - the new image at UPAGES -- kernel R, user R
+    //      (ie. perm = PTE_U | PTE_P)
+    //    - pages itself -- kernel RW, user NONE
+    // Your code goes here:
+    boot_map_region(kern_pgdir, UPAGES, PTSIZE, PADDR(pages), PTE_U);
+    boot_map_region(kern_pgdir, (uintptr_t) &pages, PGSIZE, PADDR(&pages), PTE_W);
+
+    //////////////////////////////////////////////////////////////////////
+    // Map the 'envs' array read-only by the user at linear address UENVS
+    // (ie. perm = PTE_U | PTE_P).
+    // Permissions:
+    //    - the new image at UENVS  -- kernel R, user R
+    //    - envs itself -- kernel RW, user NONE
+    // LAB 3: Your code here.
+    boot_map_region(kern_pgdir, UENVS, PTSIZE, PADDR(envs), PTE_U);
+    boot_map_region(kern_pgdir, (uintptr_t) &envs, PGSIZE, PADDR(&envs), PTE_W);
+
+    //////////////////////////////////////////////////////////////////////
+    // Use the physical memory that 'bootstack' refers to as the kernel
+    // stack.  The kernel stack grows down from virtual address KSTACKTOP.
+    // We consider the entire range from [KSTACKTOP-PTSIZE, KSTACKTOP)
+    // to be the kernel stack, but break this into two pieces:
+    //     * [KSTACKTOP-KSTKSIZE, KSTACKTOP) -- backed by physical memory
+    //     * [KSTACKTOP-PTSIZE, KSTACKTOP-KSTKSIZE) -- not backed; so if
+    //       the kernel overflows its stack, it will fault rather than
+    //       overwrite memory.  Known as a "guard page".
+    //     Permissions: kernel RW, user NONE
+    // Your code goes here:
+    boot_map_region(kern_pgdir, KSTACKTOP - KSTKSIZE, KSTKSIZE, PADDR(bootstack), PTE_W);
+
+    //////////////////////////////////////////////////////////////////////
+    // Map all of physical memory at KERNBASE.
+    // Ie.  the VA range [KERNBASE, 2^32) should map to
+    //      the PA range [0, 2^32 - KERNBASE)
+    // We might not have 2^32 - KERNBASE bytes of physical memory, but
+    // we just set up the mapping anyway.
+    // Permissions: kernel RW, user NONE
+    // Your code goes here:
+    boot_map_region(kern_pgdir, KERNBASE, 0xffffffff - KERNBASE + 1, 0, PTE_W);
+
+    // Initialize the SMP-related parts of the memory map
+    mem_init_mp();
+
+    // Check that the initial page directory has been set up correctly.
+    check_kern_pgdir();
+
+
+    // Switch from the minimal entry page directory to the full kern_pgdir
+    // page table we just created.	Our instruction pointer should be
+    // somewhere between KERNBASE and KERNBASE+4MB right now, which is
+    // mapped the same way by both page tables.
+    //
+    // If the machine reboots at this point, you've probably set up your
+    // kern_pgdir wrong.
+    lcr3(PADDR(kern_pgdir));
+
+    check_page_free_list(0);
+
+    // entry.S set the really important flags in cr0 (including enabling
+    // paging).  Here we configure the rest of the flags that we care about.
+    cr0 = rcr0();
+    cr0 |= CR0_PE|CR0_PG|CR0_AM|CR0_WP|CR0_NE|CR0_MP;
+    cr0 &= ~(CR0_TS|CR0_EM);
+    lcr0(cr0);
+
+    // Some more checks, only possible after kern_pgdir is installed.
+    check_page_installed_pgdir();
 }
 
 // Modify mappings in kern_pgdir to support SMP
@@ -238,23 +254,26 @@ mem_init(void)
 static void
 mem_init_mp(void)
 {
-	// Map per-CPU stacks starting at KSTACKTOP, for up to 'NCPU' CPUs.
-	//
-	// For CPU i, use the physical memory that 'percpu_kstacks[i]' refers
-	// to as its kernel stack. CPU i's kernel stack grows down from virtual
-	// address kstacktop_i = KSTACKTOP - i * (KSTKSIZE + KSTKGAP), and is
-	// divided into two pieces, just like the single stack you set up in
-	// mem_init:
-	//     * [kstacktop_i - KSTKSIZE, kstacktop_i)
-	//          -- backed by physical memory
-	//     * [kstacktop_i - (KSTKSIZE + KSTKGAP), kstacktop_i - KSTKSIZE)
-	//          -- not backed; so if the kernel overflows its stack,
-	//             it will fault rather than overwrite another CPU's stack.
-	//             Known as a "guard page".
-	//     Permissions: kernel RW, user NONE
-	//
-	// LAB 4: Your code here:
-
+    // Map per-CPU stacks starting at KSTACKTOP, for up to 'NCPU' CPUs.
+    //
+    // For CPU i, use the physical memory that 'percpu_kstacks[i]' refers
+    // to as its kernel stack. CPU i's kernel stack grows down from virtual
+    // address kstacktop_i = KSTACKTOP - i * (KSTKSIZE + KSTKGAP), and is
+    // divided into two pieces, just like the single stack you set up in
+    // mem_init:
+    //     * [kstacktop_i - KSTKSIZE, kstacktop_i)
+    //          -- backed by physical memory
+    //     * [kstacktop_i - (KSTKSIZE + KSTKGAP), kstacktop_i - KSTKSIZE)
+    //          -- not backed; so if the kernel overflows its stack,
+    //             it will fault rather than overwrite another CPU's stack.
+    //             Known as a "guard page".
+    //     Permissions: kernel RW, user NONE
+    //
+    // LAB 4: Your code here:
+    for (int i = 0; i < NCPU; ++i) {
+        uintptr_t kstack_top = KSTACKTOP - i * (KSTKSIZE + KSTKGAP);
+        boot_map_region(kern_pgdir, kstack_top - KSTKSIZE, KSTKSIZE, PADDR(percpu_kstacks[i]), PTE_W);
+    }
 }
 
 // --------------------------------------------------------------
@@ -272,33 +291,44 @@ mem_init_mp(void)
 void
 page_init(void)
 {
-	// LAB 4:
-	// Change your code to mark the physical page at MPENTRY_PADDR
-	// as in use
-
-	// The example code here marks all physical pages as free.
-	// However this is not truly the case.  What memory is free?
-	//  1) Mark physical page 0 as in use.
-	//     This way we preserve the real-mode IDT and BIOS structures
-	//     in case we ever need them.  (Currently we don't, but...)
-	//  2) The rest of base memory, [PGSIZE, npages_basemem * PGSIZE)
-	//     is free.
-	//  3) Then comes the IO hole [IOPHYSMEM, EXTPHYSMEM), which must
-	//     never be allocated.
-	//  4) Then extended memory [EXTPHYSMEM, ...).
-	//     Some of it is in use, some is free. Where is the kernel
-	//     in physical memory?  Which pages are already in use for
-	//     page tables and other data structures?
-	//
-	// Change the code to reflect this.
-	// NB: DO NOT actually touch the physical memory corresponding to
-	// free pages!
-	size_t i;
-	for (i = 0; i < npages; i++) {
-		pages[i].pp_ref = 0;
-		pages[i].pp_link = page_free_list;
-		page_free_list = &pages[i];
-	}
+    // LAB 4:
+    // Change your code to mark the physical page at MPENTRY_PADDR
+    // as in use
+
+    // The example code here marks all physical pages as free.
+    // However this is not truly the case.  What memory is free?
+    //  1) Mark physical page 0 as in use.
+    //     This way we preserve the real-mode IDT and BIOS structures
+    //     in case we ever need them.  (Currently we don't, but...)
+    //  2) The rest of base memory, [PGSIZE, npages_basemem * PGSIZE)
+    //     is free.
+    //  3) Then comes the IO hole [IOPHYSMEM, EXTPHYSMEM), which must
+    //     never be allocated.
+    //  4) Then extended memory [EXTPHYSMEM, ...).
+    //     Some of it is in use, some is free. Where is the kernel
+    //     in physical memory?  Which pages are already in use for
+    //     page tables and other data structures?
+    //
+    // Change the code to reflect this.
+    // NB: DO NOT actually touch the physical memory corresponding to
+    // free pages!
+    size_t mpentry_index = PGNUM(MPENTRY_PADDR);
+    for (size_t i = 1; i < npages_basemem; ++i) {
+        if (i == mpentry_index) {
+            pages[i].pp_ref = 1;
+            pages[i].pp_link = NULL;
+            continue;
+        }
+        pages[i].pp_ref = 0;
+        pages[i].pp_link = page_free_list;
+        page_free_list = &pages[i];
+    }
+    size_t boot_page = PGNUM(PADDR(boot_alloc(0)));
+    for (size_t i = boot_page; i < npages; ++i) {
+        pages[i].pp_ref = 0;
+        pages[i].pp_link = page_free_list;
+        page_free_list = &pages[i];
+    }
 }
 
 //
@@ -313,8 +343,17 @@ page_init(void)
 struct PageInfo *
 page_alloc(int alloc_flags)
 {
-	// Fill this function in
-	return 0;
+    // Fill this function in
+    struct PageInfo* res = page_free_list;
+    if (!res) {
+        return NULL;
+    }
+    page_free_list = res->pp_link;
+    if (alloc_flags & ALLOC_ZERO) {
+        void* v = page2kva(res);
+        memset(v, 0, PGSIZE);
+    }
+    return res;
 }
 
 //
@@ -324,7 +363,12 @@ page_alloc(int alloc_flags)
 void
 page_free(struct PageInfo *pp)
 {
-	// Fill this function in
+    // Fill this function in
+    if (!pp || pp->pp_ref != 0) {
+        return;
+    }
+    pp->pp_link = page_free_list;
+    page_free_list = pp;
 }
 
 //
@@ -334,8 +378,8 @@ page_free(struct PageInfo *pp)
 void
 page_decref(struct PageInfo* pp)
 {
-	if (--pp->pp_ref == 0)
-		page_free(pp);
+    if (--pp->pp_ref == 0)
+        page_free(pp);
 }
 
 // Given 'pgdir', a pointer to a page directory, pgdir_walk returns
@@ -363,8 +407,21 @@ page_decref(struct PageInfo* pp)
 pte_t *
 pgdir_walk(pde_t *pgdir, const void *va, int create)
 {
-	// Fill this function in
-	return NULL;
+    // Fill this function in
+    pde_t* pgd_entry = pgdir + PDX(va);
+    if (!(*pgd_entry & PTE_P)) {
+        if (!create) {
+            return NULL;
+        }
+        struct PageInfo* page = page_alloc(ALLOC_ZERO);
+        if (!page) {
+            return NULL;
+        }
+        page->pp_ref++;
+        *pgd_entry = page2pa(page) | PTE_P | PTE_W | PTE_U;
+    }
+    pte_t* pg_table = KADDR(PTE_ADDR(*pgd_entry));
+    return pg_table + PTX(va);
 }
 
 //
@@ -380,7 +437,14 @@ pgdir_walk(pde_t *pgdir, const void *va, int create)
 static void
 boot_map_region(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm)
 {
-	// Fill this function in
+    // Fill this function in
+    size_t old_size = size;
+    for (;size>0 && size <= old_size;size-=PGSIZE) {
+        pte_t* pte = pgdir_walk(pgdir, (const void*)va, 1);
+        *pte = pa | perm | PTE_P;
+        va += PGSIZE;
+        pa += PGSIZE;
+    }
 }
 
 //
@@ -411,8 +475,18 @@ boot_map_region(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm
 int
 page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm)
 {
-	// Fill this function in
-	return 0;
+    // Fill this function in
+    pde_t* pte = pgdir_walk(pgdir, va, 1);
+    if (!pte) {
+        return -E_NO_MEM;
+    }
+    physaddr_t pa = page2pa(pp);
+    pp->pp_ref++;
+    // page will removed only if already exists on va
+    page_remove(pgdir, va);
+    *pte = pa | perm | PTE_P;
+    tlb_invalidate(pgdir, va);
+    return 0;
 }
 
 //
@@ -429,8 +503,15 @@ page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm)
 struct PageInfo *
 page_lookup(pde_t *pgdir, void *va, pte_t **pte_store)
 {
-	// Fill this function in
-	return NULL;
+    // Fill this function in
+    pte_t* pte = pgdir_walk(pgdir, va, 0);
+    if (!pte || !(*pte & PTE_P)) {
+        return NULL;
+    }
+    if (pte_store) {
+        *pte_store = pte;
+    }
+    return pa2page(PTE_ADDR(*pte));
 }
 
 //
@@ -451,7 +532,15 @@ page_lookup(pde_t *pgdir, void *va, pte_t **pte_store)
 void
 page_remove(pde_t *pgdir, void *va)
 {
-	// Fill this function in
+    // Fill this function in
+    pte_t* pte;
+    struct PageInfo *p = page_lookup(pgdir, va, &pte);
+    if (!p) {
+        return;
+    }
+    page_decref(p);
+    *pte = 0;
+    tlb_invalidate(pgdir, va);
 }
 
 //
@@ -461,9 +550,9 @@ page_remove(pde_t *pgdir, void *va)
 void
 tlb_invalidate(pde_t *pgdir, void *va)
 {
-	// Flush the entry only if we're modifying the current address space.
-	if (!curenv || curenv->env_pgdir == pgdir)
-		invlpg(va);
+    // Flush the entry only if we're modifying the current address space.
+    if (!curenv || curenv->env_pgdir == pgdir)
+        invlpg(va);
 }
 
 //
@@ -474,31 +563,39 @@ tlb_invalidate(pde_t *pgdir, void *va)
 void *
 mmio_map_region(physaddr_t pa, size_t size)
 {
-	// Where to start the next region.  Initially, this is the
-	// beginning of the MMIO region.  Because this is static, its
-	// value will be preserved between calls to mmio_map_region
-	// (just like nextfree in boot_alloc).
-	static uintptr_t base = MMIOBASE;
-
-	// Reserve size bytes of virtual memory starting at base and
-	// map physical pages [pa,pa+size) to virtual addresses
-	// [base,base+size).  Since this is device memory and not
-	// regular DRAM, you'll have to tell the CPU that it isn't
-	// safe to cache access to this memory.  Luckily, the page
-	// tables provide bits for this purpose; simply create the
-	// mapping with PTE_PCD|PTE_PWT (cache-disable and
-	// write-through) in addition to PTE_W.  (If you're interested
-	// in more details on this, see section 10.5 of IA32 volume
-	// 3A.)
-	//
-	// Be sure to round size up to a multiple of PGSIZE and to
-	// handle if this reservation would overflow MMIOLIM (it's
-	// okay to simply panic if this happens).
-	//
-	// Hint: The staff solution uses boot_map_region.
-	//
-	// Your code here:
-	panic("mmio_map_region not implemented");
+    // Where to start the next region.  Initially, this is the
+    // beginning of the MMIO region.  Because this is static, its
+    // value will be preserved between calls to mmio_map_region
+    // (just like nextfree in boot_alloc).
+    static uintptr_t base = MMIOBASE;
+
+    // Reserve size bytes of virtual memory starting at base and
+    // map physical pages [pa,pa+size) to virtual addresses
+    // [base,base+size).  Since this is device memory and not
+    // regular DRAM, you'll have to tell the CPU that it isn't
+    // safe to cache access to this memory.  Luckily, the page
+    // tables provide bits for this purpose; simply create the
+    // mapping with PTE_PCD|PTE_PWT (cache-disable and
+    // write-through) in addition to PTE_W.  (If you're interested
+    // in more details on this, see section 10.5 of IA32 volume
+    // 3A.)
+    //
+    // Be sure to round size up to a multiple of PGSIZE and to
+    // handle if this reservation would overflow MMIOLIM (it's
+    // okay to simply panic if this happens).
+    //
+    // Hint: The staff solution uses boot_map_region.
+    //
+    // Your code here:
+    size_t actual_size = ROUNDUP(size, PGSIZE);
+    if (size >= MMIOLIM - base) {
+        panic("mmio: not enough memory");
+    }
+    boot_map_region(kern_pgdir, base, actual_size, pa, PTE_W | PTE_PCD | PTE_PWT);
+
+    uintptr_t res = base;
+    base += actual_size;
+    return (void*)res;
 }
 
 static uintptr_t user_mem_check_addr;
@@ -521,12 +618,31 @@ static uintptr_t user_mem_check_addr;
 // Returns 0 if the user program can access this range of addresses,
 // and -E_FAULT otherwise.
 //
+int user_mem_check_inner(const void* va, uintptr_t addr) {
+    user_mem_check_addr = (uintptr_t)va;
+    if (user_mem_check_addr < addr) {
+        user_mem_check_addr = addr;
+    }
+    return -E_FAULT;
+}
+
 int
 user_mem_check(struct Env *env, const void *va, size_t len, int perm)
 {
-	// LAB 3: Your code here.
-
-	return 0;
+    // LAB 3: Your code here.
+    perm = perm | PTE_P;
+    uintptr_t addr = ROUNDDOWN((uint32_t)va, PGSIZE);
+    uintptr_t end = ROUNDUP((uint32_t)va + len, PGSIZE);
+    for(; addr < end; addr += PGSIZE) {
+        if (addr >= ULIM) {
+            return user_mem_check_inner(va, addr);
+        }
+        pte_t *pte = pgdir_walk(env->env_pgdir, (void *)addr, 0);
+        if (!pte || (*pte & perm) != perm) {
+            return user_mem_check_inner(va, addr);
+        }
+    }
+    return 0;
 }
 
 //
@@ -539,11 +655,11 @@ user_mem_check(struct Env *env, const void *va, size_t len, int perm)
 void
 user_mem_assert(struct Env *env, const void *va, size_t len, int perm)
 {
-	if (user_mem_check(env, va, len, perm | PTE_U) < 0) {
-		cprintf("[%08x] user_mem_check assertion failure for "
-			"va %08x\n", env->env_id, user_mem_check_addr);
-		env_destroy(env);	// may not return
-	}
+    if (user_mem_check(env, va, len, perm | PTE_U) < 0) {
+        cprintf("[%08x] user_mem_check assertion failure for "
+            "va %08x\n", env->env_id, user_mem_check_addr);
+        env_destroy(env);	// may not return
+    }
 }
 
 
@@ -557,59 +673,59 @@ user_mem_assert(struct Env *env, const void *va, size_t len, int perm)
 static void
 check_page_free_list(bool only_low_memory)
 {
-	struct PageInfo *pp;
-	unsigned pdx_limit = only_low_memory ? 1 : NPDENTRIES;
-	int nfree_basemem = 0, nfree_extmem = 0;
-	char *first_free_page;
-
-	if (!page_free_list)
-		panic("'page_free_list' is a null pointer!");
-
-	if (only_low_memory) {
-		// Move pages with lower addresses first in the free
-		// list, since entry_pgdir does not map all pages.
-		struct PageInfo *pp1, *pp2;
-		struct PageInfo **tp[2] = { &pp1, &pp2 };
-		for (pp = page_free_list; pp; pp = pp->pp_link) {
-			int pagetype = PDX(page2pa(pp)) >= pdx_limit;
-			*tp[pagetype] = pp;
-			tp[pagetype] = &pp->pp_link;
-		}
-		*tp[1] = 0;
-		*tp[0] = pp2;
-		page_free_list = pp1;
-	}
-
-	// if there's a page that shouldn't be on the free list,
-	// try to make sure it eventually causes trouble.
-	for (pp = page_free_list; pp; pp = pp->pp_link)
-		if (PDX(page2pa(pp)) < pdx_limit)
-			memset(page2kva(pp), 0x97, 128);
-
-	first_free_page = (char *) boot_alloc(0);
-	for (pp = page_free_list; pp; pp = pp->pp_link) {
-		// check that we didn't corrupt the free list itself
-		assert(pp >= pages);
-		assert(pp < pages + npages);
-		assert(((char *) pp - (char *) pages) % sizeof(*pp) == 0);
-
-		// check a few pages that shouldn't be on the free list
-		assert(page2pa(pp) != 0);
-		assert(page2pa(pp) != IOPHYSMEM);
-		assert(page2pa(pp) != EXTPHYSMEM - PGSIZE);
-		assert(page2pa(pp) != EXTPHYSMEM);
-		assert(page2pa(pp) < EXTPHYSMEM || (char *) page2kva(pp) >= first_free_page);
-		// (new test for lab 4)
-		assert(page2pa(pp) != MPENTRY_PADDR);
-
-		if (page2pa(pp) < EXTPHYSMEM)
-			++nfree_basemem;
-		else
-			++nfree_extmem;
-	}
-
-	assert(nfree_basemem > 0);
-	assert(nfree_extmem > 0);
+    struct PageInfo *pp;
+    unsigned pdx_limit = only_low_memory ? 1 : NPDENTRIES;
+    int nfree_basemem = 0, nfree_extmem = 0;
+    char *first_free_page;
+
+    if (!page_free_list)
+        panic("'page_free_list' is a null pointer!");
+
+    if (only_low_memory) {
+        // Move pages with lower addresses first in the free
+        // list, since entry_pgdir does not map all pages.
+        struct PageInfo *pp1, *pp2;
+        struct PageInfo **tp[2] = { &pp1, &pp2 };
+        for (pp = page_free_list; pp; pp = pp->pp_link) {
+            int pagetype = PDX(page2pa(pp)) >= pdx_limit;
+            *tp[pagetype] = pp;
+            tp[pagetype] = &pp->pp_link;
+        }
+        *tp[1] = 0;
+        *tp[0] = pp2;
+        page_free_list = pp1;
+    }
+
+    // if there's a page that shouldn't be on the free list,
+    // try to make sure it eventually causes trouble.
+    for (pp = page_free_list; pp; pp = pp->pp_link)
+        if (PDX(page2pa(pp)) < pdx_limit)
+            memset(page2kva(pp), 0x97, 128);
+
+    first_free_page = (char *) boot_alloc(0);
+    for (pp = page_free_list; pp; pp = pp->pp_link) {
+        // check that we didn't corrupt the free list itself
+        assert(pp >= pages);
+        assert(pp < pages + npages);
+        assert(((char *) pp - (char *) pages) % sizeof(*pp) == 0);
+
+        // check a few pages that shouldn't be on the free list
+        assert(page2pa(pp) != 0);
+        assert(page2pa(pp) != IOPHYSMEM);
+        assert(page2pa(pp) != EXTPHYSMEM - PGSIZE);
+        assert(page2pa(pp) != EXTPHYSMEM);
+        assert(page2pa(pp) < EXTPHYSMEM || (char *) page2kva(pp) >= first_free_page);
+        // (new test for lab 4)
+        assert(page2pa(pp) != MPENTRY_PADDR);
+
+        if (page2pa(pp) < EXTPHYSMEM)
+            ++nfree_basemem;
+        else
+            ++nfree_extmem;
+    }
+
+    assert(nfree_basemem > 0);
+    assert(nfree_extmem > 0);
 }
 
 //
@@ -619,75 +735,75 @@ check_page_free_list(bool only_low_memory)
 static void
 check_page_alloc(void)
 {
-	struct PageInfo *pp, *pp0, *pp1, *pp2;
-	int nfree;
-	struct PageInfo *fl;
-	char *c;
-	int i;
-
-	if (!pages)
-		panic("'pages' is a null pointer!");
-
-	// check number of free pages
-	for (pp = page_free_list, nfree = 0; pp; pp = pp->pp_link)
-		++nfree;
-
-	// should be able to allocate three pages
-	pp0 = pp1 = pp2 = 0;
-	assert((pp0 = page_alloc(0)));
-	assert((pp1 = page_alloc(0)));
-	assert((pp2 = page_alloc(0)));
-
-	assert(pp0);
-	assert(pp1 && pp1 != pp0);
-	assert(pp2 && pp2 != pp1 && pp2 != pp0);
-	assert(page2pa(pp0) < npages*PGSIZE);
-	assert(page2pa(pp1) < npages*PGSIZE);
-	assert(page2pa(pp2) < npages*PGSIZE);
-
-	// temporarily steal the rest of the free pages
-	fl = page_free_list;
-	page_free_list = 0;
-
-	// should be no free memory
-	assert(!page_alloc(0));
-
-	// free and re-allocate?
-	page_free(pp0);
-	page_free(pp1);
-	page_free(pp2);
-	pp0 = pp1 = pp2 = 0;
-	assert((pp0 = page_alloc(0)));
-	assert((pp1 = page_alloc(0)));
-	assert((pp2 = page_alloc(0)));
-	assert(pp0);
-	assert(pp1 && pp1 != pp0);
-	assert(pp2 && pp2 != pp1 && pp2 != pp0);
-	assert(!page_alloc(0));
-
-	// test flags
-	memset(page2kva(pp0), 1, PGSIZE);
-	page_free(pp0);
-	assert((pp = page_alloc(ALLOC_ZERO)));
-	assert(pp && pp0 == pp);
-	c = page2kva(pp);
-	for (i = 0; i < PGSIZE; i++)
-		assert(c[i] == 0);
-
-	// give free list back
-	page_free_list = fl;
-
-	// free the pages we took
-	page_free(pp0);
-	page_free(pp1);
-	page_free(pp2);
-
-	// number of free pages should be the same
-	for (pp = page_free_list; pp; pp = pp->pp_link)
-		--nfree;
-	assert(nfree == 0);
-
-	cprintf("check_page_alloc() succeeded!\n");
+    struct PageInfo *pp, *pp0, *pp1, *pp2;
+    int nfree;
+    struct PageInfo *fl;
+    char *c;
+    int i;
+
+    if (!pages)
+        panic("'pages' is a null pointer!");
+
+    // check number of free pages
+    for (pp = page_free_list, nfree = 0; pp; pp = pp->pp_link)
+        ++nfree;
+
+    // should be able to allocate three pages
+    pp0 = pp1 = pp2 = 0;
+    assert((pp0 = page_alloc(0)));
+    assert((pp1 = page_alloc(0)));
+    assert((pp2 = page_alloc(0)));
+
+    assert(pp0);
+    assert(pp1 && pp1 != pp0);
+    assert(pp2 && pp2 != pp1 && pp2 != pp0);
+    assert(page2pa(pp0) < npages*PGSIZE);
+    assert(page2pa(pp1) < npages*PGSIZE);
+    assert(page2pa(pp2) < npages*PGSIZE);
+
+    // temporarily steal the rest of the free pages
+    fl = page_free_list;
+    page_free_list = 0;
+
+    // should be no free memory
+    assert(!page_alloc(0));
+
+    // free and re-allocate?
+    page_free(pp0);
+    page_free(pp1);
+    page_free(pp2);
+    pp0 = pp1 = pp2 = 0;
+    assert((pp0 = page_alloc(0)));
+    assert((pp1 = page_alloc(0)));
+    assert((pp2 = page_alloc(0)));
+    assert(pp0);
+    assert(pp1 && pp1 != pp0);
+    assert(pp2 && pp2 != pp1 && pp2 != pp0);
+    assert(!page_alloc(0));
+
+    // test flags
+    memset(page2kva(pp0), 1, PGSIZE);
+    page_free(pp0);
+    assert((pp = page_alloc(ALLOC_ZERO)));
+    assert(pp && pp0 == pp);
+    c = page2kva(pp);
+    for (i = 0; i < PGSIZE; i++)
+        assert(c[i] == 0);
+
+    // give free list back
+    page_free_list = fl;
+
+    // free the pages we took
+    page_free(pp0);
+    page_free(pp1);
+    page_free(pp2);
+
+    // number of free pages should be the same
+    for (pp = page_free_list; pp; pp = pp->pp_link)
+        --nfree;
+    assert(nfree == 0);
+
+    cprintf("check_page_alloc() succeeded!\n");
 }
 
 //
@@ -701,56 +817,56 @@ check_page_alloc(void)
 static void
 check_kern_pgdir(void)
 {
-	uint32_t i, n;
-	pde_t *pgdir;
-
-	pgdir = kern_pgdir;
-
-	// check pages array
-	n = ROUNDUP(npages*sizeof(struct PageInfo), PGSIZE);
-	for (i = 0; i < n; i += PGSIZE)
-		assert(check_va2pa(pgdir, UPAGES + i) == PADDR(pages) + i);
-
-	// check envs array (new test for lab 3)
-	n = ROUNDUP(NENV*sizeof(struct Env), PGSIZE);
-	for (i = 0; i < n; i += PGSIZE)
-		assert(check_va2pa(pgdir, UENVS + i) == PADDR(envs) + i);
-
-	// check phys mem
-	for (i = 0; i < npages * PGSIZE; i += PGSIZE)
-		assert(check_va2pa(pgdir, KERNBASE + i) == i);
-
-	// check kernel stack
-	// (updated in lab 4 to check per-CPU kernel stacks)
-	for (n = 0; n < NCPU; n++) {
-		uint32_t base = KSTACKTOP - (KSTKSIZE + KSTKGAP) * (n + 1);
-		for (i = 0; i < KSTKSIZE; i += PGSIZE)
-			assert(check_va2pa(pgdir, base + KSTKGAP + i)
-				== PADDR(percpu_kstacks[n]) + i);
-		for (i = 0; i < KSTKGAP; i += PGSIZE)
-			assert(check_va2pa(pgdir, base + i) == ~0);
-	}
-
-	// check PDE permissions
-	for (i = 0; i < NPDENTRIES; i++) {
-		switch (i) {
-		case PDX(UVPT):
-		case PDX(KSTACKTOP-1):
-		case PDX(UPAGES):
-		case PDX(UENVS):
-		case PDX(MMIOBASE):
-			assert(pgdir[i] & PTE_P);
-			break;
-		default:
-			if (i >= PDX(KERNBASE)) {
-				assert(pgdir[i] & PTE_P);
-				assert(pgdir[i] & PTE_W);
-			} else
-				assert(pgdir[i] == 0);
-			break;
-		}
-	}
-	cprintf("check_kern_pgdir() succeeded!\n");
+    uint32_t i, n;
+    pde_t *pgdir;
+
+    pgdir = kern_pgdir;
+
+    // check pages array
+    n = ROUNDUP(npages*sizeof(struct PageInfo), PGSIZE);
+    for (i = 0; i < n; i += PGSIZE)
+        assert(check_va2pa(pgdir, UPAGES + i) == PADDR(pages) + i);
+
+    // check envs array (new test for lab 3)
+    n = ROUNDUP(NENV*sizeof(struct Env), PGSIZE);
+    for (i = 0; i < n; i += PGSIZE)
+        assert(check_va2pa(pgdir, UENVS + i) == PADDR(envs) + i);
+
+    // check phys mem
+    for (i = 0; i < npages * PGSIZE; i += PGSIZE)
+        assert(check_va2pa(pgdir, KERNBASE + i) == i);
+
+    // check kernel stack
+    // (updated in lab 4 to check per-CPU kernel stacks)
+    for (n = 0; n < NCPU; n++) {
+        uint32_t base = KSTACKTOP - (KSTKSIZE + KSTKGAP) * (n + 1);
+        for (i = 0; i < KSTKSIZE; i += PGSIZE)
+            assert(check_va2pa(pgdir, base + KSTKGAP + i)
+                == PADDR(percpu_kstacks[n]) + i);
+        for (i = 0; i < KSTKGAP; i += PGSIZE)
+            assert(check_va2pa(pgdir, base + i) == ~0);
+    }
+
+    // check PDE permissions
+    for (i = 0; i < NPDENTRIES; i++) {
+        switch (i) {
+        case PDX(UVPT):
+        case PDX(KSTACKTOP-1):
+        case PDX(UPAGES):
+        case PDX(UENVS):
+        case PDX(MMIOBASE):
+            assert(pgdir[i] & PTE_P);
+            break;
+        default:
+            if (i >= PDX(KERNBASE)) {
+                assert(pgdir[i] & PTE_P);
+                assert(pgdir[i] & PTE_W);
+            } else
+                assert(pgdir[i] == 0);
+            break;
+        }
+    }
+    cprintf("check_kern_pgdir() succeeded!\n");
 }
 
 // This function returns the physical address of the page containing 'va',
@@ -761,15 +877,15 @@ check_kern_pgdir(void)
 static physaddr_t
 check_va2pa(pde_t *pgdir, uintptr_t va)
 {
-	pte_t *p;
-
-	pgdir = &pgdir[PDX(va)];
-	if (!(*pgdir & PTE_P))
-		return ~0;
-	p = (pte_t*) KADDR(PTE_ADDR(*pgdir));
-	if (!(p[PTX(va)] & PTE_P))
-		return ~0;
-	return PTE_ADDR(p[PTX(va)]);
+    pte_t *p;
+
+    pgdir = &pgdir[PDX(va)];
+    if (!(*pgdir & PTE_P))
+        return ~0;
+    p = (pte_t*) KADDR(PTE_ADDR(*pgdir));
+    if (!(p[PTX(va)] & PTE_P))
+        return ~0;
+    return PTE_ADDR(p[PTX(va)]);
 }
 
 
@@ -777,212 +893,212 @@ check_va2pa(pde_t *pgdir, uintptr_t va)
 static void
 check_page(void)
 {
-	struct PageInfo *pp, *pp0, *pp1, *pp2;
-	struct PageInfo *fl;
-	pte_t *ptep, *ptep1;
-	void *va;
-	uintptr_t mm1, mm2;
-	int i;
-	extern pde_t entry_pgdir[];
-
-	// should be able to allocate three pages
-	pp0 = pp1 = pp2 = 0;
-	assert((pp0 = page_alloc(0)));
-	assert((pp1 = page_alloc(0)));
-	assert((pp2 = page_alloc(0)));
-
-	assert(pp0);
-	assert(pp1 && pp1 != pp0);
-	assert(pp2 && pp2 != pp1 && pp2 != pp0);
-
-	// temporarily steal the rest of the free pages
-	fl = page_free_list;
-	page_free_list = 0;
-
-	// should be no free memory
-	assert(!page_alloc(0));
-
-	// there is no page allocated at address 0
-	assert(page_lookup(kern_pgdir, (void *) 0x0, &ptep) == NULL);
-
-	// there is no free memory, so we can't allocate a page table
-	assert(page_insert(kern_pgdir, pp1, 0x0, PTE_W) < 0);
-
-	// free pp0 and try again: pp0 should be used for page table
-	page_free(pp0);
-	assert(page_insert(kern_pgdir, pp1, 0x0, PTE_W) == 0);
-	assert(PTE_ADDR(kern_pgdir[0]) == page2pa(pp0));
-	assert(check_va2pa(kern_pgdir, 0x0) == page2pa(pp1));
-	assert(pp1->pp_ref == 1);
-	assert(pp0->pp_ref == 1);
-
-	// should be able to map pp2 at PGSIZE because pp0 is already allocated for page table
-	assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W) == 0);
-	assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
-	assert(pp2->pp_ref == 1);
-
-	// should be no free memory
-	assert(!page_alloc(0));
-
-	// should be able to map pp2 at PGSIZE because it's already there
-	assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W) == 0);
-	assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
-	assert(pp2->pp_ref == 1);
-
-	// pp2 should NOT be on the free list
-	// could happen in ref counts are handled sloppily in page_insert
-	assert(!page_alloc(0));
-
-	// check that pgdir_walk returns a pointer to the pte
-	ptep = (pte_t *) KADDR(PTE_ADDR(kern_pgdir[PDX(PGSIZE)]));
-	assert(pgdir_walk(kern_pgdir, (void*)PGSIZE, 0) == ptep+PTX(PGSIZE));
-
-	// should be able to change permissions too.
-	assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W|PTE_U) == 0);
-	assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
-	assert(pp2->pp_ref == 1);
-	assert(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_U);
-	assert(kern_pgdir[0] & PTE_U);
-
-	// should be able to remap with fewer permissions
-	assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W) == 0);
-	assert(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_W);
-	assert(!(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_U));
-
-	// should not be able to map at PTSIZE because need free page for page table
-	assert(page_insert(kern_pgdir, pp0, (void*) PTSIZE, PTE_W) < 0);
-
-	// insert pp1 at PGSIZE (replacing pp2)
-	assert(page_insert(kern_pgdir, pp1, (void*) PGSIZE, PTE_W) == 0);
-	assert(!(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_U));
-
-	// should have pp1 at both 0 and PGSIZE, pp2 nowhere, ...
-	assert(check_va2pa(kern_pgdir, 0) == page2pa(pp1));
-	assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp1));
-	// ... and ref counts should reflect this
-	assert(pp1->pp_ref == 2);
-	assert(pp2->pp_ref == 0);
-
-	// pp2 should be returned by page_alloc
-	assert((pp = page_alloc(0)) && pp == pp2);
-
-	// unmapping pp1 at 0 should keep pp1 at PGSIZE
-	page_remove(kern_pgdir, 0x0);
-	assert(check_va2pa(kern_pgdir, 0x0) == ~0);
-	assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp1));
-	assert(pp1->pp_ref == 1);
-	assert(pp2->pp_ref == 0);
-
-	// unmapping pp1 at PGSIZE should free it
-	page_remove(kern_pgdir, (void*) PGSIZE);
-	assert(check_va2pa(kern_pgdir, 0x0) == ~0);
-	assert(check_va2pa(kern_pgdir, PGSIZE) == ~0);
-	assert(pp1->pp_ref == 0);
-	assert(pp2->pp_ref == 0);
-
-	// so it should be returned by page_alloc
-	assert((pp = page_alloc(0)) && pp == pp1);
-
-	// should be no free memory
-	assert(!page_alloc(0));
-
-	// forcibly take pp0 back
-	assert(PTE_ADDR(kern_pgdir[0]) == page2pa(pp0));
-	kern_pgdir[0] = 0;
-	assert(pp0->pp_ref == 1);
-	pp0->pp_ref = 0;
-
-	// check pointer arithmetic in pgdir_walk
-	page_free(pp0);
-	va = (void*)(PGSIZE * NPDENTRIES + PGSIZE);
-	ptep = pgdir_walk(kern_pgdir, va, 1);
-	ptep1 = (pte_t *) KADDR(PTE_ADDR(kern_pgdir[PDX(va)]));
-	assert(ptep == ptep1 + PTX(va));
-	kern_pgdir[PDX(va)] = 0;
-	pp0->pp_ref = 0;
-
-	// check that new page tables get cleared
-	memset(page2kva(pp0), 0xFF, PGSIZE);
-	page_free(pp0);
-	pgdir_walk(kern_pgdir, 0x0, 1);
-	ptep = (pte_t *) page2kva(pp0);
-	for(i=0; i<NPTENTRIES; i++)
-		assert((ptep[i] & PTE_P) == 0);
-	kern_pgdir[0] = 0;
-	pp0->pp_ref = 0;
-
-	// give free list back
-	page_free_list = fl;
-
-	// free the pages we took
-	page_free(pp0);
-	page_free(pp1);
-	page_free(pp2);
-
-	// test mmio_map_region
-	mm1 = (uintptr_t) mmio_map_region(0, 4097);
-	mm2 = (uintptr_t) mmio_map_region(0, 4096);
-	// check that they're in the right region
-	assert(mm1 >= MMIOBASE && mm1 + 8096 < MMIOLIM);
-	assert(mm2 >= MMIOBASE && mm2 + 8096 < MMIOLIM);
-	// check that they're page-aligned
-	assert(mm1 % PGSIZE == 0 && mm2 % PGSIZE == 0);
-	// check that they don't overlap
-	assert(mm1 + 8096 <= mm2);
-	// check page mappings
-	assert(check_va2pa(kern_pgdir, mm1) == 0);
-	assert(check_va2pa(kern_pgdir, mm1+PGSIZE) == PGSIZE);
-	assert(check_va2pa(kern_pgdir, mm2) == 0);
-	assert(check_va2pa(kern_pgdir, mm2+PGSIZE) == ~0);
-	// check permissions
-	assert(*pgdir_walk(kern_pgdir, (void*) mm1, 0) & (PTE_W|PTE_PWT|PTE_PCD));
-	assert(!(*pgdir_walk(kern_pgdir, (void*) mm1, 0) & PTE_U));
-	// clear the mappings
-	*pgdir_walk(kern_pgdir, (void*) mm1, 0) = 0;
-	*pgdir_walk(kern_pgdir, (void*) mm1 + PGSIZE, 0) = 0;
-	*pgdir_walk(kern_pgdir, (void*) mm2, 0) = 0;
-
-	cprintf("check_page() succeeded!\n");
+    struct PageInfo *pp, *pp0, *pp1, *pp2;
+    struct PageInfo *fl;
+    pte_t *ptep, *ptep1;
+    void *va;
+    uintptr_t mm1, mm2;
+    int i;
+    extern pde_t entry_pgdir[];
+
+    // should be able to allocate three pages
+    pp0 = pp1 = pp2 = 0;
+    assert((pp0 = page_alloc(0)));
+    assert((pp1 = page_alloc(0)));
+    assert((pp2 = page_alloc(0)));
+
+    assert(pp0);
+    assert(pp1 && pp1 != pp0);
+    assert(pp2 && pp2 != pp1 && pp2 != pp0);
+
+    // temporarily steal the rest of the free pages
+    fl = page_free_list;
+    page_free_list = 0;
+
+    // should be no free memory
+    assert(!page_alloc(0));
+
+    // there is no page allocated at address 0
+    assert(page_lookup(kern_pgdir, (void *) 0x0, &ptep) == NULL);
+
+    // there is no free memory, so we can't allocate a page table
+    assert(page_insert(kern_pgdir, pp1, 0x0, PTE_W) < 0);
+
+    // free pp0 and try again: pp0 should be used for page table
+    page_free(pp0);
+    assert(page_insert(kern_pgdir, pp1, 0x0, PTE_W) == 0);
+    assert(PTE_ADDR(kern_pgdir[0]) == page2pa(pp0));
+    assert(check_va2pa(kern_pgdir, 0x0) == page2pa(pp1));
+    assert(pp1->pp_ref == 1);
+    assert(pp0->pp_ref == 1);
+
+    // should be able to map pp2 at PGSIZE because pp0 is already allocated for page table
+    assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W) == 0);
+    assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
+    assert(pp2->pp_ref == 1);
+
+    // should be no free memory
+    assert(!page_alloc(0));
+
+    // should be able to map pp2 at PGSIZE because it's already there
+    assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W) == 0);
+    assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
+    assert(pp2->pp_ref == 1);
+
+    // pp2 should NOT be on the free list
+    // could happen in ref counts are handled sloppily in page_insert
+    assert(!page_alloc(0));
+
+    // check that pgdir_walk returns a pointer to the pte
+    ptep = (pte_t *) KADDR(PTE_ADDR(kern_pgdir[PDX(PGSIZE)]));
+    assert(pgdir_walk(kern_pgdir, (void*)PGSIZE, 0) == ptep+PTX(PGSIZE));
+
+    // should be able to change permissions too.
+    assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W|PTE_U) == 0);
+    assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
+    assert(pp2->pp_ref == 1);
+    assert(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_U);
+    assert(kern_pgdir[0] & PTE_U);
+
+    // should be able to remap with fewer permissions
+    assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W) == 0);
+    assert(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_W);
+    assert(!(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_U));
+
+    // should not be able to map at PTSIZE because need free page for page table
+    assert(page_insert(kern_pgdir, pp0, (void*) PTSIZE, PTE_W) < 0);
+
+    // insert pp1 at PGSIZE (replacing pp2)
+    assert(page_insert(kern_pgdir, pp1, (void*) PGSIZE, PTE_W) == 0);
+    assert(!(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_U));
+
+    // should have pp1 at both 0 and PGSIZE, pp2 nowhere, ...
+    assert(check_va2pa(kern_pgdir, 0) == page2pa(pp1));
+    assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp1));
+    // ... and ref counts should reflect this
+    assert(pp1->pp_ref == 2);
+    assert(pp2->pp_ref == 0);
+
+    // pp2 should be returned by page_alloc
+    assert((pp = page_alloc(0)) && pp == pp2);
+
+    // unmapping pp1 at 0 should keep pp1 at PGSIZE
+    page_remove(kern_pgdir, 0x0);
+    assert(check_va2pa(kern_pgdir, 0x0) == ~0);
+    assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp1));
+    assert(pp1->pp_ref == 1);
+    assert(pp2->pp_ref == 0);
+
+    // unmapping pp1 at PGSIZE should free it
+    page_remove(kern_pgdir, (void*) PGSIZE);
+    assert(check_va2pa(kern_pgdir, 0x0) == ~0);
+    assert(check_va2pa(kern_pgdir, PGSIZE) == ~0);
+    assert(pp1->pp_ref == 0);
+    assert(pp2->pp_ref == 0);
+
+    // so it should be returned by page_alloc
+    assert((pp = page_alloc(0)) && pp == pp1);
+
+    // should be no free memory
+    assert(!page_alloc(0));
+
+    // forcibly take pp0 back
+    assert(PTE_ADDR(kern_pgdir[0]) == page2pa(pp0));
+    kern_pgdir[0] = 0;
+    assert(pp0->pp_ref == 1);
+    pp0->pp_ref = 0;
+
+    // check pointer arithmetic in pgdir_walk
+    page_free(pp0);
+    va = (void*)(PGSIZE * NPDENTRIES + PGSIZE);
+    ptep = pgdir_walk(kern_pgdir, va, 1);
+    ptep1 = (pte_t *) KADDR(PTE_ADDR(kern_pgdir[PDX(va)]));
+    assert(ptep == ptep1 + PTX(va));
+    kern_pgdir[PDX(va)] = 0;
+    pp0->pp_ref = 0;
+
+    // check that new page tables get cleared
+    memset(page2kva(pp0), 0xFF, PGSIZE);
+    page_free(pp0);
+    pgdir_walk(kern_pgdir, 0x0, 1);
+    ptep = (pte_t *) page2kva(pp0);
+    for(i=0; i<NPTENTRIES; i++)
+        assert((ptep[i] & PTE_P) == 0);
+    kern_pgdir[0] = 0;
+    pp0->pp_ref = 0;
+
+    // give free list back
+    page_free_list = fl;
+
+    // free the pages we took
+    page_free(pp0);
+    page_free(pp1);
+    page_free(pp2);
+
+    // test mmio_map_region
+    mm1 = (uintptr_t) mmio_map_region(0, 4097);
+    mm2 = (uintptr_t) mmio_map_region(0, 4096);
+    // check that they're in the right region
+    assert(mm1 >= MMIOBASE && mm1 + 8096 < MMIOLIM);
+    assert(mm2 >= MMIOBASE && mm2 + 8096 < MMIOLIM);
+    // check that they're page-aligned
+    assert(mm1 % PGSIZE == 0 && mm2 % PGSIZE == 0);
+    // check that they don't overlap
+    assert(mm1 + 8096 <= mm2);
+    // check page mappings
+    assert(check_va2pa(kern_pgdir, mm1) == 0);
+    assert(check_va2pa(kern_pgdir, mm1+PGSIZE) == PGSIZE);
+    assert(check_va2pa(kern_pgdir, mm2) == 0);
+    assert(check_va2pa(kern_pgdir, mm2+PGSIZE) == ~0);
+    // check permissions
+    assert(*pgdir_walk(kern_pgdir, (void*) mm1, 0) & (PTE_W|PTE_PWT|PTE_PCD));
+    assert(!(*pgdir_walk(kern_pgdir, (void*) mm1, 0) & PTE_U));
+    // clear the mappings
+    *pgdir_walk(kern_pgdir, (void*) mm1, 0) = 0;
+    *pgdir_walk(kern_pgdir, (void*) mm1 + PGSIZE, 0) = 0;
+    *pgdir_walk(kern_pgdir, (void*) mm2, 0) = 0;
+
+    cprintf("check_page() succeeded!\n");
 }
 
 // check page_insert, page_remove, &c, with an installed kern_pgdir
 static void
 check_page_installed_pgdir(void)
 {
-	struct PageInfo *pp, *pp0, *pp1, *pp2;
-	struct PageInfo *fl;
-	pte_t *ptep, *ptep1;
-	uintptr_t va;
-	int i;
-
-	// check that we can read and write installed pages
-	pp1 = pp2 = 0;
-	assert((pp0 = page_alloc(0)));
-	assert((pp1 = page_alloc(0)));
-	assert((pp2 = page_alloc(0)));
-	page_free(pp0);
-	memset(page2kva(pp1), 1, PGSIZE);
-	memset(page2kva(pp2), 2, PGSIZE);
-	page_insert(kern_pgdir, pp1, (void*) PGSIZE, PTE_W);
-	assert(pp1->pp_ref == 1);
-	assert(*(uint32_t *)PGSIZE == 0x01010101U);
-	page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W);
-	assert(*(uint32_t *)PGSIZE == 0x02020202U);
-	assert(pp2->pp_ref == 1);
-	assert(pp1->pp_ref == 0);
-	*(uint32_t *)PGSIZE = 0x03030303U;
-	assert(*(uint32_t *)page2kva(pp2) == 0x03030303U);
-	page_remove(kern_pgdir, (void*) PGSIZE);
-	assert(pp2->pp_ref == 0);
-
-	// forcibly take pp0 back
-	assert(PTE_ADDR(kern_pgdir[0]) == page2pa(pp0));
-	kern_pgdir[0] = 0;
-	assert(pp0->pp_ref == 1);
-	pp0->pp_ref = 0;
-
-	// free the pages we took
-	page_free(pp0);
-
-	cprintf("check_page_installed_pgdir() succeeded!\n");
+    struct PageInfo *pp, *pp0, *pp1, *pp2;
+    struct PageInfo *fl;
+    pte_t *ptep, *ptep1;
+    uintptr_t va;
+    int i;
+
+    // check that we can read and write installed pages
+    pp1 = pp2 = 0;
+    assert((pp0 = page_alloc(0)));
+    assert((pp1 = page_alloc(0)));
+    assert((pp2 = page_alloc(0)));
+    page_free(pp0);
+    memset(page2kva(pp1), 1, PGSIZE);
+    memset(page2kva(pp2), 2, PGSIZE);
+    page_insert(kern_pgdir, pp1, (void*) PGSIZE, PTE_W);
+    assert(pp1->pp_ref == 1);
+    assert(*(uint32_t *)PGSIZE == 0x01010101U);
+    page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W);
+    assert(*(uint32_t *)PGSIZE == 0x02020202U);
+    assert(pp2->pp_ref == 1);
+    assert(pp1->pp_ref == 0);
+    *(uint32_t *)PGSIZE = 0x03030303U;
+    assert(*(uint32_t *)page2kva(pp2) == 0x03030303U);
+    page_remove(kern_pgdir, (void*) PGSIZE);
+    assert(pp2->pp_ref == 0);
+
+    // forcibly take pp0 back
+    assert(PTE_ADDR(kern_pgdir[0]) == page2pa(pp0));
+    kern_pgdir[0] = 0;
+    assert(pp0->pp_ref == 1);
+    pp0->pp_ref = 0;
+
+    // free the pages we took
+    page_free(pp0);
+
+    cprintf("check_page_installed_pgdir() succeeded!\n");
 }
diff --git a/kern/sched.c b/kern/sched.c
index b606740..25b1084 100644
--- a/kern/sched.c
+++ b/kern/sched.c
@@ -11,27 +11,42 @@ void sched_halt(void);
 void
 sched_yield(void)
 {
-	struct Env *idle;
+    struct Env *idle;
 
-	// Implement simple round-robin scheduling.
-	//
-	// Search through 'envs' for an ENV_RUNNABLE environment in
-	// circular fashion starting just after the env this CPU was
-	// last running.  Switch to the first such environment found.
-	//
-	// If no envs are runnable, but the environment previously
-	// running on this CPU is still ENV_RUNNING, it's okay to
-	// choose that environment.
-	//
-	// Never choose an environment that's currently running on
-	// another CPU (env_status == ENV_RUNNING). If there are
-	// no runnable environments, simply drop through to the code
-	// below to halt the cpu.
+    // Implement simple round-robin scheduling.
+    //
+    // Search through 'envs' for an ENV_RUNNABLE environment in
+    // circular fashion starting just after the env this CPU was
+    // last running.  Switch to the first such environment found.
+    //
+    // If no envs are runnable, but the environment previously
+    // running on this CPU is still ENV_RUNNING, it's okay to
+    // choose that environment.
+    //
+    // Never choose an environment that's currently running on
+    // another CPU (env_status == ENV_RUNNING). If there are
+    // no runnable environments, simply drop through to the code
+    // below to halt the cpu.
 
-	// LAB 4: Your code here.
-
-	// sched_halt never returns
-	sched_halt();
+    // LAB 4: Your code here.
+    struct Env *cur_env = thiscpu->cpu_env;
+    int index = 0;
+    if (cur_env) {
+        index = ENVX(cur_env->env_id) % NENV;
+    }
+    for (int i = 0; i < NENV; ++i) {
+        index = (index + 1) % NENV;
+        if (envs[index].env_status == ENV_RUNNABLE) {
+            env_run(&envs[index]);
+            return;
+        }
+    }
+    if (cur_env && cur_env->env_status == ENV_RUNNING) {
+        env_run(cur_env);
+        return;
+    }
+    // sched_halt never returns
+    sched_halt();
 }
 
 // Halt this CPU when there is nothing to do. Wait until the
@@ -40,42 +55,42 @@ sched_yield(void)
 void
 sched_halt(void)
 {
-	int i;
+    int i;
 
-	// For debugging and testing purposes, if there are no runnable
-	// environments in the system, then drop into the kernel monitor.
-	for (i = 0; i < NENV; i++) {
-		if ((envs[i].env_status == ENV_RUNNABLE ||
-		     envs[i].env_status == ENV_RUNNING ||
-		     envs[i].env_status == ENV_DYING))
-			break;
-	}
-	if (i == NENV) {
-		cprintf("No runnable environments in the system!\n");
-		while (1)
-			monitor(NULL);
-	}
+    // For debugging and testing purposes, if there are no runnable
+    // environments in the system, then drop into the kernel monitor.
+    for (i = 0; i < NENV; i++) {
+        if ((envs[i].env_status == ENV_RUNNABLE ||
+             envs[i].env_status == ENV_RUNNING ||
+             envs[i].env_status == ENV_DYING))
+            break;
+    }
+    if (i == NENV) {
+        cprintf("No runnable environments in the system!\n");
+        while (1)
+            monitor(NULL);
+    }
 
-	// Mark that no environment is running on this CPU
-	curenv = NULL;
-	lcr3(PADDR(kern_pgdir));
+    // Mark that no environment is running on this CPU
+    curenv = NULL;
+    lcr3(PADDR(kern_pgdir));
 
-	// Mark that this CPU is in the HALT state, so that when
-	// timer interupts come in, we know we should re-acquire the
-	// big kernel lock
-	xchg(&thiscpu->cpu_status, CPU_HALTED);
+    // Mark that this CPU is in the HALT state, so that when
+    // timer interupts come in, we know we should re-acquire the
+    // big kernel lock
+    xchg(&thiscpu->cpu_status, CPU_HALTED);
 
-	// Release the big kernel lock as if we were "leaving" the kernel
-	unlock_kernel();
+    // Release the big kernel lock as if we were "leaving" the kernel
+    unlock_kernel();
 
-	// Reset stack pointer, enable interrupts and then halt.
-	asm volatile (
-		"movl $0, %%ebp\n"
-		"movl %0, %%esp\n"
-		"pushl $0\n"
-		"pushl $0\n"
-		"sti\n"
-		"hlt\n"
-	: : "a" (thiscpu->cpu_ts.ts_esp0));
+    // Reset stack pointer, enable interrupts and then halt.
+    asm volatile (
+        "movl $0, %%ebp\n"
+        "movl %0, %%esp\n"
+        "pushl $0\n"
+        "pushl $0\n"
+        "sti\n"
+        "hlt\n"
+    : : "a" (thiscpu->cpu_ts.ts_esp0));
 }
 
diff --git a/kern/syscall.c b/kern/syscall.c
index bc702a3..816cf8b 100644
--- a/kern/syscall.c
+++ b/kern/syscall.c
@@ -18,13 +18,14 @@
 static void
 sys_cputs(const char *s, size_t len)
 {
-	// Check that the user has permission to read memory [s, s+len).
-	// Destroy the environment if not.
+    // Check that the user has permission to read memory [s, s+len).
+    // Destroy the environment if not.
 
-	// LAB 3: Your code here.
+    // LAB 3: Your code here.
+    user_mem_assert(curenv, s, len, PTE_U);
 
-	// Print the string supplied by the user.
-	cprintf("%.*s", len, s);
+    // Print the string supplied by the user.
+    cprintf("%.*s", len, s);
 }
 
 // Read a character from the system console without blocking.
@@ -32,14 +33,14 @@ sys_cputs(const char *s, size_t len)
 static int
 sys_cgetc(void)
 {
-	return cons_getc();
+    return cons_getc();
 }
 
 // Returns the current environment's envid.
 static envid_t
 sys_getenvid(void)
 {
-	return curenv->env_id;
+    return curenv->env_id;
 }
 
 // Destroy a given environment (possibly the currently running environment).
@@ -50,20 +51,24 @@ sys_getenvid(void)
 static int
 sys_env_destroy(envid_t envid)
 {
-	int r;
-	struct Env *e;
+    int r;
+    struct Env *e;
 
-	if ((r = envid2env(envid, &e, 1)) < 0)
-		return r;
-	env_destroy(e);
-	return 0;
+    if ((r = envid2env(envid, &e, 1)) < 0)
+        return r;
+    if (e == curenv)
+        cprintf("[%08x] exiting gracefully\n", curenv->env_id);
+    else
+        cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
+    env_destroy(e);
+    return 0;
 }
 
 // Deschedule current environment and pick a different one to run.
 static void
 sys_yield(void)
 {
-	sched_yield();
+    sched_yield();
 }
 
 // Allocate a new environment.
@@ -73,14 +78,24 @@ sys_yield(void)
 static envid_t
 sys_exofork(void)
 {
-	// Create the new environment with env_alloc(), from kern/env.c.
-	// It should be left as env_alloc created it, except that
-	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
-	// from the current environment -- but tweaked so sys_exofork
-	// will appear to return 0.
-
-	// LAB 4: Your code here.
-	panic("sys_exofork not implemented");
+    // Create the new environment with env_alloc(), from kern/env.c.
+    // It should be left as env_alloc created it, except that
+    // status is set to ENV_NOT_RUNNABLE, and the register set is copied
+    // from the current environment -- but tweaked so sys_exofork
+    // will appear to return 0.
+
+    // LAB 4: Your code here.
+    struct Env *env;
+    int r;
+    if ((r = env_alloc(&env, thiscpu->cpu_env->env_id)) < 0) {
+        return r;
+    }
+
+    env->env_status = ENV_NOT_RUNNABLE;
+    env->env_tf = thiscpu->cpu_env->env_tf;
+    env->env_tf.tf_regs.reg_eax = 0;
+
+    return env->env_id;
 }
 
 // Set envid's env_status to status, which must be ENV_RUNNABLE
@@ -93,14 +108,22 @@ sys_exofork(void)
 static int
 sys_env_set_status(envid_t envid, int status)
 {
-	// Hint: Use the 'envid2env' function from kern/env.c to translate an
-	// envid to a struct Env.
-	// You should set envid2env's third argument to 1, which will
-	// check whether the current environment has permission to set
-	// envid's status.
-
-	// LAB 4: Your code here.
-	panic("sys_env_set_status not implemented");
+    // Hint: Use the 'envid2env' function from kern/env.c to translate an
+    // envid to a struct Env.
+    // You should set envid2env's third argument to 1, which will
+    // check whether the current environment has permission to set
+    // envid's status.
+
+    // LAB 4: Your code here.
+    if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE) {
+        return -E_INVAL;
+    }
+    struct Env *env ;
+    if (envid2env(envid, &env, 1)) {
+        return -E_BAD_ENV;
+    }
+    env->env_status = status;
+    return 0;
 }
 
 // Set envid's trap frame to 'tf'.
@@ -113,10 +136,19 @@ sys_env_set_status(envid_t envid, int status)
 static int
 sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
 {
-	// LAB 5: Your code here.
-	// Remember to check whether the user has supplied us with a good
-	// address!
-	panic("sys_env_set_trapframe not implemented");
+    // LAB 5: Your code here.
+    // Remember to check whether the user has supplied us with a good
+    // address!
+    struct Env *env ;
+    if (envid2env(envid, &env, 1)) {
+        return -E_BAD_ENV;
+    }
+    if (user_mem_check(curenv, tf, sizeof(struct Trapframe), PTE_W | PTE_U)) {
+        return -E_BAD_ENV;
+    }
+
+    memmove(&env->env_tf, tf, sizeof(struct Trapframe));
+    return 0;
 }
 
 // Set the page fault upcall for 'envid' by modifying the corresponding struct
@@ -130,8 +162,13 @@ sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
 static int
 sys_env_set_pgfault_upcall(envid_t envid, void *func)
 {
-	// LAB 4: Your code here.
-	panic("sys_env_set_pgfault_upcall not implemented");
+    // LAB 4: Your code here.
+    struct Env *env;
+    if (envid2env(envid, &env, 1)) {
+        return -E_BAD_ENV;
+    }
+    env->env_pgfault_upcall = func;
+    return 0;
 }
 
 // Allocate a page of memory and map it at 'va' with permission
@@ -153,15 +190,38 @@ sys_env_set_pgfault_upcall(envid_t envid, void *func)
 static int
 sys_page_alloc(envid_t envid, void *va, int perm)
 {
-	// Hint: This function is a wrapper around page_alloc() and
-	//   page_insert() from kern/pmap.c.
-	//   Most of the new code you write should be to check the
-	//   parameters for correctness.
-	//   If page_insert() fails, remember to free the page you
-	//   allocated!
-
-	// LAB 4: Your code here.
-	panic("sys_page_alloc not implemented");
+    // Hint: This function is a wrapper around page_alloc() and
+    //   page_insert() from kern/pmap.c.
+    //   Most of the new code you write should be to check the
+    //   parameters for correctness.
+    //   If page_insert() fails, remember to free the page you
+    //   allocated!
+
+    // LAB 4: Your code here.
+    if ((uint32_t)va >= UTOP || (uint32_t)va % PGSIZE) {
+        return -E_INVAL;
+    }
+
+    if (!(perm & PTE_U) || !(perm & PTE_P) || (perm & ~PTE_SYSCALL)) {
+        return -E_INVAL;
+    }
+
+    struct Env *env;
+    if (envid2env(envid, &env, 1)) {
+        return -E_BAD_ENV;
+    }
+
+    struct PageInfo *page = page_alloc(ALLOC_ZERO);
+    if (!page) {
+        return -E_NO_MEM;
+    }
+
+    if (page_insert(env->env_pgdir, page, va, perm)) {
+        page_free(page);
+        return -E_NO_MEM;
+    }
+
+    return 0;
 }
 
 // Map the page of memory at 'srcva' in srcenvid's address space
@@ -182,17 +242,46 @@ sys_page_alloc(envid_t envid, void *va, int perm)
 //	-E_NO_MEM if there's no memory to allocate any necessary page tables.
 static int
 sys_page_map(envid_t srcenvid, void *srcva,
-	     envid_t dstenvid, void *dstva, int perm)
+         envid_t dstenvid, void *dstva, int perm)
 {
-	// Hint: This function is a wrapper around page_lookup() and
-	//   page_insert() from kern/pmap.c.
-	//   Again, most of the new code you write should be to check the
-	//   parameters for correctness.
-	//   Use the third argument to page_lookup() to
-	//   check the current permissions on the page.
-
-	// LAB 4: Your code here.
-	panic("sys_page_map not implemented");
+    // Hint: This function is a wrapper around page_lookup() and
+    //   page_insert() from kern/pmap.c.
+    //   Again, most of the new code you write should be to check the
+    //   parameters for correctness.
+    //   Use the third argument to page_lookup() to
+    //   check the current permissions on the page.
+
+    // LAB 4: Your code here.
+
+    if ((uint32_t)srcva >= UTOP || (uint32_t)srcva % PGSIZE ||
+            (uint32_t)dstva >= UTOP || (uint32_t)dstva % PGSIZE) {
+        return -E_INVAL;
+    }
+
+    if (!(perm & PTE_U) || !(perm & PTE_P) || (perm & ~PTE_SYSCALL)) {
+        return -E_INVAL;
+    }
+
+    struct Env *src_env, *dst_env;
+    if (envid2env(srcenvid, &src_env, 1) || envid2env(dstenvid, &dst_env, 1)) {
+        return -E_BAD_ENV;
+    }
+
+    pte_t *src_pte;
+    struct PageInfo *page = page_lookup(src_env->env_pgdir, srcva, &src_pte);
+    if (!page) {
+        return -E_INVAL;
+    }
+
+    if ((perm & PTE_W) && !(*src_pte & PTE_W)) {
+        return -E_INVAL;
+    }
+
+    if (page_insert(dst_env->env_pgdir, page, dstva, perm)) {
+        return -E_NO_MEM;
+    }
+
+    return 0;
 }
 
 // Unmap the page of memory at 'va' in the address space of 'envid'.
@@ -205,10 +294,20 @@ sys_page_map(envid_t srcenvid, void *srcva,
 static int
 sys_page_unmap(envid_t envid, void *va)
 {
-	// Hint: This function is a wrapper around page_remove().
+    // Hint: This function is a wrapper around page_remove().
+
+    // LAB 4: Your code here.
+    if ((uint32_t)va >= UTOP || (uint32_t)va % PGSIZE) {
+        return -E_INVAL;
+    }
+
+    struct Env *env;
+    if (envid2env(envid, &env, 1)) {
+        return -E_BAD_ENV;
+    }
 
-	// LAB 4: Your code here.
-	panic("sys_page_unmap not implemented");
+    page_remove(env->env_pgdir, va);
+    return 0;
 }
 
 // Try to send 'value' to the target env 'envid'.
@@ -252,8 +351,51 @@ sys_page_unmap(envid_t envid, void *va)
 static int
 sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
 {
-	// LAB 4: Your code here.
-	panic("sys_ipc_try_send not implemented");
+    // LAB 4: Your code here.
+    struct Env *env;
+    if (envid2env(envid, &env, 0)) {
+        return -E_BAD_ENV;
+    }
+
+    if (!env->env_ipc_recving) {
+        return -E_IPC_NOT_RECV;
+    }
+
+    if ((uintptr_t)srcva < UTOP) {
+        if ((uintptr_t)srcva % PGSIZE) {
+            return -E_INVAL;
+        }
+
+        if (!(perm & PTE_U) || !(perm & PTE_P) || (perm & ~PTE_SYSCALL)) {
+            return -E_INVAL;
+        }
+
+        pte_t *pte;
+        struct PageInfo *page = page_lookup(curenv->env_pgdir, srcva, &pte);
+        if (!page) {
+            return -E_INVAL;
+        }
+
+        if ((perm & PTE_W) && !(*pte & PTE_W)) {
+            return -E_INVAL;
+        }
+
+
+        if ((uintptr_t)env->env_ipc_dstva < UTOP &&
+                page_insert(env->env_pgdir, page, env->env_ipc_dstva, perm)) {
+            return -E_NO_MEM;
+        }
+        env->env_ipc_perm = perm;
+    } else {
+        env->env_ipc_perm = 0;
+    }
+
+    env->env_ipc_recving = 0;
+    env->env_ipc_from = curenv->env_id;
+    env->env_ipc_value = value;
+    env->env_status = ENV_RUNNABLE;
+
+    return 0;
 }
 
 // Block until a value is ready.  Record that you want to receive
@@ -270,19 +412,66 @@ sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
 static int
 sys_ipc_recv(void *dstva)
 {
-	// LAB 4: Your code here.
-	panic("sys_ipc_recv not implemented");
-	return 0;
+    // LAB 4: Your code here.
+    if ((uintptr_t)dstva < UTOP) {
+        if ((uintptr_t)dstva % PGSIZE) {
+            return -E_INVAL;
+        }
+
+        curenv->env_ipc_dstva = dstva;
+    } else {
+        curenv->env_ipc_dstva = (void *)UTOP;
+    }
+
+    curenv->env_ipc_recving = 1;
+    curenv->env_status = ENV_NOT_RUNNABLE;
+    curenv->env_tf.tf_regs.reg_eax = 0;
+
+    sys_yield();
+
+    return 0;
 }
 
 // Dispatches to the correct kernel function, passing the arguments.
 int32_t
 syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
 {
-	// Call the function corresponding to the 'syscallno' parameter.
-	// Return any appropriate return value.
-	// LAB 3: Your code here.
-
-	panic("syscall not implemented");
+    // Call the function corresponding to the 'syscallno' parameter.
+    // Return any appropriate return value.
+    // LAB 3: Your code here.
+    switch (syscallno) {
+        case SYS_cputs:
+            sys_cputs((char*)a1, a2);
+            return 0;
+        case SYS_cgetc:
+            return sys_cgetc();
+        case SYS_getenvid:
+            return sys_getenvid();
+        case SYS_env_destroy:
+            return sys_env_destroy(a1);
+        case SYS_yield:
+            sys_yield();
+            return 0;
+        case SYS_exofork:
+            return sys_exofork();
+        case SYS_env_set_status:
+            return sys_env_set_status(a1, a2);
+        case SYS_page_alloc:
+            return sys_page_alloc(a1, (void*) a2, a3);
+        case SYS_page_map:
+            return sys_page_map(a1, (void*) a2, a3, (void*) a4, a5);
+        case SYS_page_unmap:
+            return sys_page_unmap(a1, (void*) a2);
+        case SYS_env_set_pgfault_upcall:
+            return sys_env_set_pgfault_upcall(a1, (void*) a2);
+        case SYS_ipc_try_send:
+            return sys_ipc_try_send(a1, a2, (void*) a3, a4);
+        case SYS_ipc_recv:
+            return sys_ipc_recv((void*) a1);
+        case SYS_env_set_trapframe:
+            return sys_env_set_trapframe(a1, (struct Trapframe*) a2);
+        default:
+            return -E_INVAL;
+    }
 }
 
diff --git a/kern/trap.c b/kern/trap.c
index a4fb4dd..299894c 100644
--- a/kern/trap.c
+++ b/kern/trap.c
@@ -27,288 +27,452 @@ static struct Trapframe *last_tf;
  */
 struct Gatedesc idt[256] = { { 0 } };
 struct Pseudodesc idt_pd = {
-	sizeof(idt) - 1, (uint32_t) idt
+    sizeof(idt) - 1, (uint32_t) idt
 };
 
 
 static const char *trapname(int trapno)
 {
-	static const char * const excnames[] = {
-		"Divide error",
-		"Debug",
-		"Non-Maskable Interrupt",
-		"Breakpoint",
-		"Overflow",
-		"BOUND Range Exceeded",
-		"Invalid Opcode",
-		"Device Not Available",
-		"Double Fault",
-		"Coprocessor Segment Overrun",
-		"Invalid TSS",
-		"Segment Not Present",
-		"Stack Fault",
-		"General Protection",
-		"Page Fault",
-		"(unknown trap)",
-		"x87 FPU Floating-Point Error",
-		"Alignment Check",
-		"Machine-Check",
-		"SIMD Floating-Point Exception"
-	};
-
-	if (trapno < sizeof(excnames)/sizeof(excnames[0]))
-		return excnames[trapno];
-	if (trapno == T_SYSCALL)
-		return "System call";
-	if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16)
-		return "Hardware Interrupt";
-	return "(unknown trap)";
+    static const char * const excnames[] = {
+        "Divide error",
+        "Debug",
+        "Non-Maskable Interrupt",
+        "Breakpoint",
+        "Overflow",
+        "BOUND Range Exceeded",
+        "Invalid Opcode",
+        "Device Not Available",
+        "Double Fault",
+        "Coprocessor Segment Overrun",
+        "Invalid TSS",
+        "Segment Not Present",
+        "Stack Fault",
+        "General Protection",
+        "Page Fault",
+        "(unknown trap)",
+        "x87 FPU Floating-Point Error",
+        "Alignment Check",
+        "Machine-Check",
+        "SIMD Floating-Point Exception"
+    };
+
+    if (trapno < sizeof(excnames)/sizeof(excnames[0]))
+        return excnames[trapno];
+    if (trapno == T_SYSCALL)
+        return "System call";
+    if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16)
+        return "Hardware Interrupt";
+    return "(unknown trap)";
 }
 
 
 void
 trap_init(void)
 {
-	extern struct Segdesc gdt[];
+    extern struct Segdesc gdt[];
 
-	// LAB 3: Your code here.
+    // LAB 3: Your code here.
+    int DEFAULT = 0;
+    int USER = 3;
 
-	// Per-CPU setup 
-	trap_init_percpu();
+    extern void handler_0();
+    SETGATE(idt[0], 0, GD_KT, handler_0, DEFAULT);
+
+    extern void handler_1();
+    SETGATE(idt[1], 0, GD_KT, handler_1, DEFAULT);
+
+    extern void handler_2();
+    SETGATE(idt[2], 0, GD_KT, handler_2, DEFAULT);
+
+    extern void handler_3();
+    SETGATE(idt[3], 0, GD_KT, handler_3, USER);
+
+    extern void handler_4();
+    SETGATE(idt[4], 0, GD_KT, handler_4, DEFAULT);
+
+    extern void handler_5();
+    SETGATE(idt[5], 0, GD_KT, handler_5, DEFAULT);
+
+    extern void handler_6();
+    SETGATE(idt[6], 0, GD_KT, handler_6, DEFAULT);
+
+    extern void handler_7();
+    SETGATE(idt[7], 0, GD_KT, handler_7, DEFAULT);
+
+    extern void handler_8();
+    SETGATE(idt[8], 0, GD_KT, handler_8, DEFAULT);
+
+    extern void handler_10();
+    SETGATE(idt[10], 0, GD_KT, handler_10, DEFAULT);
+
+    extern void handler_11();
+    SETGATE(idt[11], 0, GD_KT, handler_11, DEFAULT);
+
+    extern void handler_12();
+    SETGATE(idt[12], 0, GD_KT, handler_12, DEFAULT);
+
+    extern void handler_13();
+    SETGATE(idt[13], 0, GD_KT, handler_13, DEFAULT);
+
+    extern void handler_14();
+    SETGATE(idt[14], 0, GD_KT, handler_14, DEFAULT);
+
+    extern void handler_16();
+    SETGATE(idt[16], 0, GD_KT, handler_16, DEFAULT);
+
+    extern void handler_17();
+    SETGATE(idt[17], 0, GD_KT, handler_17, DEFAULT);
+
+    extern void handler_18();
+    SETGATE(idt[18], 0, GD_KT, handler_18, DEFAULT);
+
+    extern void handler_19();
+    SETGATE(idt[19], 0, GD_KT, handler_19, DEFAULT);
+
+    extern void handler_32();
+    SETGATE(idt[32], 0, GD_KT, handler_32, DEFAULT);
+
+    extern void handler_33();
+    SETGATE(idt[33], 0, GD_KT, handler_33, DEFAULT);
+
+    extern void handler_34();
+    SETGATE(idt[34], 0, GD_KT, handler_34, DEFAULT);
+
+    extern void handler_35();
+    SETGATE(idt[35], 0, GD_KT, handler_35, DEFAULT);
+
+    extern void handler_36();
+    SETGATE(idt[36], 0, GD_KT, handler_36, DEFAULT);
+
+    extern void handler_37();
+    SETGATE(idt[37], 0, GD_KT, handler_37, DEFAULT);
+
+    extern void handler_38();
+    SETGATE(idt[38], 0, GD_KT, handler_38, DEFAULT);
+
+    extern void handler_39();
+    SETGATE(idt[39], 0, GD_KT, handler_39, DEFAULT);
+
+    extern void handler_40();
+    SETGATE(idt[40], 0, GD_KT, handler_40, DEFAULT);
+
+    extern void handler_41();
+    SETGATE(idt[41], 0, GD_KT, handler_41, DEFAULT);
+
+    extern void handler_42();
+    SETGATE(idt[42], 0, GD_KT, handler_42, DEFAULT);
+
+    extern void handler_43();
+    SETGATE(idt[43], 0, GD_KT, handler_43, DEFAULT);
+
+    extern void handler_44();
+    SETGATE(idt[44], 0, GD_KT, handler_44, DEFAULT);
+
+    extern void handler_45();
+    SETGATE(idt[45], 0, GD_KT, handler_45, DEFAULT);
+
+    extern void handler_46();
+    SETGATE(idt[46], 0, GD_KT, handler_46, DEFAULT);
+
+    extern void handler_47();
+    SETGATE(idt[47], 0, GD_KT, handler_47, DEFAULT);
+
+    extern void handler_48();
+    SETGATE(idt[48], 0, GD_KT, handler_48, USER);
+
+    // Per-CPU setup
+    trap_init_percpu();
 }
 
 // Initialize and load the per-CPU TSS and IDT
 void
 trap_init_percpu(void)
 {
-	// The example code here sets up the Task State Segment (TSS) and
-	// the TSS descriptor for CPU 0. But it is incorrect if we are
-	// running on other CPUs because each CPU has its own kernel stack.
-	// Fix the code so that it works for all CPUs.
-	//
-	// Hints:
-	//   - The macro "thiscpu" always refers to the current CPU's
-	//     struct CpuInfo;
-	//   - The ID of the current CPU is given by cpunum() or
-	//     thiscpu->cpu_id;
-	//   - Use "thiscpu->cpu_ts" as the TSS for the current CPU,
-	//     rather than the global "ts" variable;
-	//   - Use gdt[(GD_TSS0 >> 3) + i] for CPU i's TSS descriptor;
-	//   - You mapped the per-CPU kernel stacks in mem_init_mp()
-	//
-	// ltr sets a 'busy' flag in the TSS selector, so if you
-	// accidentally load the same TSS on more than one CPU, you'll
-	// get a triple fault.  If you set up an individual CPU's TSS
-	// wrong, you may not get a fault until you try to return from
-	// user space on that CPU.
-	//
-	// LAB 4: Your code here:
-
-	// Setup a TSS so that we get the right stack
-	// when we trap to the kernel.
-	ts.ts_esp0 = KSTACKTOP;
-	ts.ts_ss0 = GD_KD;
-
-	// Initialize the TSS slot of the gdt.
-	gdt[GD_TSS0 >> 3] = SEG16(STS_T32A, (uint32_t) (&ts),
-					sizeof(struct Taskstate), 0);
-	gdt[GD_TSS0 >> 3].sd_s = 0;
-
-	// Load the TSS selector (like other segment selectors, the
-	// bottom three bits are special; we leave them 0)
-	ltr(GD_TSS0);
-
-	// Load the IDT
-	lidt(&idt_pd);
+    // The example code here sets up the Task State Segment (TSS) and
+    // the TSS descriptor for CPU 0. But it is incorrect if we are
+    // running on other CPUs because each CPU has its own kernel stack.
+    // Fix the code so that it works for all CPUs.
+    //
+    // Hints:
+    //   - The macro "thiscpu" always refers to the current CPU's
+    //     struct CpuInfo;
+    //   - The ID of the current CPU is given by cpunum() or
+    //     thiscpu->cpu_id;
+    //   - Use "thiscpu->cpu_ts" as the TSS for the current CPU,
+    //     rather than the global "ts" variable;
+    //   - Use gdt[(GD_TSS0 >> 3) + i] for CPU i's TSS descriptor;
+    //   - You mapped the per-CPU kernel stacks in mem_init_mp()
+    //
+    // ltr sets a 'busy' flag in the TSS selector, so if you
+    // accidentally load the same TSS on more than one CPU, you'll
+    // get a triple fault.  If you set up an individual CPU's TSS
+    // wrong, you may not get a fault until you try to return from
+    // user space on that CPU.
+    //
+    // LAB 4: Your code here:
+
+    uint8_t cpu_n = cpunum();
+
+    // Setup a TSS so that we get the right stack
+    // when we trap to the kernel.
+    thiscpu->cpu_ts.ts_esp0 = KSTACKTOP - cpu_n * (KSTKSIZE + KSTKGAP);
+    thiscpu->cpu_ts.ts_ss0 = GD_KD;
+
+    uint16_t offset = GD_TSS0 + (cpu_n << 3);
+    uint16_t index = offset >> 3;
+
+    // Initialize the TSS slot of the gdt.
+    gdt[index] = SEG16(STS_T32A, (uint32_t) (&thiscpu->cpu_ts),
+                    sizeof(struct Taskstate), 0);
+    gdt[index].sd_s = 0;
+
+    // Load the TSS selector (like other segment selectors, the
+    // bottom three bits are special; we leave them 0)
+    ltr(offset);
+
+    // Load the IDT
+    lidt(&idt_pd);
 }
 
 void
 print_trapframe(struct Trapframe *tf)
 {
-	cprintf("TRAP frame at %p from CPU %d\n", tf, cpunum());
-	print_regs(&tf->tf_regs);
-	cprintf("  es   0x----%04x\n", tf->tf_es);
-	cprintf("  ds   0x----%04x\n", tf->tf_ds);
-	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
-	// If this trap was a page fault that just happened
-	// (so %cr2 is meaningful), print the faulting linear address.
-	if (tf == last_tf && tf->tf_trapno == T_PGFLT)
-		cprintf("  cr2  0x%08x\n", rcr2());
-	cprintf("  err  0x%08x", tf->tf_err);
-	// For page faults, print decoded fault error code:
-	// U/K=fault occurred in user/kernel mode
-	// W/R=a write/read caused the fault
-	// PR=a protection violation caused the fault (NP=page not present).
-	if (tf->tf_trapno == T_PGFLT)
-		cprintf(" [%s, %s, %s]\n",
-			tf->tf_err & 4 ? "user" : "kernel",
-			tf->tf_err & 2 ? "write" : "read",
-			tf->tf_err & 1 ? "protection" : "not-present");
-	else
-		cprintf("\n");
-	cprintf("  eip  0x%08x\n", tf->tf_eip);
-	cprintf("  cs   0x----%04x\n", tf->tf_cs);
-	cprintf("  flag 0x%08x\n", tf->tf_eflags);
-	if ((tf->tf_cs & 3) != 0) {
-		cprintf("  esp  0x%08x\n", tf->tf_esp);
-		cprintf("  ss   0x----%04x\n", tf->tf_ss);
-	}
+    cprintf("TRAP frame at %p from CPU %d\n", tf, cpunum());
+    print_regs(&tf->tf_regs);
+    cprintf("  es   0x----%04x\n", tf->tf_es);
+    cprintf("  ds   0x----%04x\n", tf->tf_ds);
+    cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
+    // If this trap was a page fault that just happened
+    // (so %cr2 is meaningful), print the faulting linear address.
+    if (tf == last_tf && tf->tf_trapno == T_PGFLT)
+        cprintf("  cr2  0x%08x\n", rcr2());
+    cprintf("  err  0x%08x", tf->tf_err);
+    // For page faults, print decoded fault error code:
+    // U/K=fault occurred in user/kernel mode
+    // W/R=a write/read caused the fault
+    // PR=a protection violation caused the fault (NP=page not present).
+    if (tf->tf_trapno == T_PGFLT)
+        cprintf(" [%s, %s, %s]\n",
+            tf->tf_err & 4 ? "user" : "kernel",
+            tf->tf_err & 2 ? "write" : "read",
+            tf->tf_err & 1 ? "protection" : "not-present");
+    else
+        cprintf("\n");
+    cprintf("  eip  0x%08x\n", tf->tf_eip);
+    cprintf("  cs   0x----%04x\n", tf->tf_cs);
+    cprintf("  flag 0x%08x\n", tf->tf_eflags);
+    if ((tf->tf_cs & 3) != 0) {
+        cprintf("  esp  0x%08x\n", tf->tf_esp);
+        cprintf("  ss   0x----%04x\n", tf->tf_ss);
+    }
 }
 
 void
 print_regs(struct PushRegs *regs)
 {
-	cprintf("  edi  0x%08x\n", regs->reg_edi);
-	cprintf("  esi  0x%08x\n", regs->reg_esi);
-	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
-	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
-	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
-	cprintf("  edx  0x%08x\n", regs->reg_edx);
-	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
-	cprintf("  eax  0x%08x\n", regs->reg_eax);
+    cprintf("  edi  0x%08x\n", regs->reg_edi);
+    cprintf("  esi  0x%08x\n", regs->reg_esi);
+    cprintf("  ebp  0x%08x\n", regs->reg_ebp);
+    cprintf("  oesp 0x%08x\n", regs->reg_oesp);
+    cprintf("  ebx  0x%08x\n", regs->reg_ebx);
+    cprintf("  edx  0x%08x\n", regs->reg_edx);
+    cprintf("  ecx  0x%08x\n", regs->reg_ecx);
+    cprintf("  eax  0x%08x\n", regs->reg_eax);
 }
 
 static void
 trap_dispatch(struct Trapframe *tf)
 {
-	// Handle processor exceptions.
-	// LAB 3: Your code here.
-
-	// Handle spurious interrupts
-	// The hardware sometimes raises these because of noise on the
-	// IRQ line or other reasons. We don't care.
-	if (tf->tf_trapno == IRQ_OFFSET + IRQ_SPURIOUS) {
-		cprintf("Spurious interrupt on irq 7\n");
-		print_trapframe(tf);
-		return;
-	}
-
-	// Handle clock interrupts. Don't forget to acknowledge the
-	// interrupt using lapic_eoi() before calling the scheduler!
-	// LAB 4: Your code here.
-
-	// Handle keyboard and serial interrupts.
-	// LAB 5: Your code here.
-
-	// Unexpected trap: The user process or the kernel has a bug.
-	print_trapframe(tf);
-	if (tf->tf_cs == GD_KT)
-		panic("unhandled trap in kernel");
-	else {
-		env_destroy(curenv);
-		return;
-	}
+    // Handle processor exceptions.
+    // LAB 3: Your code here.
+    if (tf->tf_trapno == T_PGFLT) {
+        page_fault_handler(tf);
+        return;
+    }
+    if (tf->tf_trapno == T_BRKPT) {
+        print_trapframe(tf);
+        monitor(tf);
+        return;
+    }
+    if (tf->tf_trapno == T_SYSCALL) {
+        tf->tf_regs.reg_eax = (uint32_t) syscall(
+                        tf->tf_regs.reg_eax,
+                        tf->tf_regs.reg_edx,
+                        tf->tf_regs.reg_ecx,
+                        tf->tf_regs.reg_ebx,
+                        tf->tf_regs.reg_edi,
+                        tf->tf_regs.reg_esi);
+        return;
+    }
+
+    // Handle spurious interrupts
+    // The hardware sometimes raises these because of noise on the
+    // IRQ line or other reasons. We don't care.
+    if (tf->tf_trapno == IRQ_OFFSET + IRQ_SPURIOUS) {
+        cprintf("Spurious interrupt on irq 7\n");
+        print_trapframe(tf);
+        return;
+    }
+
+    // Handle clock interrupts. Don't forget to acknowledge the
+    // interrupt using lapic_eoi() before calling the scheduler!
+    // LAB 4: Your code here.
+    if (tf->tf_trapno == IRQ_OFFSET + IRQ_TIMER) {
+        lapic_eoi();
+        sched_yield();
+        return;
+    }
+
+    // Handle keyboard and serial interrupts.
+    // LAB 5: Your code here.
+
+    // Unexpected trap: The user process or the kernel has a bug.
+    print_trapframe(tf);
+    if (tf->tf_cs == GD_KT)
+        panic("unhandled trap in kernel");
+    else {
+        env_destroy(curenv);
+        return;
+    }
 }
 
 void
 trap(struct Trapframe *tf)
 {
-	// The environment may have set DF and some versions
-	// of GCC rely on DF being clear
-	asm volatile("cld" ::: "cc");
-
-	// Halt the CPU if some other CPU has called panic()
-	extern char *panicstr;
-	if (panicstr)
-		asm volatile("hlt");
-
-	// Re-acqurie the big kernel lock if we were halted in
-	// sched_yield()
-	if (xchg(&thiscpu->cpu_status, CPU_STARTED) == CPU_HALTED)
-		lock_kernel();
-	// Check that interrupts are disabled.  If this assertion
-	// fails, DO NOT be tempted to fix it by inserting a "cli" in
-	// the interrupt path.
-	assert(!(read_eflags() & FL_IF));
-
-	if ((tf->tf_cs & 3) == 3) {
-		// Trapped from user mode.
-		// Acquire the big kernel lock before doing any
-		// serious kernel work.
-		// LAB 4: Your code here.
-		assert(curenv);
-
-		// Garbage collect if current enviroment is a zombie
-		if (curenv->env_status == ENV_DYING) {
-			env_free(curenv);
-			curenv = NULL;
-			sched_yield();
-		}
-
-		// Copy trap frame (which is currently on the stack)
-		// into 'curenv->env_tf', so that running the environment
-		// will restart at the trap point.
-		curenv->env_tf = *tf;
-		// The trapframe on the stack should be ignored from here on.
-		tf = &curenv->env_tf;
-	}
-
-	// Record that tf is the last real trapframe so
-	// print_trapframe can print some additional information.
-	last_tf = tf;
-
-	// Dispatch based on what type of trap occurred
-	trap_dispatch(tf);
-
-	// If we made it to this point, then no other environment was
-	// scheduled, so we should return to the current environment
-	// if doing so makes sense.
-	if (curenv && curenv->env_status == ENV_RUNNING)
-		env_run(curenv);
-	else
-		sched_yield();
+    // The environment may have set DF and some versions
+    // of GCC rely on DF being clear
+    asm volatile("cld" ::: "cc");
+
+    // Halt the CPU if some other CPU has called panic()
+    extern char *panicstr;
+    if (panicstr)
+        asm volatile("hlt");
+
+    // Re-acqurie the big kernel lock if we were halted in
+    // sched_yield()
+    if (xchg(&thiscpu->cpu_status, CPU_STARTED) == CPU_HALTED)
+        lock_kernel();
+    // Check that interrupts are disabled.  If this assertion
+    // fails, DO NOT be tempted to fix it by inserting a "cli" in
+    // the interrupt path.
+    assert(!(read_eflags() & FL_IF));
+
+    if ((tf->tf_cs & 3) == 3) {
+        // Trapped from user mode.
+        // Acquire the big kernel lock before doing any
+        // serious kernel work.
+        // LAB 4: Your code here.
+        assert(curenv);
+        lock_kernel();
+
+        // Garbage collect if current enviroment is a zombie
+        if (curenv->env_status == ENV_DYING) {
+            env_free(curenv);
+            curenv = NULL;
+            sched_yield();
+        }
+
+        // Copy trap frame (which is currently on the stack)
+        // into 'curenv->env_tf', so that running the environment
+        // will restart at the trap point.
+        curenv->env_tf = *tf;
+        // The trapframe on the stack should be ignored from here on.
+        tf = &curenv->env_tf;
+    }
+
+    // Record that tf is the last real trapframe so
+    // print_trapframe can print some additional information.
+    last_tf = tf;
+
+    // Dispatch based on what type of trap occurred
+    trap_dispatch(tf);
+
+    // If we made it to this point, then no other environment was
+    // scheduled, so we should return to the current environment
+    // if doing so makes sense.
+    if (curenv && curenv->env_status == ENV_RUNNING)
+        env_run(curenv);
+    else
+        sched_yield();
 }
 
 
 void
 page_fault_handler(struct Trapframe *tf)
 {
-	uint32_t fault_va;
-
-	// Read processor's CR2 register to find the faulting address
-	fault_va = rcr2();
-
-	// Handle kernel-mode page faults.
-
-	// LAB 3: Your code here.
-
-	// We've already handled kernel-mode exceptions, so if we get here,
-	// the page fault happened in user mode.
-
-	// Call the environment's page fault upcall, if one exists.  Set up a
-	// page fault stack frame on the user exception stack (below
-	// UXSTACKTOP), then branch to curenv->env_pgfault_upcall.
-	//
-	// The page fault upcall might cause another page fault, in which case
-	// we branch to the page fault upcall recursively, pushing another
-	// page fault stack frame on top of the user exception stack.
-	//
-	// The trap handler needs one word of scratch space at the top of the
-	// trap-time stack in order to return.  In the non-recursive case, we
-	// don't have to worry about this because the top of the regular user
-	// stack is free.  In the recursive case, this means we have to leave
-	// an extra word between the current top of the exception stack and
-	// the new stack frame because the exception stack _is_ the trap-time
-	// stack.
-	//
-	// If there's no page fault upcall, the environment didn't allocate a
-	// page for its exception stack or can't write to it, or the exception
-	// stack overflows, then destroy the environment that caused the fault.
-	// Note that the grade script assumes you will first check for the page
-	// fault upcall and print the "user fault va" message below if there is
-	// none.  The remaining three checks can be combined into a single test.
-	//
-	// Hints:
-	//   user_mem_assert() and env_run() are useful here.
-	//   To change what the user environment runs, modify 'curenv->env_tf'
-	//   (the 'tf' variable points at 'curenv->env_tf').
-
-	// LAB 4: Your code here.
-
-	// Destroy the environment that caused the fault.
-	cprintf("[%08x] user fault va %08x ip %08x\n",
-		curenv->env_id, fault_va, tf->tf_eip);
-	print_trapframe(tf);
-	env_destroy(curenv);
+    uint32_t fault_va;
+
+    // Read processor's CR2 register to find the faulting address
+    fault_va = rcr2();
+
+    // Handle kernel-mode page faults.
+
+    // LAB 3: Your code here.
+    if ((tf->tf_cs & 0x3) == 0) {
+        print_trapframe(tf);
+        panic("page fault in kernel space");
+    }
+    // We've already handled kernel-mode exceptions, so if we get here,
+    // the page fault happened in user mode.
+
+    // Call the environment's page fault upcall, if one exists.  Set up a
+    // page fault stack frame on the user exception stack (below
+    // UXSTACKTOP), then branch to curenv->env_pgfault_upcall.
+    //
+    // The page fault upcall might cause another page fault, in which case
+    // we branch to the page fault upcall recursively, pushing another
+    // page fault stack frame on top of the user exception stack.
+    //
+    // The trap handler needs one word of scratch space at the top of the
+    // trap-time stack in order to return.  In the non-recursive case, we
+    // don't have to worry about this because the top of the regular user
+    // stack is free.  In the recursive case, this means we have to leave
+    // an extra word between the current top of the exception stack and
+    // the new stack frame because the exception stack _is_ the trap-time
+    // stack.
+    //
+    // If there's no page fault upcall, the environment didn't allocate a
+    // page for its exception stack or can't write to it, or the exception
+    // stack overflows, then destroy the environment that caused the fault.
+    // Note that the grade script assumes you will first check for the page
+    // fault upcall and print the "user fault va" message below if there is
+    // none.  The remaining three checks can be combined into a single test.
+    //
+    // Hints:
+    //   user_mem_assert() and env_run() are useful here.
+    //   To change what the user environment runs, modify 'curenv->env_tf'
+    //   (the 'tf' variable points at 'curenv->env_tf').
+
+    // LAB 4: Your code here.
+    if (curenv->env_pgfault_upcall && (tf->tf_esp < USTACKTOP || tf->tf_esp >= UXSTACKTOP - PGSIZE)) {
+        uint32_t top;
+        if (tf->tf_esp < UXSTACKTOP && tf->tf_esp >= UXSTACKTOP - PGSIZE) {
+            // recursive
+            top = tf->tf_esp - sizeof(struct UTrapframe) - 4;
+        } else {
+            top = UXSTACKTOP - sizeof(struct UTrapframe);
+        }
+
+        user_mem_assert(curenv, (void *)top, UXSTACKTOP - top, PTE_W | PTE_U);
+
+        struct UTrapframe *utf = (struct UTrapframe*)top;
+        utf->utf_eflags = tf->tf_eflags;
+        utf->utf_eip = tf->tf_eip;
+        utf->utf_err = tf->tf_err;
+        utf->utf_esp = tf->tf_esp;
+        utf->utf_fault_va = fault_va;
+        utf->utf_regs = tf->tf_regs;
+
+        // Run current env with user-page fault handler.
+        tf->tf_eip = (uint32_t)curenv->env_pgfault_upcall;
+        tf->tf_esp = top;
+        env_run(curenv);
+    }
+
+    // Destroy the environment that caused the fault.
+    cprintf("[%08x] user fault va %08x ip %08x\n",
+        curenv->env_id, fault_va, tf->tf_eip);
+    print_trapframe(tf);
+    env_destroy(curenv);
 }
 
diff --git a/kern/trapentry.S b/kern/trapentry.S
index 2dbeeca..3e74588 100644
--- a/kern/trapentry.S
+++ b/kern/trapentry.S
@@ -47,10 +47,59 @@
 /*
  * Lab 3: Your code here for generating entry points for the different traps.
  */
+TRAPHANDLER_NOEC(handler_0, 0)
+TRAPHANDLER_NOEC(handler_1, 1)
+TRAPHANDLER_NOEC(handler_2, 2)
+TRAPHANDLER_NOEC(handler_3, 3)
+TRAPHANDLER_NOEC(handler_4, 4)
+TRAPHANDLER_NOEC(handler_5, 5)
+TRAPHANDLER_NOEC(handler_6, 6)
+TRAPHANDLER_NOEC(handler_7, 7)
+TRAPHANDLER_NOEC(handler_8, 8)
+// 9
+TRAPHANDLER(handler_10, 10)
+TRAPHANDLER(handler_11, 11)
+TRAPHANDLER(handler_12, 12)
+TRAPHANDLER(handler_13, 13)
+TRAPHANDLER(handler_14, 14)
+// 15
+TRAPHANDLER_NOEC(handler_16, 16)
+TRAPHANDLER(handler_17, 17)
+TRAPHANDLER_NOEC(handler_18, 18)
+TRAPHANDLER_NOEC(handler_19, 19)
 
+TRAPHANDLER_NOEC(handler_32, 32)
+TRAPHANDLER_NOEC(handler_33, 33)
+TRAPHANDLER_NOEC(handler_34, 34)
+TRAPHANDLER_NOEC(handler_35, 35)
+TRAPHANDLER_NOEC(handler_36, 36)
+TRAPHANDLER_NOEC(handler_37, 37)
+TRAPHANDLER_NOEC(handler_38, 38)
+TRAPHANDLER_NOEC(handler_39, 39)
+TRAPHANDLER_NOEC(handler_40, 40)
+TRAPHANDLER_NOEC(handler_41, 41)
+TRAPHANDLER_NOEC(handler_42, 42)
+TRAPHANDLER_NOEC(handler_43, 43)
+TRAPHANDLER_NOEC(handler_44, 44)
+TRAPHANDLER_NOEC(handler_45, 45)
+TRAPHANDLER_NOEC(handler_46, 46)
+TRAPHANDLER_NOEC(handler_47, 47)
+
+TRAPHANDLER_NOEC(handler_48, 48)
 
 
 /*
  * Lab 3: Your code here for _alltraps
  */
 
+_alltraps:
+	pushl %ds
+	pushl %es
+	pushal
+
+	mov $GD_KD, %eax
+	mov %ax, %ds
+	mov %ax, %es
+
+	pushl %esp
+	call trap
diff --git a/lib/console.c b/lib/console.c
index fe35ff7..204378c 100644
--- a/lib/console.c
+++ b/lib/console.c
@@ -5,28 +5,28 @@
 void
 cputchar(int ch)
 {
-	char c = ch;
+    char c = ch;
 
-	// Unlike standard Unix's putchar,
-	// the cputchar function _always_ outputs to the system console.
-	sys_cputs(&c, 1);
+    // Unlike standard Unix's putchar,
+    // the cputchar function _always_ outputs to the system console.
+    sys_cputs(&c, 1);
 }
 
 int
 getchar(void)
 {
-	unsigned char c;
-	int r;
-
-	// JOS does, however, support standard _input_ redirection,
-	// allowing the user to redirect script files to the shell and such.
-	// getchar() reads a character from file descriptor 0.
-	r = read(0, &c, 1);
-	if (r < 0)
-		return r;
-	if (r < 1)
-		return -E_EOF;
-	return c;
+    unsigned char c;
+    int r;
+
+    // JOS does, however, support standard _input_ redirection,
+    // allowing the user to redirect script files to the shell and such.
+    // getchar() reads a character from file descriptor 0.
+    r = read(0, &c, 1);
+    if (r < 0)
+        return r;
+    if (r < 1)
+        return -E_EOF;
+    return c;
 }
 
 
@@ -41,88 +41,88 @@ static int devcons_stat(struct Fd*, struct Stat*);
 
 struct Dev devcons =
 {
-	.dev_id =	'c',
-	.dev_name =	"cons",
-	.dev_read =	devcons_read,
-	.dev_write =	devcons_write,
-	.dev_close =	devcons_close,
-	.dev_stat =	devcons_stat
+    .dev_id =	'c',
+    .dev_name =	"cons",
+    .dev_read =	devcons_read,
+    .dev_write =	devcons_write,
+    .dev_close =	devcons_close,
+    .dev_stat =	devcons_stat
 };
 
 int
 iscons(int fdnum)
 {
-	int r;
-	struct Fd *fd;
+    int r;
+    struct Fd *fd;
 
-	if ((r = fd_lookup(fdnum, &fd)) < 0)
-		return r;
-	return fd->fd_dev_id == devcons.dev_id;
+    if ((r = fd_lookup(fdnum, &fd)) < 0)
+        return r;
+    return fd->fd_dev_id == devcons.dev_id;
 }
 
 int
 opencons(void)
 {
-	int r;
-	struct Fd* fd;
-
-	if ((r = fd_alloc(&fd)) < 0)
-		return r;
-	if ((r = sys_page_alloc(0, fd, PTE_P|PTE_U|PTE_W|PTE_SHARE)) < 0)
-		return r;
-	fd->fd_dev_id = devcons.dev_id;
-	fd->fd_omode = O_RDWR;
-	return fd2num(fd);
+    int r;
+    struct Fd* fd;
+
+    if ((r = fd_alloc(&fd)) < 0)
+        return r;
+    if ((r = sys_page_alloc(0, fd, PTE_P|PTE_U|PTE_W|PTE_SHARE)) < 0)
+        return r;
+    fd->fd_dev_id = devcons.dev_id;
+    fd->fd_omode = O_RDWR;
+    return fd2num(fd);
 }
 
 static ssize_t
 devcons_read(struct Fd *fd, void *vbuf, size_t n)
 {
-	int c;
-
-	if (n == 0)
-		return 0;
-
-	while ((c = sys_cgetc()) == 0)
-		sys_yield();
-	if (c < 0)
-		return c;
-	if (c == 0x04)	// ctl-d is eof
-		return 0;
-	*(char*)vbuf = c;
-	return 1;
+    int c;
+
+    if (n == 0)
+        return 0;
+
+    while ((c = sys_cgetc()) == 0)
+        sys_yield();
+    if (c < 0)
+        return c;
+    if (c == 0x04)	// ctl-d is eof
+        return 0;
+    *(char*)vbuf = c;
+    return 1;
 }
 
 static ssize_t
 devcons_write(struct Fd *fd, const void *vbuf, size_t n)
 {
-	int tot, m;
-	char buf[128];
-
-	// mistake: have to nul-terminate arg to sys_cputs,
-	// so we have to copy vbuf into buf in chunks and nul-terminate.
-	for (tot = 0; tot < n; tot += m) {
-		m = n - tot;
-		if (m > sizeof(buf) - 1)
-			m = sizeof(buf) - 1;
-		memmove(buf, (char*)vbuf + tot, m);
-		sys_cputs(buf, m);
-	}
-	return tot;
+    int tot, m;
+    char buf[128];
+
+    // mistake: have to nul-terminate arg to sys_cputs,
+    // so we have to copy vbuf into buf in chunks and nul-terminate.
+    for (tot = 0; tot < n; tot += m) {
+        m = n - tot;
+        if (m > sizeof(buf) - 1)
+            m = sizeof(buf) - 1;
+        memmove(buf, (char*)vbuf + tot, m);
+        sys_cputs(buf, m);
+    }
+    return tot;
 }
 
 static int
 devcons_close(struct Fd *fd)
 {
-	USED(fd);
+    USED(fd);
 
-	return 0;
+    return 0;
 }
 
 static int
 devcons_stat(struct Fd *fd, struct Stat *stat)
 {
-	strcpy(stat->st_name, "<cons>");
-	return 0;
+    strcpy(stat->st_name, "<cons>");
+    return 0;
 }
 
diff --git a/lib/fd.c b/lib/fd.c
index 52a15ed..246e5b3 100644
--- a/lib/fd.c
+++ b/lib/fd.c
@@ -23,13 +23,13 @@
 int
 fd2num(struct Fd *fd)
 {
-	return ((uintptr_t) fd - FDTABLE) / PGSIZE;
+    return ((uintptr_t) fd - FDTABLE) / PGSIZE;
 }
 
 char*
 fd2data(struct Fd *fd)
 {
-	return INDEX2DATA(fd2num(fd));
+    return INDEX2DATA(fd2num(fd));
 }
 
 // Finds the smallest i from 0 to MAXFD-1 that doesn't have
@@ -50,18 +50,18 @@ fd2data(struct Fd *fd)
 int
 fd_alloc(struct Fd **fd_store)
 {
-	int i;
-	struct Fd *fd;
-
-	for (i = 0; i < MAXFD; i++) {
-		fd = INDEX2FD(i);
-		if ((uvpd[PDX(fd)] & PTE_P) == 0 || (uvpt[PGNUM(fd)] & PTE_P) == 0) {
-			*fd_store = fd;
-			return 0;
-		}
-	}
-	*fd_store = 0;
-	return -E_MAX_OPEN;
+    int i;
+    struct Fd *fd;
+
+    for (i = 0; i < MAXFD; i++) {
+        fd = INDEX2FD(i);
+        if ((uvpd[PDX(fd)] & PTE_P) == 0 || (uvpt[PGNUM(fd)] & PTE_P) == 0) {
+            *fd_store = fd;
+            return 0;
+        }
+    }
+    *fd_store = 0;
+    return -E_MAX_OPEN;
 }
 
 // Check that fdnum is in range and mapped.
@@ -73,21 +73,21 @@ fd_alloc(struct Fd **fd_store)
 int
 fd_lookup(int fdnum, struct Fd **fd_store)
 {
-	struct Fd *fd;
-
-	if (fdnum < 0 || fdnum >= MAXFD) {
-		if (debug)
-			cprintf("[%08x] bad fd %d\n", thisenv->env_id, fdnum);
-		return -E_INVAL;
-	}
-	fd = INDEX2FD(fdnum);
-	if (!(uvpd[PDX(fd)] & PTE_P) || !(uvpt[PGNUM(fd)] & PTE_P)) {
-		if (debug)
-			cprintf("[%08x] closed fd %d\n", thisenv->env_id, fdnum);
-		return -E_INVAL;
-	}
-	*fd_store = fd;
-	return 0;
+    struct Fd *fd;
+
+    if (fdnum < 0 || fdnum >= MAXFD) {
+        if (debug)
+            cprintf("[%08x] bad fd %d\n", thisenv->env_id, fdnum);
+        return -E_INVAL;
+    }
+    fd = INDEX2FD(fdnum);
+    if (!(uvpd[PDX(fd)] & PTE_P) || !(uvpt[PGNUM(fd)] & PTE_P)) {
+        if (debug)
+            cprintf("[%08x] closed fd %d\n", thisenv->env_id, fdnum);
+        return -E_INVAL;
+    }
+    *fd_store = fd;
+    return 0;
 }
 
 // Frees file descriptor 'fd' by closing the corresponding file
@@ -100,22 +100,22 @@ fd_lookup(int fdnum, struct Fd **fd_store)
 int
 fd_close(struct Fd *fd, bool must_exist)
 {
-	struct Fd *fd2;
-	struct Dev *dev;
-	int r;
-	if ((r = fd_lookup(fd2num(fd), &fd2)) < 0
-	    || fd != fd2)
-		return (must_exist ? r : 0);
-	if ((r = dev_lookup(fd->fd_dev_id, &dev)) >= 0) {
-		if (dev->dev_close)
-			r = (*dev->dev_close)(fd);
-		else
-			r = 0;
-	}
-	// Make sure fd is unmapped.  Might be a no-op if
-	// (*dev->dev_close)(fd) already unmapped it.
-	(void) sys_page_unmap(0, fd);
-	return r;
+    struct Fd *fd2;
+    struct Dev *dev;
+    int r;
+    if ((r = fd_lookup(fd2num(fd), &fd2)) < 0
+        || fd != fd2)
+        return (must_exist ? r : 0);
+    if ((r = dev_lookup(fd->fd_dev_id, &dev)) >= 0) {
+        if (dev->dev_close)
+            r = (*dev->dev_close)(fd);
+        else
+            r = 0;
+    }
+    // Make sure fd is unmapped.  Might be a no-op if
+    // (*dev->dev_close)(fd) already unmapped it.
+    (void) sys_page_unmap(0, fd);
+    return r;
 }
 
 
@@ -125,44 +125,44 @@ fd_close(struct Fd *fd, bool must_exist)
 
 static struct Dev *devtab[] =
 {
-	&devfile,
-	&devpipe,
-	&devcons,
-	0
+    &devfile,
+    &devpipe,
+    &devcons,
+    0
 };
 
 int
 dev_lookup(int dev_id, struct Dev **dev)
 {
-	int i;
-	for (i = 0; devtab[i]; i++)
-		if (devtab[i]->dev_id == dev_id) {
-			*dev = devtab[i];
-			return 0;
-		}
-	cprintf("[%08x] unknown device type %d\n", thisenv->env_id, dev_id);
-	*dev = 0;
-	return -E_INVAL;
+    int i;
+    for (i = 0; devtab[i]; i++)
+        if (devtab[i]->dev_id == dev_id) {
+            *dev = devtab[i];
+            return 0;
+        }
+    cprintf("[%08x] unknown device type %d\n", thisenv->env_id, dev_id);
+    *dev = 0;
+    return -E_INVAL;
 }
 
 int
 close(int fdnum)
 {
-	struct Fd *fd;
-	int r;
+    struct Fd *fd;
+    int r;
 
-	if ((r = fd_lookup(fdnum, &fd)) < 0)
-		return r;
-	else
-		return fd_close(fd, 1);
+    if ((r = fd_lookup(fdnum, &fd)) < 0)
+        return r;
+    else
+        return fd_close(fd, 1);
 }
 
 void
 close_all(void)
 {
-	int i;
-	for (i = 0; i < MAXFD; i++)
-		close(i);
+    int i;
+    for (i = 0; i < MAXFD; i++)
+        close(i);
 }
 
 // Make file descriptor 'newfdnum' a duplicate of file descriptor 'oldfdnum'.
@@ -173,148 +173,148 @@ close_all(void)
 int
 dup(int oldfdnum, int newfdnum)
 {
-	int r;
-	char *ova, *nva;
-	pte_t pte;
-	struct Fd *oldfd, *newfd;
+    int r;
+    char *ova, *nva;
+    pte_t pte;
+    struct Fd *oldfd, *newfd;
 
-	if ((r = fd_lookup(oldfdnum, &oldfd)) < 0)
-		return r;
-	close(newfdnum);
+    if ((r = fd_lookup(oldfdnum, &oldfd)) < 0)
+        return r;
+    close(newfdnum);
 
-	newfd = INDEX2FD(newfdnum);
-	ova = fd2data(oldfd);
-	nva = fd2data(newfd);
+    newfd = INDEX2FD(newfdnum);
+    ova = fd2data(oldfd);
+    nva = fd2data(newfd);
 
-	if ((uvpd[PDX(ova)] & PTE_P) && (uvpt[PGNUM(ova)] & PTE_P))
-		if ((r = sys_page_map(0, ova, 0, nva, uvpt[PGNUM(ova)] & PTE_SYSCALL)) < 0)
-			goto err;
-	if ((r = sys_page_map(0, oldfd, 0, newfd, uvpt[PGNUM(oldfd)] & PTE_SYSCALL)) < 0)
-		goto err;
+    if ((uvpd[PDX(ova)] & PTE_P) && (uvpt[PGNUM(ova)] & PTE_P))
+        if ((r = sys_page_map(0, ova, 0, nva, uvpt[PGNUM(ova)] & PTE_SYSCALL)) < 0)
+            goto err;
+    if ((r = sys_page_map(0, oldfd, 0, newfd, uvpt[PGNUM(oldfd)] & PTE_SYSCALL)) < 0)
+        goto err;
 
-	return newfdnum;
+    return newfdnum;
 
 err:
-	sys_page_unmap(0, newfd);
-	sys_page_unmap(0, nva);
-	return r;
+    sys_page_unmap(0, newfd);
+    sys_page_unmap(0, nva);
+    return r;
 }
 
 ssize_t
 read(int fdnum, void *buf, size_t n)
 {
-	int r;
-	struct Dev *dev;
-	struct Fd *fd;
-
-	if ((r = fd_lookup(fdnum, &fd)) < 0
-	    || (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
-		return r;
-	if ((fd->fd_omode & O_ACCMODE) == O_WRONLY) {
-		cprintf("[%08x] read %d -- bad mode\n", thisenv->env_id, fdnum);
-		return -E_INVAL;
-	}
-	if (!dev->dev_read)
-		return -E_NOT_SUPP;
-	return (*dev->dev_read)(fd, buf, n);
+    int r;
+    struct Dev *dev;
+    struct Fd *fd;
+
+    if ((r = fd_lookup(fdnum, &fd)) < 0
+        || (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
+        return r;
+    if ((fd->fd_omode & O_ACCMODE) == O_WRONLY) {
+        cprintf("[%08x] read %d -- bad mode\n", thisenv->env_id, fdnum);
+        return -E_INVAL;
+    }
+    if (!dev->dev_read)
+        return -E_NOT_SUPP;
+    return (*dev->dev_read)(fd, buf, n);
 }
 
 ssize_t
 readn(int fdnum, void *buf, size_t n)
 {
-	int m, tot;
-
-	for (tot = 0; tot < n; tot += m) {
-		m = read(fdnum, (char*)buf + tot, n - tot);
-		if (m < 0)
-			return m;
-		if (m == 0)
-			break;
-	}
-	return tot;
+    int m, tot;
+
+    for (tot = 0; tot < n; tot += m) {
+        m = read(fdnum, (char*)buf + tot, n - tot);
+        if (m < 0)
+            return m;
+        if (m == 0)
+            break;
+    }
+    return tot;
 }
 
 ssize_t
 write(int fdnum, const void *buf, size_t n)
 {
-	int r;
-	struct Dev *dev;
-	struct Fd *fd;
-
-	if ((r = fd_lookup(fdnum, &fd)) < 0
-	    || (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
-		return r;
-	if ((fd->fd_omode & O_ACCMODE) == O_RDONLY) {
-		cprintf("[%08x] write %d -- bad mode\n", thisenv->env_id, fdnum);
-		return -E_INVAL;
-	}
-	if (debug)
-		cprintf("write %d %p %d via dev %s\n",
-			fdnum, buf, n, dev->dev_name);
-	if (!dev->dev_write)
-		return -E_NOT_SUPP;
-	return (*dev->dev_write)(fd, buf, n);
+    int r;
+    struct Dev *dev;
+    struct Fd *fd;
+
+    if ((r = fd_lookup(fdnum, &fd)) < 0
+        || (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
+        return r;
+    if ((fd->fd_omode & O_ACCMODE) == O_RDONLY) {
+        cprintf("[%08x] write %d -- bad mode\n", thisenv->env_id, fdnum);
+        return -E_INVAL;
+    }
+    if (debug)
+        cprintf("write %d %p %d via dev %s\n",
+            fdnum, buf, n, dev->dev_name);
+    if (!dev->dev_write)
+        return -E_NOT_SUPP;
+    return (*dev->dev_write)(fd, buf, n);
 }
 
 int
 seek(int fdnum, off_t offset)
 {
-	int r;
-	struct Fd *fd;
+    int r;
+    struct Fd *fd;
 
-	if ((r = fd_lookup(fdnum, &fd)) < 0)
-		return r;
-	fd->fd_offset = offset;
-	return 0;
+    if ((r = fd_lookup(fdnum, &fd)) < 0)
+        return r;
+    fd->fd_offset = offset;
+    return 0;
 }
 
 int
 ftruncate(int fdnum, off_t newsize)
 {
-	int r;
-	struct Dev *dev;
-	struct Fd *fd;
-	if ((r = fd_lookup(fdnum, &fd)) < 0
-	    || (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
-		return r;
-	if ((fd->fd_omode & O_ACCMODE) == O_RDONLY) {
-		cprintf("[%08x] ftruncate %d -- bad mode\n",
-			thisenv->env_id, fdnum);
-		return -E_INVAL;
-	}
-	if (!dev->dev_trunc)
-		return -E_NOT_SUPP;
-	return (*dev->dev_trunc)(fd, newsize);
+    int r;
+    struct Dev *dev;
+    struct Fd *fd;
+    if ((r = fd_lookup(fdnum, &fd)) < 0
+        || (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
+        return r;
+    if ((fd->fd_omode & O_ACCMODE) == O_RDONLY) {
+        cprintf("[%08x] ftruncate %d -- bad mode\n",
+            thisenv->env_id, fdnum);
+        return -E_INVAL;
+    }
+    if (!dev->dev_trunc)
+        return -E_NOT_SUPP;
+    return (*dev->dev_trunc)(fd, newsize);
 }
 
 int
 fstat(int fdnum, struct Stat *stat)
 {
-	int r;
-	struct Dev *dev;
-	struct Fd *fd;
-
-	if ((r = fd_lookup(fdnum, &fd)) < 0
-	    || (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
-		return r;
-	if (!dev->dev_stat)
-		return -E_NOT_SUPP;
-	stat->st_name[0] = 0;
-	stat->st_size = 0;
-	stat->st_isdir = 0;
-	stat->st_dev = dev;
-	return (*dev->dev_stat)(fd, stat);
+    int r;
+    struct Dev *dev;
+    struct Fd *fd;
+
+    if ((r = fd_lookup(fdnum, &fd)) < 0
+        || (r = dev_lookup(fd->fd_dev_id, &dev)) < 0)
+        return r;
+    if (!dev->dev_stat)
+        return -E_NOT_SUPP;
+    stat->st_name[0] = 0;
+    stat->st_size = 0;
+    stat->st_isdir = 0;
+    stat->st_dev = dev;
+    return (*dev->dev_stat)(fd, stat);
 }
 
 int
 stat(const char *path, struct Stat *stat)
 {
-	int fd, r;
+    int fd, r;
 
-	if ((fd = open(path, O_RDONLY)) < 0)
-		return fd;
-	r = fstat(fd, stat);
-	close(fd);
-	return r;
+    if ((fd = open(path, O_RDONLY)) < 0)
+        return fd;
+    r = fstat(fd, stat);
+    close(fd);
+    return r;
 }
 
diff --git a/lib/file.c b/lib/file.c
index 5d13b85..28510b8 100644
--- a/lib/file.c
+++ b/lib/file.c
@@ -15,17 +15,17 @@ union Fsipc fsipcbuf __attribute__((aligned(PGSIZE)));
 static int
 fsipc(unsigned type, void *dstva)
 {
-	static envid_t fsenv;
-	if (fsenv == 0)
-		fsenv = ipc_find_env(ENV_TYPE_FS);
+    static envid_t fsenv;
+    if (fsenv == 0)
+        fsenv = ipc_find_env(ENV_TYPE_FS);
 
-	static_assert(sizeof(fsipcbuf) == PGSIZE);
+    static_assert(sizeof(fsipcbuf) == PGSIZE);
 
-	if (debug)
-		cprintf("[%08x] fsipc %d %08x\n", thisenv->env_id, type, *(uint32_t *)&fsipcbuf);
+    if (debug)
+        cprintf("[%08x] fsipc %d %08x\n", thisenv->env_id, type, *(uint32_t *)&fsipcbuf);
 
-	ipc_send(fsenv, type, &fsipcbuf, PTE_P | PTE_W | PTE_U);
-	return ipc_recv(NULL, dstva, NULL);
+    ipc_send(fsenv, type, &fsipcbuf, PTE_P | PTE_W | PTE_U);
+    return ipc_recv(NULL, dstva, NULL);
 }
 
 static int devfile_flush(struct Fd *fd);
@@ -36,11 +36,11 @@ static int devfile_trunc(struct Fd *fd, off_t newsize);
 
 struct Dev devfile =
 {
-	.dev_id =	'f',
-	.dev_name =	"file",
-	.dev_read =	devfile_read,
-	.dev_close =	devfile_flush,
-	.dev_stat =	devfile_stat,
+    .dev_id =	'f',
+    .dev_name =	"file",
+    .dev_read =	devfile_read,
+    .dev_close =	devfile_flush,
+    .dev_stat =	devfile_stat,
 };
 
 // Open a file (or directory).
@@ -52,38 +52,38 @@ struct Dev devfile =
 int
 open(const char *path, int mode)
 {
-	// Find an unused file descriptor page using fd_alloc.
-	// Then send a file-open request to the file server.
-	// Include 'path' and 'omode' in request,
-	// and map the returned file descriptor page
-	// at the appropriate fd address.
-	// FSREQ_OPEN returns 0 on success, < 0 on failure.
-	//
-	// (fd_alloc does not allocate a page, it just returns an
-	// unused fd address.  Do you need to allocate a page?)
-	//
-	// Return the file descriptor index.
-	// If any step after fd_alloc fails, use fd_close to free the
-	// file descriptor.
-
-	int r;
-	struct Fd *fd;
-
-	if (strlen(path) >= MAXPATHLEN)
-		return -E_BAD_PATH;
-
-	if ((r = fd_alloc(&fd)) < 0)
-		return r;
-
-	strcpy(fsipcbuf.open.req_path, path);
-	fsipcbuf.open.req_omode = mode;
-
-	if ((r = fsipc(FSREQ_OPEN, fd)) < 0) {
-		fd_close(fd, 0);
-		return r;
-	}
-
-	return fd2num(fd);
+    // Find an unused file descriptor page using fd_alloc.
+    // Then send a file-open request to the file server.
+    // Include 'path' and 'omode' in request,
+    // and map the returned file descriptor page
+    // at the appropriate fd address.
+    // FSREQ_OPEN returns 0 on success, < 0 on failure.
+    //
+    // (fd_alloc does not allocate a page, it just returns an
+    // unused fd address.  Do you need to allocate a page?)
+    //
+    // Return the file descriptor index.
+    // If any step after fd_alloc fails, use fd_close to free the
+    // file descriptor.
+
+    int r;
+    struct Fd *fd;
+
+    if (strlen(path) >= MAXPATHLEN)
+        return -E_BAD_PATH;
+
+    if ((r = fd_alloc(&fd)) < 0)
+        return r;
+
+    strcpy(fsipcbuf.open.req_path, path);
+    fsipcbuf.open.req_omode = mode;
+
+    if ((r = fsipc(FSREQ_OPEN, fd)) < 0) {
+        fd_close(fd, 0);
+        return r;
+    }
+
+    return fd2num(fd);
 }
 
 // Flush the file descriptor.  After this the fileid is invalid.
@@ -97,8 +97,8 @@ open(const char *path, int mode)
 static int
 devfile_flush(struct Fd *fd)
 {
-	fsipcbuf.flush.req_fileid = fd->fd_file.id;
-	return fsipc(FSREQ_FLUSH, NULL);
+    fsipcbuf.flush.req_fileid = fd->fd_file.id;
+    return fsipc(FSREQ_FLUSH, NULL);
 }
 
 // Read at most 'n' bytes from 'fd' at the current position into 'buf'.
@@ -109,35 +109,35 @@ devfile_flush(struct Fd *fd)
 static ssize_t
 devfile_read(struct Fd *fd, void *buf, size_t n)
 {
-	// Make an FSREQ_READ request to the file system server after
-	// filling fsipcbuf.read with the request arguments.  The
-	// bytes read will be written back to fsipcbuf by the file
-	// system server.
-	int r;
-
-	fsipcbuf.read.req_fileid = fd->fd_file.id;
-	fsipcbuf.read.req_n = n;
-	if ((r = fsipc(FSREQ_READ, NULL)) < 0)
-		return r;
-	assert(r <= n);
-	assert(r <= PGSIZE);
-	memmove(buf, &fsipcbuf, r);
-	return r;
+    // Make an FSREQ_READ request to the file system server after
+    // filling fsipcbuf.read with the request arguments.  The
+    // bytes read will be written back to fsipcbuf by the file
+    // system server.
+    int r;
+
+    fsipcbuf.read.req_fileid = fd->fd_file.id;
+    fsipcbuf.read.req_n = n;
+    if ((r = fsipc(FSREQ_READ, NULL)) < 0)
+        return r;
+    assert(r <= n);
+    assert(r <= PGSIZE);
+    memmove(buf, &fsipcbuf, r);
+    return r;
 }
 
 
 static int
 devfile_stat(struct Fd *fd, struct Stat *st)
 {
-	int r;
-
-	fsipcbuf.stat.req_fileid = fd->fd_file.id;
-	if ((r = fsipc(FSREQ_STAT, NULL)) < 0)
-		return r;
-	strcpy(st->st_name, fsipcbuf.statRet.ret_name);
-	st->st_size = fsipcbuf.statRet.ret_size;
-	st->st_isdir = fsipcbuf.statRet.ret_isdir;
-	return 0;
+    int r;
+
+    fsipcbuf.stat.req_fileid = fd->fd_file.id;
+    if ((r = fsipc(FSREQ_STAT, NULL)) < 0)
+        return r;
+    strcpy(st->st_name, fsipcbuf.statRet.ret_name);
+    st->st_size = fsipcbuf.statRet.ret_size;
+    st->st_isdir = fsipcbuf.statRet.ret_isdir;
+    return 0;
 }
 
 
diff --git a/lib/fork.c b/lib/fork.c
index 56c0714..a8b33b8 100644
--- a/lib/fork.c
+++ b/lib/fork.c
@@ -14,28 +14,43 @@
 static void
 pgfault(struct UTrapframe *utf)
 {
-	void *addr = (void *) utf->utf_fault_va;
-	uint32_t err = utf->utf_err;
-	int r;
+    void *addr = (void *) utf->utf_fault_va;
+    uint32_t err = utf->utf_err;
+    int r;
 
-	// Check that the faulting access was (1) a write, and (2) to a
-	// copy-on-write page.  If not, panic.
-	// Hint:
-	//   Use the read-only page table mappings at uvpt
-	//   (see <inc/memlayout.h>).
+    // Check that the faulting access was (1) a write, and (2) to a
+    // copy-on-write page.  If not, panic.
+    // Hint:
+    //   Use the read-only page table mappings at uvpt
+    //   (see <inc/memlayout.h>).
 
-	// LAB 4: Your code here.
+    // LAB 4: Your code here.
+    pte_t pte = uvpt[PGNUM(addr)];
+    if (!(err & FEC_WR) || !(pte & PTE_COW)) {
+        panic("bad access in pgfault");
+    }
 
-	// Allocate a new page, map it at a temporary location (PFTEMP),
-	// copy the data from the old page to the new page, then move the new
-	// page to the old page's address.
-	// Hint:
-	//   You should make three system calls.
-	//   No need to explicitly delete the old page's mapping.
+    // Allocate a new page, map it at a temporary location (PFTEMP),
+    // copy the data from the old page to the new page, then move the new
+    // page to the old page's address.
+    // Hint:
+    //   You should make three system calls.
+    //   No need to explicitly delete the old page's mapping.
 
-	// LAB 4: Your code here.
+    // LAB 4: Your code here.
+    r = sys_page_alloc(0, PFTEMP, PTE_W | PTE_U | PTE_P);
+    if (r) {
+        panic("not enough memory in pgfault");
+    }
 
-	panic("pgfault not implemented");
+    void *addr_ = ROUNDDOWN(addr, PGSIZE);
+    memmove(PFTEMP, addr_, PGSIZE);
+
+    // change mapping for the faulting page.
+    r = sys_page_map(0, PFTEMP, 0, addr_, PTE_W | PTE_U | PTE_P);
+    if (r) {
+        panic("map error in pgfault");
+    }
 }
 
 //
@@ -52,11 +67,34 @@ pgfault(struct UTrapframe *utf)
 static int
 duppage(envid_t envid, unsigned pn)
 {
-	int r;
+    int r;
+
+    // LAB 4: Your code here.
+    pte_t pte = uvpt[pn];
+    void *va = (void*)(pn << PGSHIFT);
+
+    if (pte & PTE_SHARE) {
+        r = sys_page_map(0, va, envid, va, PTE_SHARE | (pte & PTE_SYSCALL));
+        if (r) {
+            panic("map error in duppage");
+        }
+    } else if ((pte & PTE_W) || (pte & PTE_COW)) {
+        r = sys_page_map(0, va, envid, va, PTE_COW | PTE_U | PTE_P);
+        if (r) {
+            panic("map error in duppage");
+        }
+        r = sys_page_map(0, va, 0, va, PTE_COW | PTE_U | PTE_P);
+        if (r) {
+            panic("map error in duppage");
+        }
+    } else {
+        r = sys_page_map(0, va, envid, va, PTE_U | PTE_P);
+        if (r) {
+            panic("map error in duppager");
+        }
+    }
 
-	// LAB 4: Your code here.
-	panic("duppage not implemented");
-	return 0;
+    return 0;
 }
 
 //
@@ -78,14 +116,55 @@ duppage(envid_t envid, unsigned pn)
 envid_t
 fork(void)
 {
-	// LAB 4: Your code here.
-	panic("fork not implemented");
+    int r;
+    // LAB 4: Your code here.
+    set_pgfault_handler(pgfault);
+
+    envid_t envid = sys_exofork();
+    if (envid < 0) {
+        panic("create env error in fork");
+    } else if (envid == 0) {
+        thisenv = &envs[ENVX(sys_getenvid())];
+        return 0;
+    }
+
+    for (int ipd = 0; ipd < PDX(UTOP); ++ipd) {
+        if (!(uvpd[ipd] & PTE_P)) {
+            continue;
+        }
+
+        for (int ipt = 0; ipt != NPTENTRIES; ++ipt) {
+            unsigned pn = (unsigned) ((ipd << 10) | ipt);
+            if (pn == PGNUM(UXSTACKTOP - PGSIZE)) {
+                r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_W | PTE_U | PTE_P);
+                if (r) {
+                    panic("not enough memory in fork");
+                }
+                continue;
+            }
+            if (uvpt[pn] & PTE_P) {
+                duppage(envid, pn);
+            }
+        }
+    }
+
+    r = sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);
+    if (r) {
+        panic("error in fork");
+    }
+
+    r = sys_env_set_status(envid, ENV_RUNNABLE);
+    if (r) {
+        panic("error in fork");
+    }
+
+    return envid;
 }
 
 // Challenge!
 int
 sfork(void)
 {
-	panic("sfork not implemented");
-	return -E_INVAL;
+    panic("sfork not implemented");
+    return -E_INVAL;
 }
diff --git a/lib/ipc.c b/lib/ipc.c
index 6dfb5ff..cc71d55 100644
--- a/lib/ipc.c
+++ b/lib/ipc.c
@@ -22,9 +22,32 @@
 int32_t
 ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
 {
-	// LAB 4: Your code here.
-	panic("ipc_recv not implemented");
-	return 0;
+    // LAB 4: Your code here.
+    if (!pg) {
+        pg = (void*) UTOP;
+    }
+
+    int res = sys_ipc_recv(pg);
+    if (res) {
+        if (from_env_store) {
+            *from_env_store = 0;
+        }
+        if (perm_store) {
+            *perm_store = 0;
+        }
+
+        return res;
+    }
+
+    if (from_env_store) {
+        *from_env_store = thisenv->env_ipc_from;
+    }
+
+    if (perm_store) {
+        *perm_store = thisenv->env_ipc_perm;
+    }
+
+    return thisenv->env_ipc_value;
 }
 
 // Send 'val' (and 'pg' with 'perm', if 'pg' is nonnull) to 'toenv'.
@@ -38,8 +61,20 @@ ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
 void
 ipc_send(envid_t to_env, uint32_t val, void *pg, int perm)
 {
-	// LAB 4: Your code here.
-	panic("ipc_send not implemented");
+    // LAB 4: Your code here.
+    if (!pg) {
+        pg = (void*) UTOP;
+    }
+
+    int res = sys_ipc_try_send(to_env, val, pg, perm);
+    while (res == -E_IPC_NOT_RECV) {
+        sys_yield();
+        res = sys_ipc_try_send(to_env, val, pg, perm);
+    }
+
+    if (res) {
+        panic("error in ipc_send");
+    }
 }
 
 // Find the first environment of the given type.  We'll use this to
@@ -48,9 +83,9 @@ ipc_send(envid_t to_env, uint32_t val, void *pg, int perm)
 envid_t
 ipc_find_env(enum EnvType type)
 {
-	int i;
-	for (i = 0; i < NENV; i++)
-		if (envs[i].env_type == type)
-			return envs[i].env_id;
-	return 0;
+    int i;
+    for (i = 0; i < NENV; i++)
+        if (envs[i].env_type == type)
+            return envs[i].env_id;
+    return 0;
 }
diff --git a/lib/libmain.c b/lib/libmain.c
index 8a14b29..7443f33 100644
--- a/lib/libmain.c
+++ b/lib/libmain.c
@@ -11,18 +11,18 @@ const char *binaryname = "<unknown>";
 void
 libmain(int argc, char **argv)
 {
-	// set thisenv to point at our Env structure in envs[].
-	// LAB 3: Your code here.
-	thisenv = 0;
+    // set thisenv to point at our Env structure in envs[].
+    // LAB 3: Your code here.
+    thisenv = &envs[ENVX(sys_getenvid())];
 
-	// save the name of the program so that panic() can use it
-	if (argc > 0)
-		binaryname = argv[0];
+    // save the name of the program so that panic() can use it
+    if (argc > 0)
+        binaryname = argv[0];
 
-	// call user main routine
-	umain(argc, argv);
+    // call user main routine
+    umain(argc, argv);
 
-	// exit gracefully
-	exit();
+    // exit gracefully
+    exit();
 }
 
diff --git a/lib/pfentry.S b/lib/pfentry.S
index f40aeeb..44909b7 100644
--- a/lib/pfentry.S
+++ b/lib/pfentry.S
@@ -32,51 +32,64 @@
 .text
 .globl _pgfault_upcall
 _pgfault_upcall:
-	// Call the C page fault handler.
-	pushl %esp			// function argument: pointer to UTF
-	movl _pgfault_handler, %eax
-	call *%eax
-	addl $4, %esp			// pop function argument
-	
-	// Now the C page fault handler has returned and you must return
-	// to the trap time state.
-	// Push trap-time %eip onto the trap-time stack.
-	//
-	// Explanation:
-	//   We must prepare the trap-time stack for our eventual return to
-	//   re-execute the instruction that faulted.
-	//   Unfortunately, we can't return directly from the exception stack:
-	//   We can't call 'jmp', since that requires that we load the address
-	//   into a register, and all registers must have their trap-time
-	//   values after the return.
-	//   We can't call 'ret' from the exception stack either, since if we
-	//   did, %esp would have the wrong value.
-	//   So instead, we push the trap-time %eip onto the *trap-time* stack!
-	//   Below we'll switch to that stack and call 'ret', which will
-	//   restore %eip to its pre-fault value.
-	//
-	//   In the case of a recursive fault on the exception stack,
-	//   note that the word we're pushing now will fit in the
-	//   blank word that the kernel reserved for us.
-	//
-	// Throughout the remaining code, think carefully about what
-	// registers are available for intermediate calculations.  You
-	// may find that you have to rearrange your code in non-obvious
-	// ways as registers become unavailable as scratch space.
-	//
-	// LAB 4: Your code here.
+    // Call the C page fault handler.
+    pushl %esp			// function argument: pointer to UTF
+    movl _pgfault_handler, %eax
+    call *%eax
+    addl $4, %esp			// pop function argument
 
-	// Restore the trap-time registers.  After you do this, you
-	// can no longer modify any general-purpose registers.
-	// LAB 4: Your code here.
+    // Now the C page fault handler has returned and you must return
+    // to the trap time state.
+    // Push trap-time %eip onto the trap-time stack.
+    //
+    // Explanation:
+    //   We must prepare the trap-time stack for our eventual return to
+    //   re-execute the instruction that faulted.
+    //   Unfortunately, we can't return directly from the exception stack:
+    //   We can't call 'jmp', since that requires that we load the address
+    //   into a register, and all registers must have their trap-time
+    //   values after the return.
+    //   We can't call 'ret' from the exception stack either, since if we
+    //   did, %esp would have the wrong value.
+    //   So instead, we push the trap-time %eip onto the *trap-time* stack!
+    //   Below we'll switch to that stack and call 'ret', which will
+    //   restore %eip to its pre-fault value.
+    //
+    //   In the case of a recursive fault on the exception stack,
+    //   note that the word we're pushing now will fit in the
+    //   blank word that the kernel reserved for us.
+    //
+    // Throughout the remaining code, think carefully about what
+    // registers are available for intermediate calculations.  You
+    // may find that you have to rearrange your code in non-obvious
+    // ways as registers become unavailable as scratch space.
+    //
+    // LAB 4: Your code here.
+    movl %esp, %eax
+    movl 40(%esp), %ebx
+    movl 48(%esp), %esp
+    pushl %ebx
+    movl %esp, 48(%eax)
 
-	// Restore eflags from the stack.  After you do this, you can
-	// no longer use arithmetic operations or anything else that
-	// modifies eflags.
-	// LAB 4: Your code here.
+    // Restore the trap-time registers.  After you do this, you
+    // can no longer modify any general-purpose registers.
+    // LAB 4: Your code here.
+    movl %eax, %esp
+    addl $4, %esp
+    addl $4, %esp
+    popal
+    addl $4, %esp
 
-	// Switch back to the adjusted trap-time stack.
-	// LAB 4: Your code here.
+    // Restore eflags from the stack.  After you do this, you can
+    // no longer use arithmetic operations or anything else that
+    // modifies eflags.
+    // LAB 4: Your code here.
+    popfl
 
-	// Return to re-execute the instruction that faulted.
-	// LAB 4: Your code here.
+    // Switch back to the adjusted trap-time stack.
+    // LAB 4: Your code here.
+    popl %esp
+
+    // Return to re-execute the instruction that faulted.
+    // LAB 4: Your code here.
+    ret
\ No newline at end of file
diff --git a/lib/pgfault.c b/lib/pgfault.c
index a975518..7a30122 100644
--- a/lib/pgfault.c
+++ b/lib/pgfault.c
@@ -24,14 +24,18 @@ void (*_pgfault_handler)(struct UTrapframe *utf);
 void
 set_pgfault_handler(void (*handler)(struct UTrapframe *utf))
 {
-	int r;
+    int r;
 
-	if (_pgfault_handler == 0) {
-		// First time through!
-		// LAB 4: Your code here.
-		panic("set_pgfault_handler not implemented");
-	}
+    if (_pgfault_handler == 0) {
+        // First time through!
+        // LAB 4: Your code here.
+        r = sys_page_alloc(0, (void*)(UXSTACKTOP - PGSIZE), PTE_W | PTE_U | PTE_P);
+        if (r) {
+            panic("no mem in set_pgfault_handler");
+        }
+        sys_env_set_pgfault_upcall(0, _pgfault_upcall);
+    }
 
-	// Save handler pointer for assembly to call.
-	_pgfault_handler = handler;
+    // Save handler pointer for assembly to call.
+    _pgfault_handler = handler;
 }
diff --git a/lib/spawn.c b/lib/spawn.c
index 73f68c3..869aa9b 100644
--- a/lib/spawn.c
+++ b/lib/spawn.c
@@ -8,7 +8,7 @@
 // Helper functions for spawn.
 static int init_stack(envid_t child, const char **argv, uintptr_t *init_esp);
 static int map_segment(envid_t child, uintptr_t va, size_t memsz,
-		       int fd, size_t filesz, off_t fileoffset, int perm);
+               int fd, size_t filesz, off_t fileoffset, int perm);
 static int copy_shared_pages(envid_t child);
 
 // Spawn a child process from a program image loaded from the file system.
@@ -19,128 +19,128 @@ static int copy_shared_pages(envid_t child);
 int
 spawn(const char *prog, const char **argv)
 {
-	unsigned char elf_buf[512];
-	struct Trapframe child_tf;
-	envid_t child;
-
-	int fd, i, r;
-	struct Elf *elf;
-	struct Proghdr *ph;
-	int perm;
-
-	// This code follows this procedure:
-	//
-	//   - Open the program file.
-	//
-	//   - Read the ELF header, as you have before, and sanity check its
-	//     magic number.  (Check out your load_icode!)
-	//
-	//   - Use sys_exofork() to create a new environment.
-	//
-	//   - Set child_tf to an initial struct Trapframe for the child.
-	//
-	//   - Call the init_stack() function above to set up
-	//     the initial stack page for the child environment.
-	//
-	//   - Map all of the program's segments that are of p_type
-	//     ELF_PROG_LOAD into the new environment's address space.
-	//     Use the p_flags field in the Proghdr for each segment
-	//     to determine how to map the segment:
-	//
-	//	* If the ELF flags do not include ELF_PROG_FLAG_WRITE,
-	//	  then the segment contains text and read-only data.
-	//	  Use read_map() to read the contents of this segment,
-	//	  and map the pages it returns directly into the child
-	//        so that multiple instances of the same program
-	//	  will share the same copy of the program text.
-	//        Be sure to map the program text read-only in the child.
-	//        Read_map is like read but returns a pointer to the data in
-	//        *blk rather than copying the data into another buffer.
-	//
-	//	* If the ELF segment flags DO include ELF_PROG_FLAG_WRITE,
-	//	  then the segment contains read/write data and bss.
-	//	  As with load_icode() in Lab 3, such an ELF segment
-	//	  occupies p_memsz bytes in memory, but only the FIRST
-	//	  p_filesz bytes of the segment are actually loaded
-	//	  from the executable file - you must clear the rest to zero.
-	//        For each page to be mapped for a read/write segment,
-	//        allocate a page in the parent temporarily at UTEMP,
-	//        read() the appropriate portion of the file into that page
-	//	  and/or use memset() to zero non-loaded portions.
-	//	  (You can avoid calling memset(), if you like, if
-	//	  page_alloc() returns zeroed pages already.)
-	//        Then insert the page mapping into the child.
-	//        Look at init_stack() for inspiration.
-	//        Be sure you understand why you can't use read_map() here.
-	//
-	//     Note: None of the segment addresses or lengths above
-	//     are guaranteed to be page-aligned, so you must deal with
-	//     these non-page-aligned values appropriately.
-	//     The ELF linker does, however, guarantee that no two segments
-	//     will overlap on the same page; and it guarantees that
-	//     PGOFF(ph->p_offset) == PGOFF(ph->p_va).
-	//
-	//   - Call sys_env_set_trapframe(child, &child_tf) to set up the
-	//     correct initial eip and esp values in the child.
-	//
-	//   - Start the child process running with sys_env_set_status().
-
-	if ((r = open(prog, O_RDONLY)) < 0)
-		return r;
-	fd = r;
-
-	// Read elf header
-	elf = (struct Elf*) elf_buf;
-	if (readn(fd, elf_buf, sizeof(elf_buf)) != sizeof(elf_buf)
-	    || elf->e_magic != ELF_MAGIC) {
-		close(fd);
-		cprintf("elf magic %08x want %08x\n", elf->e_magic, ELF_MAGIC);
-		return -E_NOT_EXEC;
-	}
-
-	// Create new child environment
-	if ((r = sys_exofork()) < 0)
-		return r;
-	child = r;
-
-	// Set up trap frame, including initial stack.
-	child_tf = envs[ENVX(child)].env_tf;
-	child_tf.tf_eip = elf->e_entry;
-
-	if ((r = init_stack(child, argv, &child_tf.tf_esp)) < 0)
-		return r;
-
-	// Set up program segments as defined in ELF header.
-	ph = (struct Proghdr*) (elf_buf + elf->e_phoff);
-	for (i = 0; i < elf->e_phnum; i++, ph++) {
-		if (ph->p_type != ELF_PROG_LOAD)
-			continue;
-		perm = PTE_P | PTE_U;
-		if (ph->p_flags & ELF_PROG_FLAG_WRITE)
-			perm |= PTE_W;
-		if ((r = map_segment(child, ph->p_va, ph->p_memsz,
-				     fd, ph->p_filesz, ph->p_offset, perm)) < 0)
-			goto error;
-	}
-	close(fd);
-	fd = -1;
-
-	// Copy shared library state.
-	if ((r = copy_shared_pages(child)) < 0)
-		panic("copy_shared_pages: %e", r);
-
-	if ((r = sys_env_set_trapframe(child, &child_tf)) < 0)
-		panic("sys_env_set_trapframe: %e", r);
-
-	if ((r = sys_env_set_status(child, ENV_RUNNABLE)) < 0)
-		panic("sys_env_set_status: %e", r);
-
-	return child;
+    unsigned char elf_buf[512];
+    struct Trapframe child_tf;
+    envid_t child;
+
+    int fd, i, r;
+    struct Elf *elf;
+    struct Proghdr *ph;
+    int perm;
+
+    // This code follows this procedure:
+    //
+    //   - Open the program file.
+    //
+    //   - Read the ELF header, as you have before, and sanity check its
+    //     magic number.  (Check out your load_icode!)
+    //
+    //   - Use sys_exofork() to create a new environment.
+    //
+    //   - Set child_tf to an initial struct Trapframe for the child.
+    //
+    //   - Call the init_stack() function above to set up
+    //     the initial stack page for the child environment.
+    //
+    //   - Map all of the program's segments that are of p_type
+    //     ELF_PROG_LOAD into the new environment's address space.
+    //     Use the p_flags field in the Proghdr for each segment
+    //     to determine how to map the segment:
+    //
+    //	* If the ELF flags do not include ELF_PROG_FLAG_WRITE,
+    //	  then the segment contains text and read-only data.
+    //	  Use read_map() to read the contents of this segment,
+    //	  and map the pages it returns directly into the child
+    //        so that multiple instances of the same program
+    //	  will share the same copy of the program text.
+    //        Be sure to map the program text read-only in the child.
+    //        Read_map is like read but returns a pointer to the data in
+    //        *blk rather than copying the data into another buffer.
+    //
+    //	* If the ELF segment flags DO include ELF_PROG_FLAG_WRITE,
+    //	  then the segment contains read/write data and bss.
+    //	  As with load_icode() in Lab 3, such an ELF segment
+    //	  occupies p_memsz bytes in memory, but only the FIRST
+    //	  p_filesz bytes of the segment are actually loaded
+    //	  from the executable file - you must clear the rest to zero.
+    //        For each page to be mapped for a read/write segment,
+    //        allocate a page in the parent temporarily at UTEMP,
+    //        read() the appropriate portion of the file into that page
+    //	  and/or use memset() to zero non-loaded portions.
+    //	  (You can avoid calling memset(), if you like, if
+    //	  page_alloc() returns zeroed pages already.)
+    //        Then insert the page mapping into the child.
+    //        Look at init_stack() for inspiration.
+    //        Be sure you understand why you can't use read_map() here.
+    //
+    //     Note: None of the segment addresses or lengths above
+    //     are guaranteed to be page-aligned, so you must deal with
+    //     these non-page-aligned values appropriately.
+    //     The ELF linker does, however, guarantee that no two segments
+    //     will overlap on the same page; and it guarantees that
+    //     PGOFF(ph->p_offset) == PGOFF(ph->p_va).
+    //
+    //   - Call sys_env_set_trapframe(child, &child_tf) to set up the
+    //     correct initial eip and esp values in the child.
+    //
+    //   - Start the child process running with sys_env_set_status().
+
+    if ((r = open(prog, O_RDONLY)) < 0)
+        return r;
+    fd = r;
+
+    // Read elf header
+    elf = (struct Elf*) elf_buf;
+    if (readn(fd, elf_buf, sizeof(elf_buf)) != sizeof(elf_buf)
+        || elf->e_magic != ELF_MAGIC) {
+        close(fd);
+        cprintf("elf magic %08x want %08x\n", elf->e_magic, ELF_MAGIC);
+        return -E_NOT_EXEC;
+    }
+
+    // Create new child environment
+    if ((r = sys_exofork()) < 0)
+        return r;
+    child = r;
+
+    // Set up trap frame, including initial stack.
+    child_tf = envs[ENVX(child)].env_tf;
+    child_tf.tf_eip = elf->e_entry;
+
+    if ((r = init_stack(child, argv, &child_tf.tf_esp)) < 0)
+        return r;
+
+    // Set up program segments as defined in ELF header.
+    ph = (struct Proghdr*) (elf_buf + elf->e_phoff);
+    for (i = 0; i < elf->e_phnum; i++, ph++) {
+        if (ph->p_type != ELF_PROG_LOAD)
+            continue;
+        perm = PTE_P | PTE_U;
+        if (ph->p_flags & ELF_PROG_FLAG_WRITE)
+            perm |= PTE_W;
+        if ((r = map_segment(child, ph->p_va, ph->p_memsz,
+                     fd, ph->p_filesz, ph->p_offset, perm)) < 0)
+            goto error;
+    }
+    close(fd);
+    fd = -1;
+
+    // Copy shared library state.
+    if ((r = copy_shared_pages(child)) < 0)
+        panic("copy_shared_pages: %e", r);
+
+    if ((r = sys_env_set_trapframe(child, &child_tf)) < 0)
+        panic("sys_env_set_trapframe: %e", r);
+
+    if ((r = sys_env_set_status(child, ENV_RUNNABLE)) < 0)
+        panic("sys_env_set_status: %e", r);
+
+    return child;
 
 error:
-	sys_env_destroy(child);
-	close(fd);
-	return r;
+    sys_env_destroy(child);
+    close(fd);
+    return r;
 }
 
 // Spawn, taking command-line arguments array directly on the stack.
@@ -149,29 +149,29 @@ error:
 int
 spawnl(const char *prog, const char *arg0, ...)
 {
-	// We calculate argc by advancing the args until we hit NULL.
-	// The contract of the function guarantees that the last
-	// argument will always be NULL, and that none of the other
-	// arguments will be NULL.
-	int argc=0;
-	va_list vl;
-	va_start(vl, arg0);
-	while(va_arg(vl, void *) != NULL)
-		argc++;
-	va_end(vl);
-
-	// Now that we have the size of the args, do a second pass
-	// and store the values in a VLA, which has the format of argv
-	const char *argv[argc+2];
-	argv[0] = arg0;
-	argv[argc+1] = NULL;
-
-	va_start(vl, arg0);
-	unsigned i;
-	for(i=0;i<argc;i++)
-		argv[i+1] = va_arg(vl, const char *);
-	va_end(vl);
-	return spawn(prog, argv);
+    // We calculate argc by advancing the args until we hit NULL.
+    // The contract of the function guarantees that the last
+    // argument will always be NULL, and that none of the other
+    // arguments will be NULL.
+    int argc=0;
+    va_list vl;
+    va_start(vl, arg0);
+    while(va_arg(vl, void *) != NULL)
+        argc++;
+    va_end(vl);
+
+    // Now that we have the size of the args, do a second pass
+    // and store the values in a VLA, which has the format of argv
+    const char *argv[argc+2];
+    argv[0] = arg0;
+    argv[argc+1] = NULL;
+
+    va_start(vl, arg0);
+    unsigned i;
+    for(i=0;i<argc;i++)
+        argv[i+1] = va_arg(vl, const char *);
+    va_end(vl);
+    return spawn(prog, argv);
 }
 
 
@@ -185,122 +185,134 @@ spawnl(const char *prog, const char *arg0, ...)
 static int
 init_stack(envid_t child, const char **argv, uintptr_t *init_esp)
 {
-	size_t string_size;
-	int argc, i, r;
-	char *string_store;
-	uintptr_t *argv_store;
-
-	// Count the number of arguments (argc)
-	// and the total amount of space needed for strings (string_size).
-	string_size = 0;
-	for (argc = 0; argv[argc] != 0; argc++)
-		string_size += strlen(argv[argc]) + 1;
-
-	// Determine where to place the strings and the argv array.
-	// Set up pointers into the temporary page 'UTEMP'; we'll map a page
-	// there later, then remap that page into the child environment
-	// at (USTACKTOP - PGSIZE).
-	// strings is the topmost thing on the stack.
-	string_store = (char*) UTEMP + PGSIZE - string_size;
-	// argv is below that.  There's one argument pointer per argument, plus
-	// a null pointer.
-	argv_store = (uintptr_t*) (ROUNDDOWN(string_store, 4) - 4 * (argc + 1));
-
-	// Make sure that argv, strings, and the 2 words that hold 'argc'
-	// and 'argv' themselves will all fit in a single stack page.
-	if ((void*) (argv_store - 2) < (void*) UTEMP)
-		return -E_NO_MEM;
-
-	// Allocate the single stack page at UTEMP.
-	if ((r = sys_page_alloc(0, (void*) UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
-		return r;
-
-
-	//	* Initialize 'argv_store[i]' to point to argument string i,
-	//	  for all 0 <= i < argc.
-	//	  Also, copy the argument strings from 'argv' into the
-	//	  newly-allocated stack page.
-	//
-	//	* Set 'argv_store[argc]' to 0 to null-terminate the args array.
-	//
-	//	* Push two more words onto the child's stack below 'args',
-	//	  containing the argc and argv parameters to be passed
-	//	  to the child's umain() function.
-	//	  argv should be below argc on the stack.
-	//	  (Again, argv should use an address valid in the child's
-	//	  environment.)
-	//
-	//	* Set *init_esp to the initial stack pointer for the child,
-	//	  (Again, use an address valid in the child's environment.)
-	for (i = 0; i < argc; i++) {
-		argv_store[i] = UTEMP2USTACK(string_store);
-		strcpy(string_store, argv[i]);
-		string_store += strlen(argv[i]) + 1;
-	}
-	argv_store[argc] = 0;
-	assert(string_store == (char*)UTEMP + PGSIZE);
-
-	argv_store[-1] = UTEMP2USTACK(argv_store);
-	argv_store[-2] = argc;
-
-	*init_esp = UTEMP2USTACK(&argv_store[-2]);
-
-	// After completing the stack, map it into the child's address space
-	// and unmap it from ours!
-	if ((r = sys_page_map(0, UTEMP, child, (void*) (USTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
-		goto error;
-	if ((r = sys_page_unmap(0, UTEMP)) < 0)
-		goto error;
-
-	return 0;
+    size_t string_size;
+    int argc, i, r;
+    char *string_store;
+    uintptr_t *argv_store;
+
+    // Count the number of arguments (argc)
+    // and the total amount of space needed for strings (string_size).
+    string_size = 0;
+    for (argc = 0; argv[argc] != 0; argc++)
+        string_size += strlen(argv[argc]) + 1;
+
+    // Determine where to place the strings and the argv array.
+    // Set up pointers into the temporary page 'UTEMP'; we'll map a page
+    // there later, then remap that page into the child environment
+    // at (USTACKTOP - PGSIZE).
+    // strings is the topmost thing on the stack.
+    string_store = (char*) UTEMP + PGSIZE - string_size;
+    // argv is below that.  There's one argument pointer per argument, plus
+    // a null pointer.
+    argv_store = (uintptr_t*) (ROUNDDOWN(string_store, 4) - 4 * (argc + 1));
+
+    // Make sure that argv, strings, and the 2 words that hold 'argc'
+    // and 'argv' themselves will all fit in a single stack page.
+    if ((void*) (argv_store - 2) < (void*) UTEMP)
+        return -E_NO_MEM;
+
+    // Allocate the single stack page at UTEMP.
+    if ((r = sys_page_alloc(0, (void*) UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
+        return r;
+
+
+    //	* Initialize 'argv_store[i]' to point to argument string i,
+    //	  for all 0 <= i < argc.
+    //	  Also, copy the argument strings from 'argv' into the
+    //	  newly-allocated stack page.
+    //
+    //	* Set 'argv_store[argc]' to 0 to null-terminate the args array.
+    //
+    //	* Push two more words onto the child's stack below 'args',
+    //	  containing the argc and argv parameters to be passed
+    //	  to the child's umain() function.
+    //	  argv should be below argc on the stack.
+    //	  (Again, argv should use an address valid in the child's
+    //	  environment.)
+    //
+    //	* Set *init_esp to the initial stack pointer for the child,
+    //	  (Again, use an address valid in the child's environment.)
+    for (i = 0; i < argc; i++) {
+        argv_store[i] = UTEMP2USTACK(string_store);
+        strcpy(string_store, argv[i]);
+        string_store += strlen(argv[i]) + 1;
+    }
+    argv_store[argc] = 0;
+    assert(string_store == (char*)UTEMP + PGSIZE);
+
+    argv_store[-1] = UTEMP2USTACK(argv_store);
+    argv_store[-2] = argc;
+
+    *init_esp = UTEMP2USTACK(&argv_store[-2]);
+
+    // After completing the stack, map it into the child's address space
+    // and unmap it from ours!
+    if ((r = sys_page_map(0, UTEMP, child, (void*) (USTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
+        goto error;
+    if ((r = sys_page_unmap(0, UTEMP)) < 0)
+        goto error;
+
+    return 0;
 
 error:
-	sys_page_unmap(0, UTEMP);
-	return r;
+    sys_page_unmap(0, UTEMP);
+    return r;
 }
 
 static int
 map_segment(envid_t child, uintptr_t va, size_t memsz,
-	int fd, size_t filesz, off_t fileoffset, int perm)
+    int fd, size_t filesz, off_t fileoffset, int perm)
 {
-	int i, r;
-	void *blk;
-
-	//cprintf("map_segment %x+%x\n", va, memsz);
-
-	if ((i = PGOFF(va))) {
-		va -= i;
-		memsz += i;
-		filesz += i;
-		fileoffset -= i;
-	}
-
-	for (i = 0; i < memsz; i += PGSIZE) {
-		if (i >= filesz) {
-			// allocate a blank page
-			if ((r = sys_page_alloc(child, (void*) (va + i), perm)) < 0)
-				return r;
-		} else {
-			// from file
-			if ((r = sys_page_alloc(0, UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
-				return r;
-			if ((r = seek(fd, fileoffset + i)) < 0)
-				return r;
-			if ((r = readn(fd, UTEMP, MIN(PGSIZE, filesz-i))) < 0)
-				return r;
-			if ((r = sys_page_map(0, UTEMP, child, (void*) (va + i), perm)) < 0)
-				panic("spawn: sys_page_map data: %e", r);
-			sys_page_unmap(0, UTEMP);
-		}
-	}
-	return 0;
+    int i, r;
+    void *blk;
+
+    //cprintf("map_segment %x+%x\n", va, memsz);
+
+    if ((i = PGOFF(va))) {
+        va -= i;
+        memsz += i;
+        filesz += i;
+        fileoffset -= i;
+    }
+
+    for (i = 0; i < memsz; i += PGSIZE) {
+        if (i >= filesz) {
+            // allocate a blank page
+            if ((r = sys_page_alloc(child, (void*) (va + i), perm)) < 0)
+                return r;
+        } else {
+            // from file
+            if ((r = sys_page_alloc(0, UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
+                return r;
+            if ((r = seek(fd, fileoffset + i)) < 0)
+                return r;
+            if ((r = readn(fd, UTEMP, MIN(PGSIZE, filesz-i))) < 0)
+                return r;
+            if ((r = sys_page_map(0, UTEMP, child, (void*) (va + i), perm)) < 0)
+                panic("spawn: sys_page_map data: %e", r);
+            sys_page_unmap(0, UTEMP);
+        }
+    }
+    return 0;
 }
 
 // Copy the mappings for shared pages into the child address space.
 static int
 copy_shared_pages(envid_t child)
 {
-	// LAB 5: Your code here.
-	return 0;
+    // LAB 5: Your code here.
+    for (size_t va = 0; va < USTACKTOP; va += PGSIZE) {
+        size_t ipd = PDX(va);
+        if (!(uvpd[ipd] & PTE_P)) {
+            continue;
+        }
+        int perm = uvpt[PGNUM(va)];
+        if (perm & PTE_SHARE) {
+            if (sys_page_map(0, (void*) va, child, (void*) va, PTE_SHARE | (perm & PTE_SYSCALL))) {
+                panic("error in copy_shared_pages");
+            }
+        }
+    }
+    return 0;
 }
 
