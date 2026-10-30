#ifndef __SDMMC_ANALYZER_RESULTS
#define __SDMMC_ANALYZER_RESULTS

#include <AnalyzerResults.h>
#include <string>

class SDMMCAnalyzer;
class SDMMCAnalyzerSettings;

class SDMMCAnalyzerResults : public AnalyzerResults
{
public:
	SDMMCAnalyzerResults(SDMMCAnalyzer* analyzer, SDMMCAnalyzerSettings* settings);
	virtual ~SDMMCAnalyzerResults();
	virtual void GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id);
	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base);
	virtual void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base);
	virtual void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base);

public:
	enum FrameType {
		FRAMETYPE_HEADER,
		FRAMETYPE_COMMAND,
		FRAMETYPE_RESPONSE,
		FRAMETYPE_CRC,
		FRAMETYPE_DATA_CONTENTS,
		FRAMETYPE_DATA_CRC,
		FRAMETYPE_DATA_CRC_CHECK,
		FRAMETYPE_DATA_PACKED, // up to 16 bytes in mData1/mData2, count in mFlags
		FRAMETYPE_DATA_BLOCK,  // first 8 bytes in mData1, block length in mData2
	};

	// FRAMETYPE_COMMAND mFlags (0x40/0x80 are reserved by the SDK)
	enum { FLAG_APP_CMD = 0x01 };

protected:
	void AppendDataBytes(std::string &str, U64 data, U32 count, DisplayBase display_base);

protected:
	SDMMCAnalyzerSettings *mSettings;
	SDMMCAnalyzer *mAnalyzer;
};

#endif
