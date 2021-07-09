#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "Config.h"

////
#include "iostream"
////


BMDConfig::BMDConfig() :
	m_deckLinkIndex(-1),
	m_displayModeIndex(-2),
	m_inputFlags(bmdVideoInputFlagDefault),
	m_port(-3),
	m_pixelFormat(bmdFormat8BitYUV),
	m_deckLinkName(),
	m_displayModeName()
{
}

BMDConfig::~BMDConfig()
{
	if (m_deckLinkName)
		free(m_deckLinkName);

	if (m_displayModeName)
		free(m_displayModeName);
}

bool BMDConfig::ParseArguments(int argc,  char** argv)
{
	int		ch;
    char    *arg;

	while ((ch = getopt(argc, argv, "d:m:3:h:p")) != -1)
	{
		switch (ch)
		{
			case 'd':
				m_deckLinkIndex = int(strtol(optarg, &arg, 0));
				break;

			case 'm':
				m_displayModeIndex = int(strtol(optarg, &arg, 0));
				break;

			case '3':
				m_inputFlags |= bmdVideoInputDualStream3D;
				break;

		    case 'h':
                m_port = int(strtol(optarg, &arg, 0));
                break;

			case 'p':
				switch(int(strtol(optarg, &arg, 0)))
				{
					case 0: m_pixelFormat = bmdFormat8BitYUV; break;
					case 1: m_pixelFormat = bmdFormat10BitYUV; break;
					case 2: m_pixelFormat = bmdFormat10BitRGB; break;
					default:
						fprintf(stderr, "Invalid argument: Pixel format %d is not valid", int(strtol(optarg, &arg, 0)));
						return false;
				}
				break;
            default:
                DisplayUsage(1);
		}
	}

	if (m_deckLinkIndex < 0)
	{
		fprintf(stderr, "You must select a device\n");
		DisplayUsage(1);
	}

	if (m_port != 5005 and m_port !=5555)
	{
        printf("Wrong port : please use 5005 for LEFT and 5555 for right\n");
	}


	if (m_displayModeIndex < -1)
	{
		fprintf(stderr, "You must select a display mode\n");
		DisplayUsage(1);
	}

	// Get device and display mode names
	IDeckLink* deckLink = GetSelectedDeckLink();
	if (deckLink != nullptr)
	{
		if (m_displayModeIndex != -1)
		{
			IDeckLinkDisplayMode* displayMode = GetSelectedDeckLinkDisplayMode(deckLink);
			if (displayMode != nullptr)
			{
				displayMode->GetName((const char**)&m_displayModeName);
				displayMode->Release();
			}
			else
			{
				m_displayModeName = strdup("Invalid");
			}
		}
		else
		{
			m_displayModeName = strdup("Format Detection");
		}

		deckLink->GetDisplayName((const char**)&m_deckLinkName);
		deckLink->Release();
	}
	else
	{
		m_deckLinkName = strdup("Invalid");
	}

	return true;
}

IDeckLink* BMDConfig::GetSelectedDeckLink() const
{
	HRESULT				result;
	IDeckLink*			deckLink;
	IDeckLinkIterator*	deckLinkIterator = CreateDeckLinkIteratorInstance();
	int					i = m_deckLinkIndex;

	if (!deckLinkIterator)
	{
		fprintf(stderr, "This application requires the DeckLink drivers installed.\n");
		return nullptr;
	}

	while((result = deckLinkIterator->Next(&deckLink)) == S_OK)
	{
		IDeckLinkProfileAttributes*	deckLinkAttributes = nullptr;
		int64_t 					intAttribute;

		result = deckLink->QueryInterface(IID_IDeckLinkProfileAttributes, (void**)&deckLinkAttributes);
		if (result != S_OK)
		{
			deckLink->Release();
			break;
		}

		// Skip over devices that don't support capture
		if ((deckLinkAttributes->GetInt(BMDDeckLinkVideoIOSupport, &intAttribute) == S_OK) && 
			((intAttribute & bmdDeviceSupportsCapture) != 0))
		{
			if (i == 0)
				break;
			--i;
		}

		deckLinkAttributes->Release();
		deckLink->Release();
	}

	deckLinkIterator->Release();

	if (result != S_OK)
		return nullptr;

	return deckLink;
}

