; ModuleID = 'test/test2_mem2reg.bc'
source_filename = "test/test2.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local void @f(ptr noundef %a, ptr noundef %b, ptr noundef %c, ptr noundef %d, i32 noundef %n, i32 noundef %m) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp slt i32 %i.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %idxprom = sext i32 %i.0 to i64
  %arrayidx = getelementptr inbounds i32, ptr %a, i64 %idxprom
  store i32 0, ptr %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i.0, 1
  br label %for.cond, !llvm.loop !6

for.end:                                          ; preds = %for.cond
  br label %for.cond2

for.cond2:                                        ; preds = %for.inc9, %for.end
  %i1.0 = phi i32 [ 0, %for.end ], [ %inc10, %for.inc9 ]
  %cmp3 = icmp slt i32 %i1.0, %n
  br i1 %cmp3, label %for.body4, label %for.end11

for.body4:                                        ; preds = %for.cond2
  %idxprom5 = sext i32 %i1.0 to i64
  %arrayidx6 = getelementptr inbounds i32, ptr %a, i64 %idxprom5
  %0 = load i32, ptr %arrayidx6, align 4
  %idxprom7 = sext i32 %i1.0 to i64
  %arrayidx8 = getelementptr inbounds i32, ptr %b, i64 %idxprom7
  store i32 %0, ptr %arrayidx8, align 4
  br label %for.inc9

for.inc9:                                         ; preds = %for.body4
  %inc10 = add nsw i32 %i1.0, 1
  br label %for.cond2, !llvm.loop !8

for.end11:                                        ; preds = %for.cond2
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
