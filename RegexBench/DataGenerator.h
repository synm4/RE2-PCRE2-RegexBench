#pragma once
#include "Utils.h"
#include "pch.h"

class DataGenerator
{
public:
    DataGenerator();
    ~DataGenerator();

    bool LoadFormatList(const CString& loadFilePath);
    void SetDefaultFormatList();
    void GenerateTestData(const CString& filePath, size_t numLines);
    bool SaveFormatList(const CString& filePath); // 저장 메서드 추가
    bool ValidateFormat(const CString& format, CString& errorMessage) const;

    // Getter 메서드
    const std::vector<CString>& GetFormatList() const {
        return m_formatList;
    }

    // Setter 메서드 (필요에 따라)
    void AddFormat(const CString& format) {
        m_formatList.push_back(format);
    }

    void RemoveFormat(int index) {
        if (index >= 0 && index < static_cast<int>(m_formatList.size())) {
            m_formatList.erase(m_formatList.begin() + index);
        }
    }

private:

    std::vector<CString> m_formatList; // 형식 문자열 리스트

    // 플레이스홀더를 랜덤 값으로 대체하는 함수
    CString ReplacePlaceholders(const CString& format);

    // 플레이스홀더에 매핑되는 함수
    CString GetRandomValue(const CString& placeholder);

    bool IsValidPlaceholder(const CString& placeholder) const;

    // 랜덤 분포 객체
    std::mt19937 m_gen;
    std::uniform_int_distribution<> m_dist_event;
    std::uniform_int_distribution<> m_dist_id;
    std::uniform_int_distribution<> m_dist_ip;
    std::uniform_int_distribution<> m_dist_num;
    std::uniform_int_distribution<> m_dist_name;
    std::uniform_int_distribution<> m_dist_domain;
    std::uniform_int_distribution<> m_dist_phone;
    std::uniform_int_distribution<> m_dist_rrn_first_digit; // 뒷자리 첫번째 숫자
    std::uniform_int_distribution<> m_dist_rrn_year;        // 연도 (00-99)
    std::uniform_int_distribution<> m_dist_rrn_month;       // 월 (01-12)
    std::uniform_int_distribution<> m_dist_rrn_day;         // 일 (01-31)
    std::uniform_int_distribution<> m_dist_rrn_rest;        // 뒷자리 나머지 6자리

    // RRN 생성 함수
    CString GenerateRRN();
};
