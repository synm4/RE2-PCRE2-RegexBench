// Pattern.h
#pragma once
#include "afxwin.h"

// ���ϰ� ġȯ ���ڿ��� �����ϴ� ����ü
struct PatternReplacement {
    CString pattern;
    CString replacement;

    // �⺻ ������
    PatternReplacement() = default;

    // �Ű������� �ִ� ������
    PatternReplacement(const CString& p, const CString& r) : pattern(p), replacement(r) {}

    // ���� ������
    PatternReplacement(const PatternReplacement& other) = default;

    // �̵� ������
    PatternReplacement(PatternReplacement&& other) noexcept = default;

    // ���� �Ҵ� ������
    PatternReplacement& operator=(const PatternReplacement& other) = default;

    // �̵� �Ҵ� ������
    PatternReplacement& operator=(PatternReplacement&& other) noexcept = default;
};