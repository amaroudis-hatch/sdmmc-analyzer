#include <AnalyzerChannelData.h>
#include "SDMMCAnalyzer.h"
#include "SDMMCAnalyzerResults.h"
#include "SDMMCHelpers.h"

SDMMCAnalyzer::SDMMCAnalyzer()
:	mSimulationInitialized(false),
	mNextIsAppCmd(false)
{
	mOpenFrameStart[CMD_LANE] = mOpenFrameStart[DATA_LANE] = -1;
	SetAnalyzerSettings(&mSettings);
}

SDMMCAnalyzer::~SDMMCAnalyzer()
{
	KillThread();
}

const char* SDMMCAnalyzer::GetAnalyzerName() const
{
	return ::GetAnalyzerName();
}

const char* GetAnalyzerName()
{
	return "SDMMC";
}

void SDMMCAnalyzer::WorkerThread()
{
	mClock = GetAnalyzerChannelData(mSettings.mClockChannel);
	mCommand = GetAnalyzerChannelData(mSettings.mCommandChannel);
	if (mSettings.mBusWidth != BUS_WIDTH_0)
		mData0 = GetAnalyzerChannelData(mSettings.mDataChannel0);
	if (mSettings.mBusWidth == BUS_WIDTH_4 ||
			mSettings.mBusWidth == BUS_WIDTH_8) {
		mData1 = GetAnalyzerChannelData(mSettings.mDataChannel1);
		mData2 = GetAnalyzerChannelData(mSettings.mDataChannel2);
		mData3 = GetAnalyzerChannelData(mSettings.mDataChannel3);
	}
	if (mSettings.mBusWidth == BUS_WIDTH_8) {
		mData4 = GetAnalyzerChannelData(mSettings.mDataChannel4);
		mData5 = GetAnalyzerChannelData(mSettings.mDataChannel5);
		mData6 = GetAnalyzerChannelData(mSettings.mDataChannel6);
		mData7 = GetAnalyzerChannelData(mSettings.mDataChannel7);
	}
	mNextIsAppCmd = false;
	mPendingFrames.clear();

	while (true) {

		ReportProgress(mClock->GetSampleNumber());
		CheckIfThreadShouldExit();
		// Skip to next command
		AdvanceToNextCommand();
		AdvanceToNextClock();

		// Frame objects used by state machine functions - can't be created
		// there (lifetime)
		Frame respFrame;
		Frame dataFrame;

		// Init CMD & DATA state machines
		CommandReadState cmdState = {
			CMD_INIT, 0, 0, 0, // CMD counters & state
			// RESP counters & state (will be set after CMD has been read)
			MMC_RSP_NONE, 0, 0, 0, 0, 0, 0};
		DataReadState dataState = {DATA_NOTSTARTED, 0, 0, 0, 0, false};

		U32 clocks = 0;
		// done once both state machines are idle; the CMD state machine may
		// have been re-armed to wait for commands sent during a data block
		while (cmdState.phase != CMD_ERROR && dataState.phase != DATA_ERROR &&
				cmdState.phase != CMD_INTERRUPT &&
				!((cmdState.phase == CMD_END ||
				   (cmdState.phase == CMD_INIT && cmdState.rearmed)) &&
					(dataState.phase == DATA_END ||
					 dataState.phase == DATA_NOTSTARTED))) {
			// CMD state machine may start DATA state machine => ref needed
			ReadCommandBit(&cmdState, &dataState, &respFrame);
			ReadDataBit(&dataState, &dataFrame);
			AdvanceToNextClock();
			// long multi-block transfers stay in this loop: keep the thread
			// responsive so Logic can stop/restart the analyzer
			if ((++clocks & 0xFFF) == 0) {
				ReportProgress(mClock->GetSampleNumber());
				CheckIfThreadShouldExit();
			}
		}
		// frames still being built were abandoned (error / interrupted block)
		mOpenFrameStart[CMD_LANE] = mOpenFrameStart[DATA_LANE] = -1;
		FlushFrames();
		mResults->CommitResults();
	}
}

bool SDMMCAnalyzer::NeedsRerun()
{
	return false;
}

U32 SDMMCAnalyzer::GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	if (!mSimulationInitialized) {
	mDataGenerator.Initialize(GetSimulationSampleRate(), &mSettings);
	mSimulationInitialized = true;
	}

	return mDataGenerator.GenerateSimulationData(newest_sample_requested, sample_rate, simulation_channels);
}

