#include <Windows.h>
#include <WinUser.h>
#include <fstream>
#include "HiEasyX.h"
#include "ini.hpp"
#include "resource.h"

#pragma comment(lib,"winmm.lib")

#define APP_NAME			L"躬学"
#define INI_PATH			L"./res/settings.ini"
#define IMG_BKGND_PATH		L"./res/bk.jpg"
#define IMG_TEXT_PATH		L"./res/text.png"
#define IMG_WATER_PATH		L"./res/water.bmp"
#define IMG_ZHAO_PATH		L"./res/老赵.png"
#define IMG_GANG_PATH		L"./res/刚哥.png"
#define TXT_ENCOURAGE_PATH	L"./res/encourage.txt"
#define TXT_PROMISE_PATH	L"./res/promise.txt"
#define TXT_CONTINUE_PATH	L"./res/continue.txt"
#define TXT_EXIT_PATH		L"./res/exit.txt"

#define IDC_MENU_ABOUT		101
#define IDC_MENU_OPTIONS	102

// 让钩子过程函数启动钩子
#define WM_HOOK_KEYBORAD	WM_USER + 100
#define WM_HOOK_MOUSE		WM_USER + 101
#define WM_UNHOOK_KEYBORAD	WM_USER + 102
#define WM_UNHOOK_MOUSE		WM_USER + 103

ScreenSize g_sizeScreen;
RECT g_rctWnd;

int g_nWorkMinutes;				// 最大连续工作时长
int g_nRestMinutes;				// 强制休息时长
int g_nLeaveMinutes;			// 判定离开所需时长

int g_nDrinkMinutes;			// 提醒喝水间隔时长

int g_nSleepHour;				// 睡觉时间，小时
int g_nSleepMin;				// 睡觉时间，分钟
int g_nSleepGrace;				// 睡觉提醒的宽限时间（分钟）

bool g_bEnableStudyMode;		// 是否启用学习模式
int g_nWakeHour;				// 学习模式下，起床时间，小时
int g_nWakeMin;					// 学习模式下，起床时间，分钟
int g_nRestHour;				// 学习模式下，可娱乐的开始时间，小时
int g_nRestMin;					// 学习模式下，可娱乐的开始时间，分钟

IMAGE g_imgIcon;
IMAGE g_imgBackground;			// 强制休息背景
IMAGE g_imgTextLearn;			// 躬学文字
IMAGE g_imgWater;				// 喝水
IMAGE g_imgZhao;				// 老赵
IMAGE g_imgGang;				// 刚哥

std::wstring g_wstrEncourage;	// 激励文字（较长的）
std::wstring g_wstrPromise;		// 解锁电脑的承诺文字
std::wstring g_wstrContinue;	// 激励我继续学习的文字（较短的）
std::wstring g_wstrExit;		// 退出学习模式的确认文字

// 替换字符串中的所有指定子串
std::wstring& replace_all(std::wstring& str, const std::wstring& old_value, const std::wstring& new_value)
{
	size_t sizeOld = old_value.length(), sizeNew = new_value.length(), pos = 0;
	while ((pos = str.find(old_value, pos + sizeNew)) != std::wstring::npos)
		str.replace(pos, sizeOld, new_value);
	return str;
}

// 读取文件
std::wstring ReadFile(const wchar_t* path)
{
	std::wifstream file(path);
	file.imbue(std::locale("chs"));
	std::wstring wstr((std::istreambuf_iterator<wchar_t>(file)), std::istreambuf_iterator<wchar_t>());
	file.close();
	return wstr;
}

// 初始化
void Init()
{
	g_nWorkMinutes = GetIniFileInfoInt(INI_PATH, L"eye", L"work_min", 40);
	g_nRestMinutes = GetIniFileInfoInt(INI_PATH, L"eye", L"rest_min", 3);
	g_nLeaveMinutes = GetIniFileInfoInt(INI_PATH, L"eye", L"leave_min", 3);

	g_nDrinkMinutes = GetIniFileInfoInt(INI_PATH, L"drink", L"interval", 60);

	g_nSleepHour = GetIniFileInfoInt(INI_PATH, L"sleep", L"hour", 22);
	g_nSleepMin = GetIniFileInfoInt(INI_PATH, L"sleep", L"min", 0);
	g_nSleepGrace = GetIniFileInfoInt(INI_PATH, L"sleep", L"grace", 15);

	g_bEnableStudyMode = GetIniFileInfoInt(INI_PATH, L"study", L"enable", 0);
	g_nWakeHour = GetIniFileInfoInt(INI_PATH, L"study", L"wake_hour", 6);
	g_nWakeMin = GetIniFileInfoInt(INI_PATH, L"study", L"wake_min", 40);
	g_nRestHour = GetIniFileInfoInt(INI_PATH, L"study", L"rest_hour", 20);
	g_nRestMin = GetIniFileInfoInt(INI_PATH, L"study", L"rest_min", 0);

	loadimage(&g_imgIcon, L"MYICON", MAKEINTRESOURCE(IDB_ICON1));
	loadimage(&g_imgBackground, IMG_BKGND_PATH);
	loadimage(&g_imgTextLearn, IMG_TEXT_PATH);
	loadimage(&g_imgWater, IMG_WATER_PATH);
	loadimage(&g_imgZhao, IMG_ZHAO_PATH);
	loadimage(&g_imgGang, IMG_GANG_PATH);

	g_wstrEncourage = ReadFile(TXT_ENCOURAGE_PATH);
	g_wstrPromise = ReadFile(TXT_PROMISE_PATH);
	g_wstrContinue = ReadFile(TXT_CONTINUE_PATH);
	g_wstrExit = ReadFile(TXT_EXIT_PATH);

	replace_all(g_wstrEncourage, L"\n", L"\r\n");
	replace_all(g_wstrPromise, L"\n", L"\r\n");
	replace_all(g_wstrContinue, L"\n", L"\r\n");
	replace_all(g_wstrExit, L"\n", L"\r\n");
}

// 更新学习模式状态到配置文件
void UpdateStudyModeState(bool enable)
{
	WriteIniFileInfoInt(INI_PATH, L"study", L"enable", enable);
}

