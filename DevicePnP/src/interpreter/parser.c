#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "executor.h"
#include "lex.h"
#include "relocation.h"
#include "type_converter.h"
#include "dependency.h"

static void expression(int level) 
{

    int *id;
    int tmp;

    {
        if (!token) {
            printf("%d: unexpected token EOF of expression\n", line);
            exit(-1);
        }

        // ������ֵ
        if (token == Num) {
            match(Num);
           //TODO ��һ���ж��Ƿ��Ǹ�������
            
            if (num_type == INT){
               load_integral_number_constant(integral_token_val);
               expr_type = INT;
            }else{
            //TODO ���ظ��㳣�������㳣��double���ʹ洢
               load_real_number_constant(real_token_val);
               expr_type = FLOAT;
            }
        }

        // �����ַ�������
        else if (token == '"') {

            *++text = IMM;
            *++text = integral_token_val;

            match('"');
            while (token == '"') {
                match('"');
            }

            // �ַ�����������Ҫ�ض�λ
            // data�γ�ʼ����ʱ��Ϊ0�����Բ���Ҫ��ʾ����ĩβ���'\0'��������
            // Ϊ��ʹ��data����4�ֽڱ߽��϶��룬��������ַ����ĳ���Ϊ11���ֽ�
            // �Ļ�����ô�����ʵ�ʷ����data�ռ���12���ֽ�
            data = (char *)(((int)data + sizeof(int)) & (-sizeof(int)));

            expr_type = PTR;
        }

        // �����ʶ��
        else if (token == Id) {

            match(Id);
            id = current_id;

            //��������
            if (token == '(') {
                match('(');

                int num_args = 0; //ʵ�εĸ���
                while (token != ')') {
                    // ������ѹ��ջ��
                    // TODO ��������Ǹ������͵Ļ��Ͷ�����������
                    expression(Assign);
                    *++text = PUSH;
                    num_args++;

                    if (token == ',') {
                        match(',');
                    }

                }
                match(')');

                // ϵͳ����, id[Value]������Ǻ�����OP����
                if (id[Class] == Sys) {
                    *++text = id[Value];
                }
                // �Զ���ĺ���
                else if (id[Class] == Fun) {
                    *++text = CALL;
                    *++text = id[Value];
                }
                else {
                    printf("%d: bad function call\n", line);
                    exit(-1);
                }

                // ������������д��ݲ�������ô�������غ���Ҫ������Щ������Ӧ��
                // ջ�ռ�
                if (num_args > 0) {
                    *++text = ADJ;
                    *++text = num_args;
                }

                //����������
                expr_type = id[Type];
            }
            else if (id[Class] == Num) {
            // ö������
                *++text = IMM;
                *++text = id[Value];
                expr_type = INT;
            }
            else {
            // ��ͨ���� 
            
                if (id[Class] == Ext) {
                    *++text = IMM;                
                    *++text = id[Value]; //id[Value]���Ǳ������ַ
                }
                else if (id[Class] == Glo) {
                    *++text = IMM;                
                    *++text = id[Value]; //id[Value]���Ǳ������ַ
                    int offset = (id[Value] - (int)data_start);
                    add_relocation_item(text, offset, Data_Rel);                    
                }
                else {
                    printf("%d: undefined variable\n", line);
                    exit(-1);
                }


                expr_type = id[Type];

                //���ݱ���������ѡ����Ӧ�ļ���ָ��
                *++text = emit_load_directive(expr_type);
            }
        }

        // ǿ������ת���Լ���ͬ�����ŷ���
        else if (token == '(') {
            match('(');

            // ǿ������ת��
            if (token == Int || token == Char || token == Float || Double) {
                int cast_type = type_of_token(token);
                match(token);
                while (token == Mul) {
                    match(Mul);
                    cast_type = cast_type + PTR;
                }
                match(')');

                //ת�͵����ȼ���Inc(++)һ��
                expression(Inc); 

                check_assignment_types(cast_type, expr_type);
                
                // ǿ������ת������ı��ʽ������Ӧ�ú�ת�͵�������һ��������
                // (int **)var, ��ô����var֮ǰ��ʲô���͵ı�����ת�ͺ������
                // ����(int **)
                expr_type  = cast_type;

            } else {
            // ��ͨ�����ŷ���
                expression(Assign);
                match(')');
            }
        }

        else if (token == Mul) {
            match(Mul);

            //�����õ����ȼ���Inc(++)һ��
            expression(Inc); 

            printf("expr_type %d\n", expr_type);
            if (expr_type >= PTR) {
                expr_type = expr_type - PTR;
            } else {
                printf("%d: bad dereference\n", line);
                exit(-1);
            }

            //float** f;   1.0 + **f
            //��ôͨ��Load�����𲽽�����addr (LI) (LF)
            //
            *++text = emit_load_directive(expr_type); 
        }

        else if (token == And) {
            match(And);

            //ȡ��ַ�����ȼ���Inc(++)һ��
            expression(Inc); 

            //�����&var�Ļ���ֱ��ͨ��load����ǰ���IMM�����Ϳ��Լ������ַ��
            //"&"�����ֻ���Ǳ����������ǳ����������������һ��bug: ���&const
            //�����const����ֵǡ����LC LI LF��LD����һ��������Ϊ�˱����������
            //������������ж�;���&�����ȼ��Ƚϸ�������&(1+2)֮��Ķ��ǲ��Ϸ���
            if (!does_operate_on_constant() &&
                 (*text == LC || *text == LI || *text == LF || *text == LD)){
                text--;
            }else {
                printf("%d: bad address of\n", line);
                exit(-1);
            }

            expr_type = expr_type + PTR;
        }

        else if (token == '!') {
            match('!');

            //�߼��ǵ����ȼ���Inc(++)һ��
            expression(Inc);

            // ʹ��expr == 0 �����ж�
            // �����"!"����ı��ʽ�����Ǹ������ͣ���bx�Ĵ����е���ת�ͳ���
            // �Ͳ�������ax��ָ��BTOA�����������
            if (expr_type == FLOAT || expr_type == DOUBLE){
                *++text = BTOA;                
            }

            *++text = PUSH;
            *++text = IMM;
            *++text = 0;
            *++text = EQ;

            //����������ʽ(!<expr>)��������INT
            expr_type = INT;
        }

        else if (token == '~') {
            // bitwise not
            match('~');

            //��λ�ǵ����ȼ���Inc(++)һ��
            expression(Inc); 
        
            //λ�����Ļ����ʽ������һ��Ҫ��ȷ�������Ҫ���һЩ����
            if (expr_type == FLOAT || expr_type == DOUBLE){
                printf("%d: wrong type argument to bit-complement\n", line);
                exit(-1); 
            }

            //ʹ��<expr> XOR -1��ʱʵ�ְ�λ�ǣ�����ϸ������
            //(1111 1111)  -1
            //(0110 0011)  XOR
            //______________
            //
            //(1001 1100)
            *++text = PUSH; 
            *++text = IMM;  
            *++text = -1;
            *++text = XOR;

            //����������ʽ(~<expr>)��������INT
            expr_type = INT;
        }
        else if (token == Add) {
            // +var, ����ʵ�ʵĲ���
            match(Add);

            //�������ȼ���Inc(++)һ��
            expression(Inc);

            //����������ʽ(+<expr>)�����ͺ�<expr>��ͬ
            expr_type = expr_type;
        }
        else if (token == Sub) {
            // -var
            match(Sub);

            if (token == Num) {
                if (num_type == INT || num_type == CHAR){
                   load_integral_number_constant(-integral_token_val);
                }else{
                   load_real_number_constant(-real_token_val);
                }
                match(Num);
            } else {
                //TODO 
                *++text = IMM;
                *++text = -1;   
                *++text = PUSH;
                expression(Inc);
                *++text = MUL; 
            }

        }

        else if (token == Inc || token == Dec) {
            int save_token = token;
            match(token);
            expression(Inc);

            if (does_operate_on_constant()){
                printf("%d:Inc or Dec cannot apply on constant\n", line);
                exit(-1);
            } 

            // ��ʱ��֧�ָ������͵ı���(����ָ������)++��--����
            if (get_base_type(expr_type) > INT){
                printf("%d: sorry, Inc or Dec is not supported for floating\n",
                      line);
                exit(-1);
            }


            if (*text == LC) {
                *text = PUSH;  
                *++text = LC;
            } else if (*text == LI) {
                *text = PUSH;
                *++text = LI;
            } else {
                printf("%d: bad lvalue of pre-increment\n", line);
                exit(-1);
            }

            *++text = PUSH;
            *++text = IMM;
            *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
            *++text = (save_token == Inc) ? ADD : SUB;
            *++text = (expr_type == CHAR) ? SC : SI;
        }
        else {
            printf("%d: bad expression\n", line);
            exit(-1);
        }
    }
 

    //�����Ԫ�������Լ���׺������
    {
        // ���ݵ�ǰ�Ĳ��������ȼ����в���
        while (token >= level) {
            int left_type = expr_type;
            if (token == Assign) {
                // var = expr;
                match(Assign);

                // ��������ǳ䵱��ֵ�����޸�ָ�ʹ��PUSHָ������ַ
                // �����������ֵ�Ļ�����ʹ��Loadָ�����
                // ��ֵ���Ǳ���������
                if (*text == LC || *text == LI || *text == LF || *text == LD) {   
                    *text = PUSH; 
                } else {
                    printf("%d: bad lvalue in assignment\n", line);
                    exit(-1);
                }

                // Ȼ������ұ߱��ʽ��ֵ������������浽ax����bx
                expression(Assign);

                //���ͼ��ݵĺ���
                printf("assign left %d , right %d\n", left_type, expr_type);
                check_assignment_types(left_type, expr_type);

                //������������ͼ��ݵĻ�����ô�������ʽ�����;����������������
                expr_type = left_type; 
                *++text = emit_store_directive(expr_type);
            }

            else if (token == Cond) {
                // expr ? a : b;
                match(Cond);
                int *addr;

                // ��������float���͵ģ���ô��bx�е���ת���Ƶ�ax��
                // ת�͵ľ�����ʧ����Ӱ�������������
                if (expr_type == FLOAT || expr_type == DOUBLE){
                  *++text = BTOA;
                }

                *++text = JZ;
                addr = ++text;
                expression(Assign);
                if (token == ':') {
                    match(':');
                } else {
                    printf("%d: missing colon in conditional\n", line);
                    exit(-1);
                }
                int offset = (text + 3 - text_start)*sizeof(int);
                add_relocation_item(addr, offset, Text_Rel);
                *addr = (int)(text + 3);
                *++text = JMP;

                addr = ++text;
                expression(Cond);
                offset = (text + 1 - text_start)*sizeof(int);
                add_relocation_item(addr, offset, Text_Rel);
                *addr = (int)(text + 1);
            }

            else if (token == Lor) {
                // logic or
                match(Lor);

                int *addr;

                // ��������float���͵ģ���ô��bx�е���ת���Ƶ�ax��
                // ת�͵ľ�����ʧ����Ӱ�������������
                if (expr_type == FLOAT || expr_type == DOUBLE){
                  *++text = BTOA;
                }

                *++text = JNZ;
                addr = ++text;
                expression(Lan);

                int offset = (text + 1 - text_start)*sizeof(int);
                add_relocation_item(addr, offset, Text_Rel);
                *addr = (int)(text + 1);
                expr_type = INT;
            }
            else if (token == Lan) {
                // logic and
                match(Lan);

                int *addr;

                // ��������float���͵ģ���ô��bx�е���ת���Ƶ�ax��
                // ת�͵ľ�����ʧ����Ӱ�������������
                if (expr_type == FLOAT || expr_type == DOUBLE){
                  *++text = BTOA;
                }

                *++text = JZ;
                addr = ++text;
                expression(Or);

                int offset = (text + 1 - text_start)*sizeof(int);
                add_relocation_item(addr, offset, Text_Rel);
                *addr = (int)(text + 1);

                expr_type = INT;
            }
            else if (token == Or) {
                // bitwise or
                match(Or);
                *++text = PUSH;
                expression(Xor);

               //λ�����Ļ����ʽ������һ��Ҫ��ȷ�������Ҫ���һЩ����
               if (expr_type == FLOAT || expr_type == DOUBLE){
                   printf("%d: wrong type argument to bitwise or\n", line);
                   exit(-1); 
                }

                *++text = OR;
                expr_type = INT;
            }
            else if (token == Xor) {
                // bitwise xor
                match(Xor);
                *++text = PUSH;
                expression(And);

                //λ�����Ļ����ʽ������һ��Ҫ��ȷ�������Ҫ���һЩ����
                if (expr_type == FLOAT || expr_type == DOUBLE){
                   printf("%d: wrong type argument to bitwise xor\n", line);
                   exit(-1); 
                }

                *++text = XOR;
                expr_type = INT;
            }
            else if (token == And) {
                // bitwise and
                match(And);
                *++text = PUSH;
                expression(Eq);

                //λ�����Ļ����ʽ������һ��Ҫ��ȷ�������Ҫ���һЩ����
                if (expr_type == FLOAT || expr_type == DOUBLE){
                   printf("%d: wrong type argument to bitwise xor\n", line);
                   exit(-1); 
                }

                *++text = AND;
                expr_type = INT;
            }
            else if (token == Eq) {
                // equal ==
                match(Eq);
                int *reserve1 = NULL, *reserve2 = NULL;

                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;

                expression(Ne);

                //*++text = EQ;
                emit_code_for_binary_right(EQF, EQ, &reserve1, &reserve2);

                expr_type = INT;
            }
            else if (token == Ne) {
                // not equal !=
                match(Ne);
                int *reserve1 = NULL, *reserve2 = NULL;

                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
                
                expression(Lt);

                //*++text = NE;
                emit_code_for_binary_right(NEF, NE, &reserve1, &reserve2);

                expr_type = INT;
            }
            else if (token == Lt) {
                // less than
                match(Lt);
                int *reserve1 = NULL, *reserve2 = NULL;

                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
                
                expression(Shl);

                //*++text = LT;
                emit_code_for_binary_right(LTF, LT, &reserve1, &reserve2);

                expr_type = INT;
            }
            else if (token == Gt) {
                // greater than
                match(Gt);
                int *reserve1 = NULL, *reserve2 = NULL;

                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
                
                expression(Shl);

                //*++text = GT;
                emit_code_for_binary_right(GTF, GT, &reserve1, &reserve2);

                expr_type = INT;
            }
            else if (token == Le) {
                // less than or equal to
                match(Le);
                int *reserve1 = NULL, *reserve2 = NULL;

                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
                
                expression(Shl);

                //*++text = LE;
                emit_code_for_binary_right(LEF, LE, &reserve1, &reserve2);

                expr_type = INT;
            }
            else if (token == Ge) {
                // greater than or equal to
                match(Ge);
                int *reserve1 = NULL, *reserve2 = NULL;

                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
            
                expression(Shl);

                //*++text = GE;
                emit_code_for_binary_right(GEF, GE, &reserve1, &reserve2);

                expr_type = INT;
            }
            else if (token == Shl) {
                // shift left
                match(Shl);
                int save_type = expr_type;

                *++text = PUSH;
                expression(Add);

                // ����Ĳ�����ֻ����char�Լ�int�͵�
                if ((save_type == FLOAT || save_type == DOUBLE) ||
                    (expr_type == FLOAT || save_type == DOUBLE)){
                   printf("%d: wrong type argument to shift left\n", line);
                   exit(-1); 
                }

                *++text = SHL;
                
                expr_type = INT;
            }
            else if (token == Shr) {
                // shift right
                match(Shr);
                int save_type = expr_type;

                *++text = PUSH;
                expression(Add);

                // ����Ĳ�����ֻ����char�Լ�int�͵�
                if ((save_type == FLOAT || save_type == DOUBLE) ||
                    (expr_type == FLOAT || save_type == DOUBLE)){
                   printf("%d: wrong type argument to shitf right\n", line);
                   exit(-1); 
                }

                *++text = SHR;
                
                expr_type = INT;
            }
            //TODO �ȳ����ø���ļӷ������������� 
            else if (token == Add) {
                // add
                match(Add);

                int *reserve1 = NULL, *reserve2 = NULL;
                emit_code_for_binary_left(&reserve1, &reserve2);

                //������ʽ�ұߵ�ֵ
                expression(Mul);
                
                printf("+ right type %d\n", expr_type);
                //TODO expr_type = tmp;
                //�����������ָ�����͵Ļ�
                if (expr_type > PTR) { 
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }

                emit_code_for_binary_right(ADDF, ADD, &reserve1, &reserve2);

            }
            else if (token == Sub) {
                // sub
                match(Sub);
                *++text = PUSH;
                expression(Mul);
                if (tmp > PTR && tmp == expr_type) {
                    // pointer subtraction
                    *++text = SUB;
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = DIV;
                    expr_type = INT;
                } else if (tmp > PTR) {
                    // pointer movement
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                    *++text = SUB;

                    expr_type = tmp;
                } else {
                    // numeral subtraction
                    *++text = SUB;

                    expr_type = tmp;
                }
            }
            else if (token == Mul) { // multiply
                match(Mul);

                int *reserve1 = NULL, *reserve2 = NULL;
                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
                
                expression(Inc);

                emit_code_for_binary_right(MULF, MUL, &reserve1, &reserve2);
                //*++text = MUL;

                //TODO
                //expr_type = tmp;
            }
            else if (token == Div) {
                // divide
                match(Div);

                int *reserve1 = NULL, *reserve2 = NULL;
                emit_code_for_binary_left(&reserve1, &reserve2);
                //*++text = PUSH;
                expression(Inc);

                emit_code_for_binary_right(DIVF, DIV, &reserve1, &reserve2);
                //*++text = DIV;

                //expr_type = tmp;
            }
            else if (token == Mod) {
                // Modulo
                match(Mod);

                int save_type = expr_type;
                *++text = PUSH;

                expression(Inc);
                // ֻ����������������(CHAR��INT)�ſ���
                if (!((save_type == INT || save_type == CHAR) &&
                      (expr_type == INT || expr_type == CHAR))){
                     printf("%d:invalid operands to binary\n", line);
                     exit(-1); 
                }

                *++text = MOD;

                expr_type = INT;
                //expr_type = tmp;
            }
            else if (token == Inc || token == Dec) {
                // postfix inc(++) and dec(--)
                // we will increase the value to the variable and decrease it
                // on `ax` to get its original value.
                if (*text == LI) {
                    *text = PUSH;
                    *++text = LI;
                }
                else if (*text == LC) {
                    *text = PUSH;
                    *++text = LC;
                }
                else {
                    printf("%d: bad value in increment\n", line);
                    exit(-1);
                }

                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? ADD : SUB;
                //SC store char; SI store int
                *++text = (expr_type == CHAR) ? SC : SI;
                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? SUB : ADD;
                match(token);
            }

            //����ķ���,���Ǻ���û�����������
            else if (token == Brak) {
                // array access var[xx]
                match(Brak);
                int array_type = expr_type;

                *++text = PUSH; //��var��ֵ��Ϊ��ַ����ջ��
                expression(Assign);
                match(']');

                //ʲôʱ����Ҫ��type���浽tmp
                if (array_type > PTR) {
                    // pointer, `not char *`
                    *++text = PUSH; //xx�Ľ������ջ��(����ƫ����)
                    *++text = IMM; 
                    *++text = sizeof(int);
                    *++text = MUL; 
                }
                else if (array_type < PTR) {
                    printf("%d: pointer type expected\n", line);
                    exit(-1);
                }

                expr_type = array_type - PTR;
                *++text = ADD; //�����ַ:�׵�ַ + ƫ����

                //a[10] �ȼ��� *(a + 10)
                //LC load char; LI load int
                *++text = (expr_type == CHAR) ? LC : LI;
            }
            else {
                printf("%d: compiler error, token = %d\n", line, token);
                exit(-1);
            }
        }
    }
}



