// SingleFileComparisonDlg.h

#pragma once
#include "pch.h"
#include "SpinRWLock.h"
#include "ThreadGuard.h"

#include "chartdir.h"
#include "ChartViewer.h"

// 사용자 정의 메시지
#define WM_UPDATE_TOTAL_TIME (WM_USER + 1)
#define WM_UPDATE_CHART (WM_USER + 100)

class CSingleFileComparisonDlg : public CDialogEx
{
    // 생성

public:
    CSingleFileComparisonDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
    virtual ~CSingleFileComparisonDlg();

    // 대화 상자 데이터
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SINGLE_FILE_COMPARISON_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
    DECLARE_MESSAGE_MAP()

private:
    // Chart 관련 멤버 변수
    CChartViewer m_ChartViewer;
    HICON m_hIcon;

    // 리스트 컨트롤 변수
    CListCtrl m_DataListCtrl;

    // Combo Box 컨트롤 변수
    CComboBox m_ComboRefresh;
    CComboBox m_ComboSecond;

    // 그래프 업데이트 전용 스레드
    std::thread m_chartUpdateThread;
    std::atomic<bool> m_chartThreadRunning;

    XYChart* m_chart;          // 재사용할 차트 객체
    LineLayer* m_re2Layer;    // RE2 데이터용 레이어
    LineLayer* m_pcre2Layer;  // PCRE2 데이터용 레이어

    // 총 처리 시간을 저장할 변수 (초 단위)
    double m_totalTimeRE2;
    double m_totalTimePCRE2;

    // 처리 플래그
    bool m_isProcessingRE2;
    bool m_isProcessingPCRE2;
    bool m_isProcessing;

    // 각 라이브러리 별 총 검출수
    int m_totalDetectionsRE2;
    int m_totalDetectionsPCRE2;

    // 패턴 별 검출수 (패턴의 수에 따라 동적으로 관리)
    std::vector<int> m_detectionsPerPatternRE2;
    std::vector<int> m_detectionsPerPatternPCRE2;

    // 각 라이브러리 별 총 치환 수
    int m_totalReplacementsRE2;
    int m_totalReplacementsPCRE2;

    // Mutex를 사용하여 스레드 간 데이터 보호
    //std::mutex m_dataMutex;

    SpinRWLock m_dataLock;

    double m_globalTime; // 전역 시간 카운터 (초 단위)
    std::chrono::steady_clock::time_point m_startTime; // 시작 시간
    

    // 스레드 관련 멤버 변수
    std::unique_ptr<ThreadGuard> m_re2ThreadGuard;
    std::unique_ptr<ThreadGuard> m_pcre2ThreadGuard;

    // 기타 멤버 변수
    double m_maxDisplayDuration;
    UINT m_refreshTime;

    std::deque<double> m_re2TimeStamps;
    std::deque<int> m_re2PerformanceData;

    std::deque<double> m_pcre2TimeStamps;
    std::deque<int> m_pcre2PerformanceData;

    // 시간 문자열을 저장할 덱
    std::deque<CString> m_re2TimeStampsStr;
    std::deque<CString> m_pcre2TimeStampsStr;

    bool m_shouldStop;
    CString m_filePath;
    CString m_fileSize;

    // 그래프 업데이트를 위한 타이머 ID
    enum { IDT_TIMER_UPDATE_CHART = 1001 };

    // 메시지 핸들러
    afx_msg LRESULT OnUpdateTotalTime(WPARAM wParam, LPARAM lParam);
    DECLARE_DYNAMIC(CSingleFileComparisonDlg)
    
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedButtonUploadFile();
    afx_msg void OnBnClickedButtonSaveResults();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnDestroy();
    afx_msg void OnCbnSelchangeComboRefresh();
    afx_msg void OnCbnSelchangeComboSecond();
private:
    void StartProcessing();
    void ProcessingThreadRE2();
    void ProcessingThreadPCRE2();
    void UpdateChart();
    bool OpenFileStream(std::ifstream& fileStream);
    void OnTimer(UINT_PTR nIDEvent);
public:
};
