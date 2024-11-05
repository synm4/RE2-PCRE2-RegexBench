// Pattern.h
#pragma once
#include "afxwin.h"

// 패턴과 치환 문자열을 저장하는 구조체
struct PatternReplacement {
    CString pattern;
    CString replacement;

    // 기본 생성자
    PatternReplacement() = default;

    // 매개변수가 있는 생성자
    PatternReplacement(const CString& p, const CString& r) : pattern(p), replacement(r) {}

    // 복사 생성자
    PatternReplacement(const PatternReplacement& other) = default;

    // 이동 생성자
    PatternReplacement(PatternReplacement&& other) noexcept = default;

    // 복사 할당 연산자
    PatternReplacement& operator=(const PatternReplacement& other) = default;

    // 이동 할당 연산자
    PatternReplacement& operator=(PatternReplacement&& other) noexcept = default;
};