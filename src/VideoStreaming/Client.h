#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include <string>
#include "DeckLinkAPI.h"

class DeckLinkCaptureDelegate : public IDeckLinkInputCallback
{
public:
	explicit DeckLinkCaptureDelegate(int d, const std::string& t);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) override { return E_NOINTERFACE; }
	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE  Release() override;
	HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags) override;
	HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*) override;

private:
    std::string topic;
    int device;
	int32_t				m_refCount;
	BMDPixelFormat		m_pixelFormat;
};

#endif
