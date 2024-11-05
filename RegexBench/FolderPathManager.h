#pragma once

#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <atlstr.h> // CString 사용 시 필요

class FolderPathManager
{
public:
    // 싱글톤 인스턴스 접근자
    static FolderPathManager& GetInstance()
    {
        static FolderPathManager instance;
        return instance;
    }

    // 결과 폴더 경로 설정 (CString 사용)
    void SetResultFolderPath(const CString& path)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        resultFolderPath = EnsureTrailingSeparator(ConvertCStringToPath(path));
    }

    // 결과 폴더 경로 반환 (CString 사용)
    CString GetResultFolderPath() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return ConvertPathToCString(resultFolderPath);
    }

    // 결과 폴더 경로 설정 여부 확인
    bool IsResultFolderPathSet() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return !resultFolderPath.empty();
    }

    // 생성된 데이터 폴더 경로 설정 (CString 사용)
    void SetGeneratedDataFolderPath(const CString& path)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        generatedDataFolderPath = EnsureTrailingSeparator(ConvertCStringToPath(path));
    }

    // 생성된 데이터 폴더 경로 반환 (CString 사용)
    CString GetGeneratedDataFolderPath() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return ConvertPathToCString(generatedDataFolderPath);
    }

    // 생성된 데이터 폴더 경로 설정 여부 확인
    bool IsGeneratedDataFolderPathSet() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return !generatedDataFolderPath.empty();
    }

private:
    std::filesystem::path resultFolderPath;
    std::filesystem::path generatedDataFolderPath;

    // 싱글톤 패턴을 위한 생성자 및 소멸자
    FolderPathManager() = default;
    ~FolderPathManager() = default;

    // 복사 생성자 및 할당 연산자 삭제
    FolderPathManager(const FolderPathManager&) = delete;
    FolderPathManager& operator=(const FolderPathManager&) = delete;

    // 트레일링 구분자 보장
    std::filesystem::path EnsureTrailingSeparator(const std::filesystem::path& path) const
    {
        if (path.empty())
            throw std::invalid_argument("경로가 비어 있습니다.");

        std::filesystem::path modifiedPath = path;

        // 경로 문자열을 가져와 마지막 문자가 구분자인지 확인
        std::wstring pathStr = modifiedPath.wstring();
        if (!pathStr.empty() && pathStr.back() != std::filesystem::path::preferred_separator)
        {
            // preferred_separator를 문자열로 변환하여 추가
            modifiedPath += std::wstring(1, std::filesystem::path::preferred_separator);
        }

        return modifiedPath;
    }

    // 멀티스레드 환경을 고려한 뮤텍스
    mutable std::mutex mutex_;

    // CString을 std::filesystem::path로 변환
    std::filesystem::path ConvertCStringToPath(const CString& cstr) const
    {
        // CString을 wide 문자열로 변환
        std::wstring wideStr = static_cast<LPCTSTR>(cstr);
        return std::filesystem::path(wideStr);
    }

    // std::filesystem::path를 CString으로 변환
    CString ConvertPathToCString(const std::filesystem::path& path) const
    {
        return CString(path.wstring().c_str());
    }
};
