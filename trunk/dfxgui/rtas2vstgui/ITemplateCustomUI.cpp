#include "ITemplateCustomUI.h"
#include "CTemplateCustomUI.h"


//-----------------------------------------------------------------------------
ITemplateCustomUI * CreateCTemplateCustomUI(ITemplateProcess * processPtr)
{
	return new CTemplateCustomUI(processPtr);
}

//-----------------------------------------------------------------------------
ITemplateCustomUI::ITemplateCustomUI(void * processPtr)
:	m_process_ptr(processPtr)
{
}

//-----------------------------------------------------------------------------
ITemplateCustomUI::~ITemplateCustomUI()
{
}
