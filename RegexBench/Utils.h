// Utils.h
#pragma once
#include "pch.h"
#include "Pattern.h"

struct PCRE2ProcessResult
{
    double duration_ms; // ó�� �ð� (�и���)
    std::vector<int> detectionsPerPattern; // ���� �� ���� ��
    int totalDetections; // �� ���� ��
    int totalReplacements; // �� ġȯ ��
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

    // ���� ����
    static bool OpenFileStream(const CString& filePath, std::ifstream& fileStream);

    // RE2�� ���� ó��
    static void ProcessLineWithRE2(const std::string& line);

    // PCRE2�� ���� ó��
    static std::string ProcessLineWithPCRE2(const std::string& line, std::vector<int>& detectionsPerPattern);

    // ���� �ʱ�ȭ
    static bool InitializePatterns(const std::vector<PatternReplacement>& patterns);

    // �����ϵ� ���� ������
    static const std::vector<std::unique_ptr<RE2>>& GetCompiledRE2Patterns();
    static const std::vector<pcre2_code*>& GetCompiledPCRE2Patterns();

    // ��ü ���ڿ��� ���� ���� ���� �߰�
    static std::vector<std::string> re2_replacements;
    static std::vector<std::string> pcre2_replacements;

    // ���� ����
    static void CleanupPatterns();

    // ���ڿ� ��ȯ
    static std::string CStringToUTF8(const CString& str);
    static std::string ReadFileToString(const CString& filePath);
    static CString UTF8ToCString(const std::string& str);
    // ���� ó��
    static double NiceNumber(double range, bool round);


    // ���� ó�� �Լ�
    static double ProcessFileWithRE2(const CString& filePath, int Index);
    static PCRE2ProcessResult ProcessFileWithPCRE2(const CString& filePath, const std::vector<PatternReplacement>& patterns, std::string& outProcessedText);

private:
    // RE2 ���� ����Ʈ �� ���ؽ�
    static std::vector<std::unique_ptr<RE2>> re2_patterns;
    static std::mutex re2_mutex;

    // PCRE2 ���� ����Ʈ �� ���ؽ�
    static std::vector<pcre2_code*> pcre2_patterns;
    static std::mutex pcre2_mutex;

    // PCRE2 �ɼ�
    static constexpr uint32_t pcre2_options = PCRE2_MULTILINE;
};
