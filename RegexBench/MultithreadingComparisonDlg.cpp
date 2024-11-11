// MultithreadingComparisonDlg.cpp

#include "pch.h"
#include "MultithreadingComparisonDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "Utils.h"
#include "PatternManager.h"
#include "SpinRWLock.h"
#include "FolderPathManager.h"

#define WM_UPDATE_PROGRESS (WM_USER + 1)
#define WM_TASK_COMPLETE    (WM_USER + 2)
#define WM_UPDATE_LIST (WM_USER + 3)

IMPLEMENT_DYNAMIC(CMultithreadingComparisonDlg, CDialogEx)

CMultithreadingComparisonDlg::CMultithreadingComparisonDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_MULTITHREADING_COMPARISON_DLG, pParent)
    , m_gdiplusToken(0)
    , max_threads(0)
    , m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME))
{
}

CMultithreadingComparisonDlg::~CMultithreadingComparisonDlg()
{
    // 비동기 작업이 완료될 때까지 대기
    if (m_processingTask.valid())
    {
        m_processingTask.wait();
    }

    // GDI+ 종료
    Gdiplus::GdiplusShutdown(m_gdiplusToken);
}

void CMultithreadingComparisonDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_THREAD_COUNT, m_editThreadCount);
    DDX_Control(pDX, IDC_BUTTON_START_MT, m_buttonStart);
    DDX_Control(pDX, IDC_STATIC_GRAPH_MT, m_staticGraph);
    DDX_Control(pDX, IDC_PROGRESS_MT, m_progressBar);
    DDX_Control(pDX, IDC_STATIC_FILE_SIZE_MT, m_staticFileSize);
    DDX_Control(pDX, IDC_STATIC_FILE_PATH_MT, m_staticFilePath);
    DDX_Control(pDX, IDC_LISTCTRL_DATA2, m_DataListCtrl);
    DDX_Control(pDX, IDC_BUTTON_RESULT_MT, m_buttonResult);
}

BEGIN_MESSAGE_MAP(CMultithreadingComparisonDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_START_MT, &CMultithreadingComparisonDlg::OnBnClickedButtonStart)
    ON_BN_CLICKED(IDC_BUTTON_CHECKTHREAD_MT, &CMultithreadingComparisonDlg::OnBnClickedButtonThreadCheck)
    ON_WM_PAINT()
    ON_ACN_START(IDC_STATIC_GRAPH_MT, &CMultithreadingComparisonDlg::OnAcnStartStaticGraphMt)
    ON_MESSAGE(WM_UPDATE_PROGRESS, &CMultithreadingComparisonDlg::OnUpdateProgress) // 커스텀 메시지 매핑
    ON_MESSAGE(WM_TASK_COMPLETE, &CMultithreadingComparisonDlg::OnTaskComplete)    // 작업 완료 메시지
    ON_BN_CLICKED(IDC_BUTTON_RESULT_MT, &CMultithreadingComparisonDlg::OnBnClickedButtonResultMt)
    ON_MESSAGE(WM_UPDATE_LIST, &CMultithreadingComparisonDlg::OnUpdateList)
END_MESSAGE_MAP()

BOOL CMultithreadingComparisonDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // GDI+ 초기화
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

    // 컨트롤 초기화
    m_editThreadCount.SetWindowTextW(L"4"); // 기본 쓰레드 수 설정
    m_buttonStart.SetWindowTextW(L"처리 시작");

    // 그래프 초기화
    InitializeGraph();

    // 프로그래스 바 초기화
    m_progressBar.SetRange(0, 100); // 초기 최대값, 필요에 따라 변경
    m_progressBar.SetPos(0);

    // 파일 경로와 크기 초기화
    m_staticFilePath.SetWindowTextW(L"파일 경로: ");
    m_staticFileSize.SetWindowTextW(L"파일 크기: ");

    // 결과 버튼 비활성화
    m_buttonResult.EnableWindow(FALSE);

    InitializeListCtrl();
    return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CMultithreadingComparisonDlg::InitializeGraph()
{
    // 그래프 영역 설정
    m_staticGraph.GetWindowRect(&graph_rect);
    ScreenToClient(&graph_rect);
}