U32 SDMMCAnalyzer::GetMinimumSampleRateHz()
{
	return 400000 * 4;
}

void SDMMCAnalyzer::SetupResults()
{
	mResults.reset(new SDMMCAnalyzerResults(this, &mSettings));
	SetAnalyzerResults(mResults.get());

	// set which channels will carry bubbles
	mResults->AddChannelBubblesWillAppearOn(mSettings.mCommandChannel);
	if (mSettings.mBusWidth != BUS_WIDTH_0)
		mResults->AddChannelBubblesWillAppearOn(mSettings.mDataChannel0);
}

void SDMMCAnalyzer::AdvanceToNextClock()
{
	enum BitState search = mSettings.mSampleEdge == SAMPLE_EDGE_RISING ? BIT_HIGH : BIT_LOW;

	do {
		mClock->AdvanceToNextEdge();
	} while (mClock->GetBitState() != search);

	// Sample CMD/DAT one sample before the clock edge: in high speed mode
	// outputs change only a few ns after the edge, so sampling exactly at the
	// edge sample can pick up the next bit. Setup time guarantees the lines
	// are stable just before the edge in all modes.
	U64 sample = mClock->GetSampleNumber() - 1;
	mCommand->AdvanceToAbsPosition(sample);
	if (mSettings.mBusWidth != BUS_WIDTH_0)
		mData0->AdvanceToAbsPosition(sample);
	if (mSettings.mBusWidth == BUS_WIDTH_4 ||
			mSettings.mBusWidth == BUS_WIDTH_8) {
		mData1->AdvanceToAbsPosition(sample);
		mData2->AdvanceToAbsPosition(sample);
		mData3->AdvanceToAbsPosition(sample);
	}
	if (mSettings.mBusWidth == BUS_WIDTH_8) {
		mData4->AdvanceToAbsPosition(sample);
		mData5->AdvanceToAbsPosition(sample);
		mData6->AdvanceToAbsPosition(sample);
		mData7->AdvanceToAbsPosition(sample);
	}
}

void SDMMCAnalyzer::AdvanceToNextCommand() {
	mCommand->AdvanceToNextEdge();
	mClock->AdvanceToAbsPosition(mCommand->GetSampleNumber());
	if (mSettings.mBusWidth != BUS_WIDTH_0)
		mData0->AdvanceToAbsPosition(mCommand->GetSampleNumber());
	if (mSettings.mBusWidth == BUS_WIDTH_4 ||
			mSettings.mBusWidth == BUS_WIDTH_8) {
		mData1->AdvanceToAbsPosition(mCommand->GetSampleNumber());
		mData2->AdvanceToAbsPosition(mCommand->GetSampleNumber());
		mData3->AdvanceToAbsPosition(mCommand->GetSampleNumber());
	}
	if (mSettings.mBusWidth == BUS_WIDTH_8) {
		mData4->AdvanceToAbsPosition(mCommand->GetSampleNumber());
		mData5->AdvanceToAbsPosition(mCommand->GetSampleNumber());
		mData6->AdvanceToAbsPosition(mCommand->GetSampleNumber());
		mData7->AdvanceToAbsPosition(mCommand->GetSampleNumber());
	}
	return;
}

