// PatternDefinitionDlg.cpp
#include "pch.h"
#include "RegexBench.h"
#include "PatternDefinitionDlg.h"
#include "afxdialogex.h"
#include "PatternManager.h"

IMPLEMENT_DYNAMIC(PatternDefinitionDlg, CDialogEx)

PatternDefinitionDlg::PatternDefinitionDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_PATTERN_DEFINITION_DIALOG, pParent)
    , m_bPattern1(TRUE)
    , m_bPattern2(TRUE)
    , m_bPattern3(TRUE)
    , m_bPattern4(TRUE)
    , m_bPattern5(TRUE)
    , m_bPattern6(TRUE)
    , m_bPattern7(TRUE)
    , m_bPattern8(TRUE)
    , m_bIsModified(false)
{
}

PatternDefinitionDlg::~PatternDefinitionDlg()
{
}

void PatternDefinitionDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);

    // 먼저 컨트롤을 매핑합니다.
    DDX_Control(pDX, IDC_LIST_CUSTOM_PATTERNS, m_listCustomPatterns);

    // 그 후에 다른 매핑을 수행합니다.
    DDX_Check(pDX, IDC_CHECK_PATTERN1, m_bPattern1);
    DDX_Check(pDX, IDC_CHECK_PATTERN2, m_bPattern2);
    DDX_Check(pDX, IDC_CHECK_PATTERN3, m_bPattern3);
    DDX_Check(pDX, IDC_CHECK_PATTERN4, m_bPattern4);
    DDX_Check(pDX, IDC_CHECK_PATTERN5, m_bPattern5);
    DDX_Check(pDX, IDC_CHECK_PATTERN6, m_bPattern6);
    DDX_Check(pDX, IDC_CHECK_PATTERN7, m_bPattern7);
    DDX_Check(pDX, IDC_CHECK_PATTERN8, m_bPattern8);
    DDX_Text(pDX, IDC_EDIT_CUSTOM_PATTERN, m_customPattern);
    DDX_Text(pDX, IDC_EDIT_CUSTOM_REPLACEMENT, m_customReplacement);
}

BEGIN_MESSAGE_MAP(PatternDefinitionDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_ADD_PATTERN, &PatternDefinitionDlg::OnBnClickedButtonAddPattern)
    ON_BN_CLICKED(IDOK, &PatternDefinitionDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_CHECKALL, &PatternDefinitionDlg::OnBnClickedCheckall)
    ON_BN_CLICKED(IDC_BUTTON_DELETE_PATTERN, &PatternDefinitionDlg::OnBnClickedButtonDeletePattern)
END_MESSAGE_MAP()