static void statement() {

    int *a, *b; 

    if (token == If) {
        // Ϊif�������Ļ����룬����gcc����������������һϵ�е��Ż�����
        // if (...) <statement> [else <statement>]
        //                     //����˳����
        //   if (...)           <cond>  
        //                      JZ a    
        //     <statement>      <statement>
        //   else:              JMP b //����else����
        // a:
        //     <statement>      <statement>
        // b:                   b:
        //
        
        match(If);
        match('(');
        //��������
        expression(Assign);  
        match(')');

        // ��������float���͵ģ���ô��bx�е���ת���Ƶ�ax��
        // ת�͵ľ�����ʧ����Ӱ�������������
        if (expr_type == FLOAT || expr_type == DOUBLE){
               *++text = BTOA;
         }

        *++text = JZ;
        b = ++text; //��Ϊ���b����һ����ַ�ռ�

        //����if�е����
        statement(); //������Щϸ��     

        int offset;
        //����else����
        if (token == Else) { 
            //match������next����, �����else if��ôstatement()��ͻ�ƥ��if
            match(Else);

            // emit code for JMP B
            // TODO ������Ҫ�ض�λlocation: b, offset: text+3 - text_start
            offset = (text + 3 - text_start)*sizeof(int);
            add_relocation_item(b, offset, Text_Rel);
            *b = (int)(text + 3);
            *++text = JMP;
            b = ++text;

            statement(); //������Щϸ��
        }


        // TODO ������Ҫ�ض�λlocation: b, offset: text+1 - text_start
        offset = (text + 1 - text_start)*sizeof(int);
        add_relocation_item(b, offset, Text_Rel);
        *b = (int)(text + 1); //��������������b������
    }


    //TODO ʵ��break, continue
    else if (token == While) {
        //
        // a:                     a:
        //    while (<cond>)        <cond>
        //                          JZ b
        //     <statement>          <statement>
        //                          JMP a
        // b:                     b:
        match(While);

        a = text + 1; //a��ʼ�����<cond>

        match('(');
        expression(Assign);
        match(')');

        // ��������float���͵ģ���ô��bx�е���ת���Ƶ�ax��
        // ת�͵ľ�����ʧ����Ӱ�������������
        if (expr_type == FLOAT || expr_type == DOUBLE){
              *++text = BTOA;
        }

        *++text = JZ;
        b = ++text; //��Ϊ���b����һ����ַ�ռ�

        //TODO ��������Ŵ��ѹ���ջ�У���Ҫ��Ϊ��whileѭ����
        //�����ջΪ�յ�ʱ�򣬼���ʱ�Ļ���������whileѭ���У���ô����
        //start_label1: ,  end_label2: 
        //start_label2: ,  end_label2: 
        
        statement();

        int offset;

        //�൱��continue
        *++text = JMP;
        //TODO ������Ҫһ���ض�λlocation:text, offset=a - text_start 
        *++text = (int)a;
        offset = (a - text_start)*sizeof(int);
        add_relocation_item(text, offset, Text_Rel);


        //�൱��break
        //��������������b������, b��ʼ�����������
        //TODO ����Ҳ��Ҫһ���ض�λlocation:b, offset=text+1-text_start

        offset = (text + 1 - text_start)*sizeof(int);
        add_relocation_item(b, offset, Text_Rel);
        *b = (int)(text + 1); //b��ʼ�����������
    }

    //ƥ��if/while�е����
    else if (token == '{') {
        // { <statement> ... }
        match('{');

        while (token != '}') {
            statement();
        }

        match('}');
    }

    else if (token == Return) {
        // return [expression];
        match(Return);

        if (token != ';') {
            expression(Assign);
        }

        match(';');

        // emit code for return
        *++text = LEV;
    }

    else if (token == ';') {
        // empty statement
        match(';');
    }

    else {
        // a = b; or function_call();
        //printf("assignement\n");
        expression(Assign);
        match(';');
    }
}



