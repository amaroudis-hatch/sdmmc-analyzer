#include "SDMMCHelpers.h"

static struct MMCResponse responses[64] = {
	/*  0 */ {MMC_RSP_NONE,     0,   0, false, false, false},
	/*  1 */ {MMC_RSP_R3,      32,   5, false, false, false},
	/*  2 */ {MMC_RSP_R2_CID, 128,   5, false, false, false},
	/*  3 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/*  4 */ {MMC_RSP_NONE,     0,   0, false, false, false},
	/*  5 */ {MMC_RSP_R1,      32,  64, true, false, false},
	/*  6 */ {MMC_RSP_R1,      32,  64, true, false, false},
	/*  7 */ {MMC_RSP_R1,      32,  64, true, false, false},
	/*  8 */ {MMC_RSP_R1,      32,  64, false, true, false},
	/*  9 */ {MMC_RSP_R2_CSD, 128,  64, false, false, false},
	/* 10 */ {MMC_RSP_R2_CID, 128,  64, false, false, false},
	/* 11 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 12 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 13 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 14 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 15 */ {MMC_RSP_NONE,     0,   0, false, false, false},
	/* 16 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 17 */ {MMC_RSP_R1,      32,  64, false, true, false},
	/* 18 */ {MMC_RSP_R1,      32,  64, false, true, true},
	/* 19 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 20 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 21 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 22 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 23 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 24 */ {MMC_RSP_R1,      32,  64, false, true, false, 0, true},
	/* 25 */ {MMC_RSP_R1,      32,  64, false, true, true, 0, true},
	/* 26 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 27 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 28 */ {MMC_RSP_R1,      32,  64, true, false, false},
	/* 29 */ {MMC_RSP_R1,      32,  64, true, false, false},
	/* 30 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 31 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 32 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 33 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 34 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 35 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 36 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 37 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 38 */ {MMC_RSP_R1,      32,  64, true, false, false},
	/* 39 */ {MMC_RSP_R4,      32,  64, false, false, false},
	/* 40 */ {MMC_RSP_R5,      32,  64, false, false, false},
	/* 41 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 42 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 43 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 44 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 45 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 46 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 47 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 48 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 49 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 50 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 51 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 52 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 53 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 54 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 55 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 56 */ {MMC_RSP_R1,      32,  64, false, false, false},
	/* 57 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 58 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 59 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 60 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 61 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 62 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
	/* 63 */ {MMC_RSP_NONE,     0,   0, false, false, false}, /* reserved */
};

static struct MMCResponse invalid_response = {
	MMC_RSP_NONE,  0,   0, false, false
};

struct MMCResponse SDMMCHelpers::MMCCommandResponse(unsigned int index)
{
	if (index > 63)
		return invalid_response;

	return responses[index];
}