void SDMMCAnalyzer::ReadCommandBit(CommandReadState *state, DataReadState
		*dataState, struct Frame *frame) {
	switch(state->phase) {
		case CMD_INIT:
			if (mCommand->GetBitState() == BIT_LOW) {
				mResults->AddMarker(mClock->GetSampleNumber(),
						AnalyzerResults::Start, mSettings.mCommandChannel);
				state->phase = CMD_DIR;
			}
			return;
		case CMD_DIR:
			if (mCommand->GetBitState() != BIT_HIGH) {
				mResults->AddMarker(mClock->GetSampleNumber(),
						AnalyzerResults::X, mSettings.mCommandChannel);
				state->phase = CMD_ERROR;
			} else {
				state->phase = CMD_IDX;
			}
			return;
		case CMD_IDX:
			if (state->cmd_idx_cnt == 0) {
				OpenFrame(frame, CMD_LANE);
				frame->mData1 = 0;
				frame->mData2 = 0;
				frame->mType = SDMMCAnalyzerResults::FRAMETYPE_COMMAND;
				frame->mFlags = 0;
			}
			frame->mData1 = (frame->mData1 << 1) | mCommand->GetBitState();
			if (state->cmd_idx_cnt == 5)
				state->phase = CMD_ARG;
			state->cmd_idx_cnt++;
			return;
		case CMD_ARG:
			frame->mData2 = (frame->mData2 << 1) | mCommand->GetBitState();
			if (state->cmd_arg_cnt == 31) {
				state->phase = CMD_CRC;
				state->cmdindex = (int)frame->mData1;
				dataState->cmdindex = (int)frame->mData1;
				// SD: the command following CMD55 (APP_CMD) is an ACMD
				state->isAppCmd = mSettings.mProtocol == PROTOCOL_SD && mNextIsAppCmd;
				mNextIsAppCmd = state->cmdindex == 55;
				if (state->isAppCmd)
					frame->mFlags |= SDMMCAnalyzerResults::FLAG_APP_CMD;
				frame->mEndingSampleInclusive = mClock->GetSampleNumber();
				QueueFrame(*frame, CMD_LANE);
				mResults->CommitResults();
			}
			state->cmd_arg_cnt++;
			return;
		case CMD_CRC:
			if (state->cmd_crc_cnt == 0) {
				OpenFrame(frame, CMD_LANE);
				frame->mData1 = 0;
				frame->mData2 = 0;
				frame->mType = SDMMCAnalyzerResults::FRAMETYPE_CRC;
				frame->mFlags = 0;
			}
			frame->mData1 = (frame->mData1 << 1) | mCommand->GetBitState();
			if (state->cmd_crc_cnt == 6) {
				state->phase = CMD_STOP;
				frame->mEndingSampleInclusive = mClock->GetSampleNumber();
				QueueFrame(*frame, CMD_LANE);
				mResults->CommitResults();
			}
			state->cmd_crc_cnt++;
			return;
		case CMD_STOP:
		{
			mResults->AddMarker(mClock->GetSampleNumber(),
					AnalyzerResults::Stop, mSettings.mCommandChannel);
			struct MMCResponse response = mSettings.mProtocol == PROTOCOL_SD ?
				SDMMCHelpers::SDCommandResponse(state->cmdindex, state->isAppCmd) :
				SDMMCHelpers::MMCCommandResponse(state->cmdindex);
			if (response.mType != MMC_RSP_NONE) {
				state->phase = RESP_INIT;
				// prepare RESP parsing
				state->timeout = response.mTimeout + 3; // add some slack time
				state->resp_data_bits = response.mBits;
				state->responseType = response.mType;
				// a data block can't start while the previous one is still
				// being read (command sent during a transfer)
				if (response.hasDataBlock &&
						mSettings.mBusWidth != BUS_WIDTH_0 &&
						(dataState->phase == DATA_NOTSTARTED ||
						 dataState->phase == DATA_END)) {
					StartDataBlock(dataState, response);
				}
			} else {
				state->phase = CMD_END;
			}
			return;
		}
		case RESP_INIT:
			if (mCommand->GetBitState() != BIT_LOW) {
				// Waiting for init
				if (state->timeout == 0) {
					state->phase = CMD_ERROR;
					mResults->AddMarker(mClock->GetSampleNumber(),
							AnalyzerResults::X, mSettings.mCommandChannel);
				} else {
					state->timeout--;
				}
			} else {
				mResults->AddMarker(mClock->GetSampleNumber(),
						AnalyzerResults::Start, mSettings.mCommandChannel);
				state->phase = RESP_DIR;
			}
			return;
		case RESP_DIR:
			if (mCommand->GetBitState() != BIT_LOW) {
				/* if card is not transferring this is no response */
				mResults->AddMarker(mClock->GetSampleNumber(),
						AnalyzerResults::X, mSettings.mCommandChannel);
				state->phase = CMD_ERROR;
			} else {
				state->phase = RESP_IGNORED;
			}
			return;
		case RESP_IGNORED:
			if (state->resp_ignore_cnt == 0) {
				OpenFrame(frame, CMD_LANE);
				frame->mData1 = 0;
				frame->mData2 = 0;
				frame->mType = SDMMCAnalyzerResults::FRAMETYPE_RESPONSE;
				frame->mFlags = state->responseType;
			}
			if (state->resp_ignore_cnt == 5)
				state->phase = RESP_DATA;
			state->resp_ignore_cnt++;
			return;
		case RESP_DATA:
			if (state->resp_data_cnt < 64) {
				frame->mData1 = (frame->mData1 << 1) | mCommand->GetBitState();
				/* bits 0 to 63 */
			} else {
				frame->mData2 = (frame->mData2 << 1) | mCommand->GetBitState();
				/* bits 64 to 127 */
			}
			/* last data bit only (resp_data_cnt counts from 0) */
			if (state->resp_data_cnt == state->resp_data_bits - 1) {
				// observed: no data block following CMD8 R1 response,
				// EXCEPTION_EVENT bit (6), RESERVED (4) and APP-specific (2)
				// bits set - why?
				if (mSettings.mProtocol == PROTOCOL_MMC &&
						state->responseType == MMC_RSP_R1 &&
						((frame->mData1 & (1 << 6)) != 0)) {
					dataState->phase = DATA_NOTSTARTED;
				}
				frame->mEndingSampleInclusive = mClock->GetSampleNumber();
				QueueFrame(*frame, CMD_LANE);
				mResults->CommitResults();
				if (state->responseType != MMC_RSP_R2_CID &&
						state->responseType != MMC_RSP_R2_CSD) {
					state->phase = RESP_CRC;
					state->resp_crc_cnt = 0;
				} else {
					// R2: the 128 bits read are CID/CSD[127:1] plus the end
					// bit (CID/CSD bit 0, always 1) - response is complete
					mResults->AddMarker(mClock->GetSampleNumber(),
							AnalyzerResults::Stop, mSettings.mCommandChannel);
					state->phase = CMD_END;
				}
			}
			state->resp_data_cnt++;
			return;
		case RESP_CRC:
			/* first crc bit only */
			if (state->resp_crc_cnt == 0) {
				OpenFrame(frame, CMD_LANE);
				frame->mData1 = 0;
				frame->mData2 = 0;
				frame->mType = SDMMCAnalyzerResults::FRAMETYPE_CRC;
			}
			frame->mData1 = (frame->mData1 << 1) | mCommand->GetBitState();

			/* last crc bit only */
			if (state->resp_crc_cnt == 6) {
				frame->mEndingSampleInclusive = mClock->GetSampleNumber();
				QueueFrame(*frame, CMD_LANE);
				mResults->CommitResults();
				ReportProgress(frame->mEndingSampleInclusive);
				state->phase = RESP_STOP;
			}
			state->resp_crc_cnt++;
			return;
		case RESP_STOP:
			/* stop bit */
			mResults->AddMarker(mClock->GetSampleNumber(), AnalyzerResults::Stop, mSettings.mCommandChannel);
			if (state->cmdindex == 12 && !state->isAppCmd) {  // STOP_TRANSMISSION
				state->phase = CMD_INTERRUPT;
				return;
			}
			if (dataState->hasSeveralDataBlocks) {
				// READ MULTIPLE BLOCK / WRITE MULTIPLE BLOCK: wait for another
				// command while data is being transmitted
				ResetCommandState(state);
			} else {
				state->phase = CMD_END;
			}
			return;
		case CMD_END:
			// data block or busy still running: keep decoding commands sent
			// in the meantime (e.g. CMD13 polling while a write is busy)
			if (dataState->phase != DATA_NOTSTARTED &&
					dataState->phase != DATA_END)
				ResetCommandState(state);
			return;
		/* this method does not gets called for ERROR phase */
		case CMD_ERROR:
			return;
	}

}

