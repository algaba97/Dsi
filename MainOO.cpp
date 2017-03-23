#include  "basewin.h"
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include "winuser.h"
#include <vector>
using namespace std;


template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

float g_DPIScaleX = 1.0f;
float g_DPIScaleY = 1.0f;

void InitializeDPIScale(ID2D1Factory *pFactory)
{
	FLOAT dpiX, dpiY;

	pFactory->GetDesktopDpi(&dpiX, &dpiY);

	g_DPIScaleX = dpiX / 96.0f;
	g_DPIScaleY = dpiY / 96.0f;
}
class DPIScale
{
	static float scaleX;
	static float scaleY;

public:
	static void Initialize(ID2D1Factory *pFactory)
	{
		FLOAT dpiX, dpiY;
		pFactory->GetDesktopDpi(&dpiX, &dpiY);
		scaleX = dpiX / 96.0f;
		scaleY = dpiY / 96.0f;
	}

	template <typename T>
	static D2D1_POINT_2F PixelsToDips(T x, T y)
	{
		return D2D1::Point2F(static_cast<float>(x) / scaleX, static_cast<float>(y) / scaleY);
	}
};

float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;

template <typename T>
float PixelsToDipsX(T x)
{
	return static_cast<float>(x) / g_DPIScaleX;
}

template <typename T>
float PixelsToDipsY(T y)
{
	return static_cast<float>(y) / g_DPIScaleY;
}
enum class ClockMode {Run,Stop};
class MainWindow : public BaseWindow<MainWindow>
{
	ID2D1Factory            *pFactory;
	ID2D1HwndRenderTarget   *pRenderTarget;
	ID2D1SolidColorBrush    *pBrush;
	ID2D1SolidColorBrush    *pBrush2;
	ID2D1SolidColorBrush    *pStroke;
	D2D1_ELLIPSE            ellipse;
	D2D1_ELLIPSE            ellipse2;
	D2D1_POINT_2F           ptMouse;
	 D2D1_COLOR_F colorNuevo;
	 ClockMode reloj = ClockMode::Run;
	


	void    CalculateLayout();
	void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
	void    OnLButtonUp();
	void    OnMouseMove(int pixelX, int pixelY, DWORD flags);
	HRESULT CreateGraphicsResources();
	void    DiscardGraphicsResources();
	void    OnPaint();
	void    Resize();
	void    RenderScene();
	void    DrawClockHand(float fHandLength, float fAngle, float fStrokeWidth);
	bool HitTest(D2D1_ELLIPSE ellipseconsulta,float x, float y)
	{
		const float a = ellipseconsulta.radiusX;
		const float b =ellipseconsulta.radiusY;
		const float x1 = x - ellipseconsulta.point.x;
		const float y1 = y - ellipseconsulta.point.y;
		const float d = ((x1 * x1) / (a * a)) + ((y1 * y1) / (b * b));
		return d <= 1.0f;
	}
public:
	MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL), pBrush2(NULL), ellipse(D2D1::Ellipse(D2D1::Point2F(), 0, 0)), ellipse2(D2D1::Ellipse(D2D1::Point2F(), 0, 0)), ptMouse(D2D1::Point2F())
		
	{
		
		
	}

	PCWSTR  ClassName() const { return L"Circle Window Class"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

void MainWindow::DrawClockHand(float fHandLength, float fAngle, float fStrokeWidth)
{
        pRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Rotation(fAngle, ellipse.point)
		);

	// endPoint defines one end of the hand.
	D2D_POINT_2F endPoint = D2D1::Point2F(
		ellipse.point.x,
		ellipse.point.y - (ellipse.radiusY * fHandLength)
		);

	// Draw a line from the center of the ellipse to endPoint.
	pRenderTarget->DrawLine(
		ellipse.point, endPoint, pStroke, fStrokeWidth);
}

void MainWindow::RenderScene()
{
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));

	pRenderTarget->FillEllipse(ellipse, pBrush);
	pRenderTarget->DrawEllipse(ellipse, pBrush);

	pRenderTarget->FillEllipse(ellipse2, pBrush2);
	pRenderTarget->DrawEllipse(ellipse2, pBrush2);
	// Draw hands
	SYSTEMTIME time;
	GetLocalTime(&time);

	// 60 minutes = 30 degrees, 1 minute = 0.5 degree
	const float fHourAngle = (360.0f / 12) * (time.wHour) + (time.wMinute * 0.5f);
	const float fMinuteAngle = (360.0f / 60) * (time.wMinute);
	const float fSecAngle = (360.0f / 60) * (time.wSecond);

	DrawClockHand(0.6f, fHourAngle, 6);
	DrawClockHand(0.85f, fMinuteAngle, 4);
	DrawClockHand(0.93f, fSecAngle, 3);

	// Restore the identity transformation.
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}
// Recalculate drawing layout when the size of the window changes.
void MainWindow::CalculateLayout()
{
	if (pRenderTarget != NULL)
	{
		D2D1_SIZE_F size = pRenderTarget->GetSize();
		const float x = size.width / 8;
		const float y = size.height / 8;
		const float radius = min(x, y);
		ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
	}
}

