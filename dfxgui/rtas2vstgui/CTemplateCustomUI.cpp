#include "CTemplateCustomUI.h"
#include "ConvertUtils.h"


#if WINDOWS_VERSION
void * hInstance;	// necessary definition for VST GUI framework

CWindowClassRegistrar CTemplateCustomUI::m_WindowClassRegistrar;

//==============================================================================
//	Win32 callback for UI window proc
//
//	Since the GUI framework handles the actual drawing and such, this doesn't 
//	need to do anything except pass on any unhandled messages to the parent
//	window's default window procedure.
//==============================================================================
LRESULT CALLBACK TemplateMainWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif	// WINDOWS_VERSION


//==============================================================================
//							CTemplateCustomUI class
//==============================================================================



extern DfxGuiEditor * DFXGUI_NewEditorInstance(ITemplateProcess * inProcessInstance);

//==============================================================================
CTemplateCustomUI::CTemplateCustomUI(ITemplateProcess * processPtr)
:	ITemplateCustomUI(processPtr),
	m_VSTEditor_p(NULL)
#if WINDOWS_VERSION
	, m_LocalPIWin(NULL), m_PluginWindow(NULL)
#elif MAC_VERSION
	, m_LocalPIWin(NULL)
#endif
{
	void * instance_temp = NULL;
	if (processPtr != NULL)
		instance_temp = processPtr->ProcessGetModuleHandle();	// necessary to load resources from DLL

#if WINDOWS_VERSION
	hInstance = (HINSTANCE) instance_temp;
	m_WindowClassRegistrar.RegisterWindowClass();
#endif

	m_VSTEditor_p = DFXGUI_NewEditorInstance(processPtr);
}

//==============================================================================
// make sure to unregister any window classes your UI registers with the system
//==============================================================================
CTemplateCustomUI::~CTemplateCustomUI()
{
	if (m_VSTEditor_p != NULL)
		delete m_VSTEditor_p;	// XXX this is not a double delete?
	m_VSTEditor_p = NULL;

#if WINDOWS_VERSION
	m_PluginWindow = NULL;
	m_LocalPIWin = NULL;
#endif
}

	
//==============================================================================
//	Gets size of window created by process.
//==============================================================================
void CTemplateCustomUI::GetRect(short * left, short * top, short * right, short * bottom)
{
	if (m_VSTEditor_p != NULL)
		m_VSTEditor_p->GetBackgroundRect(&m_PIRect);
	*left = (short)m_PIRect.left;
	*top = (short)m_PIRect.top;
	*right = (short)m_PIRect.right;
	*bottom = (short)m_PIRect.bottom;
}

//==============================================================================
//	Notify the custom UI of a new window size.
//==============================================================================
void CTemplateCustomUI::SetRect(short left, short top, short right, short bottom)
{
	sRect newRect;
	newRect.left = left;
	newRect.top = top;
	newRect.right = right;
	newRect.bottom = bottom;

	if (m_VSTEditor_p != NULL)
		m_VSTEditor_p->SetBackgroundRect(&newRect);	
}

//==============================================================================
//	Create window of registered window class - message pump is started.  
//	Open VSTGUI window as child of 
//==============================================================================
bool CTemplateCustomUI::Open(void * winPtr, short left, short top)
{
#if WINDOWS_VERSION
	m_PluginWindow = (HWND)winPtr;
	if (m_PluginWindow == NULL)
		return false;

	long PITop, PILeft, PIHeight, PIWidth;

	PITop = top;
	PILeft = left;

	PIHeight = m_PIRect.bottom - m_PIRect.top;
	PIWidth = m_PIRect.right - m_PIRect.left;

	// Create Window - starts message pump
	m_LocalPIWin = CreateWindowEx(
		0,				// extended class style
		WINDOWCLASSNAME,	// class name 
		"TemplateCustomUI",				// window name 
		WS_CHILD | WS_VISIBLE,		// window style
		PILeft,			// horizontal position of window
		PITop,			// vertical position of window
		PIWidth,		// window width
		PIHeight,		// window height
		m_PluginWindow,	// handle to parent or owner window
		NULL,	// menu handle or child identifier
		NULL,	// handle to application instance; ignored in NT/2000/XP
		NULL);	// window-creation data
		
	if (!m_LocalPIWin)
	{
		DWORD err = GetLastError();
		m_PluginWindow = NULL;
		throw err;
		return false;
	}

	ShowWindow(m_LocalPIWin, SW_SHOWNORMAL); 
	InvalidateRect(m_LocalPIWin, NULL, false);
	UpdateWindow(m_LocalPIWin);
#elif MAC_VERSION
	m_LocalPIWin = (WindowRef)winPtr;
#endif

	// VSTGUI: Open VST window as child of m_LocalPIWin
	if (m_VSTEditor_p != NULL)
	{
		bool ret = m_VSTEditor_p->open( (void*)m_LocalPIWin );
		return ret;	// XXX do this?
	}

	return true;
}

/*===============================================================================
//	The dimensions passed in are the current clipping region that should be drawn
//===============================================================================*/
void CTemplateCustomUI::Draw(long left, long top, long right, long bottom)
{
	// Removed platform specific code. un-needed and also fixes XP refresh issue - DLM
	ERect curRect = { top, left, bottom, right };
	m_VSTEditor_p->draw(&curRect);	// XXX deprecated now
}

/*===============================================================================
//	Mac only:  called by Process, passing in the point clicked
//===============================================================================*/
#if MAC_VERSION
long CTemplateCustomUI::MouseCommmand(long x, long y)
{
#if VSTGUI_ENABLE_DEPRECATED_METHODS
	CFrame * frame = m_VSTEditor_p->getFrame();
	CDrawContext context(frame, NULL, frame->getSystemWindow());
	CPoint where(x, y);
	frame->mouse( &context, where );
#endif
	return 0;
}
#endif

//==============================================================================
bool CTemplateCustomUI::Close()
{
	bool result = false;
	if (m_LocalPIWin != NULL)
	{
		if (m_VSTEditor_p != NULL)
			m_VSTEditor_p->close();
#if WINDOWS_VERSION
		result = DestroyWindow(m_LocalPIWin);
#endif
	}
	return result;
}

//==============================================================================
//	Called by plug-in process to notify UI of control update. Only here should 
//	the UI update itself
//==============================================================================
long CTemplateCustomUI::UpdateGraphicControl(long index, long value)
{
	if (m_VSTEditor_p != NULL)
		m_VSTEditor_p->setParameter( DFX_ParameterID_FromRTAS(index), ConvertToVSTValue(value) );
	return 0;	// no error
}

//==============================================================================
//	Called by plug-in process to notify UI that idle processing should be done
//==============================================================================
void CTemplateCustomUI::Idle()
{
	if (m_VSTEditor_p != NULL)
		m_VSTEditor_p->idle();
}

//==============================================================================
//	Called by plug-in process to notify UI that a control's highlight has been changed
//==============================================================================
void CTemplateCustomUI::SetControlHighlight(long controlIndex, short isHighlighted, short color)
{
	if (m_VSTEditor_p != NULL)
		m_VSTEditor_p->SetControlHighlight(controlIndex, isHighlighted, color);
}

//==============================================================================
//	Called by plug-in process to get the index of the control that contains the point
//==============================================================================
void CTemplateCustomUI::GetControlIndexFromPoint(long x, long y, long * aControlIndex)
{
	if (aControlIndex != NULL)
	{
		*aControlIndex = 0;
		if (m_VSTEditor_p != NULL)
			m_VSTEditor_p->GetControlIndexFromPoint(x, y, aControlIndex);
	}
}
