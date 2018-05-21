
#ifndef FLYWHEEL_MATCH_H
#define FLYWHEEL_MATCH_H

#include "driver_match.h"

#define FLYWHEEL_TEMPLATE_NUM 10
#define MATCH_FUNCS_NUM  4

//ģ������ṹ���
static struct template_data* template_data_table;

static struct command_sequence* set_speed_template0;

static struct bytes_array_assembly_scheme* receive_template0;

static int global_match(void);

//open��ص�ģ��ƥ�亯��
static int open_template0_match(void);

//write��ص�ģ��ƥ�亯��
static int set_speed_template0_match(void);

//write��ص�ģ��ƥ�亯��
static int receive_template0_match(void);

extern int flywheel_match(void);

#endif