HRESULT MainWindow::CreateGraphicsResources()
{
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		hr = pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size),
			&pRenderTarget);

		if (SUCCEEDED(hr))
		{

			const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
			hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
		
			hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush2);
			const D2D1_COLOR_F color1 = D2D1::ColorF(1.0f, 1.0f, 1.0f);
			hr = pRenderTarget->CreateSolidColorBrush(color1, &pStroke);

			if (SUCCEEDED(hr))
			{
				CalculateLayout();
			}
		}
	}
	return hr;
}

void MainWindow::DiscardGraphicsResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&pBrush);
}

void MainWindow::OnPaint()
{
	HRESULT hr = CreateGraphicsResources();
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);

		pRenderTarget->BeginDraw();
		RenderScene();
		//pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
		//pRenderTarget->FillEllipse(ellipse, pBrush);

		hr = pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			DiscardGraphicsResources();
		}
		EndPaint(m_hwnd, &ps);
	}
}

void MainWindow::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
	SetCapture(m_hwnd);
	ellipse2.point = ptMouse = DPIScale::PixelsToDips(pixelX, pixelY);
	ellipse2.radiusX = ellipse2.radiusY = 1.0f;
	InvalidateRect(m_hwnd, NULL, FALSE);
}

void MainWindow::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
	if (flags & MK_LBUTTON)
	{
		const D2D1_POINT_2F dips = DPIScale::PixelsToDips(pixelX, pixelY);

		const float width = (dips.x - ptMouse.x) / 2;
		const float height = (dips.y - ptMouse.y) / 2;
		const float x1 = ptMouse.x + width;
		const float y1 = ptMouse.y + height;

		ellipse2 = D2D1::Ellipse(D2D1::Point2F(x1, y1), width, height);

		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}

void MainWindow::OnLButtonUp()
{
	ReleaseCapture();
}


void MainWindow::Resize()
{
	if (pRenderTarget != NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		pRenderTarget->Resize(size);
		CalculateLayout();
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	MainWindow win;

	if (!win.Create(L"Learn to Program Windows", WS_OVERLAPPEDWINDOW))
	{
		return 0;
	}

	ShowWindow(win.Window(), nCmdShow);

	// Run the message loop.

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int GET_X_LPARAM(
		LPARAM lParam
		);

	int GET_Y_LPARAM(
		LPARAM lParam
		);
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))

	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (wParam == ' ') {
			if (reloj == ClockMode::Run)reloj = ClockMode::Stop;
			else reloj = ClockMode::Run;
		}
		if (wParam == 27) {
			HandleMessage(WM_DESTROY, wParam, lParam);
		}
		return 0;
	case WM_DESTROY:
		DiscardGraphicsResources();
		SafeRelease(&pFactory);
		PostQuitMessage(0);
		KillTimer(m_hwnd, 0);
		return 0;
	case WM_CREATE:
		if (FAILED(D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
		{
			return -1;  // Fail CreateWindowEx.
		}
		DPIScale::Initialize(pFactory);
		SetTimer(m_hwnd, 1, 1000, (TIMERPROC)NULL);
		return 0;
	case WM_CLOSE:
		if (MessageBox(m_hwnd, L"Really quit?", L"My application", MB_OKCANCEL) == IDOK)
		{
			DestroyWindow(m_hwnd);
		}
		// Else: User canceled. Do nothing.
		return 0;

	case WM_PAINT:
	{
		OnPaint();
		return 0;
	}
	case WM_SIZE:
		Resize();
		return 0;
	case WM_TIMER: // process the 1-second timer
		if(reloj == ClockMode::Run)PostMessage(m_hwnd, WM_PAINT, NULL, NULL);
		return 0;

	case WM_LBUTTONDOWN:
		OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
		return 0;

	case WM_LBUTTONUP:
		OnLButtonUp();
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
		return 0;
	
	case WM_RBUTTONDOWN:
		CHOOSECOLOR cc; // common dialog box structure
		static COLORREF acrCustClr[16]; // array of custom colors
		static DWORD rgbCurrent; // initial color selection
		// Initialize CHOOSECOLOR
		ZeroMemory(&cc, sizeof(cc));
		cc.lStructSize = sizeof(cc);
		cc.hwndOwner = m_hwnd;
		cc.lpCustColors = (LPDWORD)acrCustClr;
		cc.rgbResult = rgbCurrent;
		cc.Flags = CC_FULLOPEN | CC_RGBINIT;
		if (ChooseColor(&cc) == TRUE)
		{
			colorNuevo = D2D1::ColorF(GetRValue(cc.rgbResult) / 255.0, GetGValue(cc.rgbResult) / 255.0, GetBValue(cc.rgbResult) /255.0f, 1.0f);
		   pRenderTarget->CreateSolidColorBrush(colorNuevo, &pBrush2);
			//D2D1::ColorF(1.0f, 1.0f, 0);
			//En ​cc.rgbResult​ tenemos el color seleccionado
			//Utilizarlo para configurar nuestra brocha
			//Es necesario transformarlo al formato de color de D2D
			
		
		}
		return 0;
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}