static void global_declaration() 
{
    // ������������������
    if (token == Int) {
        match(Int);
        basetype = INT;
        printf("Int token\n");
    }
    else if (token == Char) {
        match(Char);
        basetype = CHAR;
        printf("Char token\n");
    }
    else if (token == Float){
        match(Float);
        basetype = FLOAT;
        printf("Float token\n");
    }else if (token == Double){
        match(Double);
        basetype = DOUBLE;
        printf("Double token\n");
    }


    // �������ɶ��ŷָ�ı������� 
    while (token != ';' && token != '}') {
        int final_type = basetype;

        // ����ָ�����ͣ���Ϊ������� "int ****var;" �Ķ༶ָ��������Ҫ��һ��ѭ
        // ����������ע����Ϊ�ڴʷ������׶ν�����ʶ����ʱ�������Ǳ�ʶ�����ַ�
        // �ͻ�ֹͣ��������� "int**** var;" ������ʽҲ�ǿ��Ե�
        while (token == Mul) {
            match(Mul);
            final_type = final_type + PTR;
        }

        if (token != Id) {
            // ����ǺŲ��Ǳ�ʶ���Ļ���Ϊ�Ƿ�����
            printf("%d: bad global declaration\n", line);
            exit(-1);
        }
        if (current_id[Class]) {
            // ��ʶ���Ѿ�����
            printf("%d: duplicate global declaration\n", line);
            exit(-1);
        }
    
        match(Id);

        //������Type��Value���ȳ���������õ�ʱ�������ȷ����
        current_id[Type] = final_type;

        if(token == Brak){
            //TODO ����֧����������, �����±�Ҫ������      
            static int* addr_keeper;
            next();
            if (token != Num){
               printf("%d: bad index\n", line);
            }
            int num = integral_token_val;

            current_id[Class] = Glo;
            //Ϊʲôcurrent_id[Value] = (int)data�Ͳ���,��������ķ��ʾ�����ָ��
            //ʵ�ֵ� data��char*���͵�

            //���������ָ��������������ʼ��ַ�����ڸ�ָ�������
            //���ﲻ��ʹ��(int)&data��Ϊdata��ȫ�ֱ���������ֵ�����ǻ�仯��
            //�����Ҫ����һ��������
            //��data���ȷ���һ���ռ����ڴ������������׵�ַ
            addr_keeper = (int*)data;
            data = data + sizeof(int);
            *addr_keeper = (int)data;

            current_id[Value] = (int)addr_keeper;
           // current_id[Value] = (int)&data;
           // printf("saved value %p\n",&data);
            current_id[Type] = final_type + PTR;
            
            int* array_addr = (int*)data;
            printf("array addr  %p\n", array_addr);
            data = data + num * sizeof(int);
            printf("new data addr  %p\n", data);

            match(Num);
            match(']');

            
            //TODO ����֧������ĳ�ʼ��
            if (token == Assign){
                //like int array[4] = {1,3,5,6};
                int i;
                
                match(Assign);
                match('{');
                for (i=0; i<num; i++){
                   if (token != Num){
                       printf("%d: bad initailzer\n", line);
                   }

                   printf("token_val is %d\n", integral_token_val);
                   printf("address %p\n", array_addr+i);
                   array_addr[i] = integral_token_val;
                   match(Num);
                   if (token == ',') {
                      match(',');
                   }else if (token == '}'){
                      match('}');
                      break;
                   }else{
                       printf("%d: bad token\n", line);
                   }
                }
                if (token == '}'){
                   match('}');
                }
            }

        }else {

            //TODO ���ݱ��������Ͳ�ͬ���䲻ͬ��С�ռ��
            current_id[Class] = Glo; 
            current_id[Value] = (int)data; 
           
            //�����Ĵ���֧�ֳ�ʼ��
            if (token == Assign){
               // ������ӵĻ���ȥ����ʼ��
               // ���� int a = 10;
               next();
               if (token != Num){
                  printf("%d: bad initailzer\n", line);
               }

               // ���ݱ������ʹ洢��Ӧ��ֵ
               if (basetype == CHAR || basetype == INT){
                   *(int*)data = (num_type == INT) ? integral_token_val :
                                                          real_token_val;
               }else if (basetype == FLOAT){
                    // ��Ϊ���ܻ����float f = 1�����������������Ҫ�ж�
                    // �ұߵ���ֵ���������͵�
                   *(float*)data = (num_type == FLOAT) ? real_token_val : 
                                                      integral_token_val;
               }
               else if (basetype == DOUBLE){
                   // ͬ��double d = 1
                   *(double*)data = (num_type == FLOAT) ? real_token_val :
                                                       integral_token_val;
               }else{
                   // TODO ָ��ĸ�ֵ
               }


               //ע��ֻ�г�ʼ����ʱ�����Ҫƥ��Num
               match(Num);
            }

            //����data��ַ�����ձ���������
            if ((basetype == INT)  || 
                (basetype == CHAR) ||
                (final_type > PTR)){
                data = data + sizeof(int);
            }else if(basetype == FLOAT){
                // �ڲ���float�����Լ�double�������㶼��������Ϊdouble��
                // �Ĵ����ģ����Ǵ����data�ϻ���Ҫ������������������
                data = data + sizeof(float);
            }else{
                data = data + sizeof(double);
            }
        }

        if (token == ',') {
            match(',');
        }
    }

    next();
}

