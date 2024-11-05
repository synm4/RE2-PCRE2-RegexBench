#include "pch.h"
#include "DataGenerator.h"
#include "Utils.h"
#include <fstream>
#include <regex>
#include <exception>
#include <filesystem>


DataGenerator::DataGenerator()
    : m_dist_event(0, 10),
    m_dist_id(0, 9999),
    m_dist_ip(0, 255),
    m_dist_num(0, 9999),
    m_dist_name(0, 9999),
    m_dist_domain(0, 2),
    m_dist_phone(0, 9999),
    m_dist_rrn_first_digit(1, 4),  // 뒷자리 첫번째 숫자 1~4
    m_dist_rrn_year(0, 99),        // 연도 00~99
    m_dist_rrn_month(1, 12),       // 월 01~12
    m_dist_rrn_day(1, 28),         // 일 01~28 (간단하게 28일까지로 제한)
    m_dist_rrn_rest(0, 999999)     // 뒷자리 나머지 6자리
{
    std::random_device rd;
    m_gen = std::mt19937(rd());
}

DataGenerator::~DataGenerator()
{
}

bool DataGenerator::LoadFormatList(const CString& loadFilePath)
{
    std::ifstream inFile(Utils::CStringToUTF8(loadFilePath), std::ios::binary);
    if (!inFile.is_open())
    {
        return false;
    }

    m_formatList.clear();

    std::string line;
    bool isFirstLine = true;
    while (std::getline(inFile, line))
    {
        if (isFirstLine)
        {
            // UTF-8 BOM 제거
            if (line.size() >= 3 &&
                (unsigned char)line[0] == 0xEF &&
                (unsigned char)line[1] == 0xBB &&
                (unsigned char)line[2] == 0xBF)
            {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        CString cstrLine = Utils::UTF8ToCString(line);
        m_formatList.push_back(cstrLine);
    }

    inFile.close();
    return true;
}


void DataGenerator::SetDefaultFormatList()
{
    m_formatList = {
        _T("{time} 사용자 로그인: ID=user{id}, IP={ip}.{ip}.{ip}.{ip}"),
        _T("{time} 결제 완료: 카드번호={num}-{num}-{num}-{num}, 금액=₩{num},000"),
        _T("{time} 회원가입: 이름={name}, 이메일={email}, 전화번호={phone}"),
        _T("{time} 비밀번호 변경: ID=user{id}"),
        _T("{time} 데이터 삭제 요청: 주민등록번호={rrn}"),
        _T("{time} 오류 발생: 코드={num}, 메시지=내부 서버 오류"),
        _T("{time} 배송지 변경: 주소=서울특별시 중구 세종대로 {num}"),
        _T("{time} 관리자 접속: ID=admin, IP={ip}.{ip}.{ip}.{ip}"),
        _T("{time} 이메일 발송: 수신자={email}"),
        _T("{time} 파일 업로드: 파일명=document{id}.pdf")
    };
}

void DataGenerator::GenerateTestData(const CString& filePath, size_t numLines)
{
    try
    {
        // CString을 std::wstring으로 변환
        std::wstring wideFilePath = filePath.GetString();

        // std::filesystem::path 사용
        std::filesystem::path path(wideFilePath);

        // ofstream 열기 (C++17 이상에서는 std::ofstream이 유니코드 경로를 지원)
        std::ofstream fileStream(path, std::ios::binary);
        if (!fileStream.is_open())
        {
            throw std::runtime_error("파일을 생성할 수 없습니다.");
        }

        // UTF-8 BOM 추가 (옵션)
        fileStream << "\xEF\xBB\xBF";

        if (m_formatList.empty())
        {
            throw std::runtime_error("생성할 형식 문자열이 없습니다.");
        }

        // 버퍼링을 위한 문자열 스트림 사용
        std::stringstream buffer;

        for (size_t i = 0; i < numLines; ++i)
        {
            size_t formatIndex = m_dist_event(m_gen) % m_formatList.size();
            CString format = m_formatList[formatIndex];

            CString line = ReplacePlaceholders(format);

            if (line.Find(_T("{")) != -1 || line.Find(_T("}")) != -1)
            {
                throw std::runtime_error("형식 문자열에 정의되지 않은 플레이스홀더가 포함되어 있습니다.");
            }

            std::string utf8Line = Utils::CStringToUTF8(line);
            buffer << utf8Line << "\n";

            // 일정한 라인 수마다 파일에 쓰기
            if (i % 1000 == 0 && i != 0)
            {
                fileStream << buffer.str();
                buffer.str(""); // 버퍼 초기화
                buffer.clear();
            }
        }

        // 남은 데이터 쓰기
        fileStream << buffer.str();

        fileStream.close();
    }
    catch (const std::exception& ex)
    {
        // 예외를 로깅하거나 추가 처리를 할 수 있습니다.
        throw; // 예외를 상위로 전달
    }
}

bool DataGenerator::SaveFormatList(const CString& filePath)
{
    std::ofstream outFile(CT2A(filePath), std::ios::out | std::ios::trunc | std::ios::binary);
    if (!outFile.is_open())
        return false;

    // UTF-8 BOM 추가
    outFile << "\xEF\xBB\xBF";

    for (const auto& format : m_formatList)
    {
        std::string line = Utils::CStringToUTF8(format);
        outFile << line << "\n";
    }

    outFile.close();
    return true;
}

bool DataGenerator::ValidateFormat(const CString& format, CString& errorMessage) const
{
    std::vector<CString> placeholders;
    int pos = 0;
    while (true)
    {
        int startPos = format.Find(_T('{'), pos);
        if (startPos == -1)
            break;
        int endPos = format.Find(_T('}'), startPos);
        if (endPos == -1)
        {
            errorMessage = _T("형식 문자열에 닫는 중괄호('}')가 없습니다.");
            return false;
        }

        CString placeholder = format.Mid(startPos + 1, endPos - startPos - 1);
        if (placeholder.IsEmpty())
        {
            errorMessage = _T("빈 플레이스홀더('{}')가 있습니다.");
            return false;
        }

        // 플레이스홀더 유효성 검사
        if (!IsValidPlaceholder(placeholder))
        {
            CString msg;
            msg.Format(_T("정의되지 않은 플레이스홀더: {%s}"), placeholder);
            errorMessage = msg;
            return false;
        }

        pos = endPos + 1;
    }

    // 모든 플레이스홀더가 유효함
    return true;
}

CString DataGenerator::ReplacePlaceholders(const CString& format)
{
    CString result = format;
    int pos = 0;
    while (true)
    {
        int startPos = result.Find(_T('{'), pos);
        if (startPos == -1)
            break;
        int endPos = result.Find(_T('}'), startPos);
        if (endPos == -1)
            break;

        CString placeholder = result.Mid(startPos + 1, endPos - startPos - 1);
        CString placeholderValue = GetRandomValue(placeholder);

        // 정의되지 않은 플레이스홀더 처리
        if (placeholderValue.IsEmpty())
        {
            placeholderValue = _T("");
        }

        result = result.Left(startPos) + placeholderValue + result.Mid(endPos + 1);
        pos = startPos + placeholderValue.GetLength(); // 다음 위치로 이동
    }

    return result;
}


CString DataGenerator::GetRandomValue(const CString& placeholder)
{
    if (placeholder == _T("time"))
    {
        CTime time = CTime::GetCurrentTime();
        return time.Format(_T("[%Y-%m-%d %H:%M:%S]"));
    }
    else if (placeholder == _T("id"))
    {
        return CString(std::to_wstring(m_dist_id(m_gen)).c_str());
    }
    else if (placeholder == _T("ip"))
    {
        return CString(std::to_wstring(m_dist_ip(m_gen)).c_str());
    }
    else if (placeholder == _T("num"))
    {
        return CString(std::to_wstring(m_dist_num(m_gen)).c_str());
    }
    else if (placeholder == _T("name"))
    {
        int nameNum = m_dist_name(m_gen);
        CString name;
        name.Format(_T("홍길동%d"), nameNum);
        return name;
    }
    else if (placeholder == _T("email"))
    {
        std::vector<CString> emailDomains = { _T("naver.com"), _T("daum.net"), _T("gmail.com") };
        int domainIndex = m_dist_domain(m_gen);
        if (domainIndex >= emailDomains.size()) domainIndex = 0; // 안전장치
        CString email;
        email.Format(_T("user%d@%s"), m_dist_id(m_gen), emailDomains[domainIndex]);
        return email;
    }
    else if (placeholder == _T("phone"))
    {
        int phoneMid = m_dist_phone(m_gen);
        int phoneEnd = m_dist_phone(m_gen);
        CString phone;
        phone.Format(_T("010-%04d-%04d"), phoneMid, phoneEnd);
        return phone;
    }
    else if (placeholder == _T("rrn")) // 추가된 부분
    {
        return GenerateRRN();
    }
    // 추가 플레이스홀더 처리
    else
    {
        return _T(""); // 정의되지 않은 플레이스홀더는 빈 문자열로 대체
    }
}

bool DataGenerator::IsValidPlaceholder(const CString& placeholder) const
{
    // 현재 정의된 모든 플레이스홀더를 나열
    static const std::vector<CString> validPlaceholders = {
        _T("time"), _T("id"), _T("ip"), _T("num"),
        _T("name"), _T("email"), _T("phone"),
        _T("rrn")
        // 추가로 정의된 플레이스홀더가 있다면 여기에 추가
    };

    for (const auto& ph : validPlaceholders)
    {
        if (placeholder.CompareNoCase(ph) == 0)
            return true;
    }
    return false;
}

CString DataGenerator::GenerateRRN()
{
    // 뒷자리 첫번째 숫자에 따라 세기 결정
    int firstDigit = m_dist_rrn_first_digit(m_gen);
    int century;
    if (firstDigit == 1 || firstDigit == 2)
        century = 1900;
    else if (firstDigit == 3 || firstDigit == 4)
        century = 2000;
    else
        century = 1900; // 안전장치

    // 연도 생성 (예: 1900년대이면 00~99을 더하여 1900~1999)
    int year = m_dist_rrn_year(m_gen) + (century / 100);

    int month = m_dist_rrn_month(m_gen);

    // 월에 따른 일수 설정
    int maxDay;
    if (month == 2)
        maxDay = 28; // 윤년 고려하지 않음
    else if (month == 4 || month == 6 || month == 9 || month == 11)
        maxDay = 30;
    else
        maxDay = 31;

    std::uniform_int_distribution<> dist_day(1, maxDay);
    int day = dist_day(m_gen);

    CString strYear, strMonth, strDayStr;
    strYear.Format(_T("%02d"), year % 100); // 두 자리 연도로 변환
    strMonth.Format(_T("%02d"), month);
    strDayStr.Format(_T("%02d"), day);

    CString frontPart = strYear + strMonth + strDayStr;

    // 뒷자리 생성 (firstDigit 이미 결정됨)
    int restDigits = m_dist_rrn_rest(m_gen);

    CString strFirstDigit, strRestDigits;
    strFirstDigit.Format(_T("%d"), firstDigit);
    strRestDigits.Format(_T("%06d"), restDigits);

    CString backPart = strFirstDigit + strRestDigits;

    return frontPart + _T("-") + backPart;
}