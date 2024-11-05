// PatternDefinitionDlg.h
#pragma once
#include <vector>
#include "Pattern.h"
#include "PatternManager.h"



class PatternDefinitionDlg : public CDialogEx
{
    DECLARE_DYNAMIC(PatternDefinitionDlg)

public:
    PatternDefinitionDlg(CWnd* pParent = nullptr);
    virtual ~PatternDefinitionDlg();

    // 대화 상자 데이터
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_PATTERN_DEFINITION_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

    DECLARE_MESSAGE_MAP()

private:
    BOOL OnInitDialog();
    // 체크박스 상태
    BOOL m_bPattern1;
    BOOL m_bPattern2;
    BOOL m_bPattern3;
    BOOL m_bPattern4;
    BOOL m_bPattern5;
    BOOL m_bPattern6;
    BOOL m_bPattern7;
    BOOL m_bPattern8;

    // 변경 사항 추적 변수
    bool m_bIsModified = false; 

    // 커스텀 패턴
    CString m_customPattern;
    CString m_customReplacement;

    // 리스트 박스 컨트롤
    CListBox m_listCustomPatterns;

    // 초기 패턴
    std::vector<PatternReplacement> m_initialPatterns;

public:
    // 패턴 설정 및 가져오기
    void SetPatterns(const std::vector<PatternReplacement>& patterns);
    void GetSelectedPatterns();

    // 메시지 핸들러
    afx_msg void OnBnClickedButtonAddPattern();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCheckall();
    afx_msg void OnBnClickedButtonDeletePattern();
    
};
