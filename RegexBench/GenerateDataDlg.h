#pragma once
#include "DataGenerator.h"
#include "afxdialogex.h"

// GenerateDataDlg 대화 상자

class GenerateDataDlg : public CDialogEx
{
    DECLARE_DYNAMIC(GenerateDataDlg)

public:
    GenerateDataDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
    virtual ~GenerateDataDlg();

    // 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_GENRATEDATA_SETTINGS_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

    DECLARE_MESSAGE_MAP()

public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedButtonAddData();
    afx_msg void OnBnClickedButtonDeleteData();
    afx_msg void OnBnClickedButtonGenerateTestData();
    afx_msg void OnBnClickedButtonApply();
    
    virtual void OnOK(); // OnOK 오버라이드


private:
    int m_numLines; // 예: 라인 수
    bool m_bDataModified;
    CToolTipCtrl m_ToolTip; // 툴팁 컨트롤 추가
    DataGenerator m_dataGenerator; // 데이터 생성기 인스턴스
};