void CMultithreadingComparisonDlg::InitializeListCtrl()
{
    // 리스트 컨트롤을 리포트 뷰로 설정
    m_DataListCtrl.ModifyStyle(0, LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS);

    // 리스트 컨트롤 설정: 보고서(Report) 모드로 설정
    m_DataListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    // 컬럼 추가
    m_DataListCtrl.InsertColumn(0, _T("쓰레드 수"), LVCFMT_CENTER, 60);
    m_DataListCtrl.InsertColumn(1, _T("RE2 소요 시간 (ms)"), LVCFMT_CENTER, 110);
    m_DataListCtrl.InsertColumn(2, _T("PCRE2 소요 시간 (ms)"), LVCFMT_CENTER, 120);
    m_DataListCtrl.InsertColumn(3, _T("RE2 향상율 (%)"), LVCFMT_CENTER, 90);
    m_DataListCtrl.InsertColumn(4, _T("PCRE2 향상율 (%)"), LVCFMT_CENTER, 105);

    // 리스트 뷰 업데이트를 빠르게 하기 위해 비활성화
    m_DataListCtrl.SetRedraw(FALSE);
    m_DataListCtrl.DeleteAllItems(); // 기존 아이템 삭제
    m_DataListCtrl.SetRedraw(TRUE);
    m_DataListCtrl.Invalidate();
}

// 열 너비 자동 조정 함수
void CMultithreadingComparisonDlg::AutoSizeColumns()
{
    m_DataListCtrl.SetRedraw(FALSE);

    for (int i = 0; i < m_DataListCtrl.GetHeaderCtrl()->GetItemCount(); ++i)
    {
        m_DataListCtrl.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
    }

    m_DataListCtrl.SetRedraw(TRUE);
    m_DataListCtrl.Invalidate();
}

void CMultithreadingComparisonDlg::UpdateFileSizeDisplay(const CString& filePath)
{
    CFile file;
    if (file.Open(filePath, CFile::modeRead))
    {
        ULONGLONG fileSize = file.GetLength(); // 파일 크기(바이트 단위)
        file.Close();

        // 파일 크기를 적절한 단위로 변환 (KB, MB 등)
        CString fileSizeStr;
        if (fileSize < 1024)
        {
            fileSizeStr.Format(_T("파일 크기 : %llu Bytes"), fileSize);
        }
        else if (fileSize < 1024 * 1024)
        {
            double sizeKB = static_cast<double>(fileSize) / 1024.0;
            fileSizeStr.Format(_T("파일 크기 : %.2f KB"), sizeKB);
        }
        else if (fileSize < 1024 * 1024 * 1024)
        {
            double sizeMB = static_cast<double>(fileSize) / (1024.0 * 1024.0);
            fileSizeStr.Format(_T("파일 크기 : %.2f MB"), sizeMB);
        }
        else
        {
            double sizeGB = static_cast<double>(fileSize) / (1024.0 * 1024.0 * 1024.0);
            fileSizeStr.Format(_T("파일 크기 : %.2f GB"), sizeGB);
        }

        // 파일 크기 표시
        m_staticFileSize.SetWindowTextW(fileSizeStr);
    }
    else
    {
        // 파일 열기에 실패한 경우
        m_staticFileSize.SetWindowTextW(L"파일 크기를 가져올 수 없습니다.");
    }
}


void CMultithreadingComparisonDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting
        // 아이콘 그리기
    }
    else
    {
        CPaintDC dc(this); // device context for painting

        // 그래프 그리기
        DrawGraph(&dc);

        CDialogEx::OnPaint();
    }
}

HCURSOR CMultithreadingComparisonDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}


