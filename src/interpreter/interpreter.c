#include "interpreter.h"

/**
 *  �����½�һ��interpreter�ļ��ǽ��������ڲ���ʵ��ϸ�����ε�,
 *  �����ṩһ��������Ľӿ�
 */

void interpreter_init()
{
    parser_init();
    executor_init();
}
