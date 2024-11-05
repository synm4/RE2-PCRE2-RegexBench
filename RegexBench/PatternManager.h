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
    // �̱��� �ν��Ͻ� ������
    static PatternManager& GetInstance()
    {
        static PatternManager instance;
        return instance;
    }

    // ���� ���� �� ��������
    const std::vector<PatternReplacement>& GetPatterns() const { return m_patterns; }

    // RE2 �����ϵ� ���� ��������
    const std::vector<std::unique_ptr<RE2>>& GetCompiledRE2Patterns() const { return m_re2_patterns; }

    // PCRE2 �����ϵ� ���� ��������
    const std::vector<pcre2_code*>& GetCompiledPCRE2Patterns() const { return m_pcre2_patterns; }

    // ���� ���� �� ������
    void SetPatterns(const std::vector<PatternReplacement>& patterns)
    {
        std::lock_guard<std::mutex> lock(pcre2_mutex);

        // ���� PCRE2 ���� ����
        for (auto re : m_pcre2_patterns)
        {
            pcre2_code_free(re);
        }
        m_pcre2_patterns.clear();

        // ���� RE2 ���� ����
        m_re2_patterns.clear();

        // ���ο� ���� ����
        m_patterns = patterns;

        // ���ο� PCRE2 ���� ������
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
                // ������ ���� ó��
                PCRE2_UCHAR buffer[256];
                pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
                CString msg;
                msg.Format(_T("PCRE2 ���� ������ ����: %S (���� ��ġ: %d)"), buffer, (int)erroroffset);
                AfxMessageBox(msg);
                continue;
            }

            // JIT ����ȭ ����
            if (pcre2_jit_compile(re, PCRE2_JIT_COMPLETE) != 0) {
                pcre2_code_free(re);  // JIT ������ ���� �� �޸� ����
                continue;             // �Ǵ� ���� ó�� �� ��� ���� ����
            }

            m_pcre2_patterns.push_back(re);
        }

        // ���ο� RE2 ���� ������
        for (const auto& pattern : patterns)
        {
            std::string patternUTF = Utils::CStringToUTF8(pattern.pattern);
            std::unique_ptr<RE2> re2_pattern = std::make_unique<RE2>(patternUTF);

            if (!re2_pattern->ok())
            {
                //CString msg;
                //msg.Format(_T("RE2 ���� ������ ����: %S"), RE2::ErrorString(re2_pattern->error()).c_str());
                AfxMessageBox(L"RE2 ���� ������ ����");
                continue;
            }

            m_re2_patterns.push_back(std::move(re2_pattern));
        }
    }

    // �⺻ ���� ����
    void SetDefaultPatterns();

private:
    // ������, �Ҹ���, ���� ������ �� �Ҵ� ������ ��Ȱ��ȭ
    PatternManager() {}
    ~PatternManager() {}
    PatternManager(const PatternManager&) = delete;
    PatternManager& operator=(const PatternManager&) = delete;

    std::vector<PatternReplacement> m_patterns;

    // �����ϵ� RE2 ���� ����Ʈ
    std::vector<std::unique_ptr<RE2>> m_re2_patterns;

    // �����ϵ� PCRE2 ���� ����Ʈ
    std::vector<pcre2_code*> m_pcre2_patterns;

    // ����ȭ�� ���� ���ؽ�
    std::mutex pcre2_mutex;
};