IDeckLinkDisplayMode* BMDConfig::GetSelectedDeckLinkDisplayMode(IDeckLink* deckLink) const
{
	HRESULT							result;
	IDeckLinkDisplayMode*			displayMode = nullptr;
	IDeckLinkInput*					deckLinkInput = nullptr;
	IDeckLinkDisplayModeIterator*	displayModeIterator = nullptr;

	result = deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput);
	if (result != S_OK)
		goto bail;

	if (m_displayModeIndex < 0)
	{
		// For format detection mode, use 1080p30 as default mode to start with
		result = deckLinkInput->GetDisplayMode(bmdModeHD1080p30, &displayMode);
		if (result != S_OK)
			goto bail;
	}
	else
	{
		int i = m_displayModeIndex;

		result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
		if (result != S_OK)
			goto bail;

		while ((displayModeIterator->Next(&displayMode)) == S_OK)
		{
			if (i == 0)
				break;
			--i;

			displayMode->Release();
			displayMode = nullptr;
		}
	}

bail:
	if (displayModeIterator)
		displayModeIterator->Release();

	if (deckLinkInput)
		deckLinkInput->Release();

	return displayMode;
}

void BMDConfig::DisplayUsage(int status) const
{
	HRESULT							result = E_FAIL;
    IDeckLinkIterator *deckLinkIterator;
    deckLinkIterator = CreateDeckLinkIteratorInstance();
	IDeckLinkDisplayModeIterator*	displayModeIterator = nullptr;

	IDeckLink*						deckLink = nullptr;
	IDeckLink*						deckLinkSelected = nullptr;
	int								deckLinkCount = 0;
	char*							deckLinkName = nullptr;

	IDeckLinkProfileAttributes*		deckLinkAttributes = nullptr;
	bool							formatDetectionSupported;

	IDeckLinkInput*					deckLinkInput = nullptr;
	IDeckLinkDisplayMode*			displayModeUsage;
	int								displayModeCount = 0;
	char*							displayModeName;

	fprintf(stderr,
		"Usage: Capture -d <device id> -m <mode id> [OPTIONS]\n"
		"\n"
		"    -d <device id>:\n"
	);

	// Loop through all available devices
	while (deckLinkIterator->Next(&deckLink) == S_OK)
	{
		bool deckLinkActive = false;
		bool deckLinkSupportsCapture = false;

		if (deckLink->QueryInterface(IID_IDeckLinkProfileAttributes, (void**)&deckLinkAttributes) == S_OK)
		{
			int64_t intAttribute;
			deckLinkActive = ((deckLinkAttributes->GetInt(BMDDeckLinkDuplex, &intAttribute) == S_OK) && 
								(intAttribute != bmdDuplexInactive));
			
			deckLinkSupportsCapture = ((deckLinkAttributes->GetInt(BMDDeckLinkVideoIOSupport, &intAttribute) == S_OK) && 
										((intAttribute & bmdDeviceSupportsCapture) != 0));

			deckLinkAttributes->Release();
		}

		if (!deckLinkSupportsCapture)
		{
			deckLink->Release();
			continue;
		}
		
		result = deckLink->GetDisplayName((const char**)&deckLinkName);
		if (result == S_OK)
		{
			fprintf(stderr,
				"        %2d: %s%s%s\n",
				deckLinkCount,
				deckLinkName,
				deckLinkActive ? "" : " (inactive)",
				deckLinkCount == m_deckLinkIndex ? " (selected)" : ""
			);

			free(deckLinkName);
		}

		if (deckLinkCount == m_deckLinkIndex)
			deckLinkSelected = deckLink;
		else
			deckLink->Release();

		++deckLinkCount;
	}

	if (deckLinkCount == 0)
		fprintf(stderr, "        No DeckLink devices found. Is the driver loaded?\n");

	deckLinkName = nullptr;

	if (deckLinkSelected != nullptr)
		deckLinkSelected->GetModelName((const char**)&deckLinkName);

	fprintf(stderr,
		"    -m <mode id>: (%s)\n",
		deckLinkName ? deckLinkName : ""
	);

	if (deckLinkName != nullptr)
		free(deckLinkName);

	// Loop through all available display modes on the detected DeckLink device
	if (deckLinkSelected == nullptr)
	{
		fprintf(stderr, "        No DeckLink device selected\n");
		goto bail;
	}

	result = deckLinkSelected->QueryInterface(IID_IDeckLinkProfileAttributes, (void**)&deckLinkAttributes);
	if (result == S_OK)
	{
		result = deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &formatDetectionSupported);
		if (result == S_OK && formatDetectionSupported)
			fprintf(stderr, "        -1:  auto detect format\n");
	}

	result = deckLinkSelected->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput);
	if (result != S_OK)
		goto bail;

	result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
	if (result != S_OK)
		goto bail;

	while (displayModeIterator->Next(&displayModeUsage) == S_OK)
	{
		result = displayModeUsage->GetName((const char **)&displayModeName);
		if (result == S_OK)
		{
			BMDTimeValue frameRateDuration;
			BMDTimeValue frameRateScale;

			displayModeUsage->GetFrameRate(&frameRateDuration, &frameRateScale);

			fprintf(stderr,
				"        %2d:  %-20s \t %li x %li \t %g FPS\n",
				displayModeCount,
				displayModeName,
				displayModeUsage->GetWidth(),
				displayModeUsage->GetHeight(),
				(double)frameRateScale / (double)frameRateDuration
			);

			free(displayModeName);
		}

		displayModeUsage->Release();
		++displayModeCount;
	}

