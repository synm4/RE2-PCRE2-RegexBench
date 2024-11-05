#include "pch.h"
#include "RegexBench.h"
#include "RegexBenchDlg.h"
#include "afxdialogex.h"
#include "SingleFileComparisonDlg.h"
#include "MultithreadingComparisonDlg.h"
#include "ExistingFeaturesDlg.h"
#include "PatternManager.h"
#include "GenerateDataDlg.h"
#include "FolderPathManager.h"
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;


#pragma comment(lib, "Shlwapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CRegexBenchDlg 동적 생성 구현
IMPLEMENT_DYNAMIC(CRegexBenchDlg, CDialogEx)

BEGIN_MESSAGE_MAP(CRegexBenchDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_DEFINE_PATTERNS, &CRegexBenchDlg::OnBnClickedButtonDefinePatterns)
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_CONTROL, &CRegexBenchDlg::OnTcnSelchangeTabControl)
    ON_BN_CLICKED(IDC_BUTTON_GENERATEDATA_SETTINGS, &CRegexBenchDlg::OnBnClickedButtonGeneratedataSettings)
    ON_BN_CLICKED(IDC_BUTTON_PATHMANAGER, &CRegexBenchDlg::OnBnClickedButtonPathmanager)
END_MESSAGE_MAP()

// CRegexBenchDlg 생성자
CRegexBenchDlg::CRegexBenchDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_REGEXBENCH_DIALOG, pParent)
    , m_gdiplusToken(0)
    , m_pSingleFileComparisonDlg(nullptr)
    , m_pMultithreadingComparisonDlg(nullptr)
    , m_pExistingFeaturesDlg(nullptr) // 초기화
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    // GDI+ 초기화
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

}

// CRegexBenchDlg 소멸자
CRegexBenchDlg::~CRegexBenchDlg()
{
    GdiplusShutdown(m_gdiplusToken);

    // 동적 할당된 탭 페이지 삭제
    if (m_pSingleFileComparisonDlg)
    {
        m_pSingleFileComparisonDlg->DestroyWindow();
        delete m_pSingleFileComparisonDlg;
        m_pSingleFileComparisonDlg = nullptr;
    }

    if (m_pMultithreadingComparisonDlg)
    {
        m_pMultithreadingComparisonDlg->DestroyWindow();
        delete m_pMultithreadingComparisonDlg;
        m_pMultithreadingComparisonDlg = nullptr;
    }

    if (m_pExistingFeaturesDlg)
    {
        m_pExistingFeaturesDlg->DestroyWindow();
        delete m_pExistingFeaturesDlg;
        m_pExistingFeaturesDlg = nullptr;
    }
}

// CRegexBenchDlg 메시지 처리기

void CRegexBenchDlg::ShowTabPage(int nPage)
{
    // 모든 페이지 숨기기
    if (m_pSingleFileComparisonDlg)
        m_pSingleFileComparisonDlg->ShowWindow(SW_HIDE);
    if (m_pMultithreadingComparisonDlg)
        m_pMultithreadingComparisonDlg->ShowWindow(SW_HIDE);
    if (m_pExistingFeaturesDlg)
        m_pExistingFeaturesDlg->ShowWindow(SW_HIDE);

    // 선택된 페이지 표시
    switch (nPage)
    {
    case 0:
        if (m_pSingleFileComparisonDlg)
            m_pSingleFileComparisonDlg->ShowWindow(SW_SHOW);
        break;
    case 1:
        if (m_pMultithreadingComparisonDlg)
            m_pMultithreadingComparisonDlg->ShowWindow(SW_SHOW);
        break;
    case 2:
        if (m_pExistingFeaturesDlg)
            m_pExistingFeaturesDlg->ShowWindow(SW_SHOW);
        break;
    default:
        break;
    }
}

