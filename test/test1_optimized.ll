; ModuleID = 'test/test1_mem2reg.ll'
source_filename = "test/test1.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local void @f(ptr noundef %a, ptr noundef %b, ptr noundef %c, ptr noundef %d, i32 noundef %n, i32 noundef %m) #0 {
entry:
  br label %do.body

do.body:                                          ; preds = %do.cond, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %do.cond ]
  %idxprom = sext i32 %i.0 to i64
  %arrayidx = getelementptr inbounds i32, ptr %a, i64 %idxprom
  store i32 0, ptr %arrayidx, align 4
  %inc = add nsw i32 %i.0, 1
  br label %do.body1

do.cond:                                          ; preds = %do.body1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %do.body, label %do.end9, !llvm.loop !6

do.body1:                                         ; preds = %do.body
  %idxprom2 = sext i32 %i.0 to i64
  %arrayidx3 = getelementptr inbounds i32, ptr %a, i64 %idxprom2
  %0 = load i32, ptr %arrayidx3, align 4
  %idxprom4 = sext i32 %i.0 to i64
  %arrayidx5 = getelementptr inbounds i32, ptr %b, i64 %idxprom4
  store i32 %0, ptr %arrayidx5, align 4
  %inc6 = add nsw i32 %i.0, 1
  br label %do.cond

do.end9:                                          ; preds = %do.cond
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