/* According to eMMC 4.51 specs */
static struct MMCCommand commands[64] = {
		"GO_IDLE_STATE",		/* CMD00	arg:00000000 => GO_IDLE_STATE
											arg:F0F0F0F0 => GO_PRE_IDLE_STATE
											arg:FFFFFFFA => BOOT_INITIATION */
		"SEND_OP_COND",			/* CMD01 */
		"ALL_SEND_CID",			/* CMD02 */
		"SET_RELATIVE_ADDR",	/* CMD03 */
		"SET_DSR",				/* CMD04 */
		"SLEEP_AWAKE",			/* CMD05 */
		"SWITCH",				/* CMD06 */
		"SELECT/DESELECT_CARD",	/* CMD07 */
		"SEND_EXT_CSD",			/* CMD08 */
		"SEND_CSD",				/* CMD09 */
		"SEND_CID",				/* CMD10 */
		"_Obsolete",			/* CMD11 */
		"STOP_TRANSMISSION",	/* CMD12 */
		"SEND_STATUS",			/* CMD13 */
		"BUSTEST_R",			/* CMD14 */
		"GO_INACTIVE_STATE",	/* CMD15 */
		"SET_BLOCKLEN",			/* CMD16 */
		"READ_SINGLE_BLOCK",	/* CMD17 */
		"READ_MULTIPLE_BLOCK",	/* CMD18 */
		"BUSTEST_W",			/* CMD19 */
		"_Obsolete",			/* CMD20 */
		"SEND_TUNING_BLOCK",	/* CMD21 */
		"_Reserved",			/* CMD22 */
		"SET_BLOCK_COUNT",		/* CMD23 */
		"WRITE_BLOCK",			/* CMD24 */
		"WRITE_MULTIPLE_BLOCK",	/* CMD25 */
		"PROGRAM_CID",			/* CMD26 */
		"PROGRAM_CSD",			/* CMD27 */
		"SET_WRITE_PROT",		/* CMD28 */
		"CLR_WRITE_PROT",		/* CMD29 */
		"SEND_WRITE_PROT",		/* CMD30 */
		"SEND_WRITE_PROT_TYPE",	/* CMD31 */
		"_Reserved",			/* CMD32 */
		"_Reserved",			/* CMD33 */
		"_Reserved",			/* CMD34 */
		"ERASE_GROUP_START",	/* CMD35 */
		"ERASE_GROUP_END",		/* CMD36 */
		"_Reserved",			/* CMD37 */
		"ERASE",				/* CMD38 */
		"FAST_IO",				/* CMD39 */
		"GO_IRQ_STATE",			/* CMD40 */
		"_Reserved",			/* CMD41 */
		"LOCK_UNLOCK",			/* CMD42 */
		"_Reserved",			/* CMD43 */
		"_Reserved",			/* CMD44 */
		"_Reserved",			/* CMD45 */
		"_Reserved",			/* CMD46 */
		"_Reserved",			/* CMD47 */
		"_Reserved",			/* CMD48 */
		"SET_TIME",				/* CMD49 */
		"_Reserved",			/* CMD50 */
		"_Reserved",			/* CMD51 */
		"_Reserved",			/* CMD52 */
		"PROTOCOL_RD",			/* CMD53 */
		"PROTOCOL_WR",			/* CMD54 */
		"APP_CMD",				/* CMD55 */
		"GEN_CMD",				/* CMD56 */
		"_Reserved",			/* CMD57 */
		"_Reserved",			/* CMD58 */
		"_Reserved",			/* CMD59 */
		"_ReservedMFR",			/* CMD60 */
		"_ReservedMFR",			/* CMD61 */
		"_ReservedMFR",			/* CMD62 */
		"_ReservedMFR",			/* CMD63 */
};

static struct MMCCommand invalid_response_cmd = {
	"_INVALID_"
};

const char * SDMMCHelpers::MMCCommandDescription(unsigned int index, unsigned int args)
{
	if (index > 63)
		return invalid_response_cmd.desc;

	if (index == 0) {
		if (args == 0x00000000)
			return "GO_IDLE_STATE";
		else if (args == 0xF0F0F0F0)
			return "GO_PRE_IDLE_STATE";
		else if (args == 0xFFFFFFFA)
			return "BOOT_INITIATION";
		else
			return "_INVALID_CMD01_";
	}

	return commands[index].desc;
}

/* According to SD Physical Layer Simplified Specification (SD mode) */
static struct MMCResponse sd_response(enum MMCResponseType type,
		bool busy = false, unsigned int block_bytes = 0,
		bool several = false, bool write = false)
{
	/* NCR (command to response) is at most 64 clocks in SD mode */
	struct MMCResponse response = {
		type,
		(type == MMC_RSP_R2_CID || type == MMC_RSP_R2_CSD) ? 128u : 32u,
		64, busy, block_bytes != 0, several, block_bytes, write
	};
	return response;
}

struct MMCResponse SDMMCHelpers::SDCommandResponse(unsigned int index, bool app)
{
	/* application specific commands (after CMD55); data blocks of the
	 * security commands are not decoded */
	if (app) {
		switch (index) {
			case 6:  return sd_response(MMC_RSP_R1);            /* SET_BUS_WIDTH */
			case 13: return sd_response(MMC_RSP_R1, false, 64); /* SD_STATUS */
			case 18: case 25: case 26: case 38:
			case 43: case 44: case 45: case 46: case 47: case 48: case 49:
				return sd_response(MMC_RSP_R1);                  /* security */
			case 22: return sd_response(MMC_RSP_R1, false, 4);  /* SEND_NUM_WR_BLOCKS */
			case 23: return sd_response(MMC_RSP_R1);            /* SET_WR_BLK_ERASE_COUNT */
			case 41: return sd_response(MMC_RSP_R3);            /* SD_SEND_OP_COND */
			case 42: return sd_response(MMC_RSP_R1);            /* SET_CLR_CARD_DETECT */
			case 51: return sd_response(MMC_RSP_R1, false, 8);  /* SEND_SCR */
			default: break; /* not an ACMD: decoded as a regular command */
		}
	}