void CMultithreadingComparisonDlg::OnBnClickedButtonStart()
{
    // 쓰레드 수 가져오기
    CString threadCountStr;
    m_editThreadCount.GetWindowTextW(threadCountStr);
    int threadCount = _ttoi(threadCountStr);
    if (threadCount < 1)
    {
        AfxMessageBox(_T("올바른 쓰레드 수를 입력하세요. (최소 1개)"));
        return;
    }

    // 처리 시작 버튼 비활성화
    m_buttonStart.EnableWindow(FALSE);

    CString initialDir;
    // FolderPathManager에서 생성 데이터 폴더 경로가 설정되어 있는지 확인
    if (FolderPathManager::GetInstance().IsGeneratedDataFolderPathSet())
    {
        initialDir = FolderPathManager::GetInstance().GetGeneratedDataFolderPath();
    }

    // 파일 다이얼로그 열기
    CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"), this);

    // 초기 디렉토리 설정 (경로가 설정된 경우에만)
    if (!initialDir.IsEmpty())
    {
        dlg.m_ofn.lpstrInitialDir = initialDir;
    }

    if (dlg.DoModal() == IDOK)
    {
        // 실제 파일 경로 저장
        CString actualFilePath = dlg.GetPathName();
        m_filePath = actualFilePath; // m_filePath에 실제 경로 저장

        // UI에 표시할 문자열 생성
        CString displayPath;
        displayPath.Format(_T("파일 경로 : %s"), actualFilePath);

        AfxMessageBox(_T("파일이 성공적으로 업로드되었습니다."));

        // 파일 경로 표시
        m_staticFilePath.SetWindowTextW(displayPath);

        UpdateFileSizeDisplay(actualFilePath);
    }
    else
    {
        // 사용자가 파일 선택을 취소한 경우
        m_staticFilePath.SetWindowTextW(L"파일을 업로드하지 않았습니다.");
        m_staticFileSize.SetWindowTextW(L"파일 크기: N/A");
        m_buttonStart.EnableWindow(TRUE); // 버튼 다시 활성화
        return;
    }

    // FolderPathManager에서 결과 폴더 경로 가져오기
    CString resultFolderPath = FolderPathManager::GetInstance().GetResultFolderPath();

    // 결과 폴더 경로가 비어있는지 확인
    if (resultFolderPath.IsEmpty())
    {
        // 결과 폴더 경로가 설정되어 있지 않음
        AfxMessageBox(_T("결과 폴더 경로가 설정되어 있지 않습니다. 폴더를 선택해주세요."));

        // 폴더 선택 대화 상자 열기
        TCHAR exePath[MAX_PATH];
        GetModuleFileName(NULL, exePath, MAX_PATH);
        PathRemoveFileSpec(exePath); // 파일 이름 제거하여 폴더 경로만 남김

        // 첫 번째 폴더 선택: 결과 저장 폴더
        CFolderPickerDialog resultFolderDlg(exePath, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, this);
        resultFolderDlg.m_ofn.lpstrTitle = _T("결과를 저장할 폴더를 선택하세요");
        if (resultFolderDlg.DoModal() == IDOK)
        {
            CString resultPath = resultFolderDlg.GetPathName();
            FolderPathManager::GetInstance().SetResultFolderPath(resultPath);
        }
        else
        {
            AfxMessageBox(_T("결과를 저장할 폴더 선택이 취소되었습니다."));
            m_buttonStart.EnableWindow(TRUE);
            return; // 첫 번째 폴더 선택이 취소되면 함수 종료
        }

    }

    // 패턴 초기화 (중복 제거 및 초기화 호출)
    if (!Utils::InitializePatterns(PatternManager::GetInstance().GetPatterns()))
    {
        AfxMessageBox(_T("패턴 초기화에 실패했습니다."));
        m_buttonStart.EnableWindow(TRUE); // 버튼 다시 활성화
        return;
    }

    // 그래프 데이터 초기화
    re2_times.clear();
    pcre2_times.clear();
    max_threads = threadCount;

    // 스레드 수가 2부터 max_threads까지 증가하므로 총 (max_threads - 1)번의 측정이 있습니다.
    int numThreadCounts = max_threads;

    // 벡터를 미리 크기를 지정하여 초기화
    re2_times.resize(numThreadCounts, 0.0);
    pcre2_times.resize(numThreadCounts, 0.0);

    // 프로그래스 바 설정
    m_progressBar.SetRange(0, numThreadCounts * 2); // RE2와 PCRE2 각각 처리
    m_progressBar.SetPos(0);

    // 리스트 컨트롤 초기화 (기존 데이터 삭제)
    m_DataListCtrl.DeleteAllItems();

    // 비동기 작업 시작
    m_processingTask = std::async(std::launch::async, [this, threadCount, numThreadCounts]() {
        for (int idx = 0; idx < numThreadCounts; ++idx)
        {
            int currentThreadCount = idx + 1; // 스레드 수는 1부터 시작

            if (currentThreadCount == 1)
            {
                // 싱글스레드 처리
                // 파일 전체를 하나의 청크로 처리
                std::vector<FileChunk> chunks = Utils::SplitFileIntoChunks(m_filePath, 1);

                double re2_duration = Utils::ProcessChunkWithRE2(m_filePath, chunks[0], 0);
                double pcre2_duration = Utils::ProcessChunkWithPCRE2(m_filePath, chunks[0], 0);

                {
                    WriteLockGuard lock(m_dataLock);
                    re2_times[idx] = re2_duration;
                    pcre2_times[idx] = pcre2_duration;
                }

                // 프로그래스 바 업데이트
                PostMessage(WM_UPDATE_PROGRESS, 0, 0);
                PostMessage(WM_UPDATE_PROGRESS, 0, 0);

                // 리스트 컨트롤 업데이트를 위해 메시지 전송
                PostMessage(WM_UPDATE_LIST, static_cast<WPARAM>(idx), 0);
            }
            else
            {
                // 파일을 청크로 분할
                std::vector<FileChunk> chunks = Utils::SplitFileIntoChunks(m_filePath, currentThreadCount);
                if (chunks.size() != currentThreadCount)
                {
                    PostMessage(WM_UPDATE_PROGRESS, 0, 0);
                    PostMessage(WM_UPDATE_PROGRESS, 0, 0);
                    continue;
                }

                // RE2 처리
                {
                    std::vector<std::thread> threads;
                    std::vector<double> thread_times(currentThreadCount, 0.0);

                    for (int i = 0; i < currentThreadCount; ++i)
                    {
                        threads.emplace_back([this, &chunks, i, &thread_times]() {
                            // 각 스레드에서 파일 청크 처리 
                            double duration = Utils::ProcessChunkWithRE2(m_filePath, chunks[i], i);
                            //TRACE(_T("RE2 thread number = %d , %.3f ms duration\n"), i, duration);
                            thread_times[i] = duration;
                            });
                    }

                    for (auto& th : threads)
                    {
                        if (th.joinable())
                            th.join();
                    }

                    // 총 처리 시간 계산
                    double total_time = *std::max_element(thread_times.begin(), thread_times.end());
                    {
                        WriteLockGuard lock(m_dataLock);
                        re2_times[idx] = total_time;
                    }
                    // 프로그래스 바 업데이트
                    PostMessage(WM_UPDATE_PROGRESS, 0, 0);

                    // 리스트 컨트롤 업데이트를 위해 메시지 전송
                    PostMessage(WM_UPDATE_LIST, static_cast<WPARAM>(idx), 0);

                }

                // PCRE2 처리
                {
                    std::vector<std::thread> threads;
                    std::vector<double> thread_times(currentThreadCount, 0.0);

                    for (int i = 0; i < currentThreadCount; ++i)
                    {
                        threads.emplace_back([this, &chunks, i, &thread_times]() {
                            // 각 스레드에서 파일 청크 처리 
                            double duration = Utils::ProcessChunkWithPCRE2(m_filePath, chunks[i], i);
                            //TRACE(_T("PCRE2 thread number = %d , %.3f ms duration\n"), i, duration);
                            thread_times[i] = duration;
                            });
                    }

                    for (auto& th : threads)
                    {
                        if (th.joinable())
                            th.join();
                    }

                    // 총 처리 시간 계산
                    double total_time = *std::max_element(thread_times.begin(), thread_times.end());
                    {
                        WriteLockGuard lock(m_dataLock);
                        pcre2_times[idx] = total_time;
                    }
                    // 프로그래스 바 업데이트
                    PostMessage(WM_UPDATE_PROGRESS, 0, 0);

                    // 리스트 컨트롤 업데이트를 위해 메시지 전송
                    PostMessage(WM_UPDATE_LIST, static_cast<WPARAM>(idx), 0);
                }
            }

        }

        // 작업 완료 메시지 전송
        PostMessage(WM_TASK_COMPLETE, 0, 0);
        });

    // 처리 시작 버튼은 작업이 완료될 때까지 비활성화 상태 유지
}

