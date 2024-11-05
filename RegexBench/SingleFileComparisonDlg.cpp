// SingleFileComparisonDlg.cpp

#include "pch.h"
#include "SingleFileComparisonDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "Utils.h"
#include "PatternManager.h"
#include "FolderPathManager.h"

#pragma comment(lib, "gdiplus.lib")
std::mutex logMutex;
// CSingleFileComparisonDlg 대화 상자

IMPLEMENT_DYNAMIC(CSingleFileComparisonDlg, CDialogEx)

void LogMessage(const CString& message)
{
    std::lock_guard<std::mutex> lock(logMutex);

    // 로그 파일을 UTF-8로 열기
    std::ofstream logFile("release_log.txt", std::ios::app);
    if (logFile.is_open())
    {
        // UTF-8 BOM 추가 (첫 번째 열기 시 한 번만 추가)
        static bool isFirstOpen = true;
        if (isFirstOpen)
        {
            logFile << "\xEF\xBB\xBF"; // UTF-8 BOM
            isFirstOpen = false;
        }

        // CString을 UTF-8 std::string으로 변환하여 로그 파일에 기록
        try {
            std::string utf8Str = Utils::CStringToUTF8(message);
            logFile << utf8Str << std::endl;
        }
        catch (const std::runtime_error& e) {
            // 변환 실패 시 오류 메시지 로그
            logFile << "Error in CStringToUTF8 conversion: " << e.what() << std::endl;
        }
    }
}

CSingleFileComparisonDlg::CSingleFileComparisonDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_SINGLE_FILE_COMPARISON_DLG, pParent)
    , m_isProcessing(0)
    , m_shouldStop(0)
    , m_isProcessingRE2(false)
    , m_isProcessingPCRE2(false)
    , m_maxDisplayDuration(20)
    , m_refreshTime(250)
    , m_totalDetectionsRE2(0)
    , m_totalDetectionsPCRE2(0)
    , m_totalReplacementsRE2(0)
    , m_totalReplacementsPCRE2(0)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

CSingleFileComparisonDlg::~CSingleFileComparisonDlg()
{
    // 타이머 종료
    KillTimer(IDT_TIMER_UPDATE_CHART);
}

void CSingleFileComparisonDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LISTCTRL_DATA, m_DataListCtrl);  // CListCtrl 바인딩
    DDX_Control(pDX, IDC_EDIT_REFRESH, m_ComboRefresh);  // ComboBox 바인딩
    DDX_Control(pDX, IDC_EDIT_SECOND, m_ComboSecond);    // ComboBox 바인딩

}

BEGIN_MESSAGE_MAP(CSingleFileComparisonDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_UPLOAD_FILE, &CSingleFileComparisonDlg::OnBnClickedButtonUploadFile)
    ON_BN_CLICKED(IDC_BUTTON_SAVE_RESULTS, &CSingleFileComparisonDlg::OnBnClickedButtonSaveResults)
    ON_WM_TIMER()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_MESSAGE(WM_UPDATE_TOTAL_TIME, &CSingleFileComparisonDlg::OnUpdateTotalTime)
    ON_CBN_SELCHANGE(IDC_EDIT_REFRESH, &CSingleFileComparisonDlg::OnCbnSelchangeComboRefresh)
    ON_CBN_SELCHANGE(IDC_EDIT_SECOND, &CSingleFileComparisonDlg::OnCbnSelchangeComboSecond)
END_MESSAGE_MAP()

LRESULT CSingleFileComparisonDlg::OnUpdateTotalTime(WPARAM wParam, LPARAM lParam)
{
    ReadLockGuard lock(m_dataLock);

    if (wParam == 1) // RE2
    {
        CString totalTimeRE2Str;
        totalTimeRE2Str.Format(_T("RE2 Total Time: %.3f seconds"), m_totalTimeRE2);
        SetDlgItemText(IDC_STATIC_TOTAL_TIME_RE2, totalTimeRE2Str);
    }
    else if (wParam == 2) // PCRE2
    {
        CString totalTimePCRE2Str;
        totalTimePCRE2Str.Format(_T("PCRE2 Total Time: %.3f seconds"), m_totalTimePCRE2);
        SetDlgItemText(IDC_STATIC_TOTAL_TIME_PCRE2, totalTimePCRE2Str);
    }

    // 두 라이브러리 모두 처리가 완료되었는지 확인
    if (!m_isProcessingRE2 && !m_isProcessingPCRE2)
    {
        // 타이머 종료
        KillTimer(IDT_TIMER_UPDATE_CHART);
        TRACE(_T("타이머 종료: m_isProcessingRE2=%d, m_isProcessingPCRE2=%d\n"), m_isProcessingRE2, m_isProcessingPCRE2);
    }

    return 0;
}

