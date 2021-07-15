#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <csignal>

#include "Client.h"
#include "Config.h"

#include "DeckLinkAPI.h"
#include "zmq.hpp"
#include <opencv2/opencv.hpp>


static pthread_mutex_t	g_sleepMutex;
static pthread_cond_t	g_sleepCond;
static bool				g_do_exit = false;

static BMDConfig		g_config;

static IDeckLinkInput*	g_deckLinkInput = nullptr;

static unsigned long	g_frameCount = 0;

zmq::context_t context(1);
zmq::socket_t publisher(context, ZMQ_PUB);
std::string              topic;


DeckLinkCaptureDelegate::DeckLinkCaptureDelegate() : 
	m_refCount(1),
	m_pixelFormat(g_config.m_pixelFormat)
{
}

ULONG DeckLinkCaptureDelegate::AddRef()
{
	return __sync_add_and_fetch(&m_refCount, 1);
}

ULONG DeckLinkCaptureDelegate::Release()
{
	int32_t newRefValue = __sync_sub_and_fetch(&m_refCount, 1);
	if (newRefValue == 0)
	{
		delete this;
		return 0;
	}
	return newRefValue;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame)
{
	IDeckLinkVideoFrame*				rightEyeFrame = nullptr;
	IDeckLinkVideoFrame3DExtensions*	threeDExtensions = nullptr;
	void* data;
    char request [1];

	if (videoFrame)
	{
		// If 3D mode is enabled we retrieve the 3D extensions interface which gives.
		// us access to the right eye frame by calling GetFrameForRightEye() .
		if ( (videoFrame->QueryInterface(IID_IDeckLinkVideoFrame3DExtensions, (void **) &threeDExtensions) != S_OK) ||
			(threeDExtensions->GetFrameForRightEye(&rightEyeFrame) != S_OK))
		{
			rightEyeFrame = nullptr;
		}

		if (threeDExtensions)
			threeDExtensions->Release();

		if (videoFrame->GetFlags() & bmdFrameHasNoInputSource)
		{
			printf("Frame received (#%lu) - No input signal detected\n", g_frameCount);
		}
		else
		{
			printf("Frame received (#%lu) - %s - Size: %li bytes\n",
				g_frameCount,
				rightEyeFrame != nullptr ? "Valid Frame (3D left/right)" : "Valid Frame",
				videoFrame->GetRowBytes() * videoFrame->GetHeight());

            const auto width = int(videoFrame->GetWidth());
            const auto height = int(videoFrame->GetHeight());

            if (m_pixelFormat != bmdFormat8BitYUV)
            {
                IDeckLinkMutableVideoFrame* m_videoFrame8BitYUV;
                IDeckLinkVideoConversion* frameConverter = CreateVideoConversionInstance();
                frameConverter->ConvertFrame(videoFrame, m_videoFrame8BitYUV);
                m_videoFrame8BitYUV->GetBytes(&data);
            }
            else
            {
                videoFrame->GetBytes(&data);
            }

            cv::Mat uyvy(height, width, CV_8UC2, data);
            cv::UMat image(height, width, CV_8UC1);
            cvtColor(uyvy, image, cv::COLOR_YUV2RGB_UYVY);
            std::vector <uchar> buffer;
            cv::imencode(".jpg", image, buffer);

            publisher.send(topic.data(), topic.size(), ZMQ_SNDMORE);
            publisher.send(buffer.data(), buffer.size());
		}
		g_frameCount++;
	}

	return S_OK;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode, BMDDetectedVideoInputFormatFlags formatFlags)
{
	// This only gets called if bmdVideoInputEnableFormatDetection was set
	// when enabling video input
	HRESULT	result;
	char*	displayModeName = nullptr;
	BMDPixelFormat	pixelFormat = m_pixelFormat;
	
	if (events & bmdVideoInputColorspaceChanged)
	{
		// Detected a change in colorspace, change pixel format to match detected format
		if (formatFlags & bmdDetectedVideoInputRGB444)
			pixelFormat = bmdFormat10BitRGB;
		else if (formatFlags & bmdDetectedVideoInputYCbCr422)
			pixelFormat = (g_config.m_pixelFormat == bmdFormat8BitYUV) ? bmdFormat8BitYUV : bmdFormat10BitYUV;
		else
			goto bail;
	}

	// Restart streams if either display mode or pixel format have changed
	if ((events & bmdVideoInputDisplayModeChanged) || (m_pixelFormat != pixelFormat))
	{
		mode->GetName((const char**)&displayModeName);
		printf("Video format changed to %s %s\n", displayModeName, formatFlags & bmdDetectedVideoInputRGB444 ? "RGB" : "YUV");

		if (displayModeName)
			free(displayModeName);

		if (g_deckLinkInput)
		{
			g_deckLinkInput->StopStreams();

			result = g_deckLinkInput->EnableVideoInput(mode->GetDisplayMode(), pixelFormat, g_config.m_inputFlags);
			if (result != S_OK)
			{
				fprintf(stderr, "Failed to switch video mode\n");
				goto bail;
			}

			g_deckLinkInput->StartStreams();
		}

		m_pixelFormat = pixelFormat;
	}

bail:
	return S_OK;
}

static void sigfunc(int signum)
{
	if (signum == SIGINT || signum == SIGTERM)
		g_do_exit = true;

	pthread_cond_signal(&g_sleepCond);
}

