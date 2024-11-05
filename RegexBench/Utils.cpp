// Utils.cpp
#include "pch.h"
#include "Utils.h"
#include "PatternManager.h"
#include "FolderPathManager.h"

// 정적 멤버 초기화
std::vector<std::unique_ptr<RE2>> Utils::re2_patterns;
std::mutex Utils::re2_mutex;
std::vector<pcre2_code*> Utils::pcre2_patterns;
std::mutex Utils::pcre2_mutex;
// 정적 멤버 변수 정의
std::vector<std::string> Utils::re2_replacements;
std::vector<std::string> Utils::pcre2_replacements;



// UTF-8로 CString을 변환하는 유틸리티 함수
std::string Utils::CStringToUTF8(const CString& str)
{
    if (str.IsEmpty())
        return std::string();

    // 첫 번째 호출: 변환된 문자열의 길이 계산 (null 포함)
    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    if (utf8Length == 0)
    {
        // 변환 실패 시 에러 처리 (예: 예외 던지기)
        DWORD error = GetLastError();
        throw std::runtime_error("WideCharToMultiByte failed with error code " + std::to_string(error));
    }

    // 두 번째 호출: 실제 변환 수행
    std::string utf8Str(utf8Length - 1, 0); // null 종료 문자를 제외한 길이로 초기화
    int result = WideCharToMultiByte(CP_UTF8, 0, str, -1, &utf8Str[0], utf8Length, NULL, NULL);
    if (result == 0)
    {
        // 변환 실패 시 에러 처리
        DWORD error = GetLastError();
        throw std::runtime_error("WideCharToMultiByte failed with error code " + std::to_string(error));
    }

    return utf8Str;
}

// 파일을 읽어 문자열로 반환하는 유틸리티 함수
std::string Utils::ReadFileToString(const CString& filePath)
{
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file)
        return "";

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}


// UTF-8을 CString으로 변환
CString Utils::UTF8ToCString(const std::string& str)
{
    if (str.empty())
        return CString();

    // 변환할 문자열의 길이 계산
    int wideLength = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (wideLength == 0)
    {
        // 변환 실패 시 에러 처리
        DWORD error = GetLastError();
        throw std::runtime_error("MultiByteToWideChar failed with error code " + std::to_string(error));
    }

    // 실제 변환 수행
    CString wideStr;
    wideStr.GetBuffer(wideLength);
    int result = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wideStr.GetBuffer(), wideLength);
    wideStr.ReleaseBuffer();

    if (result == 0)
    {
        // 변환 실패 시 에러 처리
        DWORD error = GetLastError();
        throw std::runtime_error("MultiByteToWideChar failed with error code " + std::to_string(error));
    }

    return wideStr;
}


// NiceNumber 함수
double Utils::NiceNumber(double range, bool round)
{
    double exponent = floor(log10(range));
    double fraction = range / pow(10, exponent);
    double niceFraction;

    if (round)
    {
        if (fraction < 1.5)
            niceFraction = 1;
        else if (fraction < 3)
            niceFraction = 2;
        else if (fraction < 7)
            niceFraction = 5;
        else
            niceFraction = 10;
    }
    else
    {
        if (fraction <= 1)
            niceFraction = 1;
        else if (fraction <= 2)
            niceFraction = 2;
        else if (fraction <= 5)
            niceFraction = 5;
        else
            niceFraction = 10;
    }

    return niceFraction * pow(10, exponent);
}