BOOL CSingleFileComparisonDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 아이콘 설정
    SetIcon(m_hIcon, TRUE);   // 큰 아이콘
    SetIcon(m_hIcon, FALSE);  // 작은 아이콘

    // 전역 시간 초기화
    m_startTime = std::chrono::steady_clock::now();
    m_globalTime = 0.0;


    // ChartViewer 컨트롤 초기화
    CRect rect;
    CWnd* pPlaceholder = GetDlgItem(IDC_STATIC_CHART_PLACEHOLDER);
    if (pPlaceholder != nullptr)
    {
        pPlaceholder->GetWindowRect(&rect);
        ScreenToClient(&rect);
        if (!m_ChartViewer.Create(NULL, WS_CHILD | WS_VISIBLE, rect, this, IDC_CHARTVIEWER))
        {
            AfxMessageBox(_T("ChartViewer 컨트롤을 생성할 수 없습니다."));
        }
        else
        {
            TRACE(_T("ChartViewer 컨트롤이 성공적으로 생성되었습니다.\n"));
            // Optional: Set Z-Order to top
            m_ChartViewer.SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
    }
    else
    {
        AfxMessageBox(_T("Chart Placeholder 컨트롤을 찾을 수 없습니다."));
        return TRUE; // 초기화 계속 진행하지만, 차트는 표시되지 않습니다.
    }

    // ComboBox 초기화
    // m_refreshTime: 밀리초 단위
    m_ComboRefresh.AddString(_T("100")); // 100 ms
    m_ComboRefresh.AddString(_T("250")); // 250 ms
    m_ComboRefresh.AddString(_T("500")); // 500 ms
    m_ComboRefresh.AddString(_T("1000")); // 1000 ms
    m_ComboRefresh.SetCurSel(1); // 기본값: 250 ms

    // m_maxDisplayDuration: 초 단위
    m_ComboSecond.AddString(_T("10"));
    m_ComboSecond.AddString(_T("20"));
    m_ComboSecond.AddString(_T("30"));
    m_ComboSecond.AddString(_T("60"));
    m_ComboSecond.SetCurSel(1); // 기본값: 20 초

    // 현재 선택된 값으로 변수 설정
    UpdateData(TRUE); // 데이터 가져오기

    // m_ComboRefresh에서 선택된 텍스트 가져오기
    CString strRefresh;
    int selRefresh = m_ComboRefresh.GetCurSel();
    if (selRefresh != CB_ERR)
    {
        m_ComboRefresh.GetLBText(selRefresh, strRefresh);
        m_refreshTime = _ttoi(strRefresh);
    }
    else
    {
        m_refreshTime = 250; // 기본값 설정 (필요에 따라 변경)
    }

    // m_ComboSecond에서 선택된 텍스트 가져오기
    CString strSecond;
    int selSecond = m_ComboSecond.GetCurSel();
    if (selSecond != CB_ERR)
    {
        m_ComboSecond.GetLBText(selSecond, strSecond);
        m_maxDisplayDuration = _ttoi(strSecond);
    }
    else
    {
        m_maxDisplayDuration = 20; // 기본값 설정 (필요에 따라 변경)
    }

    UpdateData(FALSE); // UI 업데이트

    // 타이머 설정 (그래프 업데이트 주기: 250ms)
    SetTimer(IDT_TIMER_UPDATE_CHART, m_refreshTime, NULL);

    // ListCtrl 초기화
    m_DataListCtrl.InsertColumn(0, _T("처리한 시간"), LVCFMT_LEFT, 130);  // 시간 열
    m_DataListCtrl.InsertColumn(1, _T("RE2 처리줄"), LVCFMT_LEFT, 90);  // RE2 데이터 열
    m_DataListCtrl.InsertColumn(2, _T("PCRE2 처리줄"), LVCFMT_LEFT, 90);  // PCRE2 데이터 열

    // 리스트 컨트롤에 이중 버퍼링 스타일 추가
    m_DataListCtrl.SetExtendedStyle(m_DataListCtrl.GetExtendedStyle() | LVS_EX_DOUBLEBUFFER);

    return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}


void CSingleFileComparisonDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 아이콘을 가운데에 그립니다.
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

void CSingleFileComparisonDlg::OnDestroy()
{
    // 스레드 종료 신호 보내기
    m_shouldStop = true;

    // 타이머 종료
    KillTimer(IDT_TIMER_UPDATE_CHART);

    CDialogEx::OnDestroy();
}


