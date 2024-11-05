// MultithreadingComparisonDlg.h

#pragma once
#include "pch.h"
#include "SpinRWLock.h"

using namespace Gdiplus;

// 필요한 라이브러리 초기화
#pragma comment(lib, "gdiplus.lib")

class CMultithreadingComparisonDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CMultithreadingComparisonDlg)

public:
    CMultithreadingComparisonDlg(CWnd* pParent = nullptr);
    virtual ~CMultithreadingComparisonDlg();

    // 대화 상자 데이터
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MULTITHREADING_COMPARISON_DLG };
#endif

protected:
    HICON m_hIcon;

    // GDI+ 초기화
    ULONG_PTR m_gdiplusToken;

    // 그래프 데이터
    std::vector<double> re2_times;
    std::vector<double> pcre2_times;
    int max_threads;

    // 그래프 위치 및 크기
    CRect graph_rect;

    // UI 컨트롤 변수
    CEdit m_editThreadCount;
    CButton m_buttonUpload;
    CButton m_buttonStart;
    CButton m_buttonResult;
    CStatic m_staticGraph;
    CProgressCtrl m_progressBar; // 프로그래스 바 컨트롤

    // 파일 경로
    CString m_filePath;

    // 생성된 메시지 맵 함수
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBnClickedButtonStart();
    afx_msg void OnBnClickedButtonThreadCheck();
    afx_msg void OnAcnStartStaticGraphMt();
    afx_msg void InitializeListCtrl();
    afx_msg LRESULT OnUpdateProgress(WPARAM wParam, LPARAM lParam); // 커스텀 메시지 핸들러
    afx_msg LRESULT OnTaskComplete(WPARAM wParam, LPARAM lParam);    // 작업 완료
    afx_msg LRESULT OnUpdateList(WPARAM wParam, LPARAM lParam);
    void MergeOutputFiles(const CString& outputFileName, int numThreads, const CString& prefix);
    DECLARE_MESSAGE_MAP()

    CListCtrl m_DataListCtrl;
private:
    void InitializeGraph();
    void DrawGraph(CDC* pDC);
    void AutoSizeColumns();
    void UpdateFileSizeDisplay(const CString& filePath);
    CStatic m_staticFileSize;
    CStatic m_staticFilePath;

    SpinRWLock m_dataLock;

    std::future<void> m_processingTask;
public:
    afx_msg void OnBnClickedButtonResultMt();

    
};
