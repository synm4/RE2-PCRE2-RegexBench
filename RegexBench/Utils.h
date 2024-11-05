// Utils.h
#pragma once
#include "pch.h"
#include "Pattern.h"

struct PCRE2ProcessResult
{
    double duration_ms; // 처리 시간 (밀리초)
    std::vector<int> detectionsPerPattern; // 패턴 별 검출 수
    int totalDetections; // 총 검출 수
    int totalReplacements; // 총 치환 수
};

struct FileChunk
{
    std::streampos startOffset;
    std::streampos endOffset;
};

class Utils
{
public:
    static std::vector<FileChunk> SplitFileIntoChunks(const CString& filePath, int numChunks);

    static double ProcessChunkWithRE2(const CString& filePath, const FileChunk& chunk, int threadIndex);
    static double ProcessChunkWithPCRE2(const CString& filePath, const FileChunk& chunk, int threadIndex);

    // 파일 열기
    static bool OpenFileStream(const CString& filePath, std::ifstream& fileStream);

    // RE2로 라인 처리
    static void ProcessLineWithRE2(const std::string& line);

    // PCRE2로 라인 처리
    static std::string ProcessLineWithPCRE2(const std::string& line, std::vector<int>& detectionsPerPattern);

    // 패턴 초기화
    static bool InitializePatterns(const std::vector<PatternReplacement>& patterns);

    // 컴파일된 패턴 접근자
    static const std::vector<std::unique_ptr<RE2>>& GetCompiledRE2Patterns();
    static const std::vector<pcre2_code*>& GetCompiledPCRE2Patterns();

    // 대체 문자열을 위한 정적 벡터 추가
    static std::vector<std::string> re2_replacements;
    static std::vector<std::string> pcre2_replacements;

    // 패턴 정리
    static void CleanupPatterns();

    // 문자열 변환
    static std::string CStringToUTF8(const CString& str);
    static std::string ReadFileToString(const CString& filePath);
    static CString UTF8ToCString(const std::string& str);
    // 숫자 처리
    static double NiceNumber(double range, bool round);


    // 파일 처리 함수
    static double ProcessFileWithRE2(const CString& filePath, int Index);
    static PCRE2ProcessResult ProcessFileWithPCRE2(const CString& filePath, const std::vector<PatternReplacement>& patterns, std::string& outProcessedText);

private:
    // RE2 패턴 리스트 및 뮤텍스
    static std::vector<std::unique_ptr<RE2>> re2_patterns;
    static std::mutex re2_mutex;

    // PCRE2 패턴 리스트 및 뮤텍스
    static std::vector<pcre2_code*> pcre2_patterns;
    static std::mutex pcre2_mutex;

    // PCRE2 옵션
    static constexpr uint32_t pcre2_options = PCRE2_MULTILINE;
};
