
// ExTesterServerDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "ExTesterServer.h"
#include "ExTesterServerDlg.h"
#include "afxdialogex.h"

#include "CPacketHandler.h"

#include <vector>
#include <WinSock2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(lib, "ws2_32.lib")

#define PACKET_LENGTH 4096


// CExTesterServerDlg 대화 상자
//#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

bool g_serverRunningState;
int g_serverPort = 0;

SOCKET g_hListen;
SOCKET g_hClient;


UINT ExServerProc(LPVOID param)
{
	auto ptrCExTesterServerDlg = (CExTesterServerDlg*)param;
	CEdit* ptrEditLog = &ptrCExTesterServerDlg->m_editLog;

	WSADATA wsaData;
	int wasErrorCode = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (wasErrorCode != 0) {
		CString str = "";
		str.Format(">>>> WASStartup Error\n");
		ptrEditLog->SetSel(-2, -1);

		int len = ptrCExTesterServerDlg->m_editLog.GetWindowTextLength();
		ptrEditLog->SetSel(len, len);
		ptrEditLog->ReplaceSel(str);
		return 0;
	}

	g_hListen = socket(PF_INET, SOCK_STREAM, IPPROTO_HOPOPTS);

	SOCKADDR_IN tAddr = {};
	tAddr.sin_family = AF_INET;
	tAddr.sin_port = htons(g_serverPort);
	tAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(g_hListen, (SOCKADDR*)&tAddr, sizeof(tAddr));

	listen(g_hListen, SOMAXCONN);

	fd_set readSet;

	FD_ZERO(&readSet);
	FD_SET(g_hListen, &readSet);
	struct timeval tv;

	tv.tv_usec = 100;
	
	g_serverRunningState = true;

	while (g_serverRunningState)
	{
		fd_set copySet = readSet;

		int returnValue = select(0, &copySet, nullptr, nullptr, &tv);

		if (returnValue == SOCKET_ERROR) {
			break;
		}

		for (int i = 0; i < readSet.fd_count; ++i)
		{
			if (FD_ISSET(readSet.fd_array[i], &copySet))
			{
				if (g_hListen == readSet.fd_array[i])
				{
					SOCKADDR_IN tAddr = {};
					int iSize = sizeof(tAddr);
					g_hClient = accept(g_hListen, (SOCKADDR*)&tAddr, &iSize);

					if (g_hClient == SOCKET_ERROR)
					{
						closesocket(g_hListen);
						WSACleanup();
						return 0;
					}

					FD_SET(g_hClient, &readSet);

					struct sockaddr_in remoteAddress;
					socklen_t addrLen = sizeof(struct sockaddr_in);
					char clientIP[16];
					int clientPort = 0;


					if (getpeername(g_hClient, (sockaddr*)&remoteAddress, &addrLen) == -1) {
						closesocket(g_hListen);
						WSACleanup();
						return 0;
					}

					ZeroMemory(clientIP, sizeof(clientIP));
					strncpy_s(clientIP, inet_ntoa(remoteAddress.sin_addr), 16);
					clientPort = remoteAddress.sin_port;

					CString str = "";
					str.Format(">> Client Connected [%s:%d]\n", clientIP, clientPort);
					ptrEditLog->SetSel(-2, -1);

					int len = ptrCExTesterServerDlg->m_editLog.GetWindowTextLength();
					ptrEditLog->SetSel(len, len);
					ptrEditLog->ReplaceSel(str);
				}
				else
				{
					char packet[PACKET_LENGTH];

					ZeroMemory(packet, sizeof(packet));

					int recvLength = recv(readSet.fd_array[i], packet, PACKET_LENGTH, 0);

					if (recvLength <= 0)
					{
						readSet.fd_array[i];

						FD_CLR(readSet.fd_array[i], &readSet);

						closesocket(g_hClient);

						g_hClient = -1;

						CString str = "";
						str.Format(">> Client Disconnected\n");
						ptrEditLog->SetSel(-2, -1);

						int len = ptrCExTesterServerDlg->m_editLog.GetWindowTextLength();
						ptrEditLog->SetSel(len, len);
						ptrEditLog->ReplaceSel(str);

						continue;
					}
					
					if (packet[0] == 0x02) {
						packet[0] = '2';
					}

					if (packet[recvLength - 1] == 0x03) {
						packet[recvLength - 1] = '3';
					}
					
					//CString recvData = CExTesterServerDlg::ConvertUtf8((byte*)packet, recvLength);
					CString recvData((LPCSTR)packet, recvLength);

					CString str = "";
					str.Format(">> Recv [%dbytes] : ", recvLength);
					str += recvData + "\n";
					ptrEditLog->SetSel(-2, -1);

					int len = ptrCExTesterServerDlg->m_editLog.GetWindowTextLength();
					ptrEditLog->SetSel(len, len);
					ptrEditLog->ReplaceSel(str);

					ptrCExTesterServerDlg->ShowRecvData(ptrCExTesterServerDlg, packet, recvLength);
				}
			}
		}

	}

	closesocket(g_hClient);
	closesocket(g_hListen);
	WSACleanup();

	return 0;
}

