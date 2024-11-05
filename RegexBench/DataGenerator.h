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
    bool SaveFormatList(const CString& filePath); // ���� �޼��� �߰�
    bool ValidateFormat(const CString& format, CString& errorMessage) const;

    // Getter �޼���
    const std::vector<CString>& GetFormatList() const {
        return m_formatList;
    }

    // Setter �޼��� (�ʿ信 ����)
    void AddFormat(const CString& format) {
        m_formatList.push_back(format);
    }

    void RemoveFormat(int index) {
        if (index >= 0 && index < static_cast<int>(m_formatList.size())) {
            m_formatList.erase(m_formatList.begin() + index);
        }
    }

private:

    std::vector<CString> m_formatList; // ���� ���ڿ� ����Ʈ

    // �÷��̽�Ȧ���� ���� ������ ��ü�ϴ� �Լ�
    CString ReplacePlaceholders(const CString& format);

    // �÷��̽�Ȧ���� ���εǴ� �Լ�
    CString GetRandomValue(const CString& placeholder);

    bool IsValidPlaceholder(const CString& placeholder) const;

    // ���� ���� ��ü
    std::mt19937 m_gen;
    std::uniform_int_distribution<> m_dist_event;
    std::uniform_int_distribution<> m_dist_id;
    std::uniform_int_distribution<> m_dist_ip;
    std::uniform_int_distribution<> m_dist_num;
    std::uniform_int_distribution<> m_dist_name;
    std::uniform_int_distribution<> m_dist_domain;
    std::uniform_int_distribution<> m_dist_phone;
    std::uniform_int_distribution<> m_dist_rrn_first_digit; // ���ڸ� ù��° ����
    std::uniform_int_distribution<> m_dist_rrn_year;        // ���� (00-99)
    std::uniform_int_distribution<> m_dist_rrn_month;       // �� (01-12)
    std::uniform_int_distribution<> m_dist_rrn_day;         // �� (01-31)
    std::uniform_int_distribution<> m_dist_rrn_rest;        // ���ڸ� ������ 6�ڸ�

    // RRN ���� �Լ�
    CString GenerateRRN();
};