void SDMMCAnalyzer::ReadDataBit(DataReadState *state, struct Frame *frame) {
	switch(state->phase) {
		case DATA_INIT:
			if (mData0->GetBitState() == BIT_LOW) {
				/* other data lines must be at 0, too
				 * (depending on bus width (?)) */
				if (	/* BusWidth=4 or 8 */
						mSettings.mBusWidth != BUS_WIDTH_1 &&
						(mData1->GetBitState() != BIT_LOW ||
						 mData2->GetBitState() != BIT_LOW ||
						 mData3->GetBitState() != BIT_LOW) ||
						/* BusWidth=8 */
						mSettings.mBusWidth == BUS_WIDTH_8 &&
						(mData4->GetBitState() != BIT_LOW ||
						 mData5->GetBitState() != BIT_LOW ||
						 mData6->GetBitState() != BIT_LOW ||
						 mData7->GetBitState() != BIT_LOW)) {
					mResults->AddMarker(mClock->GetSampleNumber(),
							AnalyzerResults::X, mSettings.mDataChannel0);
					state->phase = DATA_ERROR;
				} else {
					mResults->AddMarker(mClock->GetSampleNumber(),
							AnalyzerResults::Start, mSettings.mDataChannel0);
					state->phase = DATA_DATA;
				}
			}
			return;
		case DATA_DATA:
		{
			if (mSettings.mDataMode != DATA_MODE_NONE) {
				const U32 lines = BusWidthLines();
				const U32 clocks_per_byte = 8 / lines;
				const U32 bytes_per_frame = DataBytesPerFrame(state);
				const U32 clocks_per_frame = clocks_per_byte * bytes_per_frame;
				const U32 frame_clock = state->data_cnt % clocks_per_frame;
				const U32 byte_in_frame = frame_clock / clocks_per_byte;

				/* Start frames if necessary */
				if (frame_clock == 0) {
					switch (mSettings.mDataMode) {
						case DATA_MODE_PACKED:
							frame->mType = SDMMCAnalyzerResults::FRAMETYPE_DATA_PACKED;
							break;
						case DATA_MODE_BLOCK:
							frame->mType = SDMMCAnalyzerResults::FRAMETYPE_DATA_BLOCK;
							break;
						default:
							frame->mType = SDMMCAnalyzerResults::FRAMETYPE_DATA_CONTENTS;
							break;
					}
					OpenFrame(frame, DATA_LANE);
					frame->mData1 = 0;
					frame->mData2 = 0;
					frame->mFlags = 0;
				}
				/* Store data: bytes 0-7 of a frame go to mData1, 8-15 to
				 * mData2 (packed mode only), first byte in the MSBs */
				U64 *target = NULL;
				if (byte_in_frame < 8)
					target = &frame->mData1;
				else if (mSettings.mDataMode == DATA_MODE_PACKED)
					target = &frame->mData2;
				if (target != NULL)
					*target = (*target << lines) | DataLinesBits();
				/* Add frame at its last clock */
				if (frame_clock == clocks_per_frame - 1) {
					frame->mEndingSampleInclusive = mClock->GetSampleNumber();
					if (mSettings.mDataMode == DATA_MODE_PACKED)
						frame->mFlags = bytes_per_frame;
					else if (mSettings.mDataMode == DATA_MODE_BLOCK)
						frame->mData2 = bytes_per_frame;
					// committed once per block (DATA_CRC) - committing every
					// frame is very expensive at high bus rates
					QueueFrame(*frame, DATA_LANE);
				}
			}
			/* last data bit only - change state */
			if (state->data_cnt == DataBlockClocks(state) - 1) {
				state->phase = DATA_CRC;
			}
			state->data_cnt++;
			return;
		}
		case DATA_CRC:
			if (state->crc_cnt == 0) {
				frame->mType = SDMMCAnalyzerResults::FRAMETYPE_DATA_CRC;
				OpenFrame(frame, DATA_LANE);
				frame->mData1 = 0;
				frame->mData2 = 0;
				frame->mFlags = 0;
			}
			/* one CRC16 per data line: D0-D3 in mData1, D4-D7 in mData2,
			 * 16 bits per line (D0/D4 in bits 15:0) */
			ShiftCrcBit(&frame->mData1, 0, mData0);
			if (mSettings.mBusWidth != BUS_WIDTH_1) { /* BusWidth=4 or 8 */
				ShiftCrcBit(&frame->mData1, 1, mData1);
				ShiftCrcBit(&frame->mData1, 2, mData2);
				ShiftCrcBit(&frame->mData1, 3, mData3);
			}
			if (mSettings.mBusWidth == BUS_WIDTH_8) { /* BusWidth=8 */
				ShiftCrcBit(&frame->mData2, 0, mData4);
				ShiftCrcBit(&frame->mData2, 1, mData5);
				ShiftCrcBit(&frame->mData2, 2, mData6);
				ShiftCrcBit(&frame->mData2, 3, mData7);
			}
			if (state->crc_cnt == 15) {
				frame->mEndingSampleInclusive = mClock->GetSampleNumber();
				QueueFrame(*frame, DATA_LANE);
				mResults->CommitResults();
				state->phase = DATA_STOP;
			}
			state->crc_cnt++;
			return;
		case DATA_STOP:
			if (mData0->GetBitState() == BIT_LOW ||
					/* BusWidth=4 or 8 */
					mSettings.mBusWidth != BUS_WIDTH_1 &&
					(mData1->GetBitState() == BIT_LOW ||
					 mData2->GetBitState() == BIT_LOW ||
					 mData3->GetBitState() == BIT_LOW) ||
					/* BusWidth=8 */
					mSettings.mBusWidth == BUS_WIDTH_8 &&
					(mData4->GetBitState() == BIT_LOW ||
					 mData5->GetBitState() == BIT_LOW ||
					 mData6->GetBitState() == BIT_LOW ||
					 mData7->GetBitState() == BIT_LOW)) {
				mResults->AddMarker(mClock->GetSampleNumber(),
						AnalyzerResults::X, mSettings.mDataChannel0);
				state->phase = DATA_ERROR;
			} else {
				mResults->AddMarker(mClock->GetSampleNumber(),
						AnalyzerResults::Stop, mSettings.mDataChannel0);
				if (state->isWrite) {
					// Write commands
					state->phase = DATA_CHECKCRC_INIT;
				} else {
					if (state->hasSeveralDataBlocks) {
						// FIXME take CMD23 (SET BLOCK COUNT) into account, stop if
						// necessary
						state->data_cnt = 0;
						state->crc_cnt = 0;
						state->checkcrc_cnt = 0;
						state->phase = DATA_INIT;
					} else {
						state->phase = DATA_END;
					}
				}
			}
			return;
		case DATA_CHECKCRC_INIT:
			if (mData0->GetBitState() == BIT_LOW) {
				state->phase = DATA_CHECKCRC_CRC;
				mResults->AddMarker(mClock->GetSampleNumber(),
						AnalyzerResults::Start, mSettings.mDataChannel0);
			}
			return;
		case DATA_CHECKCRC_CRC:
			// 010 -> OK; 101 -> NOK
			if (state->checkcrc_cnt == 0) {
				OpenFrame(frame, DATA_LANE);
				frame->mData1 = 0;
				frame->mData2 = 0;
				frame->mType = SDMMCAnalyzerResults::FRAMETYPE_DATA_CRC_CHECK;
			}
			frame->mData1 = (frame->mData1 << 1) | mData0->GetBitState();
			if (state->checkcrc_cnt == 2) {
				state->phase = DATA_CHECKCRC_STOP;
				frame->mEndingSampleInclusive = mClock->GetSampleNumber();
				QueueFrame(*frame, DATA_LANE);
				mResults->CommitResults();
				ReportProgress(frame->mEndingSampleInclusive);
			}
			state->checkcrc_cnt++;
			return;
		case DATA_CHECKCRC_STOP:
			mResults->AddMarker(mClock->GetSampleNumber(),
					AnalyzerResults::Stop, mSettings.mDataChannel0);
			state->busy_cnt = 0;
			state->busy_seen = false;
			state->phase = DATA_BUSY;
			return;
		case DATA_BUSY:
			// the card may leave D0 high for a clock or two before signalling
			// busy (D0 low); the host's next start bit can't come that early
			// (NWR >= 2 clocks), so up to 2 high clocks are not "busy ended"
			if (mData0->GetBitState() == BIT_LOW) {
				state->busy_seen = true;
			} else if (state->busy_seen || ++state->busy_cnt > 2) {
				state->phase = DATA_BUSY_END;
			}
			return;
		case DATA_BUSY_END:
			if (state->hasSeveralDataBlocks) {
				state->data_cnt = 0;
				state->crc_cnt = 0;
				state->checkcrc_cnt = 0;
				state->phase = DATA_INIT;
			} else {
				state->phase = DATA_END;
			}
			return;
			/* this method does not gets called for NOTSTARTED, END and ERROR
			 * phases */
		case DATA_NOTSTARTED:
		case DATA_ERROR:
		case DATA_END:
			return;
	}
}
/*
 * CMD and DAT frames are built in parallel, but Logic expects frames in time
 * order: frames are queued until no frame still being built can start
 * earlier than them.
 */
