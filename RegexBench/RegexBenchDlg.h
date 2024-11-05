// RegexBenchDlg.h

#pragma once

#include "SingleFileComparisonDlg.h"
#include "MultithreadingComparisonDlg.h"
#include "ExistingFeaturesDlg.h"
#include "PatternDefinitionDlg.h"
#include "Pattern.h"
#include "PatternManager.h"
#include "DataGenerator.h"
using namespace Gdiplus;

// 커스텀 메시지 정의
#define WM_USER_RE2_DONE (WM_USER + 1)
#define WM_USER_PCRE_DONE (WM_USER + 2)

// CRegexBenchDlg 대화 상자
class CRegexBenchDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CRegexBenchDlg)

public:
    CRegexBenchDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
    virtual ~CRegexBenchDlg();

    void OnDestroy();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_REGEXBENCH_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

    CTabCtrl m_TabCtrl;

    // 탭 페이지 다이얼로그 포인터
    CSingleFileComparisonDlg* m_pSingleFileComparisonDlg;
    CMultithreadingComparisonDlg* m_pMultithreadingComparisonDlg;
    CExistingFeaturesDlg* m_pExistingFeaturesDlg; // 기존 기능을 관리할 다이얼로그 포인터

    void ShowTabPage(int nPage);

protected:
    HICON m_hIcon;

    // 생성된 메시지 맵 함수
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBnClickedButtonDefinePatterns();
    DECLARE_MESSAGE_MAP()

private:
    ULONG_PTR m_gdiplusToken; // GDI+ 토큰

    DataGenerator m_dataGenerator;

public:

    // 그래프 데이터 저장
    std::vector<int> lineCounts;
    std::vector<double> re2Durations;
    std::vector<double> pcreDurations;

    // Edit Control 변경 이벤트 핸들러
    afx_msg void OnTcnSelchangeTabControl(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBnClickedButtonGeneratedataSettings();
    afx_msg void OnBnClickedButtonPathmanager();
};