// RE2를 사용한 파일 처리 함수
double Utils::ProcessFileWithRE2(const CString& filePath, int Index)
{
    // 1. 패턴 가져오기
    const auto& re2_patterns = PatternManager::GetInstance().GetCompiledRE2Patterns();
    const auto& patternList = PatternManager::GetInstance().GetPatterns(); // 패턴 리스트 가져오기

    // 2. 패턴 수 일치 확인
    if (re2_patterns.size() != patternList.size())
    {
        throw std::runtime_error("RE2: Pattern and replacement counts do not match.");
    }

    // 3. 입력 파일 열기 (텍스트 모드로 열기)
    std::ifstream inFile(CT2A(filePath), std::ios::in);
    if (!inFile.is_open())
    {
        throw std::runtime_error("RE2: Failed to open input file.");
    }

    // 4. 출력 파일 준비
    CString outputFileName;
    outputFileName.Format(_T("%s\\output_re2_%d.txt"),
        FolderPathManager::GetInstance().GetResultFolderPath(),
        Index);
    std::ofstream outFile(CT2A(outputFileName), std::ios::out | std::ios::binary);
    if (!outFile.is_open())
    {
        throw std::runtime_error("RE2: Failed to open output file for writing.");
    }

    bool isFirstLine = true; // 첫 번째 줄인지 확인

    // 5. 처리 시간 측정 시작
    auto startTime = std::chrono::high_resolution_clock::now();

    // 6. 파일을 한 줄씩 읽어 처리
    std::string line;
    while (std::getline(inFile, line))
    {
        // 6.1. 첫 번째 줄에서 BOM 제거
        if (isFirstLine)
        {
            isFirstLine = false;
            const std::string BOM = "\xEF\xBB\xBF";
            if (line.compare(0, BOM.size(), BOM) == 0)
            {
                line.erase(0, BOM.size());
            }
        }
        // 6.1. 줄 끝의 '\r' 제거 (Windows 줄 끝 처리)
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        std::string originalLine = line; // 원본 라인 저장 (디버깅용)

        // 6.2. 각 패턴에 대해 치환 수행
        for (size_t i = 0; i < re2_patterns.size(); ++i)
        {
            const auto& re = re2_patterns[i];
            const std::string& replacement = CStringToUTF8(patternList[i].replacement);

            // RE2::GlobalReplace를 사용하여 모든 매칭을 치환
            // RE2::GlobalReplace는 치환 횟수를 반환하지 않으므로, 다른 방법으로 치환 여부를 확인할 수 있음
            RE2::GlobalReplace(&line, *re, replacement);

            // 디버깅용: 치환된 후의 라인 확인 (필요 시 활성화)
            /*
            if (line != originalLine)
            {
                CString debugMsg;
                debugMsg.Format(_T("Pattern %d applied. Original: %s, Modified: %s"),
                                static_cast<int>(i + 1),
                                CA2T(originalLine.c_str()),
                                CA2T(line.c_str()));
                OutputDebugString(debugMsg);
            }
            */
        }

        // 6.3. 치환된 라인을 출력 파일에 기록
        outFile << line << '\n';
    }

    // 7. 처리 시간 측정 종료
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;

    // 8. 파일 닫기
    inFile.close();
    outFile.close();

    // 9. 처리 시간 반환
    return duration.count(); // 밀리초 단위 반환
}