// 响应消息（显示窗口）
void OnMessage(HWND hWnd)
{
	static bool flag = false;

	if (!flag)
	{
		flag = true;
		ShowWindow(hWnd, SW_SHOW);
		SetWindowPos(hWnd, HWND_TOPMOST, g_rctWnd.left, g_rctWnd.top, 0, 0, SWP_NOSIZE);

		clock_t time = clock();
		ExMessage msg;
		while (true)
		{
			while (peekmessage(&msg, EX_MOUSE, true, hWnd))
			{
				if (msg.message == WM_LBUTTONUP)
				{
					time = 0;	// 退出循环
				}
			}
			if (clock() - time > CLOCKS_PER_SEC * 5)
			{
				break;
			}
			Sleep(100);
		}
		ShowWindow(hWnd, SW_HIDE);
		flag = false;
	}
}

LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// 处理托盘消息
	case WM_TRAY:
		switch (lParam)
		{
			// 左键时显示窗口
		case WM_LBUTTONDOWN:
			std::thread(OnMessage, hWnd).detach();
			break;
		}
		break;

	case WM_CLOSE:
		break;

	default:
		return HIWINDOW_DEFAULT_PROC;
		break;
	}

	return 0;
}

// 创建顶部居中
HWND CreateTopCenterWindow(int _w, int _h, ScreenSize size)
{
	int w = size.w + size.left;	// 主屏幕大小
	int h = size.h + size.top;
	hiex::PreSetWindowPos((w - _w) / 2, 0);
	hiex::PreSetWindowStyle(WS_POPUP | WS_DLGFRAME);
	hiex::PreSetWindowShowState(SW_HIDE);
	return hiex::initgraph_win32(_w, _h, EW_NORMAL, APP_NAME, WndProc);
}

// 播放提示音
void Music(bool flagReverse = false)
{
	int music[] = { 89, 82 };
	if (flagReverse)
	{
		int temp = music[0];
		music[0] = music[1];
		music[1] = temp;
	}
	HMIDIOUT handle;
	midiOutOpen(&handle, 0, 0, 0, CALLBACK_NULL);
	midiOutShortMsg(handle, (0x007f << 16) + (music[0] << 8) + 0x90);
	Sleep(400);
	midiOutShortMsg(handle, (0x007f << 16) + (music[1] << 8) + 0x90);
	Sleep(5000);
	midiOutClose(handle);
}

void TopAlways(HWND hWnd)
{
	while (hiex::isAliveWindow(hWnd))
	{
		SetWindowPos(hWnd, HWND_TOPMOST, g_sizeScreen.left, g_sizeScreen.top, 0, 0, SWP_NOSIZE);
		Sleep(50);
	}
}

void outtextxy_shadow(int x, int y, LPCTSTR lpText, COLORREF cShadow)
{
	COLORREF c = gettextcolor();
	settextcolor(cShadow);
	outtextxy(x, y, lpText);
	settextcolor(c);
	outtextxy(x + 1, y + 1, lpText);
}

// 钩子窗口过程函数
LRESULT HookProc(HWND, UINT msg, WPARAM, LPARAM)
{
	static HHOOK hookKeyboard;
	static HHOOK hookMouse;

	// 设置一个钩子
	// type 0 - 键盘钩子，1 - 鼠标钩子
	static auto funcHook = [](int type)-> HHOOK
	{
		return SetWindowsHookEx(
			type ? WH_MOUSE_LL : WH_KEYBOARD_LL,
			[](int, WPARAM, LPARAM)->LRESULT { return 1; },
			GetModuleHandle(0),
			0
		);
		return nullptr;
	};

	// 解锁键盘
	static auto funcUnhookKeyboard = [&]()
	{
		if (hookKeyboard)
		{
			UnhookWindowsHookEx(hookKeyboard);
			hookKeyboard = nullptr;
		}
	};

	// 解锁鼠标
	static auto funcUnhookMouse = [&]()
	{
		if (hookMouse)
		{
			UnhookWindowsHookEx(hookMouse);
			hookMouse = nullptr;
		}
	};

	switch (msg)
	{
	case WM_HOOK_KEYBORAD:
		if (!hookKeyboard)
			hookKeyboard = funcHook(0);
		break;

	case WM_HOOK_MOUSE:
		if (!hookMouse)
		{
			hookMouse = funcHook(1);
			ShowCursor(false);
		}
		break;

	case WM_UNHOOK_KEYBORAD:
		funcUnhookKeyboard();
		break;

	case WM_UNHOOK_MOUSE:
		funcUnhookMouse();
		ShowCursor(true);
		break;

	case WM_DESTROY:
		funcUnhookKeyboard();
		funcUnhookMouse();
		ShowCursor(true);
		break;

		// 防止关闭窗口
	case WM_CLOSE:
		break;

	default:
		return HIWINDOW_DEFAULT_PROC;
		break;
	}

	return 0;
}

// 强制休息
void Rest()
{
	hiex::PreSetWindowPos(g_sizeScreen.left, g_sizeScreen.top);
	hiex::PreSetWindowStyle(WS_POPUP);
	HWND hWnd = hiex::initgraph_win32(
		g_sizeScreen.w,
		g_sizeScreen.h,
		EW_NORMAL,
		L"",
		HookProc
	);

	std::thread(TopAlways, hWnd).detach();

	// 锁死键盘鼠标
	SendMessage(hWnd, WM_HOOK_KEYBORAD, 0, 0);
	SendMessage(hWnd, WM_HOOK_MOUSE, 0, 0);

	IMAGE imgBk = hiex::ZoomImage_Alpha(&g_imgBackground, g_sizeScreen.w, g_sizeScreen.h);

	for (int i = 0; i < 60 * g_nRestMinutes; i++)
	{
		BEGIN_TASK_WND(hWnd);
		{
			setbkmode(TRANSPARENT);

			setorigin(0, 0);
			putimage(0, 0, &imgBk);
			putimage(g_sizeScreen.w - g_imgIcon.getwidth() - 10, g_sizeScreen.h - g_imgIcon.getheight() - 10, &g_imgIcon);

			setorigin(-g_sizeScreen.left, -g_sizeScreen.top);

			settextcolor(BLACK);
			settextstyle(110, 0, L"宋体");
			outtextxy(100, 100, L"请您休息一会。");

			settextcolor(YELLOW);
			settextstyle(36, 0, L"仿宋");
			outtextxy_shadow(100, 220, L"放松一下，工作更有效率。", BLACK);
			outtextxy_shadow(100, 270, L"看看窗外的风景吧~", BLACK);

			WCHAR buf[32] = { 0 };
			wsprintf(buf, L"%d", g_nRestMinutes - i / 60);

			settextcolor(DARKYELLOW);
			settextstyle(128, 0, L"仿宋");
			outtextxy(200, 360, buf);
			settextcolor(CLASSICGRAY);
			settextstyle(72, 0, L"仿宋");
			outtextxy(280, 400, L"分钟");
		}
		END_TASK();
		FLUSH_DRAW();

		Sleep(1000);
	}

	hiex::closegraph_win32(hWnd);
	Music(true);
}

