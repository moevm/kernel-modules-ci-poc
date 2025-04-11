diff --git a/kern/monitor.c b/kern/monitor.c
index f8a934a..f235b6d 100644
--- a/kern/monitor.c
+++ b/kern/monitor.c
@@ -14,6 +14,7 @@
 #define CMDBUF_SIZE	80	// enough for one VGA text line
 
 
+idfdfd
 struct Command {
 	const char *name;
 	const char *desc;