// 수정된 ProcessFileWithPCRE2 함수 정의
PCRE2ProcessResult Utils::ProcessFileWithPCRE2(const CString& filePath, const std::vector<PatternReplacement>& patterns, std::string& outProcessedText)
{
    PCRE2ProcessResult result;
    result.duration_ms = -1.0;
    result.totalDetections = 0;
    result.totalReplacements = 0;
    result.detectionsPerPattern.assign(patterns.size(), 0);

    // 1. 패턴 가져오기
    const auto& pcre2_patterns = PatternManager::GetInstance().GetCompiledPCRE2Patterns();
    const auto& patternList = PatternManager::GetInstance().GetPatterns(); // 패턴 리스트 가져오기

    // 2. 패턴 수 일치 확인
    if (pcre2_patterns.size() != patternList.size())
    {
        throw std::runtime_error("PCRE2: Pattern and replacement counts do not match.");
    }

    // 3. 입력 파일 열기 (텍스트 모드로 열기)
    std::ifstream inFile(CT2A(filePath), std::ios::in);
    if (!inFile.is_open())
    {
        throw std::runtime_error("PCRE2: Failed to open input file.");
    }

    // 4. 출력 파일 준비
    CString outputFileName;
    outputFileName.Format(_T("%s\\output_pcre2_%d.txt"),
        FolderPathManager::GetInstance().GetResultFolderPath(),
        1); // Index 값 필요 시 수정

    std::ofstream outFile(CT2A(outputFileName), std::ios::out | std::ios::binary);
    if (!outFile.is_open())
    {
        throw std::runtime_error("PCRE2: Failed to open output file for writing.");
    }

    // 5. 처리 시간 측정 시작
    auto startTime = std::chrono::high_resolution_clock::now();

    // 6. 파일을 한 줄씩 읽어 처리
    std::string line;
    while (std::getline(inFile, line))
    {
        // 6.1. 줄 끝의 '\r' 제거 (Windows 줄 끝 처리)
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        std::string originalLine = line; // 원본 라인 저장 (디버깅용)

        // 6.2. 각 패턴에 대해 치환 수행
        for (size_t i = 0; i < pcre2_patterns.size(); ++i)
        {
            pcre2_code* re = pcre2_patterns[i];
            const std::string& replacement = CStringToUTF8(patternList[i].replacement);

            // 출력 버퍼 크기 초기 설정 (원본 라인 크기 * 2)
            PCRE2_SIZE out_length = line.length() * 2;
            std::vector<PCRE2_UCHAR8> buffer(out_length);

            int rc = pcre2_substitute(
                re,                                   // 컴파일된 패턴
                (PCRE2_SPTR8)line.c_str(),           // 입력 문자열
                PCRE2_ZERO_TERMINATED,               // 입력 문자열 길이
                0,                                    // 시작 오프셋
                PCRE2_SUBSTITUTE_GLOBAL,             // 옵션 (전역 치환)
                NULL,                                 // 매치 데이터 (필요 없음)
                NULL,                                 // 매치 컨텍스트 (필요 없음)
                (PCRE2_SPTR8)replacement.c_str(),    // 치환 문자열
                PCRE2_ZERO_TERMINATED,               // 치환 문자열 길이
                buffer.data(),                        // 출력 버퍼
                &out_length                           // 출력 버퍼 크기
            );

            // 치환된 문자열을 저장할 새로운 라인
            std::string new_line;

            if (rc >= 0)
            {
                // 치환 성공, buffer에 치환된 문자열이 저장됨
                new_line.assign((char*)buffer.data(), out_length);

                // 치환된 횟수 기록
                result.detectionsPerPattern[i] += rc;
                result.totalDetections += rc;
                result.totalReplacements += rc;

                // 라인 업데이트
                line = new_line;
            }
            else if (rc == PCRE2_ERROR_NOMEMORY)
            {
                // 출력 버퍼 크기 재조정 (원본 라인 크기 * 4)
                out_length = line.length() * 4;
                buffer.resize(out_length);

                rc = pcre2_substitute(
                    re,
                    (PCRE2_SPTR8)line.c_str(),
                    PCRE2_ZERO_TERMINATED,
                    0,
                    PCRE2_SUBSTITUTE_GLOBAL,
                    NULL,
                    NULL,
                    (PCRE2_SPTR8)replacement.c_str(),
                    PCRE2_ZERO_TERMINATED,
                    buffer.data(),
                    &out_length
                );

                if (rc >= 0)
                {
                    new_line.assign((char*)buffer.data(), out_length);

                    // 치환된 횟수 기록
                    result.detectionsPerPattern[i] += rc;
                    result.totalDetections += rc;
                    result.totalReplacements += rc;

                    // 라인 업데이트
                    line = new_line;
                }
                else
                {
                    // 에러 처리 코드
                    PCRE2_UCHAR buffer_error[256];
                    pcre2_get_error_message(rc, buffer_error, sizeof(buffer_error));
                    CString msg;
                    msg.Format(_T("PCRE2 치환 오류: %S"), buffer_error);
                    AfxMessageBox(msg);
                    break;  
                }
            }
            else if (rc == PCRE2_ERROR_NOMATCH)
            {
                // 매칭되는 것이 없음, 그대로 유지
                continue;
            }
            else
            {
                // 기타 에러 처리
                PCRE2_UCHAR buffer_error[256];
                pcre2_get_error_message(rc, buffer_error, sizeof(buffer_error));
                CString msg;
                msg.Format(_T("PCRE2 치환 오류: %S"), buffer_error);
                AfxMessageBox(msg);
                break;
            }
        }

        // 6.3. 치환된 라인을 출력 파일에 기록
        outFile << line << '\n';
    }

    // 7. 처리 시간 측정 종료
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;

    // 8. 파일 닫기
    inFile.close();
    outFile.close();

    // 9. 처리 시간 및 결과 텍스트 반환
    result.duration_ms = duration.count(); // 밀리초 단위 반환
    outProcessedText = ReadFileToString(outputFileName); // 치환된 텍스트 반환

    return result;
}