void CMultithreadingComparisonDlg::OnBnClickedButtonThreadCheck()
{
    // 쓰레드 수 가져오기
    int n = std::thread::hardware_concurrency();
    // 메시지 형식 지정
    CString message;
    message.Format(_T("사용자의 쓰레드 갯수는 : %d개 입니다."), n);

    // 메시지 박스 표시
    AfxMessageBox(message);
    CString str;
    str.Format(L"%d", n); // 정수를 문자열로 포맷
    m_editThreadCount.SetWindowTextW(str);
}

void CMultithreadingComparisonDlg::DrawGraph(CDC* pDC)
{
    if (re2_times.empty() || pcre2_times.empty())
        return;

    Gdiplus::Graphics graphics(pDC->GetSafeHdc());

    // 배경을 흰색으로 채우기
    SolidBrush backgroundBrush(Color(255, 255, 255, 255)); // White color
    graphics.FillRectangle(&backgroundBrush, graph_rect.left, graph_rect.top, graph_rect.Width(), graph_rect.Height());

    // 그래프 영역 설정
    int margin = 50;
    CRect plotArea = graph_rect;
    plotArea.DeflateRect(margin, margin, margin, margin);

    // 그리드 펜 설정
    Pen gridPen(Color(200, 200, 200), 1); // 연한 회색 얇은 선

    // 축 설정
    double max_time = 0.0;
    for (auto t : re2_times)
        if (t > max_time) max_time = t;
    for (auto t : pcre2_times)
        if (t > max_time) max_time = t;

    max_time = Utils::NiceNumber(max_time * 1.2, true);

    // 최대 시간을 최소 1로 설정하여 0으로 나누는 상황을 방지합니다.
    if (max_time == 0.0)
        max_time = 1.0;

    // Y축 눈금
    int numYTicks = 5;
    double yTickInterval = max_time / numYTicks;

    // 글꼴 및 브러시 설정
    Gdiplus::Font font(L"Arial", 10);
    SolidBrush brush(Color(0, 0, 0));

    // Y축 그리기 및 눈금 표시
    Pen axisPen(Color(0, 0, 0), 2);
    graphics.DrawLine(&axisPen, plotArea.left, plotArea.top, plotArea.left, plotArea.bottom);
    graphics.DrawLine(&axisPen, plotArea.left, plotArea.bottom, plotArea.right, plotArea.bottom);

    for (int i = 0; i <= numYTicks; ++i)
    {
        double yValue = yTickInterval * i;
        float yPos = static_cast<float>(plotArea.bottom - (yValue / max_time) * plotArea.Height());

        // 수평 그리드 라인 그리기
        graphics.DrawLine(&gridPen, plotArea.left, (int)yPos, plotArea.right, (int)yPos);

        // 눈금 그리기
        graphics.DrawLine(&axisPen, plotArea.left - 5, (int)yPos, plotArea.left, (int)yPos);

        // 눈금 값 표시
        CString yText;
        yText.Format(_T("%.0f"), yValue);
        graphics.DrawString(yText, -1, &font, PointF(plotArea.left - 40, yPos - 10), &brush);
    }

    // X축 눈금 및 레이블
    int numBars = re2_times.size(); // 스레드 수 그룹 개수
    int groupWidth = plotArea.Width() / numBars; // 각 그룹의 너비

    // 막대 너비와 그룹 내 간격 설정
    float barWidth = groupWidth * 0.3f; // 각 막대의 너비 (그룹 너비의 30%)
    float interBarSpacing = groupWidth * 0.05f; // 그룹 내 막대 간의 간격 (그룹 너비의 5%)

    for (int i = 0; i < numBars; ++i)
    {
        int thread_num = i + 1; // 스레드 수가 2부터 시작

        // 각 그룹의 시작 X 위치
        float groupStartX = static_cast<float>(plotArea.left + i * groupWidth);

        // X축 눈금 그리기
        float xCenter = groupStartX + groupWidth / 2;
        graphics.DrawLine(&axisPen, static_cast<int>(xCenter), plotArea.bottom, static_cast<int>(xCenter), plotArea.bottom + 5);

        // X축 레이블 표시
        CString xText;
        xText.Format(_T("%d"), thread_num);
        graphics.DrawString(xText, -1, &font, PointF(xCenter - 10, plotArea.bottom + 10), &brush);

        // RE2 막대 위치 계산
        float re2_x = groupStartX + (groupWidth / 2) - barWidth - (interBarSpacing / 2);
        // RE2 막대 그리기
        double re2_time = re2_times[i];
        float re2_height = static_cast<float>((re2_time / max_time) * plotArea.Height());
        re2_height = min(re2_height, static_cast<float>(plotArea.Height())); // 클램프
        RectF re2Rect(re2_x, plotArea.bottom - re2_height, barWidth, re2_height);
        SolidBrush re2Brush(Gdiplus::Color(255, 139, 195, 74)); // RE2 색상
        graphics.FillRectangle(&re2Brush, re2Rect);

        // PCRE2 막대 위치 계산
        float pcre2_x = groupStartX + (groupWidth / 2) + (interBarSpacing / 2);
        // PCRE2 막대 그리기
        double pcre2_time = pcre2_times[i];
        float pcre2_height = static_cast<float>((pcre2_time / max_time) * plotArea.Height());
        pcre2_height = min(pcre2_height, static_cast<float>(plotArea.Height())); // 클램프
        RectF pcre2Rect(pcre2_x, plotArea.bottom - pcre2_height, barWidth, pcre2_height);
        SolidBrush pcre2Brush(Gdiplus::Color(255, 124, 181, 223)); // PCRE2 색상
        graphics.FillRectangle(&pcre2Brush, pcre2Rect);
    }

    // 축 이름
    CString yLabel = _T("처리 시간 (ms)");
    CString xLabel = _T("쓰레드 수");
    graphics.DrawString(yLabel, -1, &font, PointF(plotArea.left - 40, plotArea.top - 30), &brush);
    graphics.DrawString(xLabel, -1, &font, PointF(plotArea.right - 30, plotArea.bottom + 30), &brush);

    // 범례 그리기
    SolidBrush re2LegendBrush(Gdiplus::Color(255, 139, 195, 74));
    SolidBrush pcre2LegendBrush(Gdiplus::Color(255, 124, 181, 223));
    RectF re2LegendRect(plotArea.right - 150, plotArea.top + 10, 20, 20);
    RectF pcre2LegendRect(plotArea.right - 150, plotArea.top + 40, 20, 20);
    graphics.FillRectangle(&re2LegendBrush, re2LegendRect);
    graphics.FillRectangle(&pcre2LegendBrush, pcre2LegendRect);

    Gdiplus::Font legendFont(L"Arial", 10);
    graphics.DrawString(L"RE2", -1, &legendFont, PointF(plotArea.right - 120, plotArea.top + 10), &brush);
    graphics.DrawString(L"PCRE2", -1, &legendFont, PointF(plotArea.right - 120, plotArea.top + 40), &brush);
}