static void parse_block_code()
{ 
    char* token_name;
    
    //use
    next();
    token_name = (char*)current_id[Name];
    if (strncmp("use", token_name, 3)){
       printf("bad use block\n");
       return;
    }
    next();
    match('{');
    while (token != '}') {
        //printf("token %d golbal decalration\n", token);
        global_declaration();
    }
    match('}');

    //action
    token_name = (char*)current_id[Name];
    if (strncmp("action", token_name, 6)){
       printf("bad action block\n");
       return;
    }

    next();
    match('{');
    while (token != '}') {
        //printf("token %d golbal decalration\n", token);
        statement();
    }
    match('}');

}


//ֻ��ʼ��һ��
int parser_init()
{
    int i, fd;
    int *tmp;

    poolsize = 256 * 1024;
    line = 1;

    //����֮��Ӧ��ֻ����text data
    // allocate memory
    if (!(text = malloc(poolsize))) {
        printf("could not malloc(%d) for text area\n", poolsize);
        return -1;
    }
    text_start = text;
    
    if (!(data = malloc(poolsize))) {
        printf("could not malloc(%d) for data area\n", poolsize);
        return -1;
    }
    data_start = data;

    //ֻ�Ǳ����ʱ���ʹ�� 
    if (!(symbols = malloc(poolsize))) {
        printf("could not malloc(%d) for symbol table\n", poolsize);
        return -1;
    }

    memset(text, 0, poolsize);
    memset(data, 0, poolsize);

}