BOOL CRegexBenchDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 탭 컨트롤에 탭 추가
    m_TabCtrl.InsertItem(0, _T("단일 파일 비교 그래프"));
    m_TabCtrl.InsertItem(1, _T("멀티쓰레딩 비교 그래프"));
    m_TabCtrl.InsertItem(2, _T("라인수 별 비교 그래프"));

    // PatternManager 초기화
    if (PatternManager::GetInstance().GetPatterns().empty())
    {
        PatternManager::GetInstance().SetDefaultPatterns();
    }
    
    // 형식 리스트 로드
    CString formatFilePath = _T("format_list.txt"); // 실제 파일 경로로 변경
    if (!m_dataGenerator.LoadFormatList(formatFilePath))
    {
        AfxMessageBox(_T("형식 리스트 파일을 로드할 수 없습니다. 기본 형식 리스트를 사용합니다."));
        m_dataGenerator.SetDefaultFormatList();
        m_dataGenerator.SaveFormatList(formatFilePath);
    }
    // 형식 초기화
    // 로그 또는 디버그 메시지
    if (m_dataGenerator.GetFormatList().empty())
    {
        AfxMessageBox(_T("형식 리스트가 비어 있습니다."));
    }
    else
    {
        TRACE(_T("형식 리스트가 성공적으로 로드되었습니다.\n"));
    }

    // 각 탭 페이지 다이얼로그 생성
    m_pSingleFileComparisonDlg = new CSingleFileComparisonDlg();
    m_pMultithreadingComparisonDlg = new CMultithreadingComparisonDlg();
    m_pExistingFeaturesDlg = new CExistingFeaturesDlg(); // 새 다이얼로그 클래스 인스턴스 생성

    // 각 다이얼로그의 부모를 현재 다이얼로그로 설정하고 생성
    if (m_pSingleFileComparisonDlg->Create(IDD_SINGLE_FILE_COMPARISON_DLG, &m_TabCtrl))
    {
        m_pSingleFileComparisonDlg->SetWindowPos(NULL, 0, 0, 0, 0,
            SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
        // 탭 컨트롤의 클라이언트 영역에 맞게 위치 조정
        CRect tabRect;
        m_TabCtrl.GetClientRect(&tabRect);
        m_TabCtrl.AdjustRect(FALSE, &tabRect);

        DWORD dwStyle = WS_CHILD | WS_VISIBLE;
        m_pSingleFileComparisonDlg->ModifyStyle(0, dwStyle);

        m_pSingleFileComparisonDlg->MoveWindow(&tabRect);
    }
    else
    {
        AfxMessageBox(_T("단일 파일 비교 그래프 탭 생성 실패"));
        delete m_pSingleFileComparisonDlg;
        m_pSingleFileComparisonDlg = nullptr;
    }

    if (m_pMultithreadingComparisonDlg->Create(IDD_MULTITHREADING_COMPARISON_DLG, &m_TabCtrl))
    {
        m_pMultithreadingComparisonDlg->SetWindowPos(NULL, 0, 0, 0, 0,
            SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
        // 탭 컨트롤의 클라이언트 영역에 맞게 위치 조정
        CRect tabRect;
        m_TabCtrl.GetClientRect(&tabRect);
        m_TabCtrl.AdjustRect(FALSE, &tabRect);
        DWORD dwStyle = WS_CHILD | WS_VISIBLE;
        m_pMultithreadingComparisonDlg->ModifyStyle(0, dwStyle);
        m_pMultithreadingComparisonDlg->MoveWindow(&tabRect);
    }
    else
    {
        AfxMessageBox(_T("멀티쓰레딩 비교 그래프 탭 생성 실패"));
        delete m_pMultithreadingComparisonDlg;
        m_pMultithreadingComparisonDlg = nullptr;
    }

    if (m_pExistingFeaturesDlg->Create(IDD_EXISTING_FEATURES_DLG, &m_TabCtrl))
    {
        m_pExistingFeaturesDlg->SetWindowPos(NULL, 0, 0, 0, 0,
            SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
        // 탭 컨트롤의 클라이언트 영역에 맞게 위치 조정
        CRect tabRect;
        m_TabCtrl.GetClientRect(&tabRect);
        m_TabCtrl.AdjustRect(FALSE, &tabRect);
        DWORD dwStyle = WS_CHILD | WS_VISIBLE;
        m_pExistingFeaturesDlg->ModifyStyle(0, dwStyle);
        m_pExistingFeaturesDlg->MoveWindow(&tabRect);
    }
    else
    {
        AfxMessageBox(_T("라인수 별 비교 그래프 탭 생성 실패"));
        delete m_pExistingFeaturesDlg;
        m_pExistingFeaturesDlg = nullptr;
    }

    // 첫 번째 탭 페이지 표시
    ShowTabPage(0);

    return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}



void CRegexBenchDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TAB_CONTROL, m_TabCtrl);
}

