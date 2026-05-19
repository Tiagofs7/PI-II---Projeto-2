#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simulador.h"

int RI;
int A,B;
int ULAout;
int RDM;
int estado = BUSCA;

void instrucao_para_asm(int instrucao, char *buf) {
    int op   = (instrucao >> 12) & 0xF;
    int rs   = (instrucao >> 9)  & 0x7;
    int rt   = (instrucao >> 6)  & 0x7;
    int rd   = (instrucao >> 3)  & 0x7;
    int fn   =  instrucao & 0x7;
    int imm  =  instrucao & 0x3F; if (imm >= 32) imm -= 64;

    if (op == 0) sprintf(buf, "tipo R: $%d = $%d op $%d (funct=%d)", rd, rs, rt, fn);
    else if (op == 4)  sprintf(buf, "addi $%d = $%d + %d", rt, rs, imm);
    else if (op == 8)  sprintf(buf, "beq $%d, $%d, %d",    rs, rt, imm);
    else if (op == 11) sprintf(buf, "lw $%d, %d($%d)",     rt, imm, rs);
    else if (op == 15) sprintf(buf, "sw $%d, %d($%d)",     rt, imm, rs);
    else if (op == 2)  sprintf(buf, "jump %d", instrucao & 0xFF);
    else sprintf(buf, "op=%d", op);
}

void escolher_arquivo_mem(char nome_arquivo[]){
    FILE *arquivo;

    printf("\nDigite o nome do arquivo .mem: ");
    scanf("%s", nome_arquivo);
    arquivo = fopen(nome_arquivo, "r");
    printf("Arquivo carregado.\n");
    if (arquivo == NULL) {
        printf("Erro: o arquivo %s nao foi encontrado.\n", nome_arquivo);
    }

    fclose(arquivo);
}

int leitura_arquivo_mem(int memoria[], char nome_arquivo[]) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return 0;
    }

    char linha[100];
    int i = 0, dados = 0;

    while (fscanf(arquivo, "%s", linha) != EOF) {
        if (strcmp(linha, ".data") == 0) { dados = 1; continue; }

        if (dados) {
            char *sep = strchr(linha, ':');
            if (sep) { *sep = '\0'; memoria[atoi(linha)] = (int)strtol(sep+1, NULL, 2); }
        } else {
            memoria[i++] = (int)strtol(linha, NULL, 2);
        }
    }

    fclose(arquivo);
    return i;
}

decode campos(int instrucao){
    decode c;
    
    c.opcode = (instrucao >> 12) & 0xF;
    c.rs = (instrucao >> 9) & 0x7;
    c.rt = (instrucao >> 6) & 0x7;
    c.rd = (instrucao >> 3) & 0x7;
    c.funct = instrucao & 0x7;
    c.imm = instrucao & 0x3F;
    c.addr = instrucao & 0xFF;

    if (c.imm >= 32) { // extensao de sinal
        c.imm -= 64; 
    }
    
    return c;
}

void inicializar_registradores(int registradores[]) {
    for (int i = 0; i < 8; i++) {
        registradores[i] = 0;
    }
}

void guardarIR(int instrucao){
    RI = instrucao;
    
}

int retornarIR(){
    return RI;
}

// MUX de 2 entradas (para sinais de controle de 1 bit)
int MUX2(int entrada0, int entrada1, int controle) {
    return (controle == 0) ? entrada0 : entrada1;
}

// MUX de 3/4 entradas (para sinais de controle de 2 bits)
int MUX4(int entrada0, int entrada1, int entrada2, int entrada3, int controle) {
    switch(controle) {
        case 0: return entrada0;
        case 1: return entrada1;
        case 2: return entrada2;
        case 3: return entrada3;
        default: return entrada0;
    }
}

sinaisControle gerarSinais(int estado, int opcode, int funct) {
    sinaisControle s = {0};
    switch (estado) {
        case BUSCA:
            s.IREsc = 1;
            s.LerMem = 1;
            s.ULAFonteB = 1;
            s.ULAControle = 0;
            s.PCEsc = 1;
            s.PCFonte = 0;
            break;
        case DECODE:
            s.LerRegs = 1;
            s.ULAFonteB = 2;
            s.ULAControle = 0;
            break;
        case EXEC:
            s.ULAFonteA = 1;
            s.ULAFonteB = (opcode == 0) ? 0 : 2;
            s.ULAControle = controle_ULA(opcode, funct);
            break;
        case MEM_ADDR:
            s.ULAFonteA = 1;
            s.ULAFonteB = 2;
            s.ULAControle = 0;
            break;
        case MEM_READ:
            s.IouD = 1;
            s.LerMem = 1;
            break;
        case MEM_WRITEBACK:
            s.EscReg = 1;
            s.MemParaReg = 1;
            break;
        case MEM_WRITE:
            s.IouD = 1;
            s.EscMem = 1;
            break;
        case WRITE:
            s.EscReg = 1;
            s.RegDst = (opcode == 0) ? 1 : 0;
            break;
        case BRANCH:
            s.ULAFonteA = 1;
            s.ULAControle = 2;
            s.Branch = 1;
            s.PCFonte = 1;
            break;
        case JUMP:
            s.PCEsc = 1;
            s.PCFonte = 2;
            break;
    }
    return s;
}
int proximo_estado(int estado, int opcode) {
    switch (estado) {
        case BUSCA:return DECODE;
        case DECODE:
            if (opcode == 0 || opcode == 4) return EXEC;
            if (opcode == 11 || opcode == 15) return MEM_ADDR;
            if (opcode == 8)  return BRANCH;
            if (opcode == 2)  return JUMP;
            return BUSCA;
        case EXEC: return WRITE;
        case MEM_ADDR: return (opcode == 11) ? MEM_READ : MEM_WRITE;
        case MEM_READ: return MEM_WRITEBACK;
        default: return BUSCA;
    }
}
int controle_ULA(int opcode, int funct) {
    switch(opcode) {
    case 0: // tipo R
        switch(funct) {
            case 0: return 0; // ADD
            case 2: return 2; // SUB
            case 4: return 4; // AND
            case 5: return 5; // OR
            default: return -1;
            }
    case 11: // LW
    case 15: // SW
    case 4:  // ADDI
        return 0;
    case 8: // BEQ
        return 2; // sub para fazer a comparação
    default:
        return -1;
    }
}

