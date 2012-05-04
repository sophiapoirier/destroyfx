#ifndef CTEMPLATECUSTOMUI_H_
#define CTEMPLATECUSTOMUI_H_


#if WINDOWS_VERSION
	#define VC_EXTRALEAN	// Exclude rarely-used stuff from Windows headers
#endif

#include "ITemplateCustomUI.h"
#include "ITemplateProcess.h"
#include "dfxgui.h"


#if WINDOWS_VERSION
	// Callback to Win32 Plug-in window - processes Windows messages.
	LRESULT CALLBACK TemplateMainWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	#define WINDOWCLASSNAME	"TemplateLocal"

	extern void * gThisModule;

	//--------------------------------------------------------------------
	//	Wraps the Register/Unregister calls for the window class. 
	//  A static instance of this class is part of the CTemplateCustomUI 
	//  class, ensuring that the class is only unregistered after all 
	//	instances are destroyed.
	class CWindowClassRegistrar
	{
	public:
		CWindowClassRegistrar()
		:	m_LocalWindowID(0)
		{
			memset(&m_LocalWinClass, 0, sizeof(WNDCLASSEX));
		}

		void RegisterWindowClass()
		{
			if (m_LocalWindowID == 0)
			{
				// register our local plug-in window class
				m_LocalWinClass.cbSize			= sizeof(WNDCLASSEX);
				m_LocalWinClass.style			= CS_HREDRAW | CS_VREDRAW;
				m_LocalWinClass.lpfnWndProc		= TemplateMainWindow;
				m_LocalWinClass.hInstance		= (HINSTANCE) gThisModule;
				m_LocalWinClass.hIcon			= LoadIcon (NULL, IDI_APPLICATION) ;
				m_LocalWinClass.hCursor			= LoadCursor (NULL, IDC_ARROW) ;
				m_LocalWinClass.lpszMenuName	= NULL ;
				m_LocalWinClass.hbrBackground	= CreateSolidBrush(0);
				m_LocalWinClass.lpszClassName	= WINDOWCLASSNAME;

				m_LocalWindowID = RegisterClassEx(&m_LocalWinClass);
				if(!m_LocalWindowID)
				{
					DWORD err = GetLastError();
				}
			}
		}

		~CWindowClassRegistrar()
		{
			// a DLL must unregister any window class it creates
			if (m_LocalWindowID)
				UnregisterClass((LPCTSTR)m_LocalWinClass.lpszClassName, m_LocalWinClass.hInstance);
		}

	private:
		WNDCLASSEX	m_LocalWinClass;	// Window class struct for local window
		ATOM		m_LocalWindowID;
	};
#endif

//--------------------------------------------------------------------
//	CTemplateCustomUI class
//
//	Inherits from ITemplateCustomUI interface class. It is the main 
//	class for the Custom UI for the plug-in. 
//
//	VSTGUI: If the VSTGUI library were structured differently, this 
//	class would have included the contents of DfxGuiEditor class. 
//	However, it would have made the separation of the Mac2Win and 
//	Windows difficult, so this class instead uses the DfxGuiEditor 
//	class.
class CTemplateCustomUI : public ITemplateCustomUI
{
#if WINDOWS_VERSION
	friend LRESULT CALLBACK TemplateMainWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

public:
	CTemplateCustomUI(ITemplateProcess * processPtr);
	virtual ~CTemplateCustomUI();

	virtual bool Open(void * winPtr, short left, short top);
	virtual bool Close();
	virtual long UpdateGraphicControl(long index, long value);
	virtual void Idle();
	virtual void Draw(long left, long top, long right, long bottom);
	virtual void GetRect(short * left, short * top, short * right, short * bottom);
	virtual void SetRect(short left, short top, short right, short bottom);
	ITemplateProcess * GetProcessPtr()
		{	return (ITemplateProcess*)m_process_ptr;	}
	virtual void SetControlHighlight(long controlIndex, short isHighlighted, short color);
	virtual void GetControlIndexFromPoint(long x, long y, long * aControlIndex);

	// VSTGUI: functions specific to using the VST GUI library
#if MAC_VERSION
	virtual long MouseCommmand(long x, long y);
#endif

#if WINDOWS_VERSION
	HWND GetParentHWND()
		{	return m_PluginWindow;	}
#endif

	// Destroy FX interface
//	virtual void resetClipDetectorLights()
//		{	if (m_VSTEditor_p != NULL) m_VSTEditor_p->resetClipDetectorLights();	}


protected:

#if WINDOWS_VERSION
	static CWindowClassRegistrar m_WindowClassRegistrar;

	HWND		m_PluginWindow;		// main window created by Pro Tools
	HWND		m_LocalPIWin;		// sub-window that has local message loop

	WNDCLASSEX	m_LocalWinClass;	// Window class struct for local window
#elif MAC_VERSION
	WindowRef	m_LocalPIWin;		// On Mac, no sub-window is required, this is the main window created by Pro Tools
#endif

	sRect		m_PIRect;			// size of local rect

	// VSTGUI: data specific to using the VST GUI library
	DfxGuiEditor	* m_VSTEditor_p;
};


#endif