double Utils::ProcessChunkWithRE2(const CString& filePath, const FileChunk& chunk, int threadIndex)
{
    // PatternManager에서 컴파일된 RE2 패턴과 패턴 리스트 가져오기
    const auto& re2_patterns = PatternManager::GetInstance().GetCompiledRE2Patterns();
    const auto& patternList = PatternManager::GetInstance().GetPatterns(); // 패턴 리스트 가져오기

    // 패턴과 대체 문자열의 개수가 일치하는지 확인
    if (re2_patterns.size() != patternList.size())
    {
        throw std::runtime_error("RE2: Pattern and replacement counts do not match.");
    }

    // 파일 스트림을 바이너리 모드로 열기
    std::ifstream fileStream(CT2A(filePath), std::ios::binary);
    if (!fileStream.is_open())
    {
        throw std::runtime_error("RE2: Failed to open file");
    }

    // 청크의 시작 위치로 이동
    fileStream.seekg(chunk.startOffset);
    if (!fileStream)
    {
        fileStream.close();
        throw std::runtime_error("RE2: Failed to seek to chunk start offset");
    }

    // 청크의 데이터를 읽어서 메모리에 로드
    std::streamsize chunkSize = chunk.endOffset - chunk.startOffset;
    std::vector<char> buffer(chunkSize);
    fileStream.read(buffer.data(), chunkSize);
    if (!fileStream)
    {
        fileStream.close();
        throw std::runtime_error("RE2: Failed to read chunk data");
    }
    fileStream.close();

    // 버퍼를 스트링으로 변환
    std::string chunkData(buffer.begin(), buffer.end());

    // 스트링 스트림을 사용하여 라인 단위로 처리
    std::istringstream iss(chunkData);

    // 처리 시작 시간 기록
    auto startTime = std::chrono::high_resolution_clock::now();

    // 출력 데이터를 저장할 스트링 스트림
    std::ostringstream outputBuffer;

    std::string line;
    bool isFirstLine = true; // 첫 번째 라인 여부 확인
    while (std::getline(iss, line))
    {
        // 첫 번째 줄에서 BOM 제거
        if (isFirstLine)
        {
            isFirstLine = false;
            const std::string BOM = "\xEF\xBB\xBF";
            if (line.compare(0, BOM.size(), BOM) == 0)
            {
                line.erase(0, BOM.size());
            }
        }

        // 줄 끝의 \r 제거
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        std::string originalLine = line; // 원본 라인 저장 (디버깅용)

        // 각 라인에 대해 패턴 적용
        for (size_t i = 0; i < re2_patterns.size(); ++i)
        {
            RE2* re = re2_patterns[i].get();
            // CString을 std::string으로 변환
            std::string replacement = Utils::CStringToUTF8(patternList[i].replacement);

            // RE2의 GlobalReplace 함수 사용
            bool replaced = RE2::GlobalReplace(&line, *re, replacement);

            // 패턴 매칭 여부 확인 (디버깅용)
            if (replaced)
            {
                // 디버깅 메시지 활성화 가능
                /*
                outputBuffer << "Thread " << threadIndex + 1 << " - Pattern " << i + 1 << " applied." << '\n';
                outputBuffer << "Original Line: " << originalLine << '\n';
                outputBuffer << "Modified Line: " << line << '\n';
                */
            }
        }

        // 처리된 라인을 출력 버퍼에 저장
        outputBuffer << line << '\n';
    }

    // 처리 완료 시간 기록
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;

    // 결과를 임시 출력 파일에 저장
    CString outputFileName;
    outputFileName.Format(_T("%s\\output_re2_thread_%d.txt"), FolderPathManager::GetInstance().GetResultFolderPath(), threadIndex + 1);
    std::ofstream outFile(CT2A(outputFileName), std::ios::binary);
    if (outFile.is_open())
    {
        std::string outputData = outputBuffer.str();
        if (!outputData.size())
            throw std::runtime_error("RE2: Failed to compile anything");
        outFile.write(outputData.data(), outputData.size());
        outFile.close();
    }
    else
    {
        // 에러 처리 (예: 로깅)
        throw std::runtime_error("RE2: Failed to open output file for writing.");
    }

    return duration.count();
}

