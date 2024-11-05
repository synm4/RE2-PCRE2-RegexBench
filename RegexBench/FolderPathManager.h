#pragma once

#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <atlstr.h> // CString ��� �� �ʿ�

class FolderPathManager
{
public:
    // �̱��� �ν��Ͻ� ������
    static FolderPathManager& GetInstance()
    {
        static FolderPathManager instance;
        return instance;
    }

    // ��� ���� ��� ���� (CString ���)
    void SetResultFolderPath(const CString& path)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        resultFolderPath = EnsureTrailingSeparator(ConvertCStringToPath(path));
    }

    // ��� ���� ��� ��ȯ (CString ���)
    CString GetResultFolderPath() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return ConvertPathToCString(resultFolderPath);
    }

    // ��� ���� ��� ���� ���� Ȯ��
    bool IsResultFolderPathSet() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return !resultFolderPath.empty();
    }

    // ������ ������ ���� ��� ���� (CString ���)
    void SetGeneratedDataFolderPath(const CString& path)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        generatedDataFolderPath = EnsureTrailingSeparator(ConvertCStringToPath(path));
    }

    // ������ ������ ���� ��� ��ȯ (CString ���)
    CString GetGeneratedDataFolderPath() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return ConvertPathToCString(generatedDataFolderPath);
    }

    // ������ ������ ���� ��� ���� ���� Ȯ��
    bool IsGeneratedDataFolderPathSet() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return !generatedDataFolderPath.empty();
    }

private:
    std::filesystem::path resultFolderPath;
    std::filesystem::path generatedDataFolderPath;

    // �̱��� ������ ���� ������ �� �Ҹ���
    FolderPathManager() = default;
    ~FolderPathManager() = default;

    // ���� ������ �� �Ҵ� ������ ����
    FolderPathManager(const FolderPathManager&) = delete;
    FolderPathManager& operator=(const FolderPathManager&) = delete;

    // Ʈ���ϸ� ������ ����
    std::filesystem::path EnsureTrailingSeparator(const std::filesystem::path& path) const
    {
        if (path.empty())
            throw std::invalid_argument("��ΰ� ��� �ֽ��ϴ�.");

        std::filesystem::path modifiedPath = path;

        // ��� ���ڿ��� ������ ������ ���ڰ� ���������� Ȯ��
        std::wstring pathStr = modifiedPath.wstring();
        if (!pathStr.empty() && pathStr.back() != std::filesystem::path::preferred_separator)
        {
            // preferred_separator�� ���ڿ��� ��ȯ�Ͽ� �߰�
            modifiedPath += std::wstring(1, std::filesystem::path::preferred_separator);
        }

        return modifiedPath;
    }

    // ��Ƽ������ ȯ���� ����� ���ؽ�
    mutable std::mutex mutex_;

    // CString�� std::filesystem::path�� ��ȯ
    std::filesystem::path ConvertCStringToPath(const CString& cstr) const
    {
        // CString�� wide ���ڿ��� ��ȯ
        std::wstring wideStr = static_cast<LPCTSTR>(cstr);
        return std::filesystem::path(wideStr);
    }

    // std::filesystem::path�� CString���� ��ȯ
    CString ConvertPathToCString(const std::filesystem::path& path) const
    {
        return CString(path.wstring().c_str());
    }
};