BOOL PatternDefinitionDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_bIsModified = false;
    // 리스트 박스 유효성 검사
    if (!::IsWindow(m_listCustomPatterns.GetSafeHwnd()))
    {
        AfxMessageBox(_T("리스트 박스 컨트롤이 유효하지 않습니다."));
    }
    else
    {
        // AfxMessageBox(_T("리스트 박스 컨트롤이 유효합니다."));
    }

    // 체크박스 초기화 (기본적으로 모두 체크 해제)
    m_bPattern1 = FALSE;
    m_bPattern2 = FALSE;
    m_bPattern3 = FALSE;
    m_bPattern4 = FALSE;
    m_bPattern5 = FALSE;
    m_bPattern6 = FALSE;
    m_bPattern7 = FALSE;
    m_bPattern8 = FALSE;

    // "전체 선택" 체크박스도 체크된 상태로 설정
    CheckDlgButton(IDC_CHECKALL, BST_CHECKED);


    // 리스트 박스 초기화
    m_listCustomPatterns.ResetContent();

    // 패턴 분석 및 체크박스/리스트 박스 설정
    for (const auto& pat : m_initialPatterns)
    {
        if (pat.pattern == _T("\\b\\d{6}-\\d{7}\\b") && pat.replacement == _T("xxxxxx-xxxxxxx"))
            m_bPattern1 = TRUE;
        else if (pat.pattern == _T("\\b010-\\d{4}-\\d{4}\\b") && pat.replacement == _T("010-xxxx-xxxx"))
            m_bPattern2 = TRUE;
        else if (pat.pattern == _T("\\b\\d{4}-\\d{4}-\\d{4}-\\d{4}\\b") && pat.replacement == _T("xxxx-xxxx-xxxx-xxxx"))
            m_bPattern3 = TRUE;
        else if (pat.pattern == _T("[\\w.-]+@[\\w.-]+\\.[a-zA-Z]{2,6}") && pat.replacement == _T("email@example.com"))
            m_bPattern4 = TRUE;
        else if (pat.pattern == _T("주소=[^\\r\\n]+") && pat.replacement == _T("주소=REDACTED"))
            m_bPattern5 = TRUE;
        else if (pat.pattern == _T("\\b\\d{1,3}(?:\\.\\d{1,3}){3}\\b") && pat.replacement == _T("xxx.xxx.xxx.xxx"))
            m_bPattern6 = TRUE;
        else if (pat.pattern == _T("이름=홍길동\\d+") && pat.replacement == _T("이름=홍길동"))
            m_bPattern7 = TRUE;
        else if (pat.pattern == _T("금액=₩\\d+,000") && pat.replacement == _T("금액=₩***,000"))
            m_bPattern8 = TRUE;
        else
        {
            CString displayStr;
            displayStr.Format(_T("%s -> %s"), pat.pattern, pat.replacement);
            m_listCustomPatterns.AddString(displayStr);
        }
    }

    UpdateData(FALSE); // UI 업데이트

    return TRUE;  // 포커스 설정을 안 하면 TRUE 반환
}

void PatternDefinitionDlg::SetPatterns(const std::vector<PatternReplacement>& patterns)
{
    // 초기 패턴 저장
    m_initialPatterns = patterns;
}

void PatternDefinitionDlg::GetSelectedPatterns()
{
    std::vector<PatternReplacement> selectedPatterns;

    // 리스트 박스가 유효한지 확인
    if (!::IsWindow(m_listCustomPatterns.GetSafeHwnd()))
    {
        AfxMessageBox(_T("리스트 박스 컨트롤이 유효하지 않습니다."));
        return;
    }

    int count = m_listCustomPatterns.GetCount();

    // 체크박스에서 선택된 패턴 추가
    if (m_bPattern1)
        selectedPatterns.emplace_back(PatternReplacement(_T("\\b\\d{6}-\\d{7}\\b"), _T("xxxxxx-xxxxxxx")));
    if (m_bPattern2)
        selectedPatterns.emplace_back(PatternReplacement(_T("\\b010-\\d{4}-\\d{4}\\b"), _T("010-xxxx-xxxx")));
    if (m_bPattern3)
        selectedPatterns.emplace_back(PatternReplacement(_T("\\b\\d{4}-\\d{4}-\\d{4}-\\d{4}\\b"), _T("xxxx-xxxx-xxxx-xxxx")));
    if (m_bPattern4)
        selectedPatterns.emplace_back(PatternReplacement(_T("[\\w.-]+@[\\w.-]+\\.[a-zA-Z]{2,6}"), _T("email@example.com")));
    if (m_bPattern5)
        selectedPatterns.emplace_back(PatternReplacement(_T("주소=[^\\r\\n]+"), _T("주소=REDACTED")));
    if (m_bPattern6)
        selectedPatterns.emplace_back(PatternReplacement(_T("\\b\\d{1,3}(?:\\.\\d{1,3}){3}\\b"), _T("xxx.xxx.xxx.xxx")));
    if (m_bPattern7)
        selectedPatterns.emplace_back(PatternReplacement(_T("이름=홍길동\\d+"), _T("이름=홍길동")));
    if (m_bPattern8)
        selectedPatterns.emplace_back(PatternReplacement(_T("금액=₩\\d+,000"), _T("금액=₩***,000")));

    // 리스트 박스에서 커스텀 패턴 추가
    for (int i = 0; i < count; ++i)
    {
        CString displayStr;
        m_listCustomPatterns.GetText(i, displayStr);

        int arrowPos = displayStr.Find(_T(" -> "));
        if (arrowPos != -1)
        {
            CString patternStr = displayStr.Left(arrowPos);
            CString replacementStr = displayStr.Mid(arrowPos + 4);
            selectedPatterns.emplace_back(PatternReplacement(patternStr, replacementStr));
        }
        else
        {
            selectedPatterns.emplace_back(PatternReplacement(displayStr, _T("REDACTED")));
        }
    }
    
    // PatternManager 싱글톤에 패턴 저장
    PatternManager::GetInstance().SetPatterns(selectedPatterns);

    return;
}