// 创建学习模式的窗口
HWND CreateStudyWindow()
{
	hiex::PreSetWindowPos(g_sizeScreen.left, g_sizeScreen.top);
	hiex::PreSetWindowStyle(WS_POPUP);
	hiex::PreSetWindowStyleEx(WS_EX_TOPMOST);
	HWND hWnd = hiex::initgraph_win32(
		g_sizeScreen.w,
		g_sizeScreen.h,
		EW_NORMAL,
		L"",
		HookProc
	);

	// 锁键盘
	SendMessage(hWnd, WM_HOOK_KEYBORAD, 0, 0);

	// 窗口尺寸保护线程
	std::thread([](HWND hWnd) {
		while (hiex::isAliveWindow(hWnd))
		{
			SetWindowPos(hWnd, HWND_TOP, g_sizeScreen.left, g_sizeScreen.top, g_sizeScreen.w, g_sizeScreen.h, 0);
			Sleep(100);
		}
		}, hWnd).detach();

		return hWnd;
}

// 请求用户输入文字
// hWnd 传入学习模式窗口句柄
// 返回用户是否输入完成
bool RequestTypeText(HWND hWnd, std::wstring wstrUserText, std::wstring wstrBtnText, bool bEnableCancel)
{
	// 为了输入文本临时解封键盘
	SendMessage(hWnd, WM_UNHOOK_KEYBORAD, 0, 0);

	// 起始 y 位置
	int nY = 350;

	hiex::SysEdit editShowText;
	editShowText.PreSetStyle(true, false, true, true);
	editShowText.Create(hWnd, -g_sizeScreen.left + 100, -g_sizeScreen.top + nY, 400, 300);
	editShowText.ReadOnly(true);
	editShowText.Enable(false);
	editShowText.SetText(wstrUserText);

	hiex::SysEdit editInput;
	editInput.PreSetStyle(true, false, true, true);
	editInput.Create(hWnd, -g_sizeScreen.left + 510, -g_sizeScreen.top + nY, 400, 300);

	hiex::SysButton btnOK(hWnd, -g_sizeScreen.left + 800, -g_sizeScreen.top + nY + 310, 110, 50, wstrBtnText);
	hiex::SysButton btnCancel(hWnd, -g_sizeScreen.left + 680, -g_sizeScreen.top + nY + 310, 110, 50, L"取消");
	btnOK.Enable(false);
	if (!bEnableCancel)
		btnCancel.Enable(false);

	// 删除控件
	auto funcRemoveCtrls = [&]()
	{
		editShowText.Remove();
		editInput.Remove();
		btnOK.Remove();
		btnCancel.Remove();
	};

	bool bWantEscape = false;	// 想逃跑
	bool bDone = false;			// 输入完成

	while (true)
	{
		HWND hWndForeground = GetForegroundWindow();

		// 如果丢失焦点，抢回来（防止用户利用键盘解锁的机会）
		if (hWndForeground != hWnd)
		{
			//CloseWindow(hWndForeground);
			SetForegroundWindow(hWnd);
			SetFocus(editInput.GetHandle());
			bWantEscape = true;
		}
		else if (bWantEscape)
		{
			bWantEscape = false;

			// 重新锁一下键鼠，再放开（防止用户逃跑）
			SendMessage(hWnd, WM_HOOK_MOUSE, 0, 0);
			SendMessage(hWnd, WM_HOOK_KEYBORAD, 0, 0);
			Sleep(1000);
			SendMessage(hWnd, WM_UNHOOK_KEYBORAD, 0, 0);
			SendMessage(hWnd, WM_UNHOOK_MOUSE, 0, 0);
		}

		// 输入完成
		if (!bDone && editInput.GetText() == wstrUserText)
		{
			SendMessage(hWnd, WM_HOOK_KEYBORAD, 0, 0);
			editInput.Enable(false);
			btnOK.Enable(true);

			bDone = true;
		}

		if (btnOK.isClicked())
		{
			SendMessage(hWnd, WM_HOOK_KEYBORAD, 0, 0);
			funcRemoveCtrls();
			return true;
		}
		if (btnCancel.isClicked())
		{
			SendMessage(hWnd, WM_HOOK_KEYBORAD, 0, 0);
			funcRemoveCtrls();
			return false;
		}

		Sleep(50);
	}
}

