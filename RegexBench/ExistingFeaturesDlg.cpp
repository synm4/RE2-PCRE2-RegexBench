// ExistingFeaturesDlg.cpp

#include "pch.h"
#include "RegexBench.h"
#include "RegexBenchDlg.h"
#include "afxdialogex.h"
#include "SingleFileComparisonDlg.h"
#include "MultithreadingComparisonDlg.h"
#include "ExistingFeaturesDlg.h"
#include "DataGenerator.h"
#include "FolderPathManager.h"

#define WM_UPDATE_LISTCTRL (WM_USER + 100)

struct ProcessResult
{
    int progress;        // 진행 상태
    int numLines;        // 라인 수
    double re2Duration;  // RE2 처리 시간
    double pcreDuration; // PCRE2 처리 시간
};

// CExistingFeaturesDlg 대화 상자

IMPLEMENT_DYNAMIC(CExistingFeaturesDlg, CDialogEx)

CExistingFeaturesDlg::CExistingFeaturesDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_EXISTING_FEATURES_DLG, pParent)
    , m_numLines(1000) // 기본값 설정
    , m_progressCtrl() // 프로그레스 바 초기화
{
}

CExistingFeaturesDlg::~CExistingFeaturesDlg()
{
}

void CExistingFeaturesDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PROGRESS1, m_progressCtrl); // 프로그레스 바 연결
    DDX_Text(pDX, IDC_EDIT_NUM_LINES, m_numLines); // 입력 칸 연결
    DDV_MinMaxInt(pDX, m_numLines, 1, 100000); // 값 검증
    DDX_Control(pDX, IDC_STATIC_GRAPH_EX, ex_staticGraph); // CAnimateCtrl으로 연결
    DDX_Control(pDX, IDC_LISTCTRL_DATA2, m_listCtrlData); // 리스트 컨트롤 연결
    DDX_Control(pDX, IDC_STATIC_PATH, m_staticFolderPath); // 폴더 경로 연결

    // 디버그용 로그 추가
    //TRACE(_T("DoDataExchange: ex_staticGraph HWND = %p\n"), ex_staticGraph.GetSafeHwnd());
}

BEGIN_MESSAGE_MAP(CExistingFeaturesDlg, CDialogEx)
    ON_WM_PAINT()
    ON_BN_CLICKED(IDC_BUTTON_PROCESS_FILE, &CExistingFeaturesDlg::OnBnClickedButtonProcessFile)
    ON_EN_CHANGE(IDC_EDIT_NUM_LINES, &CExistingFeaturesDlg::OnEnChangeEditNumLines)
    ON_BN_CLICKED(IDC_BUTTON_SAVERESULTEX, &CExistingFeaturesDlg::OnBnClickedButtonSaveresultex)
END_MESSAGE_MAP()

BOOL CExistingFeaturesDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    // GDI+ 초기화
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
    InitializeGraph();
    
    // 리스트 컨트롤 설정
    m_listCtrlData.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    // 컬럼 추가
    m_listCtrlData.InsertColumn(0, _T("라인 수"), LVCFMT_LEFT, 50);
    m_listCtrlData.InsertColumn(1, _T("RE2 처리 시간 (ms)"), LVCFMT_CENTER, 110);
    m_listCtrlData.InsertColumn(2, _T("PCRE2 처리 시간 (ms)"), LVCFMT_CENTER, 120);
    m_staticFolderPath.SetWindowTextW(L"폴더 경로 : ");

    // PatternManager 초기화
    if (PatternManager::GetInstance().GetPatterns().empty())
    {
        PatternManager::GetInstance().SetDefaultPatterns();
    }

    if (!Utils::InitializePatterns(PatternManager::GetInstance().GetPatterns()))
    {
        AfxMessageBox(_T("패턴 초기화에 실패했습니다."));
    }

    return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}


void CExistingFeaturesDlg::InitializeGraph()
{
    // 그래프 영역 설정
    ex_staticGraph.GetWindowRect(&graph_rect);
    ScreenToClient(&graph_rect);
}


void CExistingFeaturesDlg::OnPaint()
{
    CPaintDC dc(this); // 그리기를 위한 DC

    // 그래프 그리기
    DrawPerformanceGraph(&dc, graph_rect, lineCounts, re2Durations, pcreDurations);

    CDialogEx::OnPaint();
}