void SDMMCAnalyzer::OpenFrame(Frame *frame, int lane) {
	frame->mStartingSampleInclusive = mClock->GetSampleNumber();
	mOpenFrameStart[lane] = frame->mStartingSampleInclusive;
}

void SDMMCAnalyzer::QueueFrame(const Frame &frame, int lane) {
	mOpenFrameStart[lane] = -1;
	std::vector<Frame>::iterator it = mPendingFrames.end();
	while (it != mPendingFrames.begin() &&
			(it - 1)->mStartingSampleInclusive > frame.mStartingSampleInclusive)
		--it;
	mPendingFrames.insert(it, frame);
	FlushFrames();
}

void SDMMCAnalyzer::FlushFrames() {
	S64 safe = -1;
	for (int lane = 0; lane < 2; lane++)
		if (mOpenFrameStart[lane] >= 0 &&
				(safe < 0 || mOpenFrameStart[lane] < safe))
			safe = mOpenFrameStart[lane];
	size_t n = 0;
	while (n < mPendingFrames.size() &&
			(safe < 0 || mPendingFrames[n].mStartingSampleInclusive < safe))
		mResults->AddFrame(mPendingFrames[n++]);
	mPendingFrames.erase(mPendingFrames.begin(), mPendingFrames.begin() + n);
}