	switch (index) {
		case 0:  return sd_response(MMC_RSP_NONE);   /* GO_IDLE_STATE */
		case 2:  return sd_response(MMC_RSP_R2_CID); /* ALL_SEND_CID */
		case 3:  return sd_response(MMC_RSP_R6);     /* SEND_RELATIVE_ADDR */
		case 4:  return sd_response(MMC_RSP_NONE);   /* SET_DSR */
		case 5:  return sd_response(MMC_RSP_R4);     /* IO_SEND_OP_COND (SDIO only) */
		case 6:  return sd_response(MMC_RSP_R1, false, 64); /* SWITCH_FUNC */
		case 7:  return sd_response(MMC_RSP_R1, true);      /* SELECT/DESELECT_CARD */
		case 8:  return sd_response(MMC_RSP_R7);     /* SEND_IF_COND */
		case 9:  return sd_response(MMC_RSP_R2_CSD); /* SEND_CSD */
		case 10: return sd_response(MMC_RSP_R2_CID); /* SEND_CID */
		case 11: return sd_response(MMC_RSP_R1);     /* VOLTAGE_SWITCH */
		case 12: return sd_response(MMC_RSP_R1, true); /* STOP_TRANSMISSION */
		case 13: return sd_response(MMC_RSP_R1);     /* SEND_STATUS */
		case 15: return sd_response(MMC_RSP_NONE);   /* GO_INACTIVE_STATE */
		case 16: return sd_response(MMC_RSP_R1);     /* SET_BLOCKLEN */
		case 17: return sd_response(MMC_RSP_R1, false, DATA_BLOCK_BYTES);       /* READ_SINGLE_BLOCK */
		case 18: return sd_response(MMC_RSP_R1, false, DATA_BLOCK_BYTES, true); /* READ_MULTIPLE_BLOCK */
		case 19: return sd_response(MMC_RSP_R1, false, 64); /* SEND_TUNING_BLOCK (4 bit only) */
		case 20: return sd_response(MMC_RSP_R1, true);      /* SPEED_CLASS_CONTROL */
		case 22: return sd_response(MMC_RSP_R1);     /* ADDRESS_EXTENSION */
		case 23: return sd_response(MMC_RSP_R1);     /* SET_BLOCK_COUNT */
		case 24: return sd_response(MMC_RSP_R1, false, DATA_BLOCK_BYTES, false, true); /* WRITE_BLOCK */
		case 25: return sd_response(MMC_RSP_R1, false, DATA_BLOCK_BYTES, true, true);  /* WRITE_MULTIPLE_BLOCK */
		case 27: return sd_response(MMC_RSP_R1, false, 16, false, true); /* PROGRAM_CSD */
		case 28: return sd_response(MMC_RSP_R1, true); /* SET_WRITE_PROT */
		case 29: return sd_response(MMC_RSP_R1, true); /* CLR_WRITE_PROT */
		case 30: return sd_response(MMC_RSP_R1, false, 4); /* SEND_WRITE_PROT */
		case 32: return sd_response(MMC_RSP_R1);     /* ERASE_WR_BLK_START */
		case 33: return sd_response(MMC_RSP_R1);     /* ERASE_WR_BLK_END */
		case 38: return sd_response(MMC_RSP_R1, true); /* ERASE */
		/* data blocks of the following commands are not decoded (length or
		 * direction depend on the argument or on previous commands) */
		case 42: return sd_response(MMC_RSP_R1);     /* LOCK_UNLOCK */
		case 43: return sd_response(MMC_RSP_R1, true); /* Q_MANAGEMENT */
		case 44: case 45: case 46: case 47:
			return sd_response(MMC_RSP_R1);          /* command queue */
		case 48: return sd_response(MMC_RSP_R1, false, DATA_BLOCK_BYTES);        /* READ_EXTR_SINGLE */
		case 49: return sd_response(MMC_RSP_R1, false, DATA_BLOCK_BYTES, false, true); /* WRITE_EXTR_SINGLE */
		case 52: return sd_response(MMC_RSP_R5);     /* IO_RW_DIRECT (SDIO only) */
		case 53: return sd_response(MMC_RSP_R5);     /* IO_RW_EXTENDED (SDIO only) */
		case 55: return sd_response(MMC_RSP_R1);     /* APP_CMD */
		case 56: return sd_response(MMC_RSP_R1);     /* GEN_CMD */
		case 58: case 59:
			return sd_response(MMC_RSP_R1);          /* READ/WRITE_EXTR_MULTI */
		default: return sd_response(MMC_RSP_NONE);   /* reserved */
	}
}

