; ModuleID = 'cmini'
source_filename = "cmini"

define i32 @main() {
entry:
  %t1 = alloca i32
  ; map x -> %t1
  %t2 = add i32 1, 2
  store i32 %t2, i32* %t1
  ; load from alloca of x
  %t3 = load i32, i32* %t1
  ret i32 %t3
}