U32 SDMMCAnalyzer::BusWidthLines() {
	switch (mSettings.mBusWidth) {
		case BUS_WIDTH_8: return 8;
		case BUS_WIDTH_4: return 4;
		default: return 1;
	}
}

U32 SDMMCAnalyzer::DataBlockClocks(DataReadState *state) {
	return state->blockBytes * 8 / BusWidthLines();
}

U32 SDMMCAnalyzer::DataBytesPerFrame(DataReadState *state) {
	switch (mSettings.mDataMode) {
		case DATA_MODE_PACKED:
			// short blocks (e.g. 8 byte SCR) end the frame early
			return state->blockBytes < 16 ? state->blockBytes : 16;
		case DATA_MODE_BLOCK: return state->blockBytes;
		default: return 1;
	}
}

void SDMMCAnalyzer::StartDataBlock(DataReadState *state, const MMCResponse &response) {
	state->phase = DATA_INIT;
	state->data_cnt = 0;
	state->crc_cnt = 0;
	state->checkcrc_cnt = 0;
	state->hasSeveralDataBlocks = response.hasSeveralDataBlocks;
	state->blockBytes = response.mBlockBytes ? response.mBlockBytes : DATA_BLOCK_BYTES;
	state->isWrite = response.mWrite;
}

void SDMMCAnalyzer::ResetCommandState(CommandReadState *state) {
	state->phase = CMD_INIT;
	state->cmd_idx_cnt = 0;
	state->cmd_arg_cnt = 0;
	state->cmd_crc_cnt = 0;
	state->responseType = MMC_RSP_NONE;
	state->timeout = 0;
	state->cmdindex = 0;
	state->resp_data_bits = 0;
	state->resp_ignore_cnt = 0;
	state->resp_data_cnt = 0;
	state->resp_crc_cnt = 0;
	state->isAppCmd = false;
	state->rearmed = true;
}

