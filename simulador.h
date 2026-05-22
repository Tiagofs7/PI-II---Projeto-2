#ifndef SIMULADOR_H
#define SIMULADOR_H

// Estados da FSM
#define BUSCA        0
#define DECODE       1
#define EXEC         2
#define WRITE        3
#define MEM_ADDR     4
#define MEM_READ     5
#define MEM_WRITEBACK 6
#define MEM_WRITE    7
#define BRANCH       8
#define JUMP         9

typedef struct decode {
    int opcode;
    int rs;
    int rt;
    int rd;
    int funct;
    int imm;
    int addr;
} decode;

typedef struct sinaisControle {
    int PCEsc, IouD, EscMem, IREsc;
    int LerRegs, LerMem, MemParaReg, EscReg, RegDst;
    int ULAFonteA, ULAFonteB, ULAControle, PCFonte, Branch;
} sinaisControle;

// Datapath
int  controle_ULA(int opcode, int funct);
int  ULA(int A, int B, int controle, int *flag);
int  MUX2(int entrada0, int entrada1, int controle);
int  MUX4(int entrada0, int entrada1, int entrada2, int entrada3, int controle);
decode        campos(int instrucao);
sinaisControle gerarSinais(int estado, int opcode, int funct);
int  proximo_estado(int estado, int opcode);

// Execucao
void ciclo(int memoria[], int registradores[], int *PC);
void run(int memoria[], int registradores[], int *PC, int num_instrucoes);
void step(int memoria[], int registradores[], int *PC, int num_instrucoes);
int  stepback(int memoria[], int registradores[], int *PC);
void salvar_estado(int memoria[], int registradores[], int PC);
void limpar_historico(void);

// Inicializacao
void inicializar_registradores(int registradores[]);
int  leitura_arquivo_mem(int memoria[], char nome_arquivo[]);

// Impressao
void instrucao_para_asm(int instrucao, char *buf);
const char *nome_estado(int estado_atual);
void imprimir_estado_cpu(int PC, int *regs);
void imprimir_registradores(int registradores[]);
void imprimir_estatisticas(int PC, int num_instrucoes);
void imprimir_memoria_ID(int memoria[], int num_instrucoes);
void imprimir_step_estado(int memoria[], int registradores[], int PC);
void imprimir_sinais_atuais(int est);
void to_bin(int val, int bits, char *buf);

// Variaveis globais
extern int RI, A, B, ULAout, RDM, estado;
extern int ciclos_executados, instrucoes_executadas;
extern int qtd_tipo_r, qtd_addi, qtd_beq, qtd_lw, qtd_sw, qtd_jump, qtd_invalidas;
extern int topo_historico;

#endif
