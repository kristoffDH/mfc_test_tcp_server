
// ExTesterServerDlg.h: 헤더 파일
//

#pragma once


// CExTesterServerDlg 대화 상자
class CExTesterServerDlg : public CDialogEx
{
// 생성입니다.
public:
	CExTesterServerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	~CExTesterServerDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EXTESTERSERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
public:
	CWinThread* m_pServerThread;
	CEdit m_editLog;
	CEdit m_editPort;
	CEdit m_editSendMsg;

	//Common Header
	CEdit m_editChLength;
	CEdit m_editChTypeSend;
	CEdit m_editChTypeRecv;
	CEdit m_editChWork;
	CEdit m_editChJob;
	CEdit m_editChDate;
	CEdit m_editChResponse;

	//Meta Common Header
	CEdit m_editMchResponseType;
	CEdit m_editMchDataType;
	CEdit m_editMchDataCount;

	//Data
	CEdit m_editData;

protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	static int ConvertMultibyteToUtf8(CString multibyte_data, byte* utf8_data, int utf8_data_size);
	static CString ConvertUtf8(byte* utf8_data, int utf8_data_size);

	void AddEditLog(CString log);
	void ShowRecvData(CExTesterServerDlg* dlg, char* packet, int packetSize);

	afx_msg void OnBnClickedBtnRun();
	afx_msg void OnBnClickedBtnStop();
	afx_msg void OnBnClickedBtnSend();
	afx_msg void OnBnClickedBtnResetEdit();
	afx_msg void OnBnClickedBtnCreatePacket();

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedBtnClearLog();
};