void CExTesterServerDlg::ShowRecvData(CExTesterServerDlg* dlg, char* packet,int packetSize) {
	
	CommonHeader commonHeader;

	memset(&commonHeader, 0x00, sizeof(commonHeader));

	int nPos = 1;
	// Common Header
	memcpy(commonHeader.length, packet + nPos, PACKET_LENGTH_SIZE);
	nPos += PACKET_LENGTH_SIZE;

	memcpy(commonHeader.systemTypeCode_Send, packet + nPos, SYSTEM_TYPE_CODE_SIZE);
	nPos += SYSTEM_TYPE_CODE_SIZE;

	memcpy(commonHeader.systemTypeCode_Recv, packet + nPos, SYSTEM_TYPE_CODE_SIZE);
	nPos += SYSTEM_TYPE_CODE_SIZE;

	memcpy(commonHeader.workCode, packet + nPos, WORK_CODE_SIZE);
	nPos += WORK_CODE_SIZE;

	memcpy(commonHeader.jobCode, packet + nPos, JOB_CODE_SIZE);
	nPos += JOB_CODE_SIZE;

	memcpy(commonHeader.dateTime, packet + nPos, DATE_TIME_SIZE);
	nPos += DATE_TIME_SIZE;

	memcpy(commonHeader.response, packet + nPos, RESPONSE_SIZE);
	nPos += RESPONSE_SIZE;

	byte data[1024];
	memset(data, 0x00, sizeof(data));
	memcpy(data, packet + nPos, packetSize - nPos - 1);

	dlg->SetDlgItemText(IDC_EDIT_RCV_LENGTH, LPSTR(commonHeader.length));
	dlg->SetDlgItemText(IDC_EDIT_RCV_SEND, LPSTR(commonHeader.systemTypeCode_Send));
	dlg->SetDlgItemText(IDC_EDIT_RCV_RECV, LPSTR(commonHeader.systemTypeCode_Recv));
	dlg->SetDlgItemText(IDC_EDIT_RCV_WORK, LPSTR(commonHeader.workCode));
	dlg->SetDlgItemText(IDC_EDIT_RCV_JOB, LPSTR(commonHeader.jobCode));
	dlg->SetDlgItemText(IDC_EDIT_RCV_DATE, LPSTR(commonHeader.dateTime));
	dlg->SetDlgItemText(IDC_EDIT_RCV_RES, LPSTR(commonHeader.response));
	dlg->SetDlgItemText(IDC_EDIT_RCV_DATA, LPSTR(data));
}

CExTesterServerDlg::CExTesterServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_EXTESTERSERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_pServerThread = nullptr;
}

CExTesterServerDlg::~CExTesterServerDlg()
{
	if (g_serverRunningState) {
		g_serverRunningState = false;
	}

	DWORD result = WaitForSingleObject(m_pServerThread, 2000);

	if (result == WAIT_TIMEOUT)
	{
		m_pServerThread->SuspendThread();
		DWORD dwResult;
		GetExitCodeThread(m_pServerThread, &dwResult);
		m_pServerThread = nullptr;
	}
}

void CExTesterServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_PORT, m_editPort);
	DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
	DDX_Control(pDX, IDC_EDIT_SEND_MSG, m_editSendMsg);
	DDX_Control(pDX, IDC_EDIT_CH_LENGTH, m_editChLength);
	DDX_Control(pDX, IDC_EDIT_CH_TYPE_SEND, m_editChTypeSend);
	DDX_Control(pDX, IDC_EDIT_CH_TYPE_RECV, m_editChTypeRecv);
	DDX_Control(pDX, IDC_EDIT_CH_WORK, m_editChWork);
	DDX_Control(pDX, IDC_EDIT_CH_JOB, m_editChJob);
	DDX_Control(pDX, IDC_EDIT_CH_DATE, m_editChDate);
	DDX_Control(pDX, IDC_EDIT_CH_RESPONSE, m_editChResponse);
	DDX_Control(pDX, IDC_EDIT_MCH_RESPONSE_TYPE, m_editMchResponseType);
	DDX_Control(pDX, IDC_EDIT_MCH_DATA_TYPE, m_editMchDataType);
	DDX_Control(pDX, IDC_EDIT_MCH_DATA_COUNT, m_editMchDataCount);
	DDX_Control(pDX, IDC_EDIT_DATA, m_editData);
}

BEGIN_MESSAGE_MAP(CExTesterServerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_RUN, &CExTesterServerDlg::OnBnClickedBtnRun)
	ON_BN_CLICKED(IDC_BTN_STOP, &CExTesterServerDlg::OnBnClickedBtnStop)
	ON_BN_CLICKED(IDC_BTN_SEND, &CExTesterServerDlg::OnBnClickedBtnSend)
	ON_BN_CLICKED(IDC_BTN_CREATE_PACKET, &CExTesterServerDlg::OnBnClickedBtnCreatePacket)
	ON_WM_SYSCOMMAND()
	ON_BN_CLICKED(IDC_BTN_RESET_EDIT, &CExTesterServerDlg::OnBnClickedBtnResetEdit)
	ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CExTesterServerDlg::OnBnClickedBtnClearLog)
END_MESSAGE_MAP()


// CExTesterServerDlg 메시지 처리기

BOOL CExTesterServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	SetDlgItemInt(IDC_EDIT_PORT, 9999);

	CTime cTime = CTime::GetCurrentTime();

	CString strDate;
	strDate.Format("%04d%02d%02d%02d%02d%02d",
		cTime.GetYear(),
		cTime.GetMonth(),
		cTime.GetDay(),
		cTime.GetHour(),
		cTime.GetMinute(),
		cTime.GetSecond());

	m_editChLength.SetWindowTextA("44");
	m_editChTypeSend.SetWindowTextA("MC0L00");
	m_editChTypeRecv.SetWindowTextA("087999");
	m_editChWork.SetWindowTextA("T007");
	m_editChJob.SetWindowTextA("0020");
	m_editChDate.SetWindowTextA(strDate);
	m_editChResponse.SetWindowTextA("0000");

	m_editMchResponseType.SetWindowTextA("N");
	m_editMchDataType.SetWindowTextA("L");
	m_editMchDataCount.SetWindowTextA("3");

	((CButton*)GetDlgItem(IDC_CHECK_META_HEADER))->SetCheck(false);

	GetDlgItem(IDC_BTN_STOP)->EnableWindow(false);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CExTesterServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CExTesterServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CExTesterServerDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
		{   // 위 VK_RETURN은 Enter, VK_ESCAPE는 ESC을 의미함. 필요시 하나만 사용.
			return true;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CExTesterServerDlg::OnBnClickedBtnResetEdit()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_editChLength.SetWindowTextA("");
	m_editChTypeSend.SetWindowTextA("");
	m_editChTypeRecv.SetWindowTextA("");
	m_editChWork.SetWindowTextA("");
	m_editChJob.SetWindowTextA("");
	m_editChDate.SetWindowTextA("");
	m_editChResponse.SetWindowTextA("");

	m_editMchResponseType.SetWindowTextA("");
	m_editMchDataType.SetWindowTextA("");
	m_editMchDataCount.SetWindowTextA("");
	m_editSendMsg.SetWindowTextA("");
}

void CExTesterServerDlg::OnBnClickedBtnRun()
{
	int port = GetDlgItemInt(IDC_EDIT_PORT);
	g_serverPort = port;

	if (!g_serverRunningState) {
		g_serverRunningState = true;

		m_pServerThread = AfxBeginThread(ExServerProc, this);
		m_pServerThread->m_bAutoDelete = true;

		CString str = "";
		str.Format("------ Start Server ------\n");
		AddEditLog(str);

		GetDlgItem(IDC_BTN_RUN)->EnableWindow(false);
		GetDlgItem(IDC_BTN_STOP)->EnableWindow(true);
	}
}

