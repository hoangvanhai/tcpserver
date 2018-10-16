#ifndef QUALITYDEFINE_H
#define QUALITYDEFINE_H

//
// Analog Tag Quality Code
//
#define QLTY_OVER_RANGE					0x0001	// 'Over Range'
#define QLTY_UNDER_RANGE				0x0002	// 'Under Range'
#define QLTY_OPEN_LOOP					0x0003	// 'Open Loop'
#define QLTY_SHORTED_LOOP				0x0004	// 'Shorted Loop'


//
// Exception Quality Code
//
#define QLTY_RESTART					0x8001	// 'Comm Error' indicates that the data has not been updated from the field since device reset.
#define QLTY_DRIVER_LOAD_FAILED         0x8002	// 'Driver Error'
#define QLTY_PORT_OPEN_FAILED           0x8003	// 'Port Error'
#define QLTY_DEVICE_ERROR				0x8004	// 'Device Error' 3+ packets in this device failed
#define QLTY_CONVERSION_CODE_ERROR      0x8005	// 'Conversion Code Error'
#define QLTY_CONVERSION_ERROR           0x8006	// 'Conversion Error'
#define QLTY_DATA_TYPE_ERROR            0x8007	// 'Data Type Error'
#define QLTY_DATA_SIZE_ERROR            0x8008	// 'Data Size Error'
#define QLTY_DATA_ERROR					0x8009	// 'Data Error'
#define QLTY_CHECKSUM_ERROR				0x800a	// 'Checksum Error'
#define QLTY_MSG_SEQ_ERROR				0x800b	// 'MsgSeq Error' Incorrect message sequence
#define QLTY_NO_OWNER					0x8080	// 'Data Source Absent'
#define QLTY_DISCONNECTED				0x8081	// 'Disconnected'
#define QLTY_DRIVER_SEPCIFIED           0x8100	// 'Driver Sepcified'

// NOTE: Use this macro to compare qualities
#define QLTY_IS_EQUAL(qlty,qlty_code)	((int16_t)(qlty) == (int16_t)(qlty_code))

#endif // QUALITYDEFINE_H