void CMultithreadingComparisonDlg::OnAcnStartStaticGraphMt()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

LRESULT CMultithreadingComparisonDlg::OnUpdateProgress(WPARAM wParam, LPARAM lParam)
{
    // 프로그래스 바의 현재 위치를 1씩 증가시킵니다.
    int pos = m_progressBar.GetPos();
    m_progressBar.SetPos(pos + 1);
    return 0;
}

LRESULT CMultithreadingComparisonDlg::OnTaskComplete(WPARAM wParam, LPARAM lParam)
{
    // 작업 완료 시 버튼 활성화
    m_buttonStart.EnableWindow(TRUE);

    // 결과 버튼 활성화
    m_buttonResult.EnableWindow(TRUE);

    // 저장 경로 가져오기
    CString resultFolderPath = FolderPathManager::GetInstance().GetResultFolderPath();

    // 경로가 설정되어 있지 않으면 사용자에게 경로를 선택하도록 요청
    if (resultFolderPath.IsEmpty())
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileDialog* pFileDialog = nullptr;

            // FOS_PICKFOLDERS 플래그를 사용하여 폴더 선택 다이얼로그 생성
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileDialog, reinterpret_cast<void**>(&pFileDialog));

            if (SUCCEEDED(hr))
            {
                DWORD dwFlags;

                // 현재 다이얼로그의 옵션 가져오기
                hr = pFileDialog->GetOptions(&dwFlags);
                if (SUCCEEDED(hr))
                {
                    // FOS_PICKFOLDERS 플래그 추가
                    hr = pFileDialog->SetOptions(dwFlags | FOS_PICKFOLDERS);
                    if (SUCCEEDED(hr))
                    {
                        // 다이얼로그 제목 설정
                        pFileDialog->SetTitle(L"결과를 저장할 폴더를 선택하세요.");

                        // 다이얼로그 표시
                        hr = pFileDialog->Show(NULL);
                        if (SUCCEEDED(hr))
                        {
                            IShellItem* pItem = nullptr;
                            hr = pFileDialog->GetResult(&pItem);
                            if (SUCCEEDED(hr))
                            {
                                PWSTR pszFilePath = nullptr;
                                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                                if (SUCCEEDED(hr))
                                {
                                    // 선택한 경로을 CString으로 변환
                                    resultFolderPath = CString(pszFilePath);
                                    // 메모리 해제
                                    CoTaskMemFree(pszFilePath);

                                    // 경로 설정
                                    FolderPathManager::GetInstance().SetResultFolderPath(resultFolderPath);
                                }
                                pItem->Release();
                            }
                        }
                        else
                        {
                            // 사용자가 취소를 누른 경우
                            AfxMessageBox(_T("결과 폴더가 선택되지 않았습니다."));
                            pFileDialog->Release();
                            CoUninitialize();
                            return 0;
                        }
                    }
                }
                pFileDialog->Release();
            }
            CoUninitialize();
        }
        else
        {
            AfxMessageBox(_T("COM 초기화에 실패했습니다."));
            return 0;
        }
    }

    // 최종 결과 파일 경로 설정
    CString outputFileRE2Path = resultFolderPath + _T("\\output_mt_re2.txt");
    CString outputFilePCRE2Path = resultFolderPath + _T("\\output_mt_pcre2.txt");

    // 임시 파일 병합
    int numThreads = max_threads; // 작업한 스레드 수
    CString prefix; // 임시 파일의 접두사 (RE2 또는 PCRE2)
    // 원하는 라이브러리에 따라 접두사를 설정합니다.
    // 여기서는 예시로 RE2 결과를 병합합니다.
    CString prefixre2 = _T("output_re2");
    CString prefixpcre2 = _T("output_pcre2");

    MergeOutputFiles(outputFileRE2Path, numThreads, prefixre2);
    MergeOutputFiles(outputFilePCRE2Path, numThreads, prefixpcre2);

    // 그래프를 다시 그리기 위해 갱신
    Invalidate();

    return 0;
}


