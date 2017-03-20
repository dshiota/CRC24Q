#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define LINE_BUF		1024

#define CRC24_INIT	0xB740CEL
#define CRC24_POLY	0x1864CFBL

#define MSGBUF			512
typedef struct _EMS_T {
	int32_t prn;
	int32_t epoch[6];
	int32_t type;
	char msg[MSGBUF];
	unsigned char msg_bytes[32];
} ems_t;

typedef long crc24;

size_t read_ems(FILE *fp, ems_t *ems);
crc24 crc_octets(unsigned char *octets, size_t len);

crc24 crc_octets(unsigned char *octets, size_t len)
{
	/*
	crc24 crc = CRC24_INIT;
	*/
	crc24 crc = 0x00L;
	int32_t i;
	while (len--) {
		crc ^= (*octets++) << 16;
		for (i = 0; i < 8; i++) {
			crc <<= 1;
			if (crc & 0x1000000) {
				crc ^= CRC24_POLY;
			}
		}
	}
	return crc & 0xFFFFFFL;
}

size_t read_ems(FILE *fp, ems_t *ems)
{
	size_t len = (size_t)0;
	char line[LINE_BUF];

	if (fp == (FILE*)NULL) {
		return len;
	}

	if (NULL != fgets(line, (int32_t)(LINE_BUF - 1), fp)) {
		/*
		fprintf(stdout, "%s", line);
		*/
		len = sscanf(line, "%d %d %d %d %d %d %d %d %s",
				&ems->prn, &ems->epoch[0], &ems->epoch[1], &ems->epoch[2],
				&ems->epoch[3], &ems->epoch[4], &ems->epoch[5],
				&ems->type, ems->msg);
	} else {
		return len;
	}

	return len;
}

int32_t main(int32_t argc, char** argv)
{
	int32_t i, j;
	FILE *fp;
	size_t len;
	char *p;
	uint32_t x;
	ems_t ems;
	crc24 crc;
	char msg_str[33];

	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			if ((fp = fopen(argv[i], "r")) != (FILE*)NULL) {
				do {
					len = read_ems(fp, &ems);
					if (ems.prn == 129) {
						memset(msg_str, 0x00, sizeof(char) * 33);
						crc = 0;
						for (j = 0; j < 32; j++) {
							ems.msg_bytes[j] = 0;
						}
						for (j = 0, p = ems.msg; j < 29 && *p; j++, p = p + 2) {
							sscanf(p, "%2X", &x);
							if (j != 28) {
								ems.msg_bytes[j] = (unsigned char)(x & 0xFF);
							} else {
								ems.msg_bytes[j] = (unsigned char)(x & 0xC0);
							}
							/*
							fprintf(stdout, "%02X", ems.msg_bytes[j]);
							*/
						}
						/*
						fprintf(stdout, "\n");
						*/
						for (j = 28; j > 0; j--) {
							ems.msg_bytes[j] >>= 6;
							ems.msg_bytes[j] |= ((ems.msg_bytes[j-1] & 0x3F) << 2);
						}
						ems.msg_bytes[0] >>= 6;
						crc = crc_octets(ems.msg_bytes, 29);
						/*
						fprintf(stdout, "%lX\n", crc << 6);
						*/
						for (j = 0; j < 28; j++) {
							ems.msg_bytes[j] <<= 6;
							ems.msg_bytes[j] |= ((ems.msg_bytes[j+1] & 0xFC) >> 2);
						}
						ems.msg_bytes[28] <<= 6;
						ems.msg_bytes[28] |= (crc >> 18) & 0x3F;
						ems.msg_bytes[29] |= (crc >> 10) & 0xFF;
						ems.msg_bytes[30] |= (crc >> 2) & 0xFF;
						ems.msg_bytes[31] |= (crc & 0x03) << 6;
						/*
						fprintf(stdout, "[brdc] %s\n", ems.msg);
						fprintf(stdout, "[calc] ");
						for (j = 0; j < 32; j++) {
							sprintf(msg_str + j * 2, "%02X", ems.msg_bytes[j]);
						}
						fprintf(stdout, "%s", msg_str);
						fprintf(stdout, "\n");
						fprintf(stdout, "[comp] %d\n", 
								strncmp(ems.msg, msg_str, 32));
								*/
					}
				} while(len != 0);
			}
			fclose(fp);
		}
	}

	return 0;
}