//ÿ�α����µĴ���Ƭ�ε�ʱ����Ҫ��������һ�·��ű�
//TODO ��Щ�ǹ����Ĳ���Ӧ��ֻ��ʼ��һ��
//��ͬ�������Լ�ע��ķ��ŵ�����һ�ŷ��ű�
//���ŷ��ű�: �������ű��Լ�˽�з��ű�
static  void init_symbol_table()
{
    memset(symbols, 0, poolsize);

    // ע�����˳��Ҫ��symbol.h�еĶ�Ӧ����������ᱨ����
    // Ȼ���symbol.h�ķ���һ������صĶ�����һ�𷽱��Ժ��޸�
    char* keyword = "char int float double if else while return";
    prepare_for_tokenization(keyword, symbols); 

    int i = Char;
    while (i <= Return) {
        next();
        current_id[Token] = i++;
    }

    // ����src�еķ��Ų�������뵽��ǰ��ʶ�У�������Ҫ
    // ������������Ϊ�����ĺ������������������������µķ���
    char* libfunc = "open read close printf malloc memset memcmp exit void";
    prepare_for_tokenization(libfunc, symbols);

    i = OPEN;
    while (i <= EXIT) {
        next();
        current_id[Class] = Sys;
        current_id[Type] = FLOAT;
        current_id[Value] = i++;
    } 

    // ����void����
    next(); current_id[Token] = Char; 
}