void CMultithreadingComparisonDlg::MergeOutputFiles(const CString& outputFileName, int numThreads, const CString& prefix)
{
    std::ofstream finalOutput(CT2A(outputFileName), std::ios::binary);
    if (!finalOutput.is_open())
    {
        // 에러 처리
        AfxMessageBox(_T("결과 파일을 열 수 없습니다."));
        return;
    }

    // 결과 폴더 경로 가져오기
    CString resultFolderPath = FolderPathManager::GetInstance().GetResultFolderPath();

    for (int i = 0; i < numThreads; ++i)
    {
        CString tempFileName;
        // 결과 폴더 경로를 포함하여 임시 파일 경로 설정
        tempFileName.Format(_T("%s\\%s_thread_%d.txt"), resultFolderPath, prefix, i + 1);

        std::ifstream tempFile(CT2A(tempFileName), std::ios::binary);
        if (tempFile.is_open())
        {
            finalOutput << tempFile.rdbuf();
            tempFile.close();
            // 임시 파일 삭제
            ::DeleteFile(tempFileName);
        }
        else
        {
            CString errorMsg;
            errorMsg.Format(_T("임시 파일을 열 수 없습니다: %s"), tempFileName);
            AfxMessageBox(errorMsg);
        }
    }

    finalOutput.close();
}