void CExTesterServerDlg::OnBnClickedBtnStop()
{
	g_serverRunningState = false;

	CString str = "";
	str.Format("------ Stop Server ------\n");
	AddEditLog(str);

	GetDlgItem(IDC_BTN_RUN)->EnableWindow(true);
	GetDlgItem(IDC_BTN_STOP)->EnableWindow(false);
}

int CExTesterServerDlg::ConvertMultibyteToUtf8(CString multibyte_data, byte* utf8_data, int utf8_data_size)
{
	wchar_t strUniCode[PACKET_LENGTH];

	ZeroMemory(strUniCode, sizeof(strUniCode));

	int nLen = MultiByteToWideChar(CP_ACP, 0, multibyte_data, strlen(multibyte_data), NULL, NULL);

	MultiByteToWideChar(CP_ACP, 0, multibyte_data, strlen(multibyte_data), strUniCode, nLen);


	nLen = WideCharToMultiByte(CP_UTF8, 0, strUniCode, lstrlenW(strUniCode), NULL, 0, NULL, NULL);
	if (utf8_data_size < nLen) {
		return -1;
	}

	WideCharToMultiByte(CP_UTF8, 0, strUniCode, lstrlenW(strUniCode), (char*)utf8_data, nLen, NULL, NULL);
	utf8_data_size = nLen;

	return nLen;
}

CString CExTesterServerDlg::ConvertUtf8(byte* utf8_data, int utf8_data_size)
{
	wchar_t strUnicode[PACKET_LENGTH];

	ZeroMemory(strUnicode, sizeof(strUnicode));

	int nLen = MultiByteToWideChar(CP_UTF8, 0, (char*)utf8_data, utf8_data_size, NULL, NULL);

	MultiByteToWideChar(CP_UTF8, 0, (char*)utf8_data, utf8_data_size, strUnicode, nLen);

	CString unicode(strUnicode);

	return unicode;
}

void CExTesterServerDlg::AddEditLog(CString log)
{
	CString str = log + "\n";
	m_editLog.SetSel(-2, -1);

	int len = m_editLog.GetWindowTextLength();
	m_editLog.SetSel(len, len);
	m_editLog.ReplaceSel(str);
}

void CExTesterServerDlg::OnBnClickedBtnSend()
{
	if (!g_serverRunningState) {
		AddEditLog("--- Server is not running....");
		return;
	}

	CString readText = "";
	m_editSendMsg.GetWindowTextA(readText);

	//byte sendMessage[PACKET_LENGTH];
	//int readMessageSize = sizeof(sendMessage);

	//ZeroMemory(sendMessage, sizeof(sendMessage));
	//memcpy(sendMessage, (void*)(LPCTSTR)(readText), readMessageSize);

	//int sendLen = ConvertMultibyteToUtf8(readText, sendMessage, readMessageSize);

	//if (sendLen <= 0) {
	//	AddEditLog(">> Data Send Error.");
	//	return;
	//}

	//int sendByte = send(g_hClient, (const char*)sendMessage, sendLen, 0);
	int sendByte = send(g_hClient, (const char*)readText, readText.GetLength(), 0);

	if (sendByte == -1) {
		AddEditLog("--- Socket send Error");
		return ;
	}

	if (readText.GetAt(0) == 0x02) {
		readText.SetAt(0, '2');
	}

	if (readText.GetAt(readText.GetLength() - 1) == 0x03) {
		readText.SetAt(readText.GetLength() - 1, '3');
	}

	CString str = "";
	str.Format("<< send [%dbytes] : ", sendByte);
	str += readText;

	AddEditLog(str);

	m_editSendMsg.SetWindowTextA("");
}


