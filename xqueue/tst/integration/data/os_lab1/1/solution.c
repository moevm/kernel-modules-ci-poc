diff --git a/kern/init.c b/kern/init.c
index e73b3c2..8fbe3c9 100644
--- a/kern/init.c
+++ b/kern/init.c
@@ -33,6 +33,9 @@ i386_init(void)
 	// Can't call cprintf until after we do this!
 	cons_init();
 
+	//unsigned int i = 0x00646c72;
+	//cprintf("H%x Wo%s", 57616, i);
+	//cprintf("%d %d\n", 3);
 	cprintf("6828 decimal is %o octal!\n", 6828);
 
 	// Test the stack backtrace function (lab 1 only)
diff --git a/kern/kdebug.c b/kern/kdebug.c
index 9547143..9326e9a 100644
--- a/kern/kdebug.c
+++ b/kern/kdebug.c
@@ -179,6 +179,28 @@ debuginfo_eip(uintptr_t addr, struct Eipdebuginfo *info)
 	//	Look at the STABS documentation and <inc/stab.h> to find
 	//	which one.
 	// Your code here.
+	stab_binsearch(stabs, &lline, &rline, N_SLINE, addr);
+	if (lline <= rline) {
+		info->eip_line = stabs[rline].n_desc;	//????
+	}
+
+	if (lfun <= rfun) {
+		// stabs[lfun] points to the function name
+		// in the string table, but check bounds just in case.
+		if (stabs[lfun].n_strx < stabstr_end - stabstr)
+			info->eip_fn_name = stabstr + stabs[lfun].n_strx;
+		info->eip_fn_addr = stabs[lfun].n_value;
+		addr -= info->eip_fn_addr;
+		// Search within the function definition for the line number.
+		lline = lfun;
+		rline = rfun;
+	} else {
+		// Couldn't find function stab!  Maybe we're in an assembly
+		// file.  Search the whole file for the line number.
+		info->eip_fn_addr = addr;
+		lline = lfile;
+		rline = rfile;
+	}
 
 
 	// Search backwards from the line number for the relevant filename
diff --git a/kern/monitor.c b/kern/monitor.c
index f8a934a..b6bbc63 100644
--- a/kern/monitor.c
+++ b/kern/monitor.c
@@ -24,6 +24,7 @@ struct Command {
 static struct Command commands[] = {
 	{ "help", "Display this list of commands", mon_help },
 	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
+	{ "backtrace", "Display all stack frames since kernel started", mon_backtrace }, 
 };
 #define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))
 
@@ -59,6 +60,28 @@ int
 mon_backtrace(int argc, char **argv, struct Trapframe *tf)
 {
 	// Your code here.
+	extern uint32_t bootstacktop;
+	uint32_t ebp;
+	struct Eipdebuginfo info;
+	char fname[256] = {0};
+
+	asm volatile("movl %%ebp, %0":"=r"(ebp));
+	cprintf("Stack backtrace:\n");
+	 while (ebp != 0) {			//在entry.S中，ebp被初始化为0
+		memset(fname, 0, sizeof(fname));
+		cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n",\
+				ebp, *((uint32_t*)ebp+1), *((uint32_t*)ebp+2),\
+				*((uint32_t*)ebp+3), *((uint32_t*)ebp+4),\
+				*((uint32_t*)ebp+5),*((uint32_t*)ebp+6));  
+
+		if (debuginfo_eip(*((uint32_t*)ebp+1), &info) == 0) {
+			strncpy(fname, info.eip_fn_name, info.eip_fn_namelen);
+			cprintf("     %s:%d: %s+%x\n", info.eip_file, info.eip_line, fname, info.eip_fn_addr);
+		} else {
+			cprintf("    <unknown>: --%#08x --\n", *((uint32_t*)ebp+2));
+		}
+		ebp = *(uint32_t*)ebp;
+	} 
 	return 0;
 }
 
diff --git a/lib/printfmt.c b/lib/printfmt.c
index 28e01c9..af23abe 100644
--- a/lib/printfmt.c
+++ b/lib/printfmt.c
@@ -206,10 +206,9 @@ vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list ap)
 		// (unsigned) octal
 		case 'o':
 			// Replace this with your code.
-			putch('X', putdat);
-			putch('X', putdat);
-			putch('X', putdat);
-			break;
+			num = getuint(&ap, lflag);
+			base = 8;
+			goto number;
 
 		// pointer
 		case 'p':
