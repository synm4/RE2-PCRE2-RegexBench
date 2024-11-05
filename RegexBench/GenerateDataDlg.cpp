#include "pch.h"
#include "RegexBench.h"
#include "GenerateDataDlg.h"
#include "afxdialogex.h"
#include "Utils.h"
#include "FolderPathManager.h"
#include <regex>
#include <exception>
#include <afxdlgs.h> // CFolderPickerDialog를 사용하기 위해 추가

#pragma comment(lib, "Shlwapi.lib")

// GenerateDataDialog 대화 상자

IMPLEMENT_DYNAMIC(GenerateDataDlg, CDialogEx)

GenerateDataDlg::GenerateDataDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_GENRATEDATA_SETTINGS_DLG, pParent),
    m_numLines(100), // 기본값 설정
    m_bDataModified(false) // 플래그 초기화
{
}

GenerateDataDlg::~GenerateDataDlg()
{
}

void GenerateDataDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_GENERATELINECOUNT, m_numLines); // 입력 칸 연결
}

BEGIN_MESSAGE_MAP(GenerateDataDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_ADD_DATA, &GenerateDataDlg::OnBnClickedButtonAddData)
    ON_BN_CLICKED(IDC_BUTTON_DELETE_DATA, &GenerateDataDlg::OnBnClickedButtonDeleteData)
    ON_BN_CLICKED(IDC_BUTTON_GENERATE_TEST_DATA, &GenerateDataDlg::OnBnClickedButtonGenerateTestData)
    ON_BN_CLICKED(ID_BUTTON_APPLY, &GenerateDataDlg::OnBnClickedButtonApply)
END_MESSAGE_MAP()

BOOL GenerateDataDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 툴팁 초기화
    if (!m_ToolTip.Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX))
    {
        TRACE0("툴팁 컨트롤을 생성하지 못했습니다.\n");
    }
    else
    {
        // 최대 너비 설정 (픽셀 단위)
        m_ToolTip.SetMaxTipWidth(300);

        // 각 컨트롤에 대한 툴팁 텍스트 추가
        m_ToolTip.AddTool(GetDlgItem(IDC_BUTTON_ADD_DATA), L"데이터 형식을 추가합니다.");
        m_ToolTip.AddTool(GetDlgItem(IDC_BUTTON_DELETE_DATA), L"선택된 데이터 형식을 삭제합니다.");
        m_ToolTip.AddTool(GetDlgItem(IDC_BUTTON_GENERATE_TEST_DATA), L"테스트 데이터를 생성합니다.");
        m_ToolTip.AddTool(GetDlgItem(ID_BUTTON_APPLY), L"변경된 내용을 적용합니다.");
        m_ToolTip.AddTool(GetDlgItem(IDC_EDIT_GENERATELINECOUNT), L"생성할 라인 수를 입력합니다.");
        m_ToolTip.AddTool(GetDlgItem(IDC_LIST_DATA), L"데이터 형식 목록입니다.");
        m_ToolTip.AddTool(GetDlgItem(IDC_TOOLTIP), L"{time} = 현재 시간\n{id} = user 0 ~ 9999\n{ip} = 0 ~ 255\n{num} = 0 ~ 9999\n{name} = 홍길동 0 ~ 9999\n{email} = 이메일\n{rrn} = 주민등록번호");
        // 필요에 따라 다른 컨트롤에도 추가

        m_ToolTip.Activate(TRUE); // 툴팁 활성화
    }

    // 리스트 박스 초기화
    CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LIST_DATA);
    pListBox->ResetContent();

    // 실행 파일의 디렉토리 경로 가져오기
    CString loadFilePath = _T("format_list.txt"); // 실제 파일 경로로 변경

    if (m_dataGenerator.LoadFormatList(loadFilePath))
    {
        // 파일에서 로드한 형식 리스트를 UI에 추가
        for (const auto& format : m_dataGenerator.GetFormatList())
        {
            pListBox->AddString(format);
        }
    }
    else
    {
        // 파일이 없으면 기본 형식 리스트 설정 및 UI에 추가
        m_dataGenerator.SetDefaultFormatList();
        for (const auto& format : m_dataGenerator.GetFormatList())
        {
            pListBox->AddString(format);
        }
    }

    return TRUE; // 포커스를 컨트롤에 설정하지 않음
}

BOOL GenerateDataDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_ToolTip.m_hWnd != NULL)
    {
        m_ToolTip.RelayEvent(pMsg); // 툴팁에 메시지 전달
    }
    return CDialogEx::PreTranslateMessage(pMsg); // 기본 처리 수행
}