double Utils::ProcessChunkWithPCRE2(const CString& filePath, const FileChunk& chunk, int threadIndex)
{
    // PatternManager에서 컴파일된 패턴과 패턴 리스트 가져오기
    const auto& pcre2_patterns = PatternManager::GetInstance().GetCompiledPCRE2Patterns();
    const auto& patternList = PatternManager::GetInstance().GetPatterns(); // GetReplacements() 대신 GetPatterns() 사용

    // 패턴과 대체 문자열의 개수가 일치하는지 확인
    if (pcre2_patterns.size() != patternList.size())
    {
        throw std::runtime_error("Pattern and replacement counts do not match.");
    }

    // 파일 스트림을 바이너리 모드로 열기
    std::ifstream fileStream(CT2A(filePath), std::ios::binary);
    if (!fileStream.is_open())
    {
        throw std::runtime_error("Failed to open file");
    }

    // 청크의 시작 위치로 이동
    fileStream.seekg(chunk.startOffset);
    if (!fileStream)
    {
        fileStream.close();
        throw std::runtime_error("Failed to seek to chunk start offset");
    }

    // 청크의 데이터를 읽어서 메모리에 로드
    std::streamsize chunkSize = chunk.endOffset - chunk.startOffset;
    std::vector<char> buffer(chunkSize);
    fileStream.read(buffer.data(), chunkSize);
    if (!fileStream)
    {
        fileStream.close();
        throw std::runtime_error("Failed to read chunk data");
    }
    fileStream.close();

    // 버퍼를 스트링으로 변환
    std::string chunkData(buffer.begin(), buffer.end());

    // 스트링 스트림을 사용하여 라인 단위로 처리
    std::istringstream iss(chunkData);

    // 처리 시작 시간 기록
    auto startTime = std::chrono::high_resolution_clock::now();

    // 출력 데이터를 저장할 스트링 스트림
    std::ostringstream outputBuffer;
    bool isFirstLine = true;
    std::string line;
    while (std::getline(iss, line))
    {
        // 첫 번째 줄에서 BOM 제거
        if (isFirstLine)
        {
            isFirstLine = false;
            const std::string BOM = "\xEF\xBB\xBF";
            if (line.compare(0, BOM.size(), BOM) == 0)
            {
                line.erase(0, BOM.size());
            }
        }

        // 각 라인에 대해 패턴 적용
        for (size_t i = 0; i < pcre2_patterns.size(); ++i)
        {
            pcre2_code* re = pcre2_patterns[i];
            // CString을 std::string으로 변환
            std::string replacement = Utils::CStringToUTF8(patternList[i].replacement);

            PCRE2_SPTR subject = (PCRE2_SPTR)line.c_str();
            PCRE2_SIZE subject_length = line.length();
            PCRE2_SIZE out_length = subject_length * 2; // 초기 버퍼 크기
            std::vector<char> buffer(out_length);

            int rc = pcre2_substitute(
                re,
                subject,
                subject_length,
                0,
                PCRE2_SUBSTITUTE_GLOBAL,
                NULL,
                NULL,
                (PCRE2_SPTR)replacement.c_str(),
                PCRE2_ZERO_TERMINATED,
                (PCRE2_UCHAR*)buffer.data(),
                &out_length
            );

            if (rc >= 0)
            {
                line.assign(buffer.data(), out_length);
            }
            else if (rc == PCRE2_ERROR_NOMEMORY)
            {
                // 버퍼 크기 재조정 후 재시도
                out_length *= 2;
                buffer.resize(out_length);
                rc = pcre2_substitute(
                    re,
                    subject,
                    subject_length,
                    0,
                    PCRE2_SUBSTITUTE_GLOBAL,
                    NULL,
                    NULL,
                    (PCRE2_SPTR)replacement.c_str(),
                    PCRE2_ZERO_TERMINATED,
                    (PCRE2_UCHAR*)buffer.data(),
                    &out_length
                );
                if (rc >= 0)
                {
                    line.assign(buffer.data(), out_length);
                }
                else
                {
                    // 추가 에러 처리 (예: 로깅)
                    continue;
                }
            }
            else
            {
                // 다른 에러 처리 (예: 로깅)
                continue;
            }
        }

        // 처리된 라인을 출력 버퍼에 저장
        outputBuffer << line << '\n';
    }

    // 처리 완료 시간 기록
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;

    // 결과를 임시 출력 파일에 저장
    CString outputFileName;
    outputFileName.Format(_T("%s\\output_pcre2_thread_%d.txt"), FolderPathManager::GetInstance().GetResultFolderPath(), threadIndex + 1);
    std::ofstream outFile(CT2A(outputFileName), std::ios::binary);
    if (outFile.is_open())
    {
        std::string outputData = outputBuffer.str();
        outFile.write(outputData.data(), outputData.size());
        outFile.close();
    }
    else
    {
        // 에러 처리 (예: 로깅)
        throw std::runtime_error("Failed to open output file for writing.");
    }

    return duration.count();
}

