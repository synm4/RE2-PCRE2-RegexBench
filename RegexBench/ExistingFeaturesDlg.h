// ExistingFeaturesDlg.h

#pragma once
#include "afxdialogex.h"
#include "Pattern.h"
#include "Utils.h"

#pragma comment (lib,"Gdiplus.lib")
using namespace Gdiplus;

class CExistingFeaturesDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CExistingFeaturesDlg)

public:
    CExistingFeaturesDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
    virtual ~CExistingFeaturesDlg();

    BOOL OnInitDialog();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_EXISTING_FEATURES_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

    DECLARE_MESSAGE_MAP()

private:
    // 컨트롤 변수
    int m_numLines; // IDC_EDIT_NUM_LINES
    CProgressCtrl m_progressCtrl; // IDC_PROGRESS1

    // 리스트 변수
    CListCtrl m_listCtrlData;

    // GDI+ 초기화
    ULONG_PTR m_gdiplusToken;

    // 그래프 데이터 저장
    std::vector<int> lineCounts;
    std::vector<double> re2Durations;
    std::vector<double> pcreDurations;

    // 폴더 경로 저장
    CStatic m_staticFolderPath;

    CRect graph_rect;
    CStatic ex_staticGraph;
public:
    // 메시지 핸들러
    afx_msg void OnBnClickedButtonProcessFile();
    afx_msg void OnEnChangeEditNumLines();
    afx_msg void OnPaint();
    void InitializeGraph();
    void PeekAndPump();
    void DrawPerformanceGraph(CDC* pDC, const CRect& graphRect,
        const std::vector<int>& lineCounts,
        const std::vector<double>& re2Durations,
        const std::vector<double>& pcreDurations);
    afx_msg void OnBnClickedButtonSaveresultex();
};