void CExistingFeaturesDlg::OnBnClickedButtonProcessFile()
{
    // 컨트롤의 데이터를 멤버 변수로 가져옴
    if (!UpdateData(TRUE))
    {
        AfxMessageBox(_T("입력한 값이 유효하지 않습니다."));
        return;
    }

    // 리스트 컨트롤 초기화
    m_listCtrlData.DeleteAllItems();

    // 최대 줄 수가 100 이상인지 확인
    if (m_numLines < 100)
    {
        AfxMessageBox(_T("최대 줄 수는 100 이상이어야 합니다."));
        return;
    }

    // 최대 줄 수와 증가량 계산
    int maxLines = m_numLines;
    int step = maxLines / 100;
    if (step < 1) step = 1;
    int loopCount = maxLines / step + (maxLines % step != 0 ? 1 : 0);

    // 진행 상태 초기화
    m_progressCtrl.SetRange(0, loopCount);
    m_progressCtrl.SetPos(0);

    // 패턴 초기화 또는 로드
    if (PatternManager::GetInstance().GetPatterns().empty())
    {
        PatternManager::GetInstance().SetDefaultPatterns();
        TRACE(_T("ExistingFeaturesDlg: PatternManager 기본 패턴으로 설정됨.\n"));
    }

    // 형식 리스트 로드
    DataGenerator generator;

    // 실행 파일의 디렉토리 경로 가져오기
    CString loadFilePath = _T("format_list.txt"); // 절대 경로로 변경

    if (!generator.LoadFormatList(loadFilePath))
    {
        generator.SetDefaultFormatList();
    }

    // FolderPathManager에서 생성 데이터 폴더 경로 확인 및 설정
    CString selectedFolderPath;
    if (FolderPathManager::GetInstance().IsGeneratedDataFolderPathSet())
    {
        selectedFolderPath = FolderPathManager::GetInstance().GetGeneratedDataFolderPath();
    }
    else
    {
        CFolderPickerDialog folderDlg(NULL, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, this);
        folderDlg.m_ofn.lpstrTitle = _T("생성 데이터를 저장할 폴더를 선택하세요");
        if (folderDlg.DoModal() == IDOK)
        {
            selectedFolderPath = folderDlg.GetPathName();

            if (::PathFileExists(selectedFolderPath))
            {
                FolderPathManager::GetInstance().SetGeneratedDataFolderPath(selectedFolderPath);
            }
            else
            {
                AfxMessageBox(_T("선택한 폴더 경로가 존재하지 않습니다. 다시 선택해주세요."));
                return;
            }
        }
        else
        {
            AfxMessageBox(_T("생성 데이터를 저장할 폴더를 선택하지 않았습니다."));
            return;
        }
    }
    // 싱글톤에 저장된 결과 경로가 있는지 확인
    if (!FolderPathManager::GetInstance().IsResultFolderPathSet())
    {
        CFolderPickerDialog folderDlg(NULL, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, this);
        folderDlg.m_ofn.lpstrTitle = _T("결과를 저장할 폴더를 선택하세요");
        if (folderDlg.DoModal() == IDOK)
        {
            selectedFolderPath = folderDlg.GetPathName();
            if (::PathFileExists(selectedFolderPath))
            {
                FolderPathManager::GetInstance().SetResultFolderPath(selectedFolderPath);
            }
            else
            {
                AfxMessageBox(_T("선택한 폴더 경로가 존재하지 않습니다. 다시 선택해주세요."));
                return;
            }
        }
        else
        {
            AfxMessageBox(_T("생성 데이터를 저장할 폴더를 선택하지 않았습니다."));
            return;
        }
    }

    // 데이터 생성 루프
    std::vector<int> tempLineCounts;
    std::vector<double> tempRe2Durations;
    std::vector<double> tempPcreDurations;
    int progress = 0;

    // 패턴 가져오기
    const auto& patterns = PatternManager::GetInstance().GetPatterns();
    if (patterns.empty())
    {
        AfxMessageBox(_T("패턴이 초기화되지 않았습니다."));
        return;
    }
    // 패턴 초기화 (중복 제거 및 초기화 호출)
    if (!Utils::InitializePatterns(patterns))
    {
        AfxMessageBox(_T("패턴 초기화에 실패했습니다."));
        return;
    }

    for (int numLines = 0; numLines <= maxLines; numLines += step)
    {
        // 파일 경로 설정
        CString filePath;
        filePath.Format(_T("%s\\test_data_%d.txt"), selectedFolderPath, numLines);

        try
        {
            generator.GenerateTestData(filePath, numLines);
        }
        catch (const std::exception& ex)
        {
            CString errorMsg = CA2W(ex.what(), CP_UTF8);
            AfxMessageBox(errorMsg);
            return;
        }

        // RE2와 PCRE 실행 시간 측정
        double re2Duration = Utils::ProcessFileWithRE2(filePath, numLines);
        std::string processedText;
        double pcreDuration = Utils::ProcessFileWithPCRE2(filePath, patterns, processedText).duration_ms;

        // 결과 저장
        tempLineCounts.push_back(numLines);
        tempRe2Durations.push_back(re2Duration);
        tempPcreDurations.push_back(pcreDuration);

        // UI 업데이트
        CString strLineCount, strRe2Duration, strPcreDuration;
        strLineCount.Format(_T("%d"), numLines);
        strRe2Duration.Format(_T("%.4f"), re2Duration);
        strPcreDuration.Format(_T("%.4f"), pcreDuration);

        int nItem = m_listCtrlData.InsertItem(progress, strLineCount);
        m_listCtrlData.SetItemText(nItem, 1, strRe2Duration);
        m_listCtrlData.SetItemText(nItem, 2, strPcreDuration);

        // 새로 추가된 아이템이 보이도록 설정
        m_listCtrlData.EnsureVisible(nItem, FALSE);

        // 진행 상태 업데이트
        progress++;
        m_progressCtrl.SetPos(progress);

        PeekAndPump();
    }

    // 그래프 데이터 업데이트 및 갱신
    lineCounts = tempLineCounts;
    re2Durations = tempRe2Durations;
    pcreDurations = tempPcreDurations;
    Invalidate(); // 그래프 영역을 다시 그리도록 요청
}