const char * SDMMCHelpers::SDCommandName(unsigned int index, bool app)
{
	if (app) {
		switch (index) {
			case 6:  return "SET_BUS_WIDTH";
			case 13: return "SD_STATUS";
			case 18: case 25: case 26: case 38:
			case 43: case 44: case 45: case 46: case 47: case 48: case 49:
				return "[security]";
			case 22: return "SEND_NUM_WR_BLOCKS";
			case 23: return "SET_WR_BLK_ERASE_COUNT";
			case 41: return "SD_SEND_OP_COND";
			case 42: return "SET_CLR_CARD_DETECT";
			case 51: return "SEND_SCR";
			default: break;
		}
	}

	switch (index) {
		case 0:  return "GO_IDLE_STATE";
		case 2:  return "ALL_SEND_CID";
		case 3:  return "SEND_RELATIVE_ADDR";
		case 4:  return "SET_DSR";
		case 5:  return "IO_SEND_OP_COND";
		case 6:  return "SWITCH_FUNC";
		case 7:  return "SELECT/DESELECT_CARD";
		case 8:  return "SEND_IF_COND";
		case 9:  return "SEND_CSD";
		case 10: return "SEND_CID";
		case 11: return "VOLTAGE_SWITCH";
		case 12: return "STOP_TRANSMISSION";
		case 13: return "SEND_STATUS";
		case 15: return "GO_INACTIVE_STATE";
		case 16: return "SET_BLOCKLEN";
		case 17: return "READ_SINGLE_BLOCK";
		case 18: return "READ_MULTIPLE_BLOCK";
		case 19: return "SEND_TUNING_BLOCK";
		case 20: return "SPEED_CLASS_CONTROL";
		case 22: return "ADDRESS_EXTENSION";
		case 23: return "SET_BLOCK_COUNT";
		case 24: return "WRITE_BLOCK";
		case 25: return "WRITE_MULTIPLE_BLOCK";
		case 27: return "PROGRAM_CSD";
		case 28: return "SET_WRITE_PROT";
		case 29: return "CLR_WRITE_PROT";
		case 30: return "SEND_WRITE_PROT";
		case 32: return "ERASE_WR_BLK_START";
		case 33: return "ERASE_WR_BLK_END";
		case 38: return "ERASE";
		case 40: return "[defined by DPS]";
		case 42: return "LOCK_UNLOCK";
		case 43: return "Q_MANAGEMENT";
		case 44: return "Q_TASK_INFO_A";
		case 45: return "Q_TASK_INFO_B";
		case 46: return "Q_RD_TASK";
		case 47: return "Q_WR_TASK";
		case 48: return "READ_EXTR_SINGLE";
		case 49: return "WRITE_EXTR_SINGLE";
		case 52: return "IO_RW_DIRECT";
		case 53: return "IO_RW_EXTENDED";
		case 55: return "APP_CMD";
		case 56: return "GEN_CMD";
		case 58: return "READ_EXTR_MULTI";
		case 59: return "WRITE_EXTR_MULTI";
		default: return "[reserved]";
	}
}

/*
  Polynomial = 0x89 (2^7 + 2^3 + 1)
  width      = 7 bit
*/
static unsigned char crc7_table[256] = {
	0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
	0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
	0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
	0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
	0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
	0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
	0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
	0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
	0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
	0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
	0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
	0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
	0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
	0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
	0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
	0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
	0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
	0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
	0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
	0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
	0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
	0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
	0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
	0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
	0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
	0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
	0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
	0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
	0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
	0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
	0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
	0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
};

U8 SDMMCHelpers::crc7(const U8 *data, unsigned int size)
{
	U8 crc7_accum = 0;
	int i;

	for (i=0;  i < size; i++) {
		crc7_accum = crc7_table[(crc7_accum << 1) ^ data[i]];
	}
	return crc7_accum;
}
