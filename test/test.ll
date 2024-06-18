; ModuleID = 'test/test.bc'
source_filename = "test/test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local void @f(ptr noundef %a, ptr noundef %b, ptr noundef %c, ptr noundef %d, i32 noundef %n, i32 noundef %m) #0 {
entry:
  %a.addr = alloca ptr, align 8
  %b.addr = alloca ptr, align 8
  %c.addr = alloca ptr, align 8
  %d.addr = alloca ptr, align 8
  %n.addr = alloca i32, align 4
  %m.addr = alloca i32, align 4
  %i = alloca i32, align 4
  %i1 = alloca i32, align 4
  %i12 = alloca i32, align 4
  %ciao = alloca i32, align 4
  store ptr %a, ptr %a.addr, align 8
  store ptr %b, ptr %b.addr, align 8
  store ptr %c, ptr %c.addr, align 8
  store ptr %d, ptr %d.addr, align 8
  store i32 %n, ptr %n.addr, align 4
  store i32 %m, ptr %m.addr, align 4
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, ptr %i, align 4
  %1 = load i32, ptr %n.addr, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load ptr, ptr %a.addr, align 8
  %3 = load i32, ptr %i, align 4
  %idxprom = sext i32 %3 to i64
  %arrayidx = getelementptr inbounds i32, ptr %2, i64 %idxprom
  store i32 0, ptr %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %4 = load i32, ptr %i, align 4
  %inc = add nsw i32 %4, 1
  store i32 %inc, ptr %i, align 4
  br label %for.cond, !llvm.loop !6

for.end:                                          ; preds = %for.cond
  store i32 0, ptr %i1, align 4
  br label %for.cond2

for.cond2:                                        ; preds = %for.inc9, %for.end
  %5 = load i32, ptr %i1, align 4
  %6 = load i32, ptr %n.addr, align 4
  %cmp3 = icmp slt i32 %5, %6
  br i1 %cmp3, label %for.body4, label %for.end11

for.body4:                                        ; preds = %for.cond2
  %7 = load ptr, ptr %a.addr, align 8
  %8 = load i32, ptr %i1, align 4
  %idxprom5 = sext i32 %8 to i64
  %arrayidx6 = getelementptr inbounds i32, ptr %7, i64 %idxprom5
  %9 = load i32, ptr %arrayidx6, align 4
  %10 = load ptr, ptr %b.addr, align 8
  %11 = load i32, ptr %i1, align 4
  %idxprom7 = sext i32 %11 to i64
  %arrayidx8 = getelementptr inbounds i32, ptr %10, i64 %idxprom7
  store i32 %9, ptr %arrayidx8, align 4
  br label %for.inc9

for.inc9:                                         ; preds = %for.body4
  %12 = load i32, ptr %i1, align 4
  %inc10 = add nsw i32 %12, 1
  store i32 %inc10, ptr %i1, align 4
  br label %for.cond2, !llvm.loop !8

for.end11:                                        ; preds = %for.cond2
  store i32 0, ptr %i12, align 4
  br label %for.cond13

for.cond13:                                       ; preds = %for.inc16, %for.end11
  %13 = load i32, ptr %i12, align 4
  %14 = load i32, ptr %n.addr, align 4
  %cmp14 = icmp slt i32 %13, %14
  br i1 %cmp14, label %for.body15, label %for.end18

for.body15:                                       ; preds = %for.cond13
  store i32 19, ptr %ciao, align 4
  br label %for.inc16

for.inc16:                                        ; preds = %for.body15
  %15 = load i32, ptr %i12, align 4
  %inc17 = add nsw i32 %15, 1
  store i32 %inc17, ptr %i12, align 4
  br label %for.cond13, !llvm.loop !9

for.end18:                                        ; preds = %for.cond13
  ret void
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 17.0.6"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
