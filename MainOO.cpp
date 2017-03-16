#include  "basewin.h"
#include <d2d1.h>
#pragma comment(lib, "d2d1")

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

class MainWindow : public BaseWindow<MainWindow>
{
	ID2D1Factory            *pFactory;
	ID2D1HwndRenderTarget   *pRenderTarget;
	ID2D1SolidColorBrush    *pBrush;
	ID2D1SolidColorBrush    *pStroke;
	D2D1_ELLIPSE            ellipse;

	void    CalculateLayout();
	HRESULT CreateGraphicsResources();
	void    DiscardGraphicsResources();
	void    OnPaint();
	void    Resize();
	void    RenderScene();
	void    DrawClockHand(float fHandLength, float fAngle, float fStrokeWidth);
public:
	MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL)
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
		const float x = size.width / 2;
		const float y = size.height / 2;
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
	switch (uMsg)
	{
	case WM_DESTROY:
		DiscardGraphicsResources();
		SafeRelease(&pFactory);
		PostQuitMessage(0);
		return 0;
	case WM_CREATE:
		if (FAILED(D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))

		{
			return -1;  // Fail CreateWindowEx.
		}
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
		PostMessage(m_hwnd, WM_PAINT, NULL, NULL);
		return 0;

		
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}