int* compile_src_code
(
   struct dependency_items* dep_itemsp,   
   const char* src_code
)
{
 
   // ��ʼ�����ű� 
   init_symbol_table();

   // ע������
   inject_dependency(dep_itemsp);
   
   // ����Դ����
   prepare_for_tokenization(src_code, symbols);

   // ���������
   parse_block_code();

   // �ֶ�����˳�����
   *++text = EXIT;

   // �ض�λ�ı���
   int* relocated_code = relocation(text_start, text, data_start, data);

   // Ϊ������һ��������ʼ�����뻷��:text�κ�data��
   reset_complie_environment();

   return relocated_code;
}


static void inject_dependency(struct dependency_items* dep_itemsp)
{
   int num_dep_items = dep_itemsp->num_items;
   struct dependency* items = dep_itemsp->items;

   int i;
   for (i=0; i<num_dep_items; i++){
      src = items[i].var_name;

      prepare_for_tokenization(src, symbols);

      next();

      current_id[Class] = Ext;
      current_id[Type] = items[i].var_type;
      current_id[Value] = (int)items[i].var_addr;
   }
}


static void reset_complie_environment()
{
    memset(text, 0, (text - text_start) * sizeof(int));
    memset(data, 0, data - data_start);
    data = (char*)data_start;
    text = (int*)text_start;
}