void CMultithreadingComparisonDlg::OnBnClickedButtonResultMt()
{
    // 데이터가 있는지 확인합니다.
    if (re2_times.empty() || pcre2_times.empty())
    {
        AfxMessageBox(_T("저장할 데이터가 없습니다. 먼저 작업을 수행하세요."));
        return;
    }

    CString filePath;

    // 싱글톤에 저장된 결과 경로가 있는지 확인
    if (FolderPathManager::GetInstance().IsResultFolderPathSet())
    {
        filePath = FolderPathManager::GetInstance().GetResultFolderPath() + _T("result_mt.txt");
    }
    else
    {
        // 결과 경로가 설정되지 않은 경우, 대화상자를 통해 경로 설정
        CFileDialog saveFileDialog(FALSE, _T("txt"), _T("result_mt.txt"),
            OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"), this);

        if (saveFileDialog.DoModal() == IDOK)
        {
            filePath = saveFileDialog.GetPathName();
            FolderPathManager::GetInstance().SetResultFolderPath(filePath); // 선택한 경로를 싱글톤에 저장
        }
        else
        {
            AfxMessageBox(_T("파일 저장이 취소되었습니다."));
            return; // 저장이 취소되었으므로 함수 종료
        }
    }

    // 파일 열기
    std::ofstream outFile(CT2A(filePath), std::ios::out | std::ios::trunc);
    if (!outFile.is_open())
    {
        AfxMessageBox(_T("파일을 열 수 없습니다."));
        return;
    }

    // 파일에 결과 작성
    CString temp;
    temp.Format(_T("작업이 완료되었습니다.\n\n각 스레드 수별 성능 비교:\n\n"));
    outFile << CT2A(temp) << std::endl;


    double singleThreadRe2Time = re2_times[0];
    double singleThreadPcre2Time = pcre2_times[0];

    for (int i = 0; i < re2_times.size(); ++i)
    {
        int thread_num = i + 1; // 스레드 수는 1부터 시작
        double re2_time = re2_times[i];
        double pcre2_time = pcre2_times[i];

        // 성능 향상 비율 계산
        double re2Improvement = ((singleThreadRe2Time - re2_time) / singleThreadRe2Time) * 100.0;
        double pcre2Improvement = ((singleThreadPcre2Time - pcre2_time) / singleThreadPcre2Time) * 100.0;

        CString resultLine;
        resultLine.Format(_T("스레드 수 %d: RE2 시간 %.2f ms (향상율: %.2f%%), PCRE2 시간 %.2f ms (향상율: %.2f%%)\n"),
            thread_num, re2_time, re2Improvement, pcre2_time, pcre2Improvement);
        outFile << CT2A(resultLine);
    }

    // 최대 성능 향상 비율 계산
    double maxRe2Improvement = 0.0;
    double maxPcre2Improvement = 0.0;

    for (int i = 1; i < re2_times.size(); ++i) // 싱글스레드는 제외
    {
        double re2Improvement = ((singleThreadRe2Time - re2_times[i]) / singleThreadRe2Time) * 100.0;
        double pcre2Improvement = ((singleThreadPcre2Time - pcre2_times[i]) / singleThreadPcre2Time) * 100.0;

        maxRe2Improvement = max(maxRe2Improvement, re2Improvement);
        maxPcre2Improvement = max(maxPcre2Improvement, pcre2Improvement);
    }

    CString summary;
    summary.Format(_T("\n싱글스레드에 비해 멀티쓰레딩의 경우 RE2는 최대 %.2f%% 빠릅니다.\n"), maxRe2Improvement);
    outFile << CT2A(summary);

    summary.Format(_T("싱글스레드에 비해 멀티쓰레딩의 경우 PCRE2는 최대 %.2f%% 빠릅니다.\n"), maxPcre2Improvement);
    outFile << CT2A(summary);
    outFile.close();

    AfxMessageBox(_T("결과가 성공적으로 저장되었습니다."));
}


LRESULT CMultithreadingComparisonDlg::OnUpdateList(WPARAM wParam, LPARAM lParam)
{
    int idx = static_cast<int>(wParam);

    // 데이터에 접근하기 전에 잠금
    {
        ReadLockGuard lock(m_dataLock);
        
        if (idx >= 0 && idx < re2_times.size())
        {
            double re2_time = re2_times[idx];
            double pcre2_time = pcre2_times[idx];

            // re2_time 또는 pcre2_time이 0이면 아직 처리가 완료되지 않았으므로 반환
            if (re2_time == 0.0 || pcre2_time == 0.0)
            {
                return 0;
            }

            int thread_num = idx + 1; // 스레드 수가 1부터 시작

            // 싱글스레드 처리 시간
            double singleThreadRe2Time = re2_times[0];
            double singleThreadPcre2Time = pcre2_times[0];

            // 성능 향상 비율 계산
            double re2Improvement = ((singleThreadRe2Time - re2_time) / singleThreadRe2Time) * 100.0;
            double pcre2Improvement = ((singleThreadPcre2Time - pcre2_time) / singleThreadPcre2Time) * 100.0;

            // 리스트 컨트롤에 데이터 추가
            CString threadStr, re2Str, pcre2Str, re2ImpStr, pcre2ImpStr;
            threadStr.Format(_T("%d"), thread_num);
            re2Str.Format(_T("%.2f"), re2_time);
            pcre2Str.Format(_T("%.2f"), pcre2_time);
            re2ImpStr.Format(_T("%.2f"), re2Improvement);
            pcre2ImpStr.Format(_T("%.2f"), pcre2Improvement);

            int nItem = m_DataListCtrl.InsertItem(idx, threadStr);
            m_DataListCtrl.SetItemText(nItem, 1, re2Str);
            m_DataListCtrl.SetItemText(nItem, 2, pcre2Str);
            m_DataListCtrl.SetItemText(nItem, 3, re2ImpStr);
            m_DataListCtrl.SetItemText(nItem, 4, pcre2ImpStr);

            // 열 너비 자동 조정
            AutoSizeColumns();
        }
    }

    return 0;
}
