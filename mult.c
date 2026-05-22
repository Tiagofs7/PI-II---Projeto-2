#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simulador.h"

#define MAX_HISTORICO 1000

typedef struct {
    int memoria[256];
    int registradores[8];
    int PC;
    int RI;
    int A;
    int B;
    int ULAout;
    int RDM;
    int estado;
    int ciclos_executados;
    int instrucoes_executadas;
    int qtd_tipo_r;
    int qtd_addi;
    int qtd_beq;
    int qtd_lw;
    int qtd_sw;
    int qtd_jump;
    int qtd_invalidas;
} EstadoSimulador;

int RI;
int A,B;
int ULAout;
int RDM;
int estado = BUSCA;
int ciclos_executados = 0;
int instrucoes_executadas = 0;
int qtd_tipo_r = 0;
int qtd_addi = 0;
int qtd_beq = 0;
int qtd_lw = 0;
int qtd_sw = 0;
int qtd_jump = 0;
int qtd_invalidas = 0;
EstadoSimulador historico[MAX_HISTORICO];
int topo_historico = 0;

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

void salvar_estado(int memoria[], int registradores[], int PC) {
    if (topo_historico >= MAX_HISTORICO) {
        printf("Historico cheio. Nao foi possivel salvar este estado.\n");
        return;
    }

    for (int i = 0; i < 256; i++) {
        historico[topo_historico].memoria[i] = memoria[i];
    }

    for (int i = 0; i < 8; i++) {
        historico[topo_historico].registradores[i] = registradores[i];
    }

    historico[topo_historico].PC = PC;
    historico[topo_historico].RI = RI;
    historico[topo_historico].A = A;
    historico[topo_historico].B = B;
    historico[topo_historico].ULAout = ULAout;
    historico[topo_historico].RDM = RDM;
    historico[topo_historico].estado = estado;
    historico[topo_historico].ciclos_executados = ciclos_executados;
    historico[topo_historico].instrucoes_executadas = instrucoes_executadas;
    historico[topo_historico].qtd_tipo_r = qtd_tipo_r;
    historico[topo_historico].qtd_addi = qtd_addi;
    historico[topo_historico].qtd_beq = qtd_beq;
    historico[topo_historico].qtd_lw = qtd_lw;
    historico[topo_historico].qtd_sw = qtd_sw;
    historico[topo_historico].qtd_jump = qtd_jump;
    historico[topo_historico].qtd_invalidas = qtd_invalidas;

    topo_historico++;
}

int stepback(int memoria[], int registradores[], int *PC) {
    if (topo_historico == 0) {
        printf("Nao ha estado anterior para voltar.\n");
        return 0;
    }

    topo_historico--;

    for (int i = 0; i < 256; i++) {
        memoria[i] = historico[topo_historico].memoria[i];
    }

    for (int i = 0; i < 8; i++) {
        registradores[i] = historico[topo_historico].registradores[i];
    }

    *PC = historico[topo_historico].PC;
    RI = historico[topo_historico].RI;
    A = historico[topo_historico].A;
    B = historico[topo_historico].B;
    ULAout = historico[topo_historico].ULAout;
    RDM = historico[topo_historico].RDM;
    estado = historico[topo_historico].estado;
    ciclos_executados = historico[topo_historico].ciclos_executados;
    instrucoes_executadas = historico[topo_historico].instrucoes_executadas;
    qtd_tipo_r = historico[topo_historico].qtd_tipo_r;
    qtd_addi = historico[topo_historico].qtd_addi;
    qtd_beq = historico[topo_historico].qtd_beq;
    qtd_lw = historico[topo_historico].qtd_lw;
    qtd_sw = historico[topo_historico].qtd_sw;
    qtd_jump = historico[topo_historico].qtd_jump;
    qtd_invalidas = historico[topo_historico].qtd_invalidas;

    printf("\n[STEPBACK] Voltou para Estado: %s | PC=%d\n", nome_estado(estado), *PC);
    return 1;
}

void limpar_historico() {
    topo_historico = 0;
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
    int estado_anterior = estado;
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
    ciclos_executados++;

    if (estado == BUSCA && estado_anterior != BUSCA) {
        instrucoes_executadas++;

        switch (c.opcode) {
            case 0:
                qtd_tipo_r++;
                break;
            case 4:
                qtd_addi++;
                break;
            case 8:
                qtd_beq++;
                break;
            case 11:
                qtd_lw++;
                break;
            case 15:
                qtd_sw++;
                break;
            case 2:
                qtd_jump++;
                break;
            default:
                qtd_invalidas++;
                break;
        }
    }
}
void imprimir_estado_cpu(int PC, int *regs){
    printf("\n=== Estado da CPU ===\n");
    printf("RI = %d\n", RI);
    printf("ULAout = %d\n", ULAout);
    printf("RDM = %d\n", RDM);
    
    if (estado == DECODE) {
        decode c = campos(RI);
        printf("A = %d ($%d)\n", regs[c.rs], c.rs);
        printf("B = %d ($%d)\n", regs[c.rt], c.rt);
    } else {
        printf("A = %d\n", A);
        printf("B = %d\n", B);
    }
    
    printf("Estado = %d\n", estado);
}
void imprimir_registradores(int registradores[]) {
    printf("\n=== Registradores ===\n");
    for (int i = 0; i < 8; i++) {
        printf("$%d = %d\n", i, registradores[i]);
    }
}

