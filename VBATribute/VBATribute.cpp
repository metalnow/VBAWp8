#include "pch.h"
#include "VBATribute.h"
#include "BasicTimer.h"
#include <ppl.h>
#include <ppltasks.h>
#include "vba\VBA.h"

#include <string>

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace concurrency;

VBATribute::VBATribute() :
	m_windowClosed(false),
	m_windowVisible(true),
	m_currentOrientation(DisplayOrientations::Portrait)
{
}

void VBATribute::Initialize(CoreApplicationView^ applicationView)
{
	applicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &VBATribute::OnActivated);

	CoreApplication::Suspending +=
		ref new EventHandler<SuspendingEventArgs^>(this, &VBATribute::OnSuspending);

	CoreApplication::Resuming +=
		ref new EventHandler<Platform::Object^>(this, &VBATribute::OnResuming);

	m_renderer = ref new VBARenderer();
	//    m_location = Package::Current->InstalledLocation \\Assets\\;
//	Platform::String^ file = Windows::ApplicationModel::Package::Current->InstalledLocation->Path + "\\Assets\\Zelda - A Link to the Past.gba";
	Platform::String^ localfolder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
	localfolder += "\\Zelda - A Link to the Past.gba";
	std::wstring fooW(localfolder->Begin());
	std::string fooA(fooW.begin(), fooW.end());
	theVBA.FileRun(fooA.c_str());
}

void VBATribute::SetWindow(CoreWindow^ window)
{
	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &VBATribute::OnVisibilityChanged);

	window->Closed += 
		ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &VBATribute::OnWindowClosed);

	window->PointerPressed +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &VBATribute::OnPointerPressed);

	window->PointerMoved +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &VBATribute::OnPointerMoved);

	window->PointerReleased +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &VBATribute::OnPointerReleased);

	m_renderer->Initialize(CoreWindow::GetForCurrentThread());
	/*
	Windows::UI::Core::CoreWindow^ externalHandle = reinterpret_cast<Windows::UI::Core::CoreWindow^>(CoreWindow::GetForCurrentThread());
	theVBA.display->initialize( (void*)externalHandle );
	*/
}

void VBATribute::OnOrientationChanged(Platform::Object^ sender)
{
	m_currentOrientation = DisplayProperties::CurrentOrientation;
	switch (m_currentOrientation)
	{
	case Windows::Graphics::Display::DisplayOrientations::None:
		break;
	case Windows::Graphics::Display::DisplayOrientations::Landscape:
		m_renderer->setOrientation(orientLandscape);
		break;
	case Windows::Graphics::Display::DisplayOrientations::Portrait:
		m_renderer->setOrientation(orientPortrait);
		break;
	case Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped:
		m_renderer->setOrientation(orientLandscapeFlipped);
		break;
	case Windows::Graphics::Display::DisplayOrientations::PortraitFlipped:
		m_renderer->setOrientation(orientPortraitFlipped);
		break;
	default:
		break;
	}
}

void VBATribute::Load(Platform::String^ entryPoint)
{
}

void VBATribute::Run()
{
	BasicTimer^ timer = ref new BasicTimer();

	while (!m_windowClosed)
	{
		if (m_windowVisible)
		{
			timer->Update();
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			for ( int i=0; i < 128; ++i )
			{
				theVBA.render();
			}
			
			m_renderer->Update(timer->Total, timer->Delta);
			m_renderer->Render();
			m_renderer->Present(); // This call is synchronized to the display frame rate.
			
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

void VBATribute::Uninitialize()
{
}

void VBATribute::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void VBATribute::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

void VBATribute::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
	// Insert your code here.
}

void VBATribute::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
	// Insert your code here.
}

void VBATribute::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
	// Insert your code here.
}

void VBATribute::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	CoreWindow::GetForCurrentThread()->Activate();
	DisplayProperties::OrientationChanged += ref new DisplayPropertiesEventHandler(this, &VBATribute::OnOrientationChanged);
}

void VBATribute::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();
	m_renderer->ReleaseResourcesForSuspending();

	create_task([this, deferral]()
	{
		// Insert your code here.

		deferral->Complete();
	});
}
 
void VBATribute::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.
	m_renderer->CreateWindowSizeDependentResources();
}

IFrameworkView^ Direct3DApplicationSource::CreateView()
{
	return ref new VBATribute();
}

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto direct3DApplicationSource = ref new Direct3DApplicationSource();
	CoreApplication::Run(direct3DApplicationSource);
	return 0;
}