static int emit_store_directive(int type)
{
   // �������������ָ�����͵ģ��Ƚ��������ַ
   // ����expr_type��Ϊ���������ʱ������ȷ��
   // �洢ָ�����Ӧ���͵�ֵ
   if (type >= PTR) return SI;

   return  (type == CHAR) ? SC : 
           (type == INT) ? SI : 
           (type == FLOAT) ? SF: SD;
}


static int emit_load_directive(int type)
{
   // �������������ָ�����͵�Ҫ�Ƚ��������ַ������expr_type
   // ��Ϊ���������ʱ������Ӧ�ļ���ָ�������Ӧ���͵�ֵ������
   // f��������float**����ôΪ���ʽ**f������ָ������������ʾ:
   // IMM f_addr LI LI LF 
   if (type >= PTR) return LI;

   // �����������char int float double
   return  (type == CHAR) ? LC : 
           (type == INT) ? LI :
           (type == FLOAT) ? LF : LD;
   
}


static int type_of_token(int token)
{
    return (token == Char) ? CHAR : 
           (token == Int) ? INT :
           (token == Float) ? FLOAT : DOUBLE; 

}


static void load_real_number_constant(double float_const)
{
    //���ظ��㳣��
    double* addr;

    *++text = FIMM;
    addr = (double*)(text + 1);
    *addr = float_const;

    // �ڲ�����������ʹ��double���ʹ洢ռ8���ֽڣ���Ϊtext��
    // int*���͵�text+2�൱����ƫ����8���ֽڵĴ�С
    text += 2;
}


