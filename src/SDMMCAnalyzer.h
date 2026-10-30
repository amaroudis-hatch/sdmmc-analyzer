#ifndef __SDMMC_ANALYZER_H
#define __SDMMC_ANALYZER_H

#include <Analyzer.h>
#include <vector>
#include "SDMMCAnalyzerSettings.h"
#include "SDMMCAnalyzerResults.h"
#include "SDMMCSimulationDataGenerator.h"
#include "SDMMCAnalyzer.h"
#include "SDMMCHelpers.h"

class SDMMCAnalyzer : public Analyzer2
{
public:
	SDMMCAnalyzer();
	virtual ~SDMMCAnalyzer();

	virtual const char *GetAnalyzerName() const;

	virtual void WorkerThread();
	virtual bool NeedsRerun();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();
	virtual void SetupResults();

protected:
	void AdvanceToNextClock();
	void AdvanceToNextCommand();
	void ReadCommandBit(CommandReadState *state, DataReadState *dataState,
			struct Frame *respFrame);
	void ReadDataBit(DataReadState *state, struct Frame *dataFrame);
	U32 BusWidthLines();
	U32 DataBlockClocks(DataReadState *state);
	U32 DataBytesPerFrame(DataReadState *state);
	void StartDataBlock(DataReadState *state, const MMCResponse &response);
	void ResetCommandState(CommandReadState *state);
	void OpenFrame(Frame *frame, int lane);
	void QueueFrame(const Frame &frame, int lane);
	void FlushFrames();
	U64 DataLinesBits();
	void ShiftCrcBit(U64 *crcs, int lane, AnalyzerChannelData *line);

protected:
	SDMMCAnalyzerSettings mSettings;
	std::auto_ptr<SDMMCAnalyzerResults> mResults;

	SDMMCSimulationDataGenerator mDataGenerator;
	bool mSimulationInitialized;
	bool mNextIsAppCmd; // last command was CMD55 (APP_CMD)

	enum { CMD_LANE, DATA_LANE };
	S64 mOpenFrameStart[2];            // start of frame being built, -1: none
	std::vector<Frame> mPendingFrames; // sorted by start sample

	AnalyzerChannelData *mClock;
	AnalyzerChannelData *mCommand;
	AnalyzerChannelData *mData0;
	AnalyzerChannelData *mData1;
	AnalyzerChannelData *mData2;
	AnalyzerChannelData *mData3;
	AnalyzerChannelData *mData4;
	AnalyzerChannelData *mData5;
	AnalyzerChannelData *mData6;
	AnalyzerChannelData *mData7;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif
