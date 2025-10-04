; ModuleID = 'cmini'
source_filename = "cmini"

define i32 @main() {
entry:
  %t1 = alloca i32*
  ; map x -> %t1
  %t2 = getelementptr i32, i32* %t1, i32 1
  %t3 = getelementptr i32, i32* %t2, i32 2
  store i32 5, i32* %t3
  %t4 = getelementptr i32, i32* %t1, i32 1
  %t5 = getelementptr i32, i32* %t4, i32 2
  %t6 = load i32, i32* %t5
  ret i32 %t6
}