void CExTesterServerDlg::OnBnClickedBtnCreatePacket()
{
	//Common Header
	int packetLength = GetDlgItemInt(IDC_EDIT_CH_LENGTH);
	CString strChTypeSend;
	CString strChTypeRecv;
	CString strChWork;
	CString strChJob;
	CString strChDate;
	CString strChResponse;
	CString strMchResponseType;
	CString strMchDataType;
	int dataCount = GetDlgItemInt(IDC_EDIT_MCH_DATA_COUNT);
	CString strData;

	m_editChTypeSend.GetWindowTextA(strChTypeSend);
	m_editChTypeRecv.GetWindowTextA(strChTypeRecv);
	m_editChWork.GetWindowTextA(strChWork);
	m_editChJob.GetWindowTextA(strChJob);
	m_editChDate.GetWindowTextA(strChDate);
	m_editChResponse.GetWindowTextA(strChResponse);
	m_editMchResponseType.GetWindowTextA(strMchResponseType);
	m_editMchDataType.GetWindowTextA(strMchDataType);
	m_editData.GetWindowTextA(strData);

	if (strChTypeSend.GetLength() != SYSTEM_TYPE_CODE_SIZE) {

		CString str = "";
		str.Format("--- System Type Send Code Size is not %d byte", SYSTEM_TYPE_CODE_SIZE);
		AddEditLog(str);
		return;
	}

	if (strChTypeRecv.GetLength() != SYSTEM_TYPE_CODE_SIZE) {

		CString str = "";
		str.Format("--- System Type Recv Code Size is not %d byte", SYSTEM_TYPE_CODE_SIZE);
		AddEditLog(str);
		return;
	}

	if (strChWork.GetLength() != WORK_CODE_SIZE) {

		CString str = "";
		str.Format("--- Work Code Size is not %d byte", WORK_CODE_SIZE);
		AddEditLog(str);
		return;
	}

	if (strChJob.GetLength() != JOB_CODE_SIZE) {

		CString str = "";
		str.Format("--- Job Code Size is not %d byte", JOB_CODE_SIZE);
		AddEditLog(str);
		return;
	}

	if (strChDate.GetLength() != DATE_TIME_SIZE) {

		CString str = "";
		str.Format("--- Date Size is not %d byte", DATE_TIME_SIZE);
		AddEditLog(str);
		return;
	}

	if (strChResponse.GetLength() != RESPONSE_SIZE) {
		CString str = "";
		str.Format("--- Response Size is not %d byte", RESPONSE_SIZE);
		AddEditLog(str);
		return;
	}

	CommonHeader commonHeader;
	MetaCommonHeader metaCommonHeader;
	CPacketHandler packetHandler;

	ZeroMemory(&commonHeader, sizeof(commonHeader));
	ZeroMemory(&metaCommonHeader, sizeof(metaCommonHeader));

	// Common Header
	sprintf_s((char*)commonHeader.length, sizeof(commonHeader.length), "%04d", packetLength);
	memcpy(commonHeader.systemTypeCode_Send, (byte*)(LPCSTR)strChTypeSend, 6);
	memcpy(commonHeader.systemTypeCode_Recv, (byte*)(LPCSTR)strChTypeRecv, 6);
	memcpy(commonHeader.workCode, (byte*)(LPCSTR)strChWork, 4);
	memcpy(commonHeader.jobCode, (byte*)(LPCSTR)strChJob, 4);
	memcpy(commonHeader.dateTime, (byte*)(LPCSTR)strChDate, 14);
	memcpy(commonHeader.response, (byte*)(LPCSTR)strChResponse, 4);

	packetHandler.SetCommonHeader(&commonHeader);

	// Meta Common Header
	bool addMetaHeader = false;
	if (((CButton*)GetDlgItem(IDC_CHECK_META_HEADER))->GetCheck()) {
		memcpy(&metaCommonHeader.responseType, (byte*)(LPCSTR)strMchResponseType, 1);
		memcpy(&metaCommonHeader.dataType, (byte*)(LPCSTR)strMchDataType, 1);
		sprintf_s((char*)metaCommonHeader.dataCount, sizeof(metaCommonHeader.dataCount), "%03d", dataCount);

		packetHandler.SetMetaCommonHeader(&metaCommonHeader);
		addMetaHeader = true;
	}

	std::vector<byte> vtData;
	int dataLength = strData.GetLength();
	vtData.resize(dataLength);

	std::copy((LPCSTR)strData, (LPCSTR)strData + dataLength, vtData.begin());

	packetHandler.SetData(vtData);

	byte packet[PACKET_LENGTH];
	CString strPacket = "";

	ZeroMemory(packet, sizeof(packet));

	int length = packetHandler.MakePacket(packet, sizeof(packet), addMetaHeader);

	strPacket.Format("%s", packet);
	SetDlgItemInt(IDC_EDIT_CH_LENGTH, length);

	m_editSendMsg.SetWindowTextA(strPacket);

}


void CExTesterServerDlg::OnBnClickedBtnClearLog()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_editLog.SetWindowTextA("");
}
