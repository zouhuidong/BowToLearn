#include "CursorTip.h"
#include "HiEasyX.h"

#define SPEED_THRESHOLD		2000

hiex::Window wnd;
const int nBoxLen = 100;

// get the speed of the mouse per sec.
float GetCursorSpeed(POINT p1, POINT p2, clock_t t1, clock_t t2)
{
	long dx = p2.x - p1.x;
	long dy = p2.y - p1.y;
	float ds = sqrtf((float)(dx * dx + dy * dy));
	clock_t dt = t2 - t1;
	float v = ds / (dt / (float)CLOCKS_PER_SEC);
	return v;
}

bool IsOverspeed(float speed)
{
	return speed >= SPEED_THRESHOLD;
}

void TipBoxSetup()
{
	wnd.PreSetStyle(WS_POPUP);
	wnd.PreSetShowState(SW_HIDE);
	wnd.Create(nBoxLen, nBoxLen);

	/*RECT rcClient, rcWind;
	GetClientRect(wnd.GetHandle(), &rcClient);
	GetWindowRect(wnd.GetHandle(), &rcWind);
	int cx = ((rcWind.right - rcWind.left) - rcClient.right) / 2;
	int cy = ((rcWind.bottom - rcWind.top + GetSystemMetrics(SM_CYCAPTION)) - rcClient.bottom) / 2;*/

	HRGN rgn = CreateEllipticRgn(0, 0, nBoxLen, nBoxLen);
	SetWindowRgn(wnd.GetHandle(), rgn, true);

	if (wnd.BeginTask())
	{
		setbkcolor(RED);
		cleardevice();
		wnd.EndTask();
		FLUSH_DRAW();
	}
}

void ShowTipBox(POINT pt)
{
	ShowWindow(wnd.GetHandle(), SW_SHOW);
	SetWindowPos(wnd.GetHandle(), HWND_TOPMOST, pt.x - nBoxLen / 2, pt.y - nBoxLen / 2, 0, 0, SWP_NOSIZE);
}

void HideTipBox()
{
	ShowWindow(wnd.GetHandle(), SW_HIDE);
}

void Process(POINT pt, float speed)
{
	//OutputDebugString((std::to_wstring(speed) + L"\n").c_str());

	static int count = 0;
	static bool hover = false;

	const int threshold_count = 6;

	// count
	if (IsOverspeed(speed))
	{
		count++;
	}
	else if (count > 0)
	{
		count--;
	}

	// action
	if (count > threshold_count)
	{
		// gave user a free hover for seconds.
		if (!hover)
		{
			count = threshold_count + 10;
			hover = true;
		}

		ShowTipBox(pt);
	}
	else
	{
		HideTipBox();
		hover = false;
	}

	//OutputDebugString((std::to_wstring(count) + L"---\n").c_str());
}

void CursorTipProcess()
{
	static bool is_firstrun = true;
	static POINT ptCursorRecord;
	static clock_t cTimeRecord;

	// update data
	POINT ptCursor = {};
	clock_t cTime;
	GetCursorPos(&ptCursor);
	cTime = clock();

	if (is_firstrun)
	{
		TipBoxSetup();

		is_firstrun = false;
		ptCursorRecord = ptCursor;
		cTimeRecord = cTime;
		return;
	}

	// process
	float fSpeed = GetCursorSpeed(ptCursorRecord, ptCursor, cTimeRecord, cTime);
	Process(ptCursor, fSpeed);

	ptCursorRecord = ptCursor;
	cTimeRecord = cTime;
}