static void load_integral_number_constant(int int_const)
{
    *++text = IMM;
    *++text = int_const;
}


static int get_base_type(int type)
{
    // CHAR INT FLOAT DOUBLE PTR
    // ��ΪPTR��ֵ��������ǻ������Ͷ���4���������ͼ������ɸ�
    // PTR�õ��ģ���˿���ͨ��ȡ����ȥ��ָ�����͵õ���������
    return (type % PTR); 
}


static Boolean does_operate_on_constant()
{
  // TODO �������ֻ�Ǳ�Ҫ�����������һ����������
  // ���ڲ���һ�������Ļ�����ô�ڸú��������õ�ʱ��
  // ��������Ӧ��������������������ǻ�û���ҵ����
  // ����
  return (*(text-1) == IMM || *(text-2) == FIMM);
}


static void emit_code_for_binary_left
(
   int** reserve1,
   int** reserve2
)
{
    printf("+ left type %d\n", expr_type);
    if (expr_type == FLOAT || expr_type == DOUBLE){
      // �����ص�bx����ѹ�˵�fspջ��
        *++text = PUSF;
     }else{
      // �������ı��ʽ�������Ǹ���Ļ�����Ҫ�޸�ָ��
        *++text = NOP;
        *reserve1 = text;
        *++text = PUSH; 
        *reserve2 = text;
     }        
}


static void emit_code_for_binary_right
(
   int operator_for_real_number,
   int operator_for_integral_number,
   int** reserve1,
   int** reserve2
)
{
     printf("+ right type %d\n", expr_type);
     if (expr_type == FLOAT || expr_type == DOUBLE){
         // ��ߵı��ʽ��������͵Ļ���Ҫʹ��ATOB��ax��ֵת��
         // ��double���ʹ��bx��, ָ���޸�����
         if (*reserve1 != NULL){
              *(*reserve1) = ATOB;
              *(*reserve2) = PUSF;
           }

           expr_type = DOUBLE;
           *++text = operator_for_real_number;  
      }else{
          // ǰ����Ǹ��㣬����������
          if (*reserve1 == NULL){
               // ֱ�ӽ�ax����ֵת�Ͳ������bx�У�ǰ��Ĳ������Ѿ�ѹ��
               // fspջ����
               *++text = ATOB; 
               *++text = operator_for_real_number;  
               expr_type = DOUBLE;
           }else{
            // �������������Ͷ������͵�
               *++text = operator_for_integral_number;  
               expr_type = INT;
          }
     }
}


// ��⸳ֵ"left_type = right_type"��ת��"(left_type)right_type"
// ת��Ҳ���Կ�����һ�ָ�ֵ����int(10.5) + 10.0 �����20������
// 20.5������������ͬһ���������м��
static void check_assignment_types(int left_type, int right_type)
{
    if (left_type == right_type) return;
 
    // Ϊ�˰�ȫ�����ͬ���͵�ָ���Լ��������ͺ�ָ�����Ͳ����໥��ֵ
    // ��ת�ͣ���������ת����"���޵�ת��"
    if (left_type >= PTR || right_type >= PTR){
        
        Boolean is_same_level = (left_type/PTR == right_type/PTR);
        if (is_same_level) return;

        char left_str_repr[64], right_str_repr[64];
        numtype_to_strtype(left_type, left_str_repr);
        numtype_to_strtype(right_type, right_str_repr);
        printf("%d: bad types in assignment or cast:\n", line);
        printf("left type: %s, right type: %s\n", left_str_repr, right_str_repr);
        exit(-1);
    }

    // ��ֵ����Ǹ����ͣ��ұ�������
    if ((left_type == FLOAT || left_type == DOUBLE) &&
        (right_type == INT || right_type == CHAR)){

            *++text = ATOB; 
        
     }else if ((left_type == INT || left_type == CHAR) &&
              (right_type == FLOAT || right_type == DOUBLE)){
    // ��ֵ��������ͣ��ұ��Ǹ�����

            *++text = BTOA; 
     }else{
    // ��������Ķ�������Ķ��Ҳ���Ҫ���ж���Ĺ��� 
     }
}