void CRegexBenchDlg::OnDestroy()
{
    CDialogEx::OnDestroy();

    // 동적 할당된 탭 페이지 삭제 (소멸자에서 이미 처리됨)
    // 추가적으로 필요한 정리 작업이 있으면 수행
}


void CRegexBenchDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 그리기를 위한 DC

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 클라이언트 사각형 가져오기
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 아이콘 그리기
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CPaintDC dc(this); // 그리기를 위한 DC

        CDialogEx::OnPaint();
    }
}

HCURSOR CRegexBenchDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CRegexBenchDlg::OnBnClickedButtonDefinePatterns()
{
    PatternDefinitionDlg dlg;

    // 현재 패턴을 싱글톤에서 가져와 다이얼로그에 전달
    dlg.SetPatterns(PatternManager::GetInstance().GetPatterns());

    // 다이얼로그를 모달로 실행
    if (dlg.DoModal() == IDOK)
    {
        // Utils 클래스의 패턴 초기화 함수 호출
        if (!Utils::InitializePatterns(PatternManager::GetInstance().GetPatterns()))
        {
            AfxMessageBox(_T("패턴 초기화에 실패했습니다."));
        }
    }
}

void CRegexBenchDlg::OnBnClickedButtonGeneratedataSettings()
{
    GenerateDataDlg dlg;

    // 다이얼로그를 모달로 실행
    if (dlg.DoModal() == IDOK)
    {

    }
}

void CRegexBenchDlg::OnTcnSelchangeTabControl(NMHDR* pNMHDR, LRESULT* pResult)
{
    int nSel = m_TabCtrl.GetCurSel();
    ShowTabPage(nSel);
    *pResult = 0;
}

void CRegexBenchDlg::OnBnClickedButtonPathmanager()
{
    // 프로그램 실행 파일 경로 가져오기
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
        return; // 첫 번째 폴더 선택이 취소되면 함수 종료
    }

    // 두 번째 폴더 선택: 생성 데이터 저장 폴더
    CFolderPickerDialog generatedDataFolderDlg(exePath, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, this);
    generatedDataFolderDlg.m_ofn.lpstrTitle = _T("생성 데이터를 저장할 폴더를 선택하세요");
    if (generatedDataFolderDlg.DoModal() == IDOK)
    {
        CString generatedDataPath = generatedDataFolderDlg.GetPathName();
        FolderPathManager::GetInstance().SetGeneratedDataFolderPath(generatedDataPath);
    }
    else
    {
        AfxMessageBox(_T("생성 데이터를 저장할 폴더 선택이 취소되었습니다."));
        return; // 두 번째 폴더 선택이 취소되면 함수 종료
    }

    // 선택된 경로를 출력하여 확인 (정보 아이콘 사용)
    CString message;
    message.Format(_T("결과 폴더 경로: %s\n생성 데이터 폴더 경로: %s"),
        FolderPathManager::GetInstance().GetResultFolderPath(),
        FolderPathManager::GetInstance().GetGeneratedDataFolderPath());
    MessageBox(message, _T("폴더 경로 정보"), MB_ICONINFORMATION | MB_OK);
}
