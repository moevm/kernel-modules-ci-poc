diff --git a/kern/monitor.c b/kern/monitor.c
index f8a934a..4cda581 100644
--- a/kern/monitor.c
+++ b/kern/monitor.c
@@ -59,6 +59,7 @@ int
 mon_backtrace(int argc, char **argv, struct Trapframe *tf)
 {
 	// Your code here.
+        cprintf("Kernel executable memory footprint: ");
 	return 0;
 }
 