void PatternDefinitionDlg::OnBnClickedButtonAddPattern()
{
    UpdateData(TRUE); // UI 데이터를 멤버 변수로 가져옴
    
    if (!m_customPattern.IsEmpty())
    {
        if (m_customReplacement.IsEmpty())
        {
            // 대체 문자열이 비어 있으면 기본값 설정
            m_customReplacement = _T("REDACTED");
        }

        CString displayStr;
        displayStr.Format(_T("%s -> %s"), m_customPattern, m_customReplacement);
        m_listCustomPatterns.AddString(displayStr);
        m_customPattern.Empty();
        m_customReplacement.Empty();
        UpdateData(FALSE); // UI 업데이트

        m_bIsModified = true;
    }
    else
    {
        AfxMessageBox(_T("패턴을 입력하세요."));
    }
    
}

void PatternDefinitionDlg::OnBnClickedOk()
{
    UpdateData(TRUE); // UI 데이터를 멤버 변수로 가져옴

    // 패턴을 가져옴
    GetSelectedPatterns();

    if (m_bIsModified)
    {
        // 성공 메시지 표시
        AfxMessageBox(_T("패턴이 성공적으로 저장되었습니다."));
    }
    m_bIsModified = false;
    CDialogEx::OnOK();
}


void PatternDefinitionDlg::OnBnClickedCheckall()
{
    UpdateData(TRUE);

    // 모두 켜져 있는지 확인
    if (m_bPattern1 && m_bPattern2 && m_bPattern3 && m_bPattern4 &&
        m_bPattern5 && m_bPattern6 && m_bPattern7 && m_bPattern8) {
        // 모두 켜져 있으면, 모두 끔
        m_bPattern1 = false;
        m_bPattern2 = false;
        m_bPattern3 = false;
        m_bPattern4 = false;
        m_bPattern5 = false;
        m_bPattern6 = false;
        m_bPattern7 = false;
        m_bPattern8 = false;

        // "전체 선택" 체크박스 체크 해제
        CheckDlgButton(IDC_CHECKALL, BST_UNCHECKED);
    }
    else {
        // 하나라도 꺼져 있으면, 모두 켬
        m_bPattern1 = true;
        m_bPattern2 = true;
        m_bPattern3 = true;
        m_bPattern4 = true;
        m_bPattern5 = true;
        m_bPattern6 = true;
        m_bPattern7 = true;
        m_bPattern8 = true;

        // "전체 선택" 체크박스 체크
        CheckDlgButton(IDC_CHECKALL, BST_CHECKED);

    }

    // 데이터 동기화: 멤버 변수 -> 컨트롤
    UpdateData(FALSE);
}


void PatternDefinitionDlg::OnBnClickedButtonDeletePattern()
{
    // 리스트 박스 컨트롤의 포인터
    CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LIST_CUSTOM_PATTERNS);

    // 선택된 항목의 인덱스를 가져옵니다.
    int selectedIndex = pListBox->GetCurSel();

    // 선택된 항목이 없을 경우
    if (selectedIndex == LB_ERR)
    {
        AfxMessageBox(_T("선택된 항목이 없습니다."));
        return;
    }

    // 선택된 항목 삭제
    pListBox->DeleteString(selectedIndex);

    // 선택된 항목 삭제 후, 항목이 하나 이상 남아 있으면 그 다음 항목을 선택
    int count = pListBox->GetCount();
    if (count > 0 && selectedIndex < count)
    {
        pListBox->SetCurSel(selectedIndex);
    }
    else if (count > 0)
    {
        pListBox->SetCurSel(count - 1); // 마지막 항목 선택
    }

    m_bIsModified = true;
}