HCURSOR CSingleFileComparisonDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CSingleFileComparisonDlg::OnBnClickedButtonUploadFile()
{
    CString initialDir;

    // FolderPathManager에서 생성 데이터 폴더 경로 확인
    if (FolderPathManager::GetInstance().IsGeneratedDataFolderPathSet())
    {
        initialDir = FolderPathManager::GetInstance().GetGeneratedDataFolderPath();
    }

    // 파일 다이얼로그 열기
    CFileDialog dlg(TRUE, _T("txt"), NULL, OFN_FILEMUSTEXIST, _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"));

    // 설정된 경로가 있는 경우 초기 경로로 설정
    if (!initialDir.IsEmpty())
    {
        dlg.m_ofn.lpstrInitialDir = initialDir;
    }

    if (dlg.DoModal() == IDOK)
    {
        m_filePath = dlg.GetPathName();

        // 파일 크기 가져오기
        CFileStatus status;
        if (CFile::GetStatus(m_filePath, status))
        {
            ULONGLONG size = status.m_size;
            CString sizeStr;
            sizeStr.Format(_T("파일 크기: %.2f MB"), (double)size / (1024 * 1024));
            m_fileSize = sizeStr;
        }
        else
        {
            m_fileSize = _T("파일 크기: 알 수 없음");
        }

        // 파일 경로와 크기 업데이트
        SetDlgItemText(IDC_STATIC_FILE_PATH, m_filePath);
        SetDlgItemText(IDC_STATIC_FILE_SIZE, m_fileSize);

        // 패턴 초기화
        if (!Utils::InitializePatterns(PatternManager::GetInstance().GetPatterns()))
        {
            AfxMessageBox(_T("패턴 초기화에 실패했습니다. 파일 처리를 중단합니다."));
            return;
        }

        // 그래프 데이터 초기화
        {
            WriteLockGuard lock(m_dataLock);
            m_re2TimeStamps.clear();
            m_pcre2TimeStamps.clear();
            m_re2PerformanceData.clear();
            m_pcre2PerformanceData.clear();

            m_re2TimeStampsStr.clear(); // 시간 문자열 덱 초기화
            m_pcre2TimeStampsStr.clear(); // 시간 문자열 덱 초기화

            // 패턴 수에 맞게 검출수 벡터 초기화
            size_t patternCount = PatternManager::GetInstance().GetPatterns().size();
            m_detectionsPerPatternRE2.assign(patternCount, 0);
            m_detectionsPerPatternPCRE2.assign(patternCount, 0);
            m_totalDetectionsRE2 = 0;
            m_totalDetectionsPCRE2 = 0;
            m_totalReplacementsRE2 = 0;
            m_totalReplacementsPCRE2 = 0;
        }

        // 그래프 그리기 갱신
        Invalidate();

        // 데이터 처리 시작
        StartProcessing();
    }
}


void CSingleFileComparisonDlg::StartProcessing()
{
    WriteLockGuard lock(m_dataLock);
    if (m_isProcessing)
    {
        return; // 이미 처리 중이면 반환
    }
    m_isProcessing = true;

    LogMessage(_T("StartProcessing: 처리 시작"));

    // 그래프 데이터 초기화
    m_re2TimeStamps.clear();
    m_pcre2TimeStamps.clear();
    m_re2PerformanceData.clear();
    m_pcre2PerformanceData.clear();
    m_re2TimeStampsStr.clear();
    m_pcre2TimeStampsStr.clear();

    // 리스트 컨트롤 초기화
    m_DataListCtrl.DeleteAllItems();

    // 기타 필요한 변수 초기화
    m_totalDetectionsRE2 = 0;
    m_totalDetectionsPCRE2 = 0;
    m_totalReplacementsRE2 = 0;
    m_totalReplacementsPCRE2 = 0;
    m_globalTime = 0.0;
    m_startTime = std::chrono::steady_clock::now();

    // 패턴 수에 맞게 검출수 벡터 초기화
    size_t patternCount = PatternManager::GetInstance().GetPatterns().size();
    m_detectionsPerPatternRE2.assign(patternCount, 0);
    m_detectionsPerPatternPCRE2.assign(patternCount, 0);

    // 그래프 그리기 갱신
    Invalidate();
    UpdateWindow(); // 추가된 부분: UI 강제 업데이트

    // **타이머 재설정**
    SetTimer(IDT_TIMER_UPDATE_CHART, m_refreshTime, NULL);

    try
    {
        LogMessage(_T("스레드 생성 시작: ProcessingThreadRE2\n"));
        m_re2ThreadGuard = std::make_unique<ThreadGuard>(std::thread(&CSingleFileComparisonDlg::ProcessingThreadRE2, this));
        LogMessage(_T("ProcessingThreadRE2 스레드 생성 완료.\n"));

        LogMessage(_T("스레드 생성 시작: ProcessingThreadPCRE2\n"));
        m_pcre2ThreadGuard = std::make_unique<ThreadGuard>(std::thread(&CSingleFileComparisonDlg::ProcessingThreadPCRE2, this));
        LogMessage(_T("ProcessingThreadPCRE2 스레드 생성 완료.\n"));
    }
    catch (const std::system_error& e)
    {
        CString msg;
        msg.Format(_T("스레드 생성 중 시스템 오류가 발생했습니다: %S (코드: %d)"), e.what(), e.code().value());
        AfxMessageBox(msg);
        TRACE(_T("스레드 생성 중 시스템 오류 발생: %S (코드: %d)\n"), e.what(), e.code().value());
        m_isProcessing = false;
    }
    catch (const std::exception& e)
    {
        CString msg;
        msg.Format(_T("스레드 생성 중 예외가 발생했습니다: %S"), e.what());
        AfxMessageBox(msg);
        TRACE(_T("스레드 생성 중 예외 발생: %S\n"), e.what());
        m_isProcessing = false;
    }
    catch (...)
    {
        AfxMessageBox(_T("스레드 생성 중 알 수 없는 예외가 발생했습니다."));
        LogMessage(_T("스레드 생성 중 알 수 없는 예외 발생.\n"));
        m_isProcessing = false;
    }
    TRACE(_T("UpdateChart 함수 완료\n"));
}


void CSingleFileComparisonDlg::ProcessingThreadRE2()
{
    {
        WriteLockGuard lock(m_dataLock);
        m_isProcessingRE2 = true;
    }

    try
    {
        std::ifstream fileStream;
        if (!OpenFileStream(fileStream))
        {
            WriteLockGuard lock(m_dataLock);
            m_isProcessingRE2 = false;
            if (!m_isProcessingPCRE2)
            {
                m_isProcessing = false;
            }
            return;
        }
        LogMessage(_T("ProcessingThreadRE2\n"));

        // FolderPathManager를 사용하여 폴더 경로 설정
        std::filesystem::path folderPath;
        if (FolderPathManager::GetInstance().IsGeneratedDataFolderPathSet())
        {
            folderPath = FolderPathManager::GetInstance().GetGeneratedDataFolderPath().GetString();
        }
        else
        {
            folderPath = "output_folder";
        }

        std::filesystem::path outputFilePath = folderPath / "output_re2.txt";

        // 폴더가 존재하지 않으면 생성합니다.
        if (!std::filesystem::exists(folderPath))
        {
            if (!std::filesystem::create_directories(folderPath))
            {
                AfxMessageBox(_T("출력 폴더를 생성할 수 없습니다."));
                m_isProcessingPCRE2 = false;
                if (!m_isProcessingRE2)
                {
                    m_isProcessing = false;
                }
                return;
            }
        }

        // 출력 파일 열기
        std::ofstream outFile(outputFilePath.string(), std::ios::out | std::ios::binary);
        if (!outFile.is_open())
        {
            AfxMessageBox(_T("RE2 출력 파일을 열 수 없습니다."));
            WriteLockGuard lock(m_dataLock);
            m_isProcessingRE2 = false;
            if (!m_isProcessingPCRE2)
            {
                m_isProcessing = false;
            }
            return;
        }

        // UTF-8 BOM 추가
        outFile << "\xEF\xBB\xBF";

        // 패턴을 미리 컴파일하여 저장
        std::vector<std::unique_ptr<RE2>> regexes;
        std::vector<std::string> replacements;

        if (PatternManager::GetInstance().GetPatterns().empty())
        {
            PatternManager::GetInstance().SetDefaultPatterns();
        }

        const std::vector<PatternReplacement>& m_patterns = PatternManager::GetInstance().GetPatterns();

        for (const auto& pattern : m_patterns)
        {
            // CString을 UTF-8 std::string으로 변환
            std::string patternStr = Utils::CStringToUTF8(pattern.pattern);
            std::string replacementStr = Utils::CStringToUTF8(pattern.replacement);

            // RE2 객체 생성 - std::string 사용
            regexes.emplace_back(std::make_unique<RE2>(patternStr));
            replacements.push_back(replacementStr);
        }

        std::string line;
        int linesProcessed = 0;
        auto lastUpdateTime = m_startTime;

        // 총 처리 시간 측정을 위한 시작 시간 기록
        auto startTime = std::chrono::steady_clock::now();

        while (std::getline(fileStream, line))
        {
            if (m_shouldStop)
                break;

            // RE2로 라인 처리
            std::string processedLine = line;
            bool lineModified = false;

            for (size_t i = 0; i < regexes.size(); ++i)
            {
                if (RE2::GlobalReplace(&processedLine, *regexes[i], replacements[i]))
                {
                    WriteLockGuard lock(m_dataLock);
                    int matchCount = 1; // GlobalReplace의 호출이 성공하면 matchCount를 1 증가시킵니다.
                    m_detectionsPerPatternRE2[i] += matchCount;
                    m_totalDetectionsRE2 += matchCount;
                    m_totalReplacementsRE2 += matchCount;
                    lineModified = true;
                }
            }

            // 처리된 라인을 출력 파일에 기록
            outFile << processedLine << "\n";

            linesProcessed++;

            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastUpdateTime).count();

            if (elapsed >= m_refreshTime)
            {
                double chartTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_startTime).count() / 1000.0;

                CString formattedTime;
                formattedTime.Format(_T("%.3f"), chartTime);

                {
                    WriteLockGuard lock(m_dataLock);

                    m_re2TimeStamps.push_back(chartTime);
                    m_re2PerformanceData.push_back(linesProcessed);
                    m_re2TimeStampsStr.push_back(formattedTime);

                    // 오래된 데이터 제거
                    while (!m_re2TimeStamps.empty() && (m_re2TimeStamps.back() - m_re2TimeStamps.front() > m_maxDisplayDuration))
                    {
                        m_re2TimeStamps.pop_front();
                        m_re2PerformanceData.pop_front();
                        m_re2TimeStampsStr.pop_front();
                    }
                }

                linesProcessed = 0;
                lastUpdateTime = currentTime;  // 여기서 lastUpdateTime을 현재 시간으로 갱신합니다.

                // UI 업데이트 요청
                PostMessage(WM_UPDATE_CHART, 0, 0);
            }
        }

        // 총 처리 시간 계산
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> totalElapsed = endTime - startTime;

        {
            WriteLockGuard lock(m_dataLock);

            m_totalTimeRE2 = totalElapsed.count();
            m_isProcessingRE2 = false;

            if (!m_isProcessingPCRE2)
            {
                m_isProcessing = false;
            }
        }

        // 총 처리 시간 업데이트 메시지 전송 (wParam = 1은 RE2)
        PostMessage(WM_UPDATE_TOTAL_TIME, 1, 0);

        fileStream.close();
        outFile.close();
    }
    catch (const std::exception& ex)
    {
        CString msg;
        msg.Format(_T("RE2 처리 중 예외가 발생했습니다: %S"), ex.what());
        AfxMessageBox(msg);
        LogMessage(msg);
        WriteLockGuard lock(m_dataLock);

        m_isProcessingRE2 = false;
        if (!m_isProcessingPCRE2)
        {
            m_isProcessing = false;
        }
    }
    catch (...)
    {
        CString msg;
        AfxMessageBox(_T("RE2 처리 중 알 수 없는 예외가 발생했습니다."));
        AfxMessageBox(msg);
        LogMessage(msg);
        WriteLockGuard lock(m_dataLock);
        m_isProcessingRE2 = false;
        if (!m_isProcessingPCRE2)
        {
            m_isProcessing = false;
        }
    }
}