bail:
	fprintf(stderr,
		"    -p <pixel_format>\n"
		"         0:  8 bit YUV (4:2:2) (default)\n"
		"         1:  10 bit YUV (4:2:2)\n"
		"         2:  10 bit RGB (4:4:4)\n"
		"    -3                   Capture Stereoscopic 3D (Requires 3D Hardware support)\n"
		"\n"
		"    Capture -d 0 -m 2 -n 50 \n"
	);

    deckLinkIterator->Release();

	if (displayModeIterator != nullptr)
		displayModeIterator->Release();

	if (deckLinkInput != nullptr)
		deckLinkInput->Release();

	if (deckLinkAttributes != nullptr)
		deckLinkAttributes->Release();

	if (deckLinkSelected != nullptr)
		deckLinkSelected->Release();

	exit(status);
}

void BMDConfig::DisplayConfiguration()
{
	fprintf(stderr, "Capturing with the following configuration:\n"
		" - Capture device: %s\n"
		" - Video mode: %s %s\n"
		" - Pixel format: %s\n",
		m_deckLinkName,
		m_displayModeName,
		(m_inputFlags & bmdVideoInputDualStream3D) ? "3D" : "",
		GetPixelFormatName(m_pixelFormat)
	);
}


const char* BMDConfig::SetAdressZMQ() {
    std::string adress = "tcp://*:"+ std::to_string(m_port);
    std::cout<<adress<<std::endl;
    return adress.c_str();
}

const char* BMDConfig::GetPixelFormatName(BMDPixelFormat pixelFormat)
{
	switch (pixelFormat)
	{
		case bmdFormat8BitYUV:
			return "8 bit YUV (4:2:2)";
		case bmdFormat10BitYUV:
			return "10 bit YUV (4:2:2)";
		case bmdFormat10BitRGB:
			return "10 bit RGB (4:4:4)";
        default:
            return "unknown";
	}
}

