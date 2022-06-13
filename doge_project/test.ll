; ModuleID = 'Doge Language'
source_filename = "Doge Language"

%string = type { i8* }

@0 = private unnamed_addr constant [16 x i8] c"Enter precision\00", align 1
@1 = private unnamed_addr constant [3 x i8] c": \00", align 1
@2 = private unnamed_addr constant [2 x i8] c"\0A\00", align 1

declare i8* @charsIn()

declare void @printChars_chars(i8*)

declare i8* @toChars_float(float)

declare i8* @toChars_int(i32)

declare float @toFloat_int(i32)

declare float @toFloat_chars(i8*)

declare i32 @toInt_float(float)

declare i8* @concat_chars_chars(i8*, i8*)

declare float @power_float_float(float, float)

define void @string_class_string_chars(i8* %c, %string* %this) {
entry:
  %m_chars = getelementptr %string, %string* %this, i64 0, i32 0
  store i8* %c, i8** %m_chars, align 8
  ret void
}

define void @string_class_string_int(i32 %i, %string* %this) {
entry:
  %0 = call i8* @toChars_int(i32 %i)
  %m_chars = getelementptr %string, %string* %this, i64 0, i32 0
  store i8* %0, i8** %m_chars, align 8
  ret void
}

define void @string_class_string_float(float %f, %string* %this) {
entry:
  %0 = call i8* @toChars_float(float %f)
  %m_chars = getelementptr %string, %string* %this, i64 0, i32 0
  store i8* %0, i8** %m_chars, align 8
  ret void
}

define %string @string_class_plus_chars(i8* %c, %string* %this) {
entry:
  %m_chars = getelementptr %string, %string* %this, i64 0, i32 0
  %m_chars_loaded = load i8*, i8** %m_chars, align 8
  %0 = call i8* @concat_chars_chars(i8* %m_chars_loaded, i8* %c)
  call void @string_class_string_chars(i8* %0, %string* %this)
  ret void <badref>
}

define %string @string_class_plus_string(%string %b, %string* %this) {
entry:
  %b1 = alloca %string, align 8
  %0 = extractvalue %string %b, 0
  %1 = getelementptr inbounds %string, %string* %b1, i64 0, i32 0
  store i8* %0, i8** %1, align 8
  %m_chars = getelementptr %string, %string* %this, i64 0, i32 0
  %m_chars_loaded = load i8*, i8** %m_chars, align 8
  %2 = call i8* @concat_chars_chars(i8* %m_chars_loaded, i8* %0)
  call void @string_class_string_chars(i8* %2, %string* nonnull %b1)
  ret void <badref>
}

define void @string_class_print(%string* %this) {
entry:
  %m_chars = getelementptr %string, %string* %this, i64 0, i32 0
  %m_chars_loaded = load i8*, i8** %m_chars, align 8
  call void @printChars_chars(i8* %m_chars_loaded)
  ret void
}

define void @print_int(i32 %i) {
entry:
  %temp_class = alloca %string, align 8
  %s = alloca %string, align 8
  call void @string_class_string_int(i32 %i, %string* nonnull %temp_class)
  %0 = getelementptr inbounds %string, %string* %temp_class, i64 0, i32 0
  %temp_class_constructor_loaded.unpack = load i8*, i8** %0, align 8
  %1 = getelementptr inbounds %string, %string* %s, i64 0, i32 0
  store i8* %temp_class_constructor_loaded.unpack, i8** %1, align 8
  call void @string_class_print(%string* nonnull %s)
  ret void
}

define void @print_float(float %f) {
entry:
  %temp_class = alloca %string, align 8
  %s = alloca %string, align 8
  call void @string_class_string_float(float %f, %string* nonnull %temp_class)
  %0 = getelementptr inbounds %string, %string* %temp_class, i64 0, i32 0
  %temp_class_constructor_loaded.unpack = load i8*, i8** %0, align 8
  %1 = getelementptr inbounds %string, %string* %s, i64 0, i32 0
  store i8* %temp_class_constructor_loaded.unpack, i8** %1, align 8
  call void @string_class_print(%string* nonnull %s)
  ret void
}

define void @print_chars(i8* %s) {
entry:
  call void @printChars_chars(i8* %s)
  ret void
}

define void @print_string(%string %s) {
entry:
  %s1 = alloca %string, align 8
  %0 = extractvalue %string %s, 0
  %1 = getelementptr inbounds %string, %string* %s1, i64 0, i32 0
  store i8* %0, i8** %1, align 8
  call void @string_class_print(%string* nonnull %s1)
  ret void
}

define float @floatIn() {
entry:
  %0 = call i8* @charsIn()
  %1 = call float @toFloat_chars(i8* %0)
  ret float %1
}

define float @pi_float(float %n) {
entry:
  br label %loop

loop:                                             ; preds = %loop, %entry
  %sum.0 = phi float [ 0.000000e+00, %entry ], [ %add_float5, %loop ]
  %sign.0 = phi float [ 1.000000e+00, %entry ], [ %multiply_float8, %loop ]
  %i.0 = phi float [ 0.000000e+00, %entry ], [ %add_float11, %loop ]
  %multiply_float = fmul float %i.0, 2.000000e+00
  %add_float = fadd float %multiply_float, 1.000000e+00
  %divide_float = fdiv float %sign.0, %add_float
  %add_float5 = fadd float %sum.0, %divide_float
  %multiply_float8 = fneg float %sign.0
  %add_float11 = fadd float %i.0, 1.000000e+00
  %less_floats = fcmp ult float %add_float11, %n
  br i1 %less_floats, label %loop, label %after_loop

after_loop:                                       ; preds = %loop
  %multiply_float16 = fmul float %add_float5, 4.000000e+00
  ret float %multiply_float16
}

define i32 @main() {
entry:
  %temp_class = alloca %string, align 8
  %s = alloca %string, align 8
  call void @string_class_string_chars(i8* getelementptr inbounds ([16 x i8], [16 x i8]* @0, i64 0, i64 0), %string* nonnull %temp_class)
  %0 = getelementptr inbounds %string, %string* %temp_class, i64 0, i32 0
  %temp_class_constructor_loaded.unpack = load i8*, i8** %0, align 8
  %1 = getelementptr inbounds %string, %string* %s, i64 0, i32 0
  store i8* %temp_class_constructor_loaded.unpack, i8** %1, align 8
  %plus_overload = call %string @string_class_plus_chars(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @1, i64 0, i64 0), %string* nonnull %s)
  %2 = extractvalue %string %plus_overload, 0
  store i8* %2, i8** %1, align 8
  call void @print_string(%string %plus_overload)
  %3 = call float @floatIn()
  call void @print_chars(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @2, i64 0, i64 0))
  %4 = call float @pi_float(float %3)
  call void @print_float(float %4)
  ret i32 0
}