// 响应菜单消息
void OnMenu(UINT uId)
{
	static HWND hWndAbout = NULL;	// 关于窗口
	static HWND hWndOptions = NULL;	// 选项窗口

	switch (uId)
	{
		// "关于" 按钮
	case IDC_MENU_ABOUT:
	{
		// 若前一个窗口还未关闭
		if (hWndAbout && hiex::isAliveWindow(hWndAbout))
		{
			SetForegroundWindow(hWndAbout);
			return;
		}
		else
		{
			hWndAbout = hiex::initgraph_win32(480, 400, EW_NOMINIMIZE, L"关于 躬学系统");
			BEGIN_TASK_WND(hWndAbout);
			{
				setbkcolor(CLASSICGRAY);
				cleardevice();
				settextcolor(BLACK);
				putimage(20, 20, &g_imgIcon);
				settextstyle(40, 0, L"黑体");
				outtextxy(110, 10, L"躬学系统");
				settextstyle(18, 0, L"system");
				outtextxy(110, 60, L"作者：邹汇东 <mailhuid@163.com>");
				outtextxy(110, 80, L"网站：huidong.xyz");
				outtextxy(110, 130, L"2022.12.02 献给奋斗中的我");

				float fZoomRatio = (float)getwidth() / g_imgTextLearn.getwidth();
				IMAGE imgText = hiex::ZoomImage_Alpha(
					&g_imgTextLearn,
					(int)(g_imgTextLearn.getwidth() * fZoomRatio),
					(int)(g_imgTextLearn.getheight() * fZoomRatio)
				);
				putimage(0, 180, &imgText);
			}
			END_TASK();
			FLUSH_DRAW();
		}
		break;
	}

	case IDC_MENU_OPTIONS:
	{
		// 若前一个窗口还未关闭
		if (hWndOptions && hiex::isAliveWindow(hWndOptions))
		{
			SetForegroundWindow(hWndOptions);
			return;
		}
		else
		{
			hWndOptions = hiex::initgraph_win32(480, 400, EW_NOMINIMIZE, L"躬学 设置");

			// 获取按钮文字
			bool bEnable = g_bEnableStudyMode;	// 不能直接在全局变量上设置，否则会生效
			auto funcGetBtnText = [&]()->std::wstring {
				std::wstring wstrBtnText;
				if (!bEnable)
					wstrBtnText = L"关闭学习模式";
				else
					wstrBtnText = L"开启学习模式";
				return wstrBtnText;
			};

			hiex::SysButton btnStudy(hWndOptions, 20, 20, 100, 40, funcGetBtnText());
			hiex::SysButton btnExit(hWndOptions, 20, 65, 100, 40, L"退出程序");
			hiex::SysStatic text(hWndOptions, 130, 30, 200, 30, L"需要重启程序才能生效");
			text.Enable(false);

			while (hiex::isAliveWindow(hWndOptions))
			{
				if (btnStudy.isClicked())
				{
					// 此处不直接设置，需要重启程序才能生效。（否则会导致一些问题，懒得改了）
					bEnable = !bEnable;
					//g_bEnableStudyMode = !g_bEnableStudyMode;
					UpdateStudyModeState(bEnable);
					btnStudy.SetText(funcGetBtnText());
				}
				if (btnExit.isClicked())
				{
					if (MessageBox(hWndOptions, L"真的很重要，需要退出程序吗？请您三思", L"提示", MB_OKCANCEL) == IDOK)
					{
						exit(0);
					}
				}
				Sleep(100);
			}
		}
		break;
	}

	}
}