std::vector<FileChunk> Utils::SplitFileIntoChunks(const CString& filePath, int numChunks)
{
    std::vector<FileChunk> chunks;

    if (numChunks <= 0)
    {
        // 에러 처리
        return chunks;
    }

    // 파일을 바이너리 모드로 열기
    std::ifstream file(CT2A(filePath), std::ios::binary);
    if (!file.is_open())
    {
        // 에러 처리
        return chunks;
    }

    // 파일 크기 구하기
    file.seekg(0, std::ios::end);
    std::ifstream::pos_type fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // 청크 크기 계산
    std::ifstream::pos_type chunkSize = fileSize / numChunks;

    std::ifstream::pos_type currentOffset = 0;

    for (int i = 0; i < numChunks; ++i)
    {
        FileChunk chunk;
        chunk.startOffset = currentOffset;

        if (i == numChunks - 1)
        {
            // 마지막 청크는 파일의 끝까지
            chunk.endOffset = fileSize;
        }
        else
        {
            // 청크의 끝 위치 계산
            chunk.endOffset = chunk.startOffset + chunkSize;

            // 청크의 끝 위치를 다음 개행 문자까지 이동
            file.seekg(chunk.endOffset);
            std::string dummyLine;
            std::getline(file, dummyLine);
            chunk.endOffset = file.tellg();
        }

        chunks.push_back(chunk);
        currentOffset = chunk.endOffset;
    }

    file.close();
    return chunks;
}

bool Utils::OpenFileStream(const CString& filePath, std::ifstream& fileStream)
{
    CStringA filePathA(filePath); // CString을 ANSI로 변환
    fileStream.open(filePathA, std::ios::in);
    if (!fileStream.is_open())
    {
        return false;
    }
    return true;
}

// 패턴 초기화 함수
bool Utils::InitializePatterns(const std::vector<PatternReplacement>& patterns)
{
    // RE2 패턴 컴파일
    {
        std::lock_guard<std::mutex> lock(re2_mutex);
        re2_patterns.clear();
        re2_replacements.clear(); // 기존 대체 문자열 초기화

        for (const auto& pr : patterns)
        {
            // 패턴과 대체 문자열 변환
            std::string patternStr = CStringToUTF8(pr.pattern);
            std::string replacementStr = CStringToUTF8(pr.replacement);

            // 패턴 컴파일
            std::unique_ptr<RE2> re = std::make_unique<RE2>(patternStr);
            if (!re->ok())
            {
                // 에러 처리...
                return false;
            }
            re2_patterns.emplace_back(std::move(re));
            re2_replacements.push_back(replacementStr); // 대체 문자열 저장
        }
    }

    // PCRE2 패턴 컴파일
    {
        std::lock_guard<std::mutex> lock(pcre2_mutex);
        for (auto re : pcre2_patterns) {
            pcre2_code_free(re);
        }
        pcre2_patterns.clear();
        pcre2_replacements.clear(); // 기존 대체 문자열 초기화

        for (const auto& pr : patterns) {
            // 패턴과 대체 문자열 변환
            std::string patternStr = CStringToUTF8(pr.pattern);
            std::string replacementStr = CStringToUTF8(pr.replacement);

            // 패턴 컴파일
            int errornumber;
            PCRE2_SIZE erroffset;
            pcre2_code* re = pcre2_compile(
                (PCRE2_SPTR8)patternStr.c_str(),
                PCRE2_ZERO_TERMINATED,
                PCRE2_UTF,
                &errornumber,
                &erroffset,
                NULL
            );

            if (re == NULL) {
                // 에러 처리...
                return false;
            }

            // JIT 최적화 적용
            if (pcre2_jit_compile(re, PCRE2_JIT_COMPLETE) != 0) {
                pcre2_code_free(re);  // JIT 컴파일 실패 시 메모리 해제
                return false;         // 또는 실패 처리 후 계속 진행 가능
            }

            pcre2_patterns.push_back(re);
            pcre2_replacements.push_back(replacementStr); // 대체 문자열 저장
        }
    }

    return true;
}