void imprimir_estatisticas(int PC, int num_instrucoes) {
    double cpi = 0.0;

    if (instrucoes_executadas > 0) {
        cpi = (double)ciclos_executados / instrucoes_executadas;
    }

    printf("\n=== Estatisticas ===\n");
    printf("Instrucoes carregadas: %d\n", num_instrucoes);
    printf("Instrucoes concluidas: %d\n", instrucoes_executadas);
    printf("Ciclos executados: %d\n", ciclos_executados);
    printf("CPI medio: %.2f\n", cpi);
    printf("PC atual: %d\n", PC);
    printf("Estado atual: %s\n", nome_estado(estado));

    printf("\n--- Por tipo ---\n");
    printf("Tipo R: %d\n", qtd_tipo_r);
    printf("ADDI: %d\n", qtd_addi);
    printf("BEQ: %d\n", qtd_beq);
    printf("LW: %d\n", qtd_lw);
    printf("SW: %d\n", qtd_sw);
    printf("JUMP: %d\n", qtd_jump);

    if (qtd_invalidas > 0) {
        printf("Invalidas/desconhecidas: %d\n", qtd_invalidas);
    }
}

void imprimir_memoria_ID(int memoria[], int num_instrucoes, int tam_dados) {
    printf("\n=== MEMORIA ===\n\n");
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
const char *nome_estado(int estado_atual) {
    const char *nomes_estado[] = {
        "BUSCA","DECODE","EXEC","WRITE","MEM_ADDR",
        "MEM_READ","MEM_WRITEBACK","MEM_WRITE","BRANCH","JUMP"
    };

    if (estado_atual < BUSCA || estado_atual > JUMP) {
        return "DESCONHECIDO";
    }

    return nomes_estado[estado_atual];
}

void imprimir_step_estado(int memoria[], int registradores[], int PC) {
    decode c = campos(RI);
    char asm_str[64];
    int instrucao_atual = (estado == BUSCA) ? memoria[PC] : RI;

    instrucao_para_asm(instrucao_atual, asm_str);

    printf("\n[STEP] Estado: %s | PC=%d | %s\n", nome_estado(estado), PC, asm_str);
    printf("Instrucao binaria: ");
    for (int i = 15; i >= 0; i--) {
        printf("%d", (instrucao_atual >> i) & 1);
    }
    printf("\n");

    switch (estado) {
        case BUSCA:
            printf("Memoria[%d] -> RI = %d\n", PC, memoria[PC]);
            printf("ULA: PC + 1 = %d\n", PC + 1);
            printf("PC recebe %d\n", PC + 1);
            break;

        case DECODE:
            printf("RI = %d\n", RI);
            printf("rs = $%d -> A = %d\n", c.rs, registradores[c.rs]);
            printf("rt = $%d -> B = %d\n", c.rt, registradores[c.rt]);
            printf("ULA: PC + imm = %d + %d = %d\n", PC, c.imm, PC + c.imm);
            break;

        case EXEC:
            if (c.opcode == 0) {
                printf("ULA: A(%d) op B(%d), funct=%d\n", A, B, c.funct);
            } else {
                printf("ULA: A(%d) op imm(%d)\n", A, c.imm);
            }
            break;

        case WRITE:
            printf("ULAout = %d\n", ULAout);
            printf("Registrador destino: $%d\n", c.opcode == 0 ? c.rd : c.rt);
            break;

        case MEM_ADDR:
            printf("ULA: A(%d) + imm(%d) = endereco de memoria\n", A, c.imm);
            break;

        case MEM_READ:
            printf("Endereco: ULAout = %d\n", ULAout);
            printf("Memoria[%d] -> RDM\n", ULAout);
            break;

        case MEM_WRITEBACK:
            printf("RDM = %d\n", RDM);
            printf("Registrador destino: $%d\n", c.rt);
            break;

        case MEM_WRITE:
            printf("Endereco: ULAout = %d\n", ULAout);
            printf("Valor: B = %d\n", B);
            printf("Memoria[%d] recebe %d\n", ULAout, B);
            break;

        case BRANCH:
            printf("ULA: A(%d) - B(%d)\n", A, B);
            printf("Se resultado for zero, PC recebe ULAout(%d)\n", ULAout);
            break;

        case JUMP:
            printf("Endereco do jump: %d\n", c.addr);
            printf("PC recebe %d\n", c.addr);
            break;
    }
}

void step(int memoria_instrucao[], int registradores[], int *PC, int num_instrucoes){
    (void)num_instrucoes;

    salvar_estado(memoria_instrucao, registradores, *PC);
    imprimir_step_estado(memoria_instrucao, registradores, *PC);
    ciclo(memoria_instrucao, registradores, PC);
    printf("[STEP] Proximo estado: %s | PC=%d\n", nome_estado(estado), *PC);
}

void run(int memoria_instrucao[], int registradores[], int *PC, int num_instrucoes) {
    while (estado != BUSCA || *PC < 128) {
        printf("\n--- STEP ---\n");
        ciclo(memoria_instrucao, registradores, PC);        
        imprimir_estado_cpu(*PC, registradores);
        imprimir_registradores(registradores);
    }
    imprimir_memoria_ID(memoria_instrucao, num_instrucoes, 256 - num_instrucoes);
}