void CExistingFeaturesDlg::OnEnChangeEditNumLines()
{
}

void CExistingFeaturesDlg::DrawPerformanceGraph(CDC* pDC, const CRect& graphRect,
    const std::vector<int>& lineCounts,
    const std::vector<double>& re2Durations,
    const std::vector<double>& pcreDurations)
{
    if (lineCounts.empty() || re2Durations.empty() || pcreDurations.empty())
        return;

    // GDI+ Graphics 객체 생성
    Graphics graphics(pDC->GetSafeHdc());
    graphics.SetSmoothingMode(SmoothingModeNone); // 안티앨리어싱 비활성화

    // 전체 배경을 흰색으로 채우기
    SolidBrush overallBackgroundBrush(Color(255, 255, 255, 255));
    graphics.FillRectangle(&overallBackgroundBrush, graphRect.left, graphRect.top, graphRect.Width(), graphRect.Height());

    // 그래프 영역 설정
    int margin = 50;
    CRect plotArea = graphRect;
    plotArea.DeflateRect(margin, margin, margin, margin);

    // 그리드 펜 설정
    Pen gridPen(Color(200, 200, 200), 1); // 연한 회색 얇은 선

    // 최대값 계산
    double maxDuration = max(
        *std::max_element(re2Durations.begin(), re2Durations.end()),
        *std::max_element(pcreDurations.begin(), pcreDurations.end())
    );
    maxDuration = Utils::NiceNumber(maxDuration * 1.2, true);

    // Y축 그리기 및 눈금 표시
    Pen axisPen(Color(0, 0, 0), 2);
    graphics.DrawLine(&axisPen, plotArea.left, plotArea.top, plotArea.left, plotArea.bottom);
    graphics.DrawLine(&axisPen, plotArea.left, plotArea.bottom, plotArea.right, plotArea.bottom);

    // Y축 레이블과 눈금 설정
    int numYTicks = 5;
    double yTickInterval = Utils::NiceNumber(maxDuration / numYTicks, true);
    Gdiplus::Font yFont(L"Arial", 10);
    SolidBrush textBrush(Color(0, 0, 0));

    for (int i = 0; i <= numYTicks; ++i)
    {
        double yValue = yTickInterval * i;
        float yPos = static_cast<float>(plotArea.bottom - (yValue / maxDuration) * plotArea.Height());
        // Y 축 눈금 그리기
        graphics.DrawLine(&gridPen, plotArea.left, (int)yPos, plotArea.right, (int)yPos);

        // 회색 실선 그리기
        graphics.DrawLine(&axisPen, plotArea.left - 5, (int)yPos, plotArea.left, (int)yPos);

        // Y 축 레이블 그리기
        CString yText;
        yText.Format(_T("%.0f"), yValue);
        graphics.DrawString(yText, -1, &yFont, PointF(plotArea.left - 40, yPos - 10), &textBrush);
    }

    // 막대 그래프 그리기 (간격 없이 꽉 차도록 조정)
    int numberOfBars = static_cast<int>(lineCounts.size() * 2);
    float barWidth = static_cast<float>(plotArea.Width()) / static_cast<float>(numberOfBars);

    for (size_t i = 0; i < lineCounts.size(); ++i)
    {
        // RE2 막대
        float re2Left = plotArea.left + i * 2 * barWidth;
        float re2Height = static_cast<float>((re2Durations[i] / maxDuration) * plotArea.Height());
        RectF re2Rect(re2Left, plotArea.top + plotArea.Height() - re2Height, barWidth, re2Height);
        SolidBrush re2Brush(Color(255, 139, 195, 74));
        graphics.FillRectangle(&re2Brush, re2Rect);

        // PCRE2 막대
        float pcre2Left = re2Left + barWidth;
        float pcre2Height = static_cast<float>((pcreDurations[i] / maxDuration) * plotArea.Height());
        RectF pcre2Rect(pcre2Left, plotArea.top + plotArea.Height() - pcre2Height, barWidth, pcre2Height);
        SolidBrush pcre2Brush(Color(255, 124, 181, 223));
        graphics.FillRectangle(&pcre2Brush, pcre2Rect);
    }

    // X축 레이블 그리기
    Gdiplus::Font xFont(L"Arial", 15, FontStyleRegular, UnitPixel);
    int numXTicks = 6;
    double xTickInterval = static_cast<double>(lineCounts.size()) / (numXTicks - 1);

    for (int i = 0; i < numXTicks; ++i)
    {
        int index = static_cast<int>(i * xTickInterval);
        if (index >= lineCounts.size()) index = static_cast<int>(lineCounts.size()) - 1;

        CString xText;
        xText.Format(_T("%d"), lineCounts[index]);
        float xPos = plotArea.left + index * 2 * barWidth + barWidth / 2 - 10; // 레이블 위치 조정
        graphics.DrawString(xText, -1, &xFont, PointF(xPos, plotArea.bottom + 10), &textBrush);
    }

    // X축 이름
    CString xAxisLabel = _T("라인 수");
    graphics.DrawString(xAxisLabel, -1, &xFont, PointF(plotArea.right - 30, plotArea.bottom + 30), &textBrush);

    // Y축 이름
    CString yAxisLabel = _T("처리 시간 (ms)");
    graphics.DrawString(yAxisLabel, -1, &yFont, PointF(plotArea.left - 40, plotArea.top - 30), &textBrush);

    // 범례 추가 (왼쪽 끝으로 이동)
    SolidBrush re2LegendBrush(Color(255, 139, 195, 74));
    SolidBrush pcre2LegendBrush(Color(255, 124, 181, 223));

    float legendX = plotArea.left + 10.0f; // 패딩을 고려한 적절한 값으로 조정
    RectF re2LegendRect(legendX, plotArea.top + 10, 20, 20);
    RectF pcre2LegendRect(legendX, plotArea.top + 40, 20, 20);
    graphics.FillRectangle(&re2LegendBrush, re2LegendRect);
    graphics.FillRectangle(&pcre2LegendBrush, pcre2LegendRect);

    Gdiplus::Font legendFont(L"Arial", 10);
    graphics.DrawString(L"RE2", -1, &legendFont, PointF(legendX + 30, plotArea.top + 10), &textBrush);
    graphics.DrawString(L"PCRE2", -1, &legendFont, PointF(legendX + 30, plotArea.top + 40), &textBrush);
}