int main()
{
	Init();
	g_sizeScreen = GetScreenSize();
	hiex::SetCustomIcon(MAKEINTRESOURCE(IDI_ICON1), MAKEINTRESOURCE(IDI_ICON1));
	int w = 340, h = 140;
	HWND hWndMessage = CreateTopCenterWindow(w, h, g_sizeScreen);	// 顶上的消息窗口
	GetWindowRect(hWndMessage, &g_rctWnd);

	// 创建托盘，绑定菜单
	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, IDC_MENU_OPTIONS, L"选项");
	AppendMenu(hMenu, MF_GRAYED, 0, 0);
	AppendMenu(hMenu, MF_STRING, IDC_MENU_ABOUT, L"关于");
	hiex::CreateTray(APP_NAME);
	hiex::SetTrayMenu(hMenu);
	hiex::SetTrayMenuProcFunc(OnMenu);

	// 学习模式的窗口
	HWND hWndStudy = nullptr;
	if (g_bEnableStudyMode)
	{
		hWndStudy = CreateStudyWindow();
	}

	BEGIN_TASK();
	{
		setbkcolor(CLASSICGRAY);
		setbkmode(TRANSPARENT);
		cleardevice();
	}
	END_TASK();
	FLUSH_DRAW();

	/////////////// 初始化结束 /////////////////

	clock_t cWorkingRecord = clock();		// 开始工作的时刻
	clock_t cStayRecord = clock();			// 鼠标开始停留的时刻
	clock_t cWaterRecord = clock();			// 上一次提醒喝水的时刻

	POINT ptCursorOld = { 0,0 };			// 记录鼠标位置
	POINT ptCursor = { 0,0 };				// 记录鼠标位置
	bool flagStay = false;					// 标记鼠标不动

	bool flagWelcome = false;				// 标记绘制“欢迎回来”
	clock_t cWelcomeRecord = 0;				// 开始显示欢迎文字的时刻
	int nLeaveMinutes = 0;					// 离开时长

	bool flagPreTip = false;				// 标记绘制预提醒文字
	clock_t cPreTipRecord = 0;				// 开始显示预提醒文字的时刻
	bool flagAlreadyPreTip = false;			// 标记已经预提醒过

	bool flagWater = false;					// 标记绘制提醒喝水文字
	clock_t cWaterTipRecord = 0;			// 开始绘制提醒喝水文字的时刻

	bool flagSleep = false;					// 标记绘制提醒睡觉文字（到睡觉点了）
	clock_t cSleepWaitRecord = 0;			// 开始等待睡觉的时刻（超额后强制关机）

	HWND hWndSleepCountdown = nullptr;		// 睡觉等待倒计时窗口

	////// 以下是学习模式的变量 //////

	bool bCheckClockIn = false;				// 是否起床打卡（无论是否准时）

	bool bTimer = false;					// 是否选了计时器
	bool bStart = false;					// 计时器是否开始了（暂停是另一码事）
	bool bPause = false;					// 计时器是否暂停
	clock_t cTimerStartRecord = 0;			// 计时器的开始时刻
	clock_t cTimerPauseRecord = 0;			// 计时器开始暂停的时刻
	clock_t cTimerEndRecord = 0;			// 计时器结束的时刻
	int nTimerMinutes = 0;					// 计时器时长

	bool bUsePC = false;					// 是否正在使用电脑
	clock_t cUsePCRecord = 0;				// 什么时候开始用的电脑
	int nUsePCLimitMin = 0;					// 能用几分钟电脑
	HWND hWndUsePCReminder = nullptr;		// 使用电脑时的提示框

	bool bHaveReminedToContinue = false;	// 是否已经提醒我继续学习（开始休息之后）
	clock_t cRestRecord = 0;				// 开始休息（即没有计时或请求使用电脑）的时刻

	while (true)
	{
		clock_t cNow = clock();
		SYSTEMTIME stSystime;
		GetLocalTime(&stSystime);

		// 鼠标不动，开始计时，时长超过阈值后可判定为离开
		GetCursorPos(&ptCursor);
		if (ptCursor.x == ptCursorOld.x && ptCursor.y == ptCursorOld.y)
		{
			if (!flagStay)
			{
				flagStay = true;
				cStayRecord = clock();
			}
		}
		else
		{
			flagStay = false;
			ptCursorOld = ptCursor;
		}

		int nWorkTime = (cNow - cWorkingRecord) / CLOCKS_PER_SEC / 60;		// 工作时长
		int nStayTime = (cNow - cStayRecord) / CLOCKS_PER_SEC / 60;			// 鼠标停留时长
		int nWaterTime = (cNow - cWaterRecord) / CLOCKS_PER_SEC / 60;		// 距离上次喝水的时长

		// 学习模式下或等待睡觉时不可强制休息
		if (!g_bEnableStudyMode && !flagSleep)
		{
			// 工作时间超额，需要休息
			if (nWorkTime >= g_nWorkMinutes)
			{
				std::thread(Music, false).detach();
				Sleep(2000);
				Rest();

				// 休息已结束，等待回来
				while (ptCursor.x == ptCursorOld.x && ptCursor.y == ptCursorOld.y)
				{
					GetCursorPos(&ptCursor);
					Sleep(500);
				}

				// 重新计时，显示欢迎文字
				nLeaveMinutes = (clock() - cNow) / CLOCKS_PER_SEC / 60 + g_nRestMinutes;
				cWorkingRecord = clock();
				flagWelcome = true;
				cWelcomeRecord = clock();
				flagAlreadyPreTip = false;

				std::thread(OnMessage, hWndMessage).detach();
			}

			// 休息前一分钟，预提示
			else if (nWorkTime >= g_nWorkMinutes - 1 && !flagAlreadyPreTip)
			{
				flagPreTip = true;
				cPreTipRecord = clock();
				std::thread(OnMessage, hWndMessage).detach();
				flagAlreadyPreTip = true;
			}
		}

		// 鼠标不动超时，说明已经离开（学习模式下不判断）
		if (!g_bEnableStudyMode && flagStay && nStayTime >= g_nLeaveMinutes)
		{
			// 等待回来
			while (ptCursor.x == ptCursorOld.x && ptCursor.y == ptCursorOld.y)
			{
				GetCursorPos(&ptCursor);
				Sleep(500);
			}

			// 重新计时，显示欢迎文字
			flagStay = false;
			cWorkingRecord = clock();

			flagWelcome = true;
			cWelcomeRecord = clock();
			nLeaveMinutes = (clock() - cStayRecord) / CLOCKS_PER_SEC / 60;

			// 如果正在等待睡觉，则刚才离开的时间不算为睡觉等待时间
			if (flagSleep)
			{
				// 将开始等待时刻后移，抵消刚才的离开时间
				cSleepWaitRecord += clock() - cStayRecord;
			}

			std::thread(OnMessage, hWndMessage).detach();
		}

		// 喝水提醒
		if (nWaterTime >= g_nDrinkMinutes)
		{
			flagWater = true;
			cWaterRecord = clock();
			cWaterTipRecord = clock();

			std::thread(OnMessage, hWndMessage).detach();
		}

		// 睡觉时间到了
		if (!flagSleep)
		{
			if (stSystime.wHour > g_nSleepHour ||
				(stSystime.wHour == g_nSleepHour && stSystime.wMinute >= g_nSleepMin))
			{
				flagSleep = true;
				cSleepWaitRecord = clock();

				std::thread(OnMessage, hWndMessage).detach();

				// 创建一个倒计时窗口
				hiex::PreSetWindowStyle(WS_POPUP | WS_DLGFRAME);
				hiex::PreSetWindowStyleEx(WS_EX_TOPMOST);
				hWndSleepCountdown = hiex::initgraph_win32(300, 100);

				// 窗口过程
				auto funcWndProc = [&]()
				{
					BEGIN_TASK_WND(hWndSleepCountdown);
					{
						settextstyle(32, 0, L"宋体");
					}
					END_TASK();

					MOUSEMSG msg;
					while (true)
					{
						// 允许窗口拖动
						while (PeekMouseMsg(&msg, true, hWndSleepCountdown))
						{
							switch (msg.uMsg)
							{
							case WM_LBUTTONDOWN:
								PostMessage(hWndSleepCountdown, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(msg.x, msg.y));
								break;
							}
						}

						std::wstring wstrText;
						int nConsumedMinutes = (clock() - cSleepWaitRecord) / CLOCKS_PER_SEC / 60;
						int nRestMinutes = g_nSleepGrace - nConsumedMinutes;

						// 等待超时，强制关机
						if (nRestMinutes <= 0)
						{
							system("shutdown -s -f -t 30");
						}

						wstrText = std::to_wstring(nRestMinutes) + L" 分钟后关机";

						BEGIN_TASK_WND(hWndSleepCountdown);
						{
							cleardevice();
							outtextxy(30, 30, wstrText.c_str());
						}
						END_TASK();
						FLUSH_DRAW();

						Sleep(100);
					}
				};

				std::thread(funcWndProc).detach();
			}
		}

		//////// 学习模式的处理 ////////
		if (g_bEnableStudyMode)
		{
			// 若尚未创建窗口，则现在创建
			if (!hWndStudy)
			{
				hWndStudy = CreateStudyWindow();
			}

			// 不能丢失焦点
			if (GetForegroundWindow() != hWndStudy)
			{
				SetForegroundWindow(hWndStudy);

				// 可能钩子坏了，重新锁一下
				SendMessage(hWndStudy, WM_UNHOOK_KEYBORAD, 0, 0);
				SendMessage(hWndStudy, WM_HOOK_KEYBORAD, 0, 0);
			}

			// 到解锁（娱乐）时间了
			if (stSystime.wHour > g_nRestHour ||
				(stSystime.wHour == g_nRestHour && stSystime.wMinute >= g_nRestMin))
			{
				if (MessageBox(hWndStudy,
					L"休息时间到了，接下来可以休息了。\r\n点击取消延长一个小时学习时间。",
					L"提示",
					MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
				{
					g_bEnableStudyMode = false;
					SendMessage(hWndStudy, WM_UNHOOK_KEYBORAD, 0, 0);
					SendMessage(hWndStudy, WM_UNHOOK_MOUSE, 0, 0);
					hiex::closegraph_win32(hWndStudy);
					//ShowWindow(hWndStudy, SW_HIDE);

					// 记一下开始工作的时间
					cWorkingRecord = clock();

					continue;
				}
				else
				{
					g_nRestHour = stSystime.wHour + 1;
					g_nRestMin = stSystime.wMinute;
				}
			}

			// 当前页面的控件集合
			static std::vector<hiex::SysControlBase*> vecCtrl;

			// 退出
			static hiex::SysButton btnExit;

			// 关机
			static hiex::SysButton btnShutdown;

			// 使用电脑的按钮
			static const int nUsePCTimeTypeNum = 7;
			static int pnUsePCMinutes[nUsePCTimeTypeNum] = { 3,5,10,15,20,40,50 };	// 使用电脑的时间
			static hiex::SysButton pbtnStudy[nUsePCTimeTypeNum];

			// 计时器
			static const int nTimerNum = 14;
			static int pnTimerMinutes[nTimerNum] = { 5,10,15,20,25,30,35,40,45,50,60,70,80,120 };
			static hiex::SysButton pbtnTimer[nTimerNum];

			// 计时器的暂停 / 继续
			static hiex::SysButton btnToggle;

			// 取消当前计时器
			static hiex::SysButton btnCancel;

			// 完成当前计时
			static hiex::SysButton btnOK;

			// 设置所有控件的显示状态
			auto funcShowAllControls = [&](bool show)
			{
				for (auto& pCtrl : vecCtrl)
				{
					pCtrl->Show(show);
				}
			};

			// 更新切换按钮的文字
			auto funcUpdateToggleText = [&]()
			{
				// 切换按钮的文字设置
				if (bTimer && bStart)
				{
					if (bPause)
					{
						btnToggle.SetText(L"继续");
					}
					else
					{
						btnToggle.SetText(L"暂停");
					}
				}
				else
				{
					btnToggle.SetText(L"开始");
				}
			};

			// 还没起床打卡（一运行程序就算打卡），说明这是首次进入窗口
			if (!bCheckClockIn)
			{
				bCheckClockIn = true;

				BEGIN_TASK_WND(hWndStudy);
				{
					setbkcolor(RGB(180, 180, 180));
					settextstyle(100, 0, L"宋体");
					cleardevice();

					setorigin(-g_sizeScreen.left, -g_sizeScreen.top);
				}
				END_TASK();

				// 准时打卡
				if (stSystime.wHour < g_nWakeHour
					|| (stSystime.wHour == g_nWakeHour && stSystime.wMinute <= g_nWakeMin))
				{
					BEGIN_TASK_WND(hWndStudy);
					{
						settextcolor(GREEN);
						//outtextxy(50, 30, L"成功早起，奋战学习！");
					}
					END_TASK();
				}
				else
				{
					BEGIN_TASK_WND(hWndStudy);
					{
						settextcolor(RED);
						//outtextxy(50, 30, L"起晚了，没关系，现在努力！");
					}
					END_TASK();
				}
				FLUSH_DRAW();

				RequestTypeText(hWndStudy, g_wstrEncourage, L"进入", false);

				////////// 创建控件 //////////
				btnExit.Create(hWndStudy, -g_sizeScreen.left, -g_sizeScreen.top, 120, 50, L"退出");
				btnShutdown.Create(hWndStudy, g_sizeScreen.w - 120, -g_sizeScreen.top, 120, 50, L"关机");

				for (int i = 0; i < nUsePCTimeTypeNum; i++)
				{
					pbtnStudy[i].Create(
						hWndStudy,
						-g_sizeScreen.left,
						-g_sizeScreen.top + 60 + i * 40,
						250, 40,
						L"【用会电脑】--" + std::to_wstring(pnUsePCMinutes[i]) + L" 分钟"
					);
				}

				for (int i = 0; i < nTimerNum; i++)
				{
					pbtnTimer[i].Create(
						hWndStudy,
						-g_sizeScreen.left + i * 80,
						-g_sizeScreen.top + 350,
						80, 40,
						L"【" + std::to_wstring(pnTimerMinutes[i]) + L" 分钟】"
					);
				}

				btnToggle.Create(hWndStudy, -g_sizeScreen.left, -g_sizeScreen.top + 400, 120, 50, L"开始");
				btnCancel.Create(hWndStudy, -g_sizeScreen.left + 120, -g_sizeScreen.top + 400, 120, 50, L"取消");
				btnOK.Create(hWndStudy, -g_sizeScreen.left + 240, -g_sizeScreen.top + 400, 120, 50, L"完成");

				// 登记控件
				vecCtrl.push_back(&btnExit);
				vecCtrl.push_back(&btnShutdown);
				for (int i = 0; i < nUsePCTimeTypeNum; i++)
					vecCtrl.push_back(&pbtnStudy[i]);
				for (int i = 0; i < nTimerNum; i++)
					vecCtrl.push_back(&pbtnTimer[i]);
				vecCtrl.push_back(&btnToggle);
				vecCtrl.push_back(&btnCancel);
				vecCtrl.push_back(&btnOK);
			}

			// 提前绘制的部分
			BEGIN_TASK_WND(hWndStudy);
			{
				int nMainWidth = g_sizeScreen.left + g_sizeScreen.w;
				setbkcolor(RGB(20, 20, 20));
				cleardevice();
				putimage((nMainWidth - g_imgTextLearn.getwidth()) / 2, 0, &g_imgTextLearn);

				//putimage(200, 800, &g_imgGang);
				//putimage(300, 800, &g_imgZhao);
			}
			END_TASK();

			////////// 处理按键 //////////

			// 退出学习模式
			if (btnExit.isClicked())
			{
				funcShowAllControls(false);
				if (RequestTypeText(hWndStudy, g_wstrExit, L"退出", true))
				{
					hiex::closegraph_win32(hWndStudy);
					g_bEnableStudyMode = false;

					// 记一下开始工作的时间
					cWorkingRecord = clock();
				}
				else
				{
					funcShowAllControls(true);
				}
			}

			// 关机
			if (btnShutdown.isClicked())
			{
				system("shutdown -s -f -t 0");
			}

			// 请求用电脑学习
			for (int i = 0; i < nUsePCTimeTypeNum; i++)
			{
				// 点到按钮了
				if (pbtnStudy[i].isClicked())
				{
					// 先问一下自己，是不是真的学习
					funcShowAllControls(false);
					if (RequestTypeText(hWndStudy, g_wstrPromise, L"用电脑", true))
					{
						bUsePC = true;
						cUsePCRecord = clock();
						nUsePCLimitMin = pnUsePCMinutes[i];

						// 先让学习模式窗口歇一歇
						ShowWindow(hWndStudy, SW_HIDE);
						SendMessage(hWndStudy, WM_UNHOOK_KEYBORAD, 0, 0);

						// 创建一个倒计时窗口
						hiex::PreSetWindowStyle(WS_POPUP | WS_DLGFRAME);
						hiex::PreSetWindowStyleEx(WS_EX_TOPMOST);
						hWndUsePCReminder = hiex::initgraph_win32(320, 110);

						hiex::SysButton btnOK(hWndUsePCReminder, 220, 70, 80, 30, L"OK 了~");

						hiex::SysStatic text(hWndUsePCReminder, 10, 80, 200, 30, L"水平可胜发际线？");
						text.Enable(false);

						// 接下来死循环，直到用完电脑。
						MOUSEMSG msg;
						while (true)
						{
							// 允许窗口拖动
							while (PeekMouseMsg(&msg, true, hWndUsePCReminder))
							{
								switch (msg.uMsg)
								{
								case WM_LBUTTONDOWN:
									PostMessage(hWndUsePCReminder, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(msg.x, msg.y));
									break;
								}
							}

							std::wstring wstrText;
							int nConsumedMinutes = (clock() - cUsePCRecord) / CLOCKS_PER_SEC / 60;
							int nRestMinutes = nUsePCLimitMin - nConsumedMinutes;

							// 超时了，回来，或者手动点击按钮结束上网
							if (nRestMinutes <= 0 || btnOK.isClicked())
							{
								hiex::closegraph_win32(hWndUsePCReminder);
								break;	// 退出死循环
							}

							wstrText = L"还可使用 " + std::to_wstring(nRestMinutes) + L" 分钟";

							BEGIN_TASK_WND(hWndUsePCReminder);
							{
								cleardevice();
								settextstyle(32, 0, L"仿宋");
								outtextxy(30, 30, wstrText.c_str());
							}
							END_TASK();
							FLUSH_DRAW();

							Sleep(100);
						}

						SendMessage(hWndStudy, WM_HOOK_KEYBORAD, 0, 0);
						ShowWindow(hWndStudy, SW_SHOW);
						SetForegroundWindow(hWndStudy);
					}
					funcShowAllControls(true);
					break;
				}
			}

			// 启用计时器
			for (int i = 0; i < nTimerNum; i++)
			{
				if (pbtnTimer[i].isClicked())
				{
					bTimer = true;	// 选了计时器
					nTimerMinutes = pnTimerMinutes[i];
					break;
				}
			}

			// 切换状态
			if (btnToggle.isClicked())
			{
				if (bTimer)
				{
					if (bStart)
					{
						if (bPause)
						{
							bPause = false;			// 继续了
							// 抵消掉暂停消耗的时间
							cTimerStartRecord += clock() - cTimerPauseRecord;
						}
						else
						{
							bPause = true;			// 暂停了
							cTimerPauseRecord = clock();
						}
					}
					else
					{
						bStart = true;				// 选好计时器之后，开始了
						cTimerStartRecord = clock();
					}
				}
				funcUpdateToggleText();
			}

			// 取消
			if (btnCancel.isClicked())
			{
				bTimer = false;
				bStart = false;
				bPause = false;
				cTimerStartRecord = 0;
				cTimerPauseRecord = 0;
				cTimerEndRecord = 0;
				nTimerMinutes = 0;
				funcUpdateToggleText();
			}

			// 完成计时
			if (btnOK.isClicked() && bStart)
			{
				bTimer = true;
				bStart = false;
				cTimerEndRecord = clock();
				funcUpdateToggleText();

				// 完成计时后，算是开始休息了
				//// 开始休息 ////
				cRestRecord = clock();
				bHaveReminedToContinue = false;
			}

			// 完成了计时，现在没有计时，且五分钟没有任何动作，要提醒一下我继续学习
			// 如果提醒过一次就算了。
			if (cRestRecord && !bStart && !bHaveReminedToContinue && clock() - cRestRecord >= CLOCKS_PER_SEC * 60 * 5)
			{
				// 已经提醒过我要继续学习了
				bHaveReminedToContinue = true;

				funcShowAllControls(false);
				RequestTypeText(hWndStudy, g_wstrContinue, L"嗯。", false);
				funcShowAllControls(true);
			}

			//////// 绘制剩余部分 ////////
			BEGIN_TASK_WND(hWndStudy);
			{
				// 时刻显示
				settextstyle(22, 0, L"Consolas");
				settextcolor(WHITE);
				std::wstring wstrClock =
					std::to_wstring(stSystime.wMonth) + L" 月 " + std::to_wstring(stSystime.wDay) + L" 日 "
					+ std::to_wstring(stSystime.wHour) + L" : " + std::to_wstring(stSystime.wMinute);
				outtextxy(140, 20, wstrClock.c_str());

				// 计时器部分
				{
					int nY = 530;			// 开始绘制 Y 坐标
					int nTotalLength = 600;	// 进度条总长度
					int nWidth = 60;		// 进度条宽度

					// 进度条颜色
					COLORREF colorNormal = GREEN;
					COLORREF colorPause = YELLOW;
					COLORREF colorOut = RED;		// 超时颜色

					clock_t cNow;			// 当前时间
					bool bDone = false;		// 标记是否完成一次计时
					if (bTimer)
					{
						if (!bStart)
						{
							cNow = cTimerEndRecord;
							if (cTimerEndRecord)	// 如果未开始，但是有结束时刻的记录，说明已经完成一次计时
								bDone = true;
						}
						else if (bPause)
							cNow = cTimerPauseRecord;
						else
							cNow = clock();
					}
					else
					{
						cNow = cTimerEndRecord;
					}
					clock_t cTotal = nTimerMinutes * CLOCKS_PER_SEC * 60;						// 总时间
					clock_t cUsed = cNow - cTimerStartRecord;									// 已用去时间
					clock_t cLast = cTotal - cUsed;												// 还剩下的时间
					int nUsedMin = cUsed / CLOCKS_PER_SEC / 60;									// 用去的分钟
					int nUsedSec = (cUsed - nUsedMin * CLOCKS_PER_SEC * 60) / CLOCKS_PER_SEC;	// 用去的秒
					int nLastMin = cLast / CLOCKS_PER_SEC / 60;									// 剩下的分钟
					int nLastSec = (cLast - nLastMin * CLOCKS_PER_SEC * 60) / CLOCKS_PER_SEC;	// 剩下的秒

					float fRate = cTotal ? (float)cUsed / cTotal : 0;
					int nLen = (int)((fRate > 1 ? 1 : fRate) * nTotalLength);	// 进度条长度

					std::wstring wstrTimer = L"计时时长：" + std::to_wstring(nTimerMinutes) + L" 分钟" + (bDone ? L"（已完成）" : L"");
					std::wstring wstrConsumption = L"已计时：" + std::to_wstring(nUsedMin) + L"′" + std::to_wstring(nUsedSec) + L"″";
					std::wstring wstrLast = L"还剩：" + std::to_wstring(nLastMin) + L"′" + std::to_wstring(nLastSec) + L"″";

					int nTextHeight = 48;
					settextstyle(nTextHeight, 0, L"仿宋");
					outtextxy(0, nY, wstrTimer.c_str());
					outtextxy(0, nY + nTextHeight, wstrConsumption.c_str());
					outtextxy(0, nY + nTextHeight * 2, wstrLast.c_str());

					int nProgressX = 10;
					int nProgressY = nY + nTextHeight * 3 + 20;

					setlinecolor(WHITE);
					if (bPause)
					{
						setfillcolor(colorPause);
					}
					else if (cLast < 0)
					{
						setfillcolor(colorOut);
					}
					else
					{
						setfillcolor(colorNormal);
					}
					solidrectangle(nProgressX, nProgressY, nProgressX + nLen, nProgressY + nWidth);
					rectangle(nProgressX, nProgressY, nProgressX + nTotalLength, nProgressY + nWidth);
				}

				// 显示多久未操作
				if (!bStart && cRestRecord)
				{
					int nRestMin = (clock() - cRestRecord) / CLOCKS_PER_SEC / 60;
					settextstyle(40, 0, L"仿宋");
					std::wstring wstrRest = std::to_wstring(nRestMin) + L" 分钟未操作";
					outtextxy(380, 410, wstrRest.c_str());
				}

				// 显示名言
				int nX = 640;
				settextstyle(28, 0, L"仿宋");
				std::wstring wstrWisdom[1] = {
					L"实力要对得起发际线",
				};
				outtextxy(nX, 420, wstrWisdom[0].c_str());
			}
			END_TASK();
			FLUSH_DRAW();
		}

		//** 绘制小消息窗口 **//
		BEGIN_TASK_WND(hWndMessage);
		{
			int nTimeTextYOffset = 20;		// 输出工作时长文字的 y 轴位移
			cleardevice();
			putimage(20, 20, &g_imgIcon);

			settextstyle(32, 0, L"仿宋");
			if (flagWelcome)	// 欢迎文字
			{
				// 超时关闭
				if ((cNow - cWelcomeRecord) / CLOCKS_PER_SEC >= 7)
				{
					flagWelcome = false;
				}
				outtextxy(100, 10, L"您回来了。");
				nTimeTextYOffset = 30;
			}
			else if (flagPreTip)	// 预提醒文字
			{
				// 超时关闭
				if ((cNow - cPreTipRecord) / CLOCKS_PER_SEC >= 7)
				{
					flagPreTip = false;
				}
				outtextxy(100, 10, L"快休息了。");
				nTimeTextYOffset = 30;
			}
			else if (flagWater)		// 提醒喝水文字
			{
				// 超时关闭
				if ((cNow - cWaterTipRecord) / CLOCKS_PER_SEC >= 7)
				{
					flagWater = false;
				}
				putimage(20, 20, &g_imgWater);
				outtextxy(100, 10, L"请您喝口水吧。");
				nTimeTextYOffset = 30;
			}
			else if (flagSleep)
			{
				// 睡觉提醒开启后，一直保留这个提醒，直到睡觉等待结束后，电脑就会关闭
				outtextxy(100, 10, L"请您关机睡觉吧。");
				nTimeTextYOffset = 30;
			}

			WCHAR buf[32] = { 0 };
			wsprintf(buf, L"%d 分钟。", flagWelcome ? nLeaveMinutes : nWorkTime);

			settextstyle(22, 0, L"仿宋");
			outtextxy(100, 17 + nTimeTextYOffset, flagWelcome ? L"您已离开" : L"您已工作");
			settextstyle(30, 0, L"仿宋");
			outtextxy(100, 40 + nTimeTextYOffset, buf);
		}
		END_TASK();
		FLUSH_DRAW();

		Sleep(100);
	}

	hiex::closegraph_win32(hWndMessage);
	return 0;
}