int main(int argc, char *argv[])
{
    HRESULT							result;
	int								exitStatus = 1;

	IDeckLink*						deckLink = nullptr;

	IDeckLinkProfileAttributes*		deckLinkAttributes = nullptr;
	bool							formatDetectionSupported;
	int64_t							duplexMode;

	IDeckLinkDisplayMode*			displayMode = nullptr;
	char*							displayModeName = nullptr;
	bool							supported;

	DeckLinkCaptureDelegate*		delegate = nullptr;

	pthread_mutex_init(&g_sleepMutex, nullptr);
	pthread_cond_init(&g_sleepCond, nullptr);

	signal(SIGINT, sigfunc);
	signal(SIGTERM, sigfunc);
	signal(SIGHUP, sigfunc);

	// Process the command line arguments
	if (!g_config.ParseArguments(argc, argv))
	{
		g_config.DisplayUsage(exitStatus);
		goto bail;
	}

	// get topic information and bind to specified port
	topic= g_config.SetTopicPublisher();
    publisher.bind(g_config.SetAddressZMQ().c_str());



    // Get the DeckLink device
	deckLink = g_config.GetSelectedDeckLink();
	if (deckLink == nullptr)
	{
		fprintf(stderr, "Unable to get DeckLink device %u\n", g_config.m_deckLinkIndex);
		goto bail;
	}

	result = deckLink->QueryInterface(IID_IDeckLinkProfileAttributes, (void**)&deckLinkAttributes);
	if (result != S_OK)
	{
		fprintf(stderr, "Unable to get DeckLink attributes interface\n");
		goto bail;
	}

	// Check the DeckLink device is active
	result = deckLinkAttributes->GetInt(BMDDeckLinkDuplex, &duplexMode);
	if ((result != S_OK) || (duplexMode == bmdDuplexInactive))
	{
		fprintf(stderr, "The selected DeckLink device is inactive\n");
		goto bail;
	}

	// Get the input (capture) interface of the DeckLink device
	result = deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&g_deckLinkInput);
	if (result != S_OK)
	{
		fprintf(stderr, "The selected device does not have an input interface\n");
		goto bail;
	}

	// Get the display mode
	if (g_config.m_displayModeIndex == -1)
	{
		// Check the card supports format detection
		result = deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &formatDetectionSupported);
		if (result != S_OK || !formatDetectionSupported)
		{
			fprintf(stderr, "Format detection is not supported on this device\n");
			goto bail;
		}

		g_config.m_inputFlags |= bmdVideoInputEnableFormatDetection;
	}

	displayMode = g_config.GetSelectedDeckLinkDisplayMode(deckLink);

	if (displayMode == nullptr)
	{
		fprintf(stderr, "Unable to get display mode %d\n", g_config.m_displayModeIndex);
		goto bail;
	}

	// Get display mode name
	result = displayMode->GetName((const char**)&displayModeName);
	if (result != S_OK)
	{
		displayModeName = (char *)malloc(32);
		snprintf(displayModeName, 32, "[index %d]", g_config.m_displayModeIndex);
	}

	// Check display mode is supported with given options
	result = g_deckLinkInput->DoesSupportVideoMode(bmdVideoConnectionUnspecified, displayMode->GetDisplayMode(),
                                                   g_config.m_pixelFormat, bmdNoVideoInputConversion,
                                                   bmdSupportedVideoModeDefault, nullptr, &supported);
	if (result != S_OK)
		goto bail;

	if (! supported)
	{
		fprintf(stderr, "The display mode %s is not supported with the selected pixel format\n", displayModeName);
		goto bail;
	}

	if (g_config.m_inputFlags & bmdVideoInputDualStream3D)
	{
		if (!(displayMode->GetFlags() & bmdDisplayModeSupports3D))
		{
			fprintf(stderr, "The display mode %s is not supported with 3D\n", displayModeName);
			goto bail;
		}
	}

	// Print the selected configuration
	g_config.DisplayConfiguration();

	// Configure the capture callback
	delegate = new DeckLinkCaptureDelegate();
	g_deckLinkInput->SetCallback(delegate);





	// Block main thread until signal occurs
	while (!g_do_exit)
	{
		// Start capturing
		result = g_deckLinkInput->EnableVideoInput(displayMode->GetDisplayMode(), g_config.m_pixelFormat, g_config.m_inputFlags);
		if (result != S_OK)
		{
			fprintf(stderr, "Failed to enable video input. Is another application using the card?\n");
			goto bail;
		}

		result = g_deckLinkInput->StartStreams();
		if (result != S_OK)
			goto bail;

		// All Okay.
		exitStatus = 0;

		pthread_mutex_lock(&g_sleepMutex);
		pthread_cond_wait(&g_sleepCond, &g_sleepMutex);
		pthread_mutex_unlock(&g_sleepMutex);

		fprintf(stderr, "Stopping Client\n");
		g_deckLinkInput->StopStreams();
		g_deckLinkInput->DisableAudioInput();
		g_deckLinkInput->DisableVideoInput();
	}

bail:


	if (displayModeName != nullptr)
		free(displayModeName);

	if (displayMode != nullptr)
		displayMode->Release();

	if (delegate != nullptr)
		delegate->Release();

	if (g_deckLinkInput != nullptr)
	{
		g_deckLinkInput->Release();
		g_deckLinkInput = nullptr;
	}

	if (deckLinkAttributes != nullptr)
		deckLinkAttributes->Release();

	if (deckLink != nullptr)
		deckLink->Release();

	return exitStatus;
}
