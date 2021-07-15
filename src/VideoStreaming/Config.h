#ifndef BMD_CONFIG_H
#define BMD_CONFIG_H
#include "iostream"
#include "DeckLinkAPI.h"

class BMDConfig
{
public:
	BMDConfig();
	virtual ~BMDConfig();

	bool ParseArguments(int argc,  char** argv);
	void DisplayUsage(int status) const;
	void DisplayConfiguration();
	std::string SetAddressZMQ() const ;
	std::string SetTopicPublisher() const ;

	int						m_deckLinkIndex;
	int						m_displayModeIndex;
	int                     m_port;
	int                     m_side;

	BMDVideoInputFlags		m_inputFlags;
	BMDPixelFormat			m_pixelFormat;


	IDeckLink* GetSelectedDeckLink() const;
	IDeckLinkDisplayMode* GetSelectedDeckLinkDisplayMode(IDeckLink* deckLink) const;

private:
	char*					m_deckLinkName;
	char*					m_displayModeName;

	static const char* GetPixelFormatName(BMDPixelFormat pixelFormat);
};

#endif