void CSingleFileComparisonDlg::ProcessingThreadPCRE2()
{
    {
        WriteLockGuard lock(m_dataLock);
        m_isProcessingPCRE2 = true;
    }

    try
    {
        std::ifstream fileStream;
        if (!OpenFileStream(fileStream))
        {
            WriteLockGuard lock(m_dataLock);
            m_isProcessingPCRE2 = false;
            if (!m_isProcessingRE2)
            {
                m_isProcessing = false;
            }
            return;
        }
        LogMessage(_T("ProcessingThreadRE2\n"));

        // FolderPathManager를 사용하여 폴더 경로 설정
        std::filesystem::path folderPath;
        if (FolderPathManager::GetInstance().IsGeneratedDataFolderPathSet())
        {
            folderPath = FolderPathManager::GetInstance().GetGeneratedDataFolderPath().GetString();
        }
        else
        {
            folderPath = "output_folder";
        }

        std::filesystem::path outputFilePath = folderPath / "output_pcre2.txt";

        // 폴더가 존재하지 않으면 생성합니다.
        if (!std::filesystem::exists(folderPath))   
        {
            if (!std::filesystem::create_directories(folderPath))
            {
                AfxMessageBox(_T("출력 폴더를 생성할 수 없습니다."));
                m_isProcessingPCRE2 = false;
                if (!m_isProcessingRE2)
                {
                    m_isProcessing = false;
                }
                return;
            }
        }

        // 출력 파일 열기
        std::ofstream outFile(outputFilePath.string(), std::ios::out | std::ios::binary);
        if (!outFile.is_open())
        {
            AfxMessageBox(_T("RE2 출력 파일을 열 수 없습니다."));
            WriteLockGuard lock(m_dataLock);
            m_isProcessingRE2 = false;
            if (!m_isProcessingPCRE2)
            {
                m_isProcessing = false;
            }
            return;
        }

        // UTF-8 BOM 추가
        outFile << "\xEF\xBB\xBF";

        // 패턴 컴파일 (이미 초기화되었다면 생략 가능)
        if (!Utils::InitializePatterns(PatternManager::GetInstance().GetPatterns()))
        {
            AfxMessageBox(_T("패턴 초기화에 실패했습니다."));
            return;
        }

        std::string line;
        int linesProcessed = 0;
        auto lastUpdateTime = m_startTime;


        // 총 처리 시간 측정을 위한 시작 시간 기록
        auto startTime = std::chrono::steady_clock::now();

        while (std::getline(fileStream, line))
        {
            if (m_shouldStop)
                break;

            // detectionsPerPattern for this line
            std::vector<int> detectionsPerPattern(PatternManager::GetInstance().GetPatterns().size(), 0);

            // PCRE2로 라인 처리
            std::string processedLine = Utils::ProcessLineWithPCRE2(line, detectionsPerPattern);

            // 처리된 라인을 출력 파일에 기록
            outFile << processedLine << "\n";

            linesProcessed++;

            // Update detection counts
            {
                WriteLockGuard lock(m_dataLock);
                for (size_t i = 0; i < detectionsPerPattern.size(); ++i)
                {
                    m_detectionsPerPatternPCRE2[i] += detectionsPerPattern[i];
                    m_totalDetectionsPCRE2 += detectionsPerPattern[i];
                    m_totalReplacementsPCRE2 += detectionsPerPattern[i];
                }
            }

            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastUpdateTime).count();

            if (elapsed >= m_refreshTime)
            {
                double chartTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_startTime).count() / 1000.0;

                CString formattedTime;
                formattedTime.Format(_T("%.3f"), chartTime);

                {
                    WriteLockGuard lock(m_dataLock);

                    m_pcre2TimeStamps.push_back(chartTime);
                    m_pcre2PerformanceData.push_back(linesProcessed);
                    m_pcre2TimeStampsStr.push_back(formattedTime);

                    // 오래된 데이터 제거
                    while (!m_pcre2TimeStamps.empty() && (m_pcre2TimeStamps.back() - m_pcre2TimeStamps.front() > m_maxDisplayDuration))
                    {
                        m_pcre2TimeStamps.pop_front();
                        m_pcre2PerformanceData.pop_front();
                        m_pcre2TimeStampsStr.pop_front();
                    }
                }

                linesProcessed = 0;
                lastUpdateTime = currentTime;  // 여기서 lastUpdateTime을 현재 시간으로 갱신합니다.

                // UI 업데이트 요청
                PostMessage(WM_UPDATE_CHART, 0, 0);
            }
        }

        // 총 처리 시간 계산
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> totalElapsed = endTime - startTime;

        {
            WriteLockGuard lock(m_dataLock);

            m_totalTimePCRE2 = totalElapsed.count();
            m_isProcessingPCRE2 = false;

            if (!m_isProcessingRE2)
            {
                m_isProcessing = false;
            }
        }

        // 총 처리 시간 업데이트 메시지 전송 (wParam = 2는 PCRE2)
        PostMessage(WM_UPDATE_TOTAL_TIME, 2, 0);

        fileStream.close();
        outFile.close();
    }
    catch (const std::exception& ex)
    {
        CString msg;
        msg.Format(_T("PCRE2 처리 중 예외가 발생했습니다: %S"), ex.what());
        AfxMessageBox(msg);

        WriteLockGuard lock(m_dataLock);
        m_isProcessingPCRE2 = false;
        if (!m_isProcessingRE2)
        {
            m_isProcessing = false;
        }
    }
    catch (...)
    {
        AfxMessageBox(_T("PCRE2 처리 중 알 수 없는 예외가 발생했습니다."));

        WriteLockGuard lock(m_dataLock);
        m_isProcessingPCRE2 = false;
        if (!m_isProcessingRE2)
        {
            m_isProcessing = false;
        }
    }
}

