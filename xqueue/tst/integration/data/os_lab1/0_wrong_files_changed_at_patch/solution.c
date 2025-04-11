diff --git a/grade-lab1 b/grade-lab1
index 94bcb0f..c83cd93 100755
--- a/grade-lab1
+++ b/grade-lab1
@@ -10,6 +10,9 @@ r = Runner(save("jos.out"),
 def test_jos():
     r.run_qemu()
 
+
+
+
 @test(20, parent=test_jos)
 def test_printf():
     r.match("6828 decimal is 15254 octal!")