void GenerateDataDlg::OnBnClickedButtonAddData()
{
    // IDC_EDIT_ADDDATA 컨트롤에서 텍스트 가져오기
    CString newFormat;
    GetDlgItemText(IDC_EDIT_ADDDATA, newFormat);

    // 입력된 형식 문자열이 비어있는지 확인
    if (newFormat.IsEmpty())
    {
        AfxMessageBox(_T("추가할 형식 문자열을 입력하세요."));
        return;
    }

    // 형식 문자열 유효성 검사
    CString errorMsg;
    if (!m_dataGenerator.ValidateFormat(newFormat, errorMsg))
    {
        AfxMessageBox(errorMsg);
        return;
    }

    // 형식 문자열 리스트에 추가
    m_dataGenerator.AddFormat(newFormat);
    m_bDataModified = true; // 데이터가 변경되었음을 표시

    // 리스트 박스에 추가
    CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LIST_DATA);
    pListBox->AddString(newFormat);

    // 입력 필드 비우기
    SetDlgItemText(IDC_EDIT_ADDDATA, _T(""));
}

void GenerateDataDlg::OnBnClickedButtonDeleteData()
{
    CListBox* pListBox = (CListBox*)GetDlgItem(IDC_LIST_DATA);

    // 선택된 항목의 인덱스를 가져오기
    int selectedIndex = pListBox->GetCurSel();

    // 선택된 항목이 없을 경우
    if (selectedIndex == LB_ERR)
    {
        AfxMessageBox(_T("선택된 항목이 없습니다."));
        return;
    }

    // 리스트 박스에서 항목 삭제
    pListBox->DeleteString(selectedIndex);

    // 내부 형식 리스트에서도 삭제
    m_dataGenerator.RemoveFormat(selectedIndex);
    m_bDataModified = true; // 데이터가 변경되었음을 표시

    // 남아있는 항목 중 다음 항목 선택
    int count = pListBox->GetCount();
    if (count > 0)
    {
        if (selectedIndex < count)
            pListBox->SetCurSel(selectedIndex);
        else
            pListBox->SetCurSel(count - 1);
    }
}

void GenerateDataDlg::OnBnClickedButtonGenerateTestData()
{
    if (m_bDataModified)
    {
        if (IDYES == AfxMessageBox(L"변경된 내용이 있습니다. 저장하시겠습니까?", MB_YESNO))
        {
            OnBnClickedButtonApply();
            if (m_bDataModified) // 저장에 실패한 경우 중단
                return;
        }
    }

    if (!UpdateData(TRUE))
    {
        AfxMessageBox(_T("데이터를 가져오는 데 실패했습니다."));
        return;
    }

    if (m_numLines <= 0)
    {
        AfxMessageBox(_T("유효한 라인 수를 입력해주세요."));
        return;
    }

    CString defaultFileName;
    defaultFileName.Format(_T("test_data_%d.txt"), m_numLines);

    CString folderPath;
    if (FolderPathManager::GetInstance().IsGeneratedDataFolderPathSet())
    {
        folderPath = FolderPathManager::GetInstance().GetGeneratedDataFolderPath();
    }
    else
    {
        // CFolderPickerDialog를 사용하여 폴더 선택 대화상자 표시
        CFolderPickerDialog folderDlg(NULL, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, this);
        if (folderDlg.DoModal() == IDOK)
        {
            folderPath = folderDlg.GetPathName();
            if (::PathFileExists(folderPath))
            {
                FolderPathManager::GetInstance().SetGeneratedDataFolderPath(folderPath);
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

    CString filePath = folderPath + _T("\\") + defaultFileName; // 경로 구분자 추가

    try
    {
        m_dataGenerator.GenerateTestData(filePath, m_numLines);
        AfxMessageBox(_T("데이터 생성이 완료되었습니다."));
    }
    catch (const std::exception& ex)
    {
        CString errorMsg = CA2W(ex.what(), CP_UTF8);
        AfxMessageBox(errorMsg);
    }
}

void GenerateDataDlg::OnOK()
{
    if (m_bDataModified)
    {
        int res = AfxMessageBox(_T("데이터가 변경되었습니다. 저장하시겠습니까?"), MB_YESNOCANCEL | MB_ICONQUESTION);
        if (res == IDYES)
        {
            OnBnClickedButtonApply();
            if (m_bDataModified) // 저장에 실패한 경우
                return;
        }
        else if (res == IDNO)
        {
            // 저장하지 않고 창을 닫습니다.
        }
        else
        {
            // IDCANCEL을 선택하면 창을 닫지 않습니다.
            return;
        }
    }

    CDialogEx::OnOK();
}

void GenerateDataDlg::OnBnClickedButtonApply()
{
    if (!m_bDataModified)
    {
        AfxMessageBox(_T("변경된 내용이 없습니다."));
        return;
    }

    // 실행 파일의 디렉토리 경로 가져오기
    CString filePath = _T("format_list.txt"); // 실제 파일 경로로 변경

    if (m_dataGenerator.SaveFormatList(filePath))
    {
        AfxMessageBox(_T("데이터가 성공적으로 저장되었습니다."));
        m_bDataModified = false;
    }
    else
    {
        AfxMessageBox(_T("데이터 저장에 실패했습니다."));
    }
}