void CSingleFileComparisonDlg::UpdateChart()
{
    // 전역 시간 업데이트
    auto currentTimePoint = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = currentTimePoint - m_startTime;
    m_globalTime = elapsed.count();

    ReadLockGuard lock(m_dataLock);

    if (!m_isProcessing)
    {
        if (m_re2TimeStamps.empty() && m_pcre2TimeStamps.empty())
            return; // 두 덱이 모두 비어있을 경우에만 추가 작업을 중단
    }

    // 리스트 컨트롤의 그리기를 일시적으로 중지
    m_DataListCtrl.SetRedraw(FALSE);
    
    // 새로운 시간 문자열 생성
    CString formattedTime;
    formattedTime.Format(_T("%.3f"), m_globalTime);

    // RE2 데이터 가져오기
    CString re2DataStr;
    {
        if (m_isProcessingRE2)
        {
            if (!m_re2PerformanceData.empty())
            {
                re2DataStr.Format(_T("%d"), m_re2PerformanceData.back());
            }
            else
            {
                CString message;
                message.Format(_T("m_re2PerformanceData 데이터가 없을 경우 0으로 표시\n경과 시간: %.2f 초", m_globalTime));
                LogMessage(message);
                re2DataStr = _T("0"); // 데이터가 없을 경우 0으로 표시
            }
        }
        else
        {
            CString message;
            message.Format(_T("m_isProcessingRE2 처리가 완료된 경우 0으로 표시\n경과 시간: %.2f 초", m_globalTime));
            LogMessage(message);
            re2DataStr = _T("0"); // 처리가 완료된 경우 0으로 표시
        }
    }

    // PCRE2 데이터 가져오기
    CString pcre2DataStr;
    {
        if (m_isProcessingPCRE2)
        {
            if (!m_pcre2PerformanceData.empty())
            {
                pcre2DataStr.Format(_T("%d"), m_pcre2PerformanceData.back());
            }
            else
            {
                CString message;
                message.Format(_T("m_pcre2PerformanceData 처리가 완료된 경우 0으로 표시\n경과 시간: %.2f 초"), m_globalTime);
                LogMessage(message);
                pcre2DataStr = _T("0"); // 데이터가 없을 경우 0으로 표시
            }
        }
        else
        {
            CString message;
            message.Format(_T("m_isProcessingPCRE2 처리가 완료된 경우 0으로 표시\n경과 시간: %.2f 초", m_globalTime));
            LogMessage(message);
            pcre2DataStr = _T("0"); // 처리가 완료된 경우 0으로 표시
        }
    }

    // 리스트 컨트롤에 새로운 아이템 추가
    int index = m_DataListCtrl.InsertItem(m_DataListCtrl.GetItemCount(), formattedTime);
    m_DataListCtrl.SetItemText(index, 1, re2DataStr);
    m_DataListCtrl.SetItemText(index, 2, pcre2DataStr);

    // 로그 추가
    /*CString message;
    message.Format(_T("리스트 컨트롤 업데이트: 시간=%s, RE2=%s, PCRE2=%s"), formattedTime, re2DataStr, pcre2DataStr);
    LogMessage(message);*/
    // 최대 아이템 수 제한 (예: 100)
    const int MAX_ITEMS = 10000;
    while (m_DataListCtrl.GetItemCount() > MAX_ITEMS)
    {
        m_DataListCtrl.DeleteItem(0); // 가장 오래된 아이템 삭제
    }

    // 리스트 컨트롤의 최신 항목이 보이도록 스크롤
    if (m_DataListCtrl.GetItemCount() > 0)
    {
        m_DataListCtrl.EnsureVisible(m_DataListCtrl.GetItemCount() - 1, FALSE);
    }   

    // 그리기 활성화 및 화면 갱신
    m_DataListCtrl.SetRedraw(TRUE);
    m_DataListCtrl.Invalidate();
    m_DataListCtrl.UpdateWindow();


    // 공통된 시간 범위 찾기
    if (m_re2TimeStamps.empty() && m_pcre2TimeStamps.empty())
    {
        return; // 두 덱이 모두 비어있을 경우 추가 작업을 중단
    }

    // 현재 시간을 기준으로 X축 범위 설정
    double currentTime = m_globalTime;

    double minChartTime = currentTime - m_maxDisplayDuration;
    double maxChartTime = currentTime;

    // 그래프 데이터 준비
    std::vector<double> re2Times, re2Data;
    std::vector<double> pcre2Times, pcre2Data;

    // RE2 데이터가 있을 경우
    if (!m_re2TimeStamps.empty())
    {
        re2Times.assign(m_re2TimeStamps.begin(), m_re2TimeStamps.end());
        re2Data.assign(m_re2PerformanceData.begin(), m_re2PerformanceData.end());
    }

    // PCRE2 데이터가 있을 경우
    if (!m_pcre2TimeStamps.empty())
    {
        pcre2Times.assign(m_pcre2TimeStamps.begin(), m_pcre2TimeStamps.end());
        pcre2Data.assign(m_pcre2PerformanceData.begin(), m_pcre2PerformanceData.end());
    }


    // 그래프 그리기
    XYChart* c = new XYChart(900, 530);

    // 차트 제목 설정
    c->addTitle("RE2 and PCRE2 throughput comparison", "Mono Space Bold", 15);

    // 플롯 영역 설정
    c->setPlotArea(55, 55, 800, 430, 0xffffff, -1, -1, 0xcccccc, 0xcccccc);
    c->setClipping();

    minChartTime = currentTime - m_maxDisplayDuration;
    maxChartTime = currentTime;

    // X축 설정
    c->xAxis()->setDateScale(minChartTime, maxChartTime);
    c->xAxis()->setLabelFormat("{value|nn:ss}");
    c->xAxis()->setTickDensity(75, 15);
    c->xAxis()->setWidth(2);
    c->xAxis()->setMinTickInc(m_refreshTime * 0.001); // 250ms 단위

    // Y축 설정 // 줄 250
    std::string title = "throughput (Lines/" + std::to_string(m_refreshTime) + "ms)";
    c->yAxis()->setTitle(title.c_str(), "Mono Space Bold", 10);
    c->yAxis()->setWidth(2);

    // 범례 설정
    LegendBox* legendBox = c->addLegend(60, 30, false, "", 9);
    legendBox->setBackground(Chart::Transparent);
    legendBox->setFontSize(10); // 폰트 크기 조정
    legendBox->setFontColor(Chart::TextColor);


    // 안티앨리어싱 활성화
    c->setAntiAlias(true);

    // RE2 데이터 추가
    if (!re2Data.empty())
    {
        LineLayer* re2Layer = c->addSplineLayer();
        re2Layer->addDataSet(DoubleArray(&re2Data[0], (int)re2Data.size()), 0x8bc34a, "RE2");
        re2Layer->setXData(DoubleArray(&re2Times[0], (int)re2Times.size()));
        re2Layer->setLineWidth(2);
    }

    // PCRE2 데이터 추가
    if (!pcre2Data.empty())
    {
        LineLayer* pcre2Layer = c->addSplineLayer();
        pcre2Layer->addDataSet(DoubleArray(&pcre2Data[0], (int)pcre2Data.size()), 0x7cb5df, "PCRE2");
        pcre2Layer->setXData(DoubleArray(&pcre2Times[0], (int)pcre2Times.size()));
        pcre2Layer->setLineWidth(2);
    }

    // 차트 뷰어에 차트 설정
    m_ChartViewer.setChart(c);
    m_ChartViewer.updateViewPort(true, false);
}