void CExistingFeaturesDlg::OnBnClickedButtonSaveresultex()
{
    // 데이터가 있는지 확인합니다.
    if (lineCounts.empty() || re2Durations.empty() || pcreDurations.empty())
    {
        AfxMessageBox(_T("저장할 데이터가 없습니다. 먼저 데이터를 생성하세요."));
        return;
    }

    // FolderPathManager에서 생성 데이터 폴더 경로 확인 및 설정
    CString selectedFolderPath;
    if (FolderPathManager::GetInstance().IsGeneratedDataFolderPathSet())
    {
        selectedFolderPath = FolderPathManager::GetInstance().GetGeneratedDataFolderPath();
    }
    else
    {
        // 사용자에게 폴더 선택 대화상자를 표시합니다.
        CFolderPickerDialog folderDlg(NULL, 0, this);
        if (folderDlg.DoModal() == IDOK)
        {
            selectedFolderPath = folderDlg.GetPathName();
            if (::PathFileExists(selectedFolderPath))
            {
                FolderPathManager::GetInstance().SetGeneratedDataFolderPath(selectedFolderPath);
            }
            else
            {
                AfxMessageBox(_T("선택한 폴더 경로가 존재하지 않습니다. 다시 선택해주세요."));
                return;
            }
        }
        else
        {
            AfxMessageBox(_T("생성 데이터를 저장할 폴더를 선택하지 않았습니다."));
            return;
        }
    }

    // 저장할 파일 경로 설정 (선택된 폴더에 "result.txt" 파일을 생성)
    CString filePath = selectedFolderPath + _T("\\result.txt");

    // 파일 열기
    std::ofstream outFile(CT2A(filePath), std::ios::out | std::ios::trunc);
    if (!outFile.is_open())
    {
        AfxMessageBox(_T("파일을 열 수 없습니다."));
        return;
    }

    // 데이터 쓰기
    std::vector<double> ratios; // 각 라인 수별 처리 시간 비율(RE2 / PCRE2 또는 그 반대)

    size_t dataCount = lineCounts.size();

    outFile << std::fixed << std::setprecision(2); // 소수점 두 자리까지 표시
    outFile << "----------------------------------" << std::endl;
    for (size_t i = 0; i < dataCount; ++i)
    {
        CString ratioStr;
        ratioStr.Format(_T("라인 수: %d"), lineCounts[i]);
        outFile << CT2A(ratioStr) << std::endl;
        ratioStr.Format(_T("RE2 처리 시간: %.2f"), re2Durations[i]);
        outFile << CT2A(ratioStr) << std::endl;
        ratioStr.Format(_T("PCRE2 처리 시간: %.2f"), pcreDurations[i]);
        outFile << CT2A(ratioStr) << std::endl;

        // 각 라인 수별 처리 시간 비율 계산
        double ratio = 0.0;
        

        if (re2Durations[i] > 0 && pcreDurations[i] > 0)
        {
            if (re2Durations[i] < pcreDurations[i])
            {
                ratio = pcreDurations[i] / re2Durations[i];
                ratioStr.Format(_T("RE2가 PCRE2보다 %.2f배 빠름"), ratio);
            }
            else if (pcreDurations[i] < re2Durations[i])
            {
                ratio = re2Durations[i] / pcreDurations[i];
                ratioStr.Format(_T("PCRE2가 RE2보다 %.2f배 빠름"), ratio);
            }
            else
            {
                ratio = 1.0;
                ratioStr = _T("RE2와 PCRE2의 처리 시간이 동일함");
            }
            ratios.push_back(ratio);
        }
        else
        {
            ratioStr = _T("처리 시간 중 하나가 0이어서 비율을 계산할 수 없음");
        }

        outFile << CT2A(ratioStr) << std::endl;
        outFile << "----------------------------------" << std::endl;
    }

    // 비율들의 평균 계산
    double averageRatio = 0.0;
    if (!ratios.empty())
    {
        double sumRatios = std::accumulate(ratios.begin(), ratios.end(), 0.0);
        averageRatio = sumRatios / ratios.size();
    }

    // 어느 라이브러리가 더 빠른지 판단
    CString summary;

    if (!ratios.empty())
    {
        if (averageRatio > 1.0)
        {
            summary.Format(_T("PCRE2 라이브러리가 RE2 라이브러리보다 평균적으로 %.2f배 더 빠릅니다."), averageRatio);
        }
        else if (averageRatio < 1.0)
        {
            summary.Format(_T("RE2 라이브러리가 PCRE2 라이브러리보다 평균적으로 %.2f배 더 빠릅니다."), 1.0 / averageRatio);
        }
        else
        {
            summary = _T("두 라이브러리의 평균 처리 시간이 동일합니다.");
        }
    }
    else
    {
        summary = _T("평균 처리 시간 비율을 계산할 수 없습니다.");
    }

    // 요약을 파일에 씁니다.
    outFile << std::endl;
    outFile << "==================================" << std::endl;
    CString avgRatioStr;
    avgRatioStr.Format(_T("라인 수별 처리 시간 비율들의 평균: %.2f"), averageRatio);
    outFile << CT2A(avgRatioStr) << std::endl;
    outFile << CT2A(summary) << std::endl;
    outFile << "==================================" << std::endl;

    outFile.close();

    AfxMessageBox(_T("결과가 저장되었습니다."));
}

void CExistingFeaturesDlg::PeekAndPump()
{
    MSG msg;
    while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (!AfxGetApp()->PumpMessage())
        {
            ::PostQuitMessage(0);
            return;
        }
    }
    // CPU 사용량을 낮추기 위해 잠시 대기
    Sleep(1);
}