int ULA(int A, int B, int controle, int *flag) {
    int resultado = 0;
    
    switch(controle) {
        case 0:
            resultado = A + B; 
            break;
        case 2: 
            resultado = A - B;
            break;
        case 4: 
            resultado = A & B; 
            break;
        case 5: 
            resultado = A | B; 
            break;
        default: 
            resultado = 0;
    }
    
    *flag = (resultado == 0);
    
    if (resultado > 127 || resultado < -128) {
        printf("Overflow.\n");
    }
    
    return resultado;
}

void carregarULAout(int resultado){
    ULAout = resultado;
}

int lerULAout(){
    return ULAout;
}
void ciclo(int mem[], int regs[], int *PC) {
    decode c = campos(RI);
    sinaisControle s = gerarSinais(estado, c.opcode, c.funct);
    int flag = 0;

    int ula_A = MUX2(*PC, A, s.ULAFonteA);
    int ula_B = MUX4(B, 1, c.imm, c.imm, s.ULAFonteB);
    int res = ULA(ula_A, ula_B, s.ULAControle, &flag);
    int end = MUX2(*PC, ULAout, s.IouD);

    if (s.IREsc) RI  = mem[end];
    if (s.LerRegs) { A   = regs[c.rs]; B = regs[c.rt]; }
    if (s.LerMem && !s.IREsc) RDM = mem[end];
    if (s.EscMem) mem[end] = B;

    if (estado==BUSCA || estado==DECODE || estado==EXEC || estado==MEM_ADDR)
        ULAout = res;

    if (s.EscReg)
        regs[MUX2(c.rt, c.rd, s.RegDst)] = MUX2(ULAout, RDM, s.MemParaReg);

    if (s.PCEsc) *PC = MUX4(res, ULAout, c.addr, 0, s.PCFonte);
    if (s.Branch && flag)*PC = ULAout;

    estado = proximo_estado(estado, c.opcode);
}
void imprimir_estado_cpu(int PC){
    printf("\n=== Estado da CPU ===\n");
    printf("PC = %d\n", PC);
    printf("RI = %d\n", RI);
    printf("ULAout = %d\n", ULAout);
    printf("RDM = %d\n", RDM);
    printf("Estado = %d\n", estado);
}
void imprimir_registradores(int registradores[]) {
    printf("\n=== Registradores ===\n");
    for (int i = 0; i < 8; i++) {
        printf("$%d = %d\n", i, registradores[i]);
    }
}
void imprimir_memoria_ID(int memoria[], int num_instrucoes, int tam_dados) {
    printf("\n=== MEMÓRIA ===\n\n");
    for (int i = 0; i < num_instrucoes; i++) {
        char buf[100];
        instrucao_para_asm(memoria[i], buf);
        printf("mem[%d] = %d -> %s\n", i, memoria[i], buf);
    }
    int inicio_dados = num_instrucoes;
    int fim_dados = num_instrucoes + tam_dados - 1;
    
    for (int i = inicio_dados; i <= fim_dados; i++) {
        printf("mem[%d] = %d\n", i, memoria[i]);
    }
}
void step(int memoria_instrucao[], int registradores[], int *PC, int num_instrucoes){
    ciclo(memoria_instrucao, registradores, PC);
    imprimir_estado_cpu(*PC);
    imprimir_registradores(registradores);
}

void run(int memoria_instrucao[], int registradores[], int *PC, int num_instrucoes) {
    while (estado != BUSCA || *PC < num_instrucoes) {
        ciclo(memoria_instrucao, registradores, PC);        
        imprimir_estado_cpu(*PC);
        imprimir_registradores(registradores);
    }
    imprimir_memoria_ID(memoria_instrucao, num_instrucoes, 256 - num_instrucoes);
}