U64 SDMMCAnalyzer::DataLinesBits() {
	/* bits sent on one clock, highest data line first */
	U64 bits = 0;
	if (mSettings.mBusWidth == BUS_WIDTH_8) {
		bits = (bits << 1) | mData7->GetBitState();
		bits = (bits << 1) | mData6->GetBitState();
		bits = (bits << 1) | mData5->GetBitState();
		bits = (bits << 1) | mData4->GetBitState();
	}
	if (mSettings.mBusWidth == BUS_WIDTH_8 ||
			mSettings.mBusWidth == BUS_WIDTH_4) {
		bits = (bits << 1) | mData3->GetBitState();
		bits = (bits << 1) | mData2->GetBitState();
		bits = (bits << 1) | mData1->GetBitState();
	}
	bits = (bits << 1) | mData0->GetBitState();
	return bits;
}

void SDMMCAnalyzer::ShiftCrcBit(U64 *crcs, int lane, AnalyzerChannelData *line) {
	const int shift = lane * 16;
	U64 crc = (*crcs >> shift) & 0xFFFF;
	crc = ((crc << 1) | (line->GetBitState() == BIT_HIGH ? 1 : 0)) & 0xFFFF;
	*crcs = (*crcs & ~(0xFFFFull << shift)) | (crc << shift);
}

/*
 * loader hooks
 */

Analyzer* CreateAnalyzer()
{
	return new SDMMCAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
	delete analyzer;
}
