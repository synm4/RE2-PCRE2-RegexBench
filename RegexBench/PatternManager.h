// PatternManager.h
#pragma once

#include "pch.h"
#include "Pattern.h"
#include <afxstr.h>
#include "Utils.h"
#include <re2/re2.h>

class PatternManager
{
public:
    // 싱글톤 인스턴스 접근자
    static PatternManager& GetInstance()
    {
        static PatternManager instance;
        return instance;
    }

    // 패턴 설정 및 가져오기
    const std::vector<PatternReplacement>& GetPatterns() const { return m_patterns; }

    // RE2 컴파일된 패턴 가져오기
    const std::vector<std::unique_ptr<RE2>>& GetCompiledRE2Patterns() const { return m_re2_patterns; }

    // PCRE2 컴파일된 패턴 가져오기
    const std::vector<pcre2_code*>& GetCompiledPCRE2Patterns() const { return m_pcre2_patterns; }

    // 패턴 설정 및 컴파일
    void SetPatterns(const std::vector<PatternReplacement>& patterns)
    {
        std::lock_guard<std::mutex> lock(pcre2_mutex);

        // 기존 PCRE2 패턴 해제
        for (auto re : m_pcre2_patterns)
        {
            pcre2_code_free(re);
        }
        m_pcre2_patterns.clear();

        // 기존 RE2 패턴 해제
        m_re2_patterns.clear();

        // 새로운 패턴 저장
        m_patterns = patterns;

        // 새로운 PCRE2 패턴 컴파일
        for (const auto& pattern : patterns) {
            int errornumber;
            PCRE2_SIZE erroroffset;
            std::string patternUTF = Utils::CStringToUTF8(pattern.pattern);
            pcre2_code* re = pcre2_compile(
                (PCRE2_SPTR8)patternUTF.c_str(),
                PCRE2_ZERO_TERMINATED,
                PCRE2_UTF,
                &errornumber,
                &erroroffset,
                NULL
            );

            if (re == NULL) {
                // 컴파일 오류 처리
                PCRE2_UCHAR buffer[256];
                pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
                CString msg;
                msg.Format(_T("PCRE2 패턴 컴파일 오류: %S (오류 위치: %d)"), buffer, (int)erroroffset);
                AfxMessageBox(msg);
                continue;
            }

            // JIT 최적화 적용
            if (pcre2_jit_compile(re, PCRE2_JIT_COMPLETE) != 0) {
                pcre2_code_free(re);  // JIT 컴파일 실패 시 메모리 해제
                continue;             // 또는 실패 처리 후 계속 진행 가능
            }

            m_pcre2_patterns.push_back(re);
        }

        // 새로운 RE2 패턴 컴파일
        for (const auto& pattern : patterns)
        {
            std::string patternUTF = Utils::CStringToUTF8(pattern.pattern);
            std::unique_ptr<RE2> re2_pattern = std::make_unique<RE2>(patternUTF);

            if (!re2_pattern->ok())
            {
                //CString msg;
                //msg.Format(_T("RE2 패턴 컴파일 오류: %S"), RE2::ErrorString(re2_pattern->error()).c_str());
                AfxMessageBox(L"RE2 패턴 컴파일 오류");
                continue;
            }

            m_re2_patterns.push_back(std::move(re2_pattern));
        }
    }

    // 기본 패턴 설정
    void SetDefaultPatterns();

private:
    // 생성자, 소멸자, 복사 생성자 및 할당 연산자 비활성화
    PatternManager() {}
    ~PatternManager() {}
    PatternManager(const PatternManager&) = delete;
    PatternManager& operator=(const PatternManager&) = delete;

    std::vector<PatternReplacement> m_patterns;

    // 컴파일된 RE2 패턴 리스트
    std::vector<std::unique_ptr<RE2>> m_re2_patterns;

    // 컴파일된 PCRE2 패턴 리스트
    std::vector<pcre2_code*> m_pcre2_patterns;

    // 동기화를 위한 뮤텍스
    std::mutex pcre2_mutex;
};
