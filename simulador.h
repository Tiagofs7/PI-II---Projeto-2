#ifndef SIMULADOR_H
#define SIMULADOR_H

//estados fsm
#define BUSCA 0
#define DECODE 1
#define EXEC 2
#define WRITE 3
#define MEM_ADDR 4
#define MEM_READ 5
#define MEM_WRITEBACK 6
#define MEM_WRITE 7
#define BRANCH 8
#define JUMP 9
typedef struct decode{
    int opcode;
    int rs;
    int rt;
    int rd;
    int funct;
    int imm;
    int addr;
} decode;
typedef struct sinaisControle {
    int PCEsc;
    int IouD;
    int EscMem; 
    int IREsc;
    int LerRegs;
    int LerMem; 
    int MemParaReg; 
    int EscReg;
    int RegDst;
    int ULAFonteA, ULAFonteB, ULAControle, PCFonte, Branch;
} sinaisControle;

int controle_ULA(int opcode, int funct);
int ULA(int A, int B, int controle, int *flag);
int MUX2(int entrada0, int entrada1, int controle);
int MUX4(int entrada0, int entrada1, int entrada2, int entrada3, int controle);
void ciclo(int memoria[], int registradores[], int *PC);
void inicializar_registradores(int registradores[]);
void step(int memoria[], int registradores[], int *PC, int num_instrucoes);
int stepback(int memoria[], int registradores[], int *PC);
void limpar_historico();
void run(int memoria[], int registradores[], int *PC, int num_instrucoes);
void imprimir_estatisticas(int PC, int num_instrucoes);
decode campos(int instrucao);
sinaisControle gerarSinais(int estado, int opcode, int funct);
int proximo_estado(int estado, int opcode);
const char *nome_estado(int estado_atual);
int leitura_arquivo_mem(int memoria[], char nome_arquivo[]);
void instrucao_para_asm(int instrucao, char *buf);
void imprimir_estado_cpu(int PC, int *regs);
void imprimir_registradores(int registradores[]);
void imprimir_memoria_ID(int memoria[], int num_instrucoes, int tam_dados);


extern int RI, A, B, ULAout, RDM, estado;

#endif