bool CSingleFileComparisonDlg::OpenFileStream(std::ifstream& fileStream)
{
    fileStream.open(CT2A(m_filePath), std::ios::in);
    if (!fileStream.is_open())
    {
        AfxMessageBox(_T("파일을 열 수 없습니다."));
        return false;
    }
    return true;
}

void CSingleFileComparisonDlg::OnBnClickedButtonSaveResults()
{
    CString defaultFileName = _T("output_single.txt");
    CString savePath;

    // FolderPathManager에서 경로가 설정되어 있는지 확인
    if (FolderPathManager::GetInstance().IsResultFolderPathSet())
    {
        // 기존에 저장된 폴더 경로에 기본 파일 이름을 결합
        savePath = FolderPathManager::GetInstance().GetResultFolderPath() + _T("\\") + defaultFileName;
    }
    else
    {
        // 경로가 설정되어 있지 않다면 파일 다이얼로그를 열어 사용자가 경로를 선택하도록 함
        CFileDialog dlg(
            FALSE,                           // 저장용 다이얼로그
            _T("txt"),                       // 기본 파일 확장자
            defaultFileName,                 // 기본 파일 이름
            OFN_OVERWRITEPROMPT,             // 옵션
            _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||") // 파일 필터
        );

        if (dlg.DoModal() == IDOK)
        {
            savePath = dlg.GetPathName();

            // 파일 경로에서 폴더 경로만 추출
            CString folderPath = savePath.Left(savePath.ReverseFind(_T('\\')));

            // 폴더 경로를 FolderPathManager에 저장
            FolderPathManager::GetInstance().SetResultFolderPath(folderPath);
        }
        else
        {
            AfxMessageBox(_T("저장할 파일을 선택하지 않았습니다."));
            return;
        }
    }

    // 파일 열기
    CStringA savePathA(savePath);
    std::ofstream outFile(savePathA.GetString(), std::ios::out);

    if (!outFile.is_open())
    {
        AfxMessageBox(_T("파일을 열 수 없습니다."));
        return;
    }

    {
        ReadLockGuard lock(m_dataLock);

        // 1. 성능 비교 섹션
        outFile << "=== 성능 비교 ===\n";
        outFile << "RE2 총 처리 시간: " << m_totalTimeRE2 << " 초\n";
        outFile << "PCRE2 총 처리 시간: " << m_totalTimePCRE2 << " 초\n";

        if (m_totalTimePCRE2 > 0)
        {
            double ratio = m_totalTimePCRE2 / m_totalTimeRE2;
            outFile << "RE2는 PCRE2 대비 약 " << ratio << " 배 빠릅니다.\n";
        }
        else
        {
            outFile << "PCRE2 처리 시간이 0초여서 비교할 수 없습니다.\n";
        }

        outFile << "\n";

        // 2. 검출 통계 섹션
        outFile << "=== 검출 통계 ===\n";

        // RE2 검출 통계
        outFile << "RE2 검출 통계:\n";
        outFile << "총 검출 수: " << m_totalDetectionsRE2 << " 회\n";
        outFile << "패턴 별 검출 수:\n";
        const auto& re2Patterns = PatternManager::GetInstance().GetPatterns();
        for (size_t i = 0; i < re2Patterns.size(); ++i)
        {
            CStringA patternStrA(re2Patterns[i].pattern);
            outFile << "  패턴 " << i + 1 << " (" << patternStrA.GetString() << "): " << m_detectionsPerPatternRE2[i] << " 회\n";
        }

        outFile << "\n";

        // PCRE2 검출 통계
        outFile << "PCRE2 검출 통계:\n";
        outFile << "총 검출 수: " << m_totalDetectionsPCRE2 << " 회\n";
        outFile << "패턴 별 검출 수:\n";
        const auto& pcre2Patterns = PatternManager::GetInstance().GetPatterns();
        for (size_t i = 0; i < pcre2Patterns.size(); ++i)
        {
            CStringA patternStrA(pcre2Patterns[i].pattern);
            outFile << "  패턴 " << i + 1 << " (" << patternStrA.GetString() << "): " << m_detectionsPerPatternPCRE2[i] << " 회\n";
        }

        outFile << "\n";

        // 3. 치환 통계 섹션
        outFile << "=== 치환 통계 ===\n";

        // RE2 치환 통계
        outFile << "RE2 치환 통계:\n";
        outFile << "총 치환 수: " << m_totalReplacementsRE2 << " 회\n";
        outFile << "패턴 별 치환 수:\n";
        for (size_t i = 0; i < re2Patterns.size(); ++i)
        {
            CStringA patternStrA(re2Patterns[i].pattern);
            outFile << "  패턴 " << i + 1 << " (" << patternStrA.GetString() << "): " << m_detectionsPerPatternRE2[i] << " 회\n";
        }

        outFile << "\n";

        // PCRE2 치환 통계
        outFile << "PCRE2 치환 통계:\n";
        outFile << "총 치환 수: " << m_totalReplacementsPCRE2 << " 회\n";
        outFile << "패턴 별 치환 수:\n";
        for (size_t i = 0; i < pcre2Patterns.size(); ++i)
        {
            CStringA patternStrA(pcre2Patterns[i].pattern);
            outFile << "  패턴 " << i + 1 << " (" << patternStrA.GetString() << "): " << m_detectionsPerPatternPCRE2[i] << " 회\n";
        }

        outFile << "\n\n";

        for (size_t i = 0; i < re2Patterns.size(); i++)
        {
            if (m_detectionsPerPatternRE2[i] != m_detectionsPerPatternPCRE2[i])
            {
                outFile << "패턴 " << i << "번째의 치환수가 다릅니다.\n";
            }
        }
    }

    outFile.close();
    AfxMessageBox(_T("결과가 성공적으로 저장되었습니다."));
}


void CSingleFileComparisonDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == IDT_TIMER_UPDATE_CHART)
    {
        UpdateChart();
    }

    CDialogEx::OnTimer(nIDEvent);
}

void CSingleFileComparisonDlg::OnCbnSelchangeComboRefresh()
{
    // 콤보 박스의 선택이 변경되면 호출됩니다.
    // 현재 선택된 값을 가져와 m_refreshTime에 설정합니다.
    int sel = m_ComboRefresh.GetCurSel();
    if (sel != CB_ERR)
    {
        CString strValue;
        m_ComboRefresh.GetLBText(sel, strValue);
        int newRefreshTime = _ttoi(strValue);

        {
            WriteLockGuard lock(m_dataLock);
            m_refreshTime = newRefreshTime;
        }

        // 타이머 재설정
        KillTimer(IDT_TIMER_UPDATE_CHART);
        SetTimer(IDT_TIMER_UPDATE_CHART, m_refreshTime, NULL);

        // 그래프 업데이트 주기 변경을 반영하기 위해 UpdateChart 호출
        UpdateChart();
    }
}

void CSingleFileComparisonDlg::OnCbnSelchangeComboSecond()
{
    // 콤보 박스의 선택이 변경되면 호출됩니다.
    // 현재 선택된 값을 가져와 m_maxDisplayDuration에 설정합니다.
    int sel = m_ComboSecond.GetCurSel();
    if (sel != CB_ERR)
    {
        CString strValue;
        m_ComboSecond.GetLBText(sel, strValue);
        int newMaxDisplayDuration = _ttoi(strValue);
        {
            WriteLockGuard lock(m_dataLock);
            m_maxDisplayDuration = newMaxDisplayDuration;
        }

        // 그래프 그리기 갱신
        Invalidate();
        UpdateChart();
    }
}
