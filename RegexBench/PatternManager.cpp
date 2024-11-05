// PatternManager.cpp
#include "pch.h"
#include "PatternManager.h"

// 소스 파일에는 별도의 구현이 필요 없습니다.
// 모든 로직은 헤더 파일 내에 포함되어 있습니다.

// 외부 전역 변수로 pcre2_patterns와 pcre2_mutex 선언

void PatternManager::SetDefaultPatterns()
{
    std::vector<PatternReplacement> defaultPatterns = {
        PatternReplacement(_T("\\b\\d{6}-\\d{7}\\b"), _T("xxxxxx-xxxxxxx")),
        PatternReplacement(_T("\\b010-\\d{4}-\\d{4}\\b"), _T("010-xxxx-xxxx")),
        PatternReplacement(_T("\\b\\d{4}-\\d{4}-\\d{4}-\\d{4}\\b"), _T("xxxx-xxxx-xxxx-xxxx")),
        PatternReplacement(_T("[\\w.-]+@[\\w.-]+\\.[a-zA-Z]{2,6}"), _T("email@example.com")),
        PatternReplacement(_T("주소=[^\\r\\n]+"), _T("주소=REDACTED")),
        PatternReplacement(_T("\\b\\d{1,3}(?:\\.\\d{1,3}){3}\\b"), _T("xxx.xxx.xxx.xxx")),
        PatternReplacement(_T("이름=홍길동\\d+"), _T("이름=홍길동")),
        PatternReplacement(_T("(https?:\\/\\/)?(www\\.)?[-a-zA-Z0-9@:%._\\+~#=]{1,256}\\.[a-zA-Z0-9()]{1,6}\\b([-a-zA-Z0-9()@:%_\\+.~#?&//=]*)"), _T("[URL 보호됨]")),
        PatternReplacement(_T("금액=₩\\d+,000"), _T("금액=₩***,000"))
    };

    SetPatterns(defaultPatterns);
}

