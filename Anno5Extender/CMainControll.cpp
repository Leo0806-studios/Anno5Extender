#include "AddressTemplate.h"
#include "CMainControll.h"

A5Ext::AddressLibrary::Anno2070::Runtime::Application::CMainControll::CMainControll(Undefined8 param1)
{
	static AddressTemplate<0x571490, CMainControll(*)(CMainControll*, Undefined8)> instanceAddress;
	(*instanceAddress.Get())(this, param1);
	
}

A5Ext::AddressLibrary::Anno2070::Runtime::Application::CMainControll::~CMainControll()
{

}

void A5Ext::AddressLibrary::Anno2070::Runtime::Application::CMainControll::DelettingDestrictor()
{}

void A5Ext::AddressLibrary::Anno2070::Runtime::Application::CMainControll::ProcessGameJobBody()
{}