const std::vector<std::unique_ptr<RE2>>& Utils::GetCompiledRE2Patterns()
{
    return re2_patterns;
}

const std::vector<pcre2_code*>& Utils::GetCompiledPCRE2Patterns()
{
    return pcre2_patterns;
}

// 패턴 정리 함수
void Utils::CleanupPatterns()
{
    // RE2는 unique_ptr로 자동 관리되므로 별도 해제 필요 없음

    // PCRE2 패턴 해제
    {
        std::lock_guard<std::mutex> lock(pcre2_mutex);
        for (auto re : pcre2_patterns)
        {
            pcre2_code_free(re);
        }
        pcre2_patterns.clear();
    }
}

// RE2로 라인 처리
void Utils::ProcessLineWithRE2(const std::string& line)
{
    std::lock_guard<std::mutex> lock(re2_mutex);
    for (const auto& re : re2_patterns)
    {
        // RE2::PartialMatch는 부분 일치를 검사합니다.
        bool match = RE2::PartialMatch(line, *re);
        if (match)
        {
            // 매칭된 경우의 처리 (예: 로그 기록, 카운트 증가 등)
            // 현재는 매칭 여부를 판단만 합니다.
        }
    }
}

// PCRE2로 라인 처리
std::string Utils::ProcessLineWithPCRE2(const std::string& line, std::vector<int>& detectionsPerPattern)
{
    std::string processedLine = line;

    for (size_t i = 0; i < pcre2_patterns.size(); ++i)
    {
        pcre2_code* re = pcre2_patterns[i];
        const std::string& replacement = pcre2_replacements[i]; // 사전 계산된 대체 문자열 사용

        // 출력 버퍼 크기 추정 (원본의 2배 크기)
        PCRE2_SIZE out_length = processedLine.length() * 2;
        std::vector<char> buffer(out_length);

        int rc = pcre2_substitute(
            re,                                   // 컴파일된 패턴
            (PCRE2_SPTR8)processedLine.c_str(),   // 입력 문자열
            PCRE2_ZERO_TERMINATED,                // 입력 문자열 길이
            0,                                    // 시작 오프셋
            PCRE2_SUBSTITUTE_GLOBAL,              // 옵션 (전역 치환)
            NULL,                                 // 매치 데이터 (필요 없음)
            NULL,                                 // 매치 컨텍스트 (필요 없음)
            (PCRE2_SPTR8)replacement.c_str(),     // 치환 문자열
            PCRE2_ZERO_TERMINATED,                // 치환 문자열 길이
            (PCRE2_UCHAR8*)buffer.data(),         // 출력 버퍼
            &out_length                           // 출력 버퍼 크기
        );

        if (rc >= 0)
        {
            // 치환 성공, 결과를 processedLine에 저장
            processedLine.assign(buffer.data(), out_length);

            // 치환 횟수 기록
            detectionsPerPattern[i] += rc;
        }
        else if (rc == PCRE2_ERROR_NOMEMORY)
        {
            // 출력 버퍼 크기 재조정
            buffer.resize(out_length);
            rc = pcre2_substitute(
                re,
                (PCRE2_SPTR8)processedLine.c_str(),
                PCRE2_ZERO_TERMINATED,
                0,
                PCRE2_SUBSTITUTE_GLOBAL,
                NULL,
                NULL,
                (PCRE2_SPTR8)replacement.c_str(),
                PCRE2_ZERO_TERMINATED,
                (PCRE2_UCHAR8*)buffer.data(),
                &out_length
            );

            if (rc >= 0)
            {
                processedLine.assign(buffer.data(), out_length);

                // 치환 횟수 기록
                detectionsPerPattern[i] += rc;
            }
            else
            {
                // 에러 처리 코드
            }
        }
        else if (rc == PCRE2_ERROR_NOMATCH)
        {
            // 매칭되는 것이 없음, 그대로 유지
            continue;
        }
        else
        {
            // 기타 에러 처리
        }
    }

    return processedLine;
}

