#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simulador.h"

int main() {
    int memoria[256] = {0};
    int registradores[8] = {0};
    int PC = 0;
    int num_instrucoes = 0;
    int opcao;

    printf("         =======================================\n");
    printf("              SIMULADOR MIPS MULTICICLO\n");
    printf("         =======================================\n");

    inicializar_registradores(registradores);
    do {
        printf("\n");
        printf(" ╔══════════════════════════════════════════════════╗\n");
        printf(" ║           Menu do Simulador MiniMips             ║\n");
        printf(" ╠══════════════════════════════════════════════════╣\n");
        printf(" ║                                                  ║\n");
        printf(" ║         [1] Carregar memoria (.mem)              ║\n");
        printf(" ║         [2] Imprimir memorias                    ║\n");
        printf(" ║         [3] Imprimir registradores               ║\n");
        printf(" ║                                                  ║\n");
        printf(" ║         [4] Executar programa  (Run)             ║\n");
        printf(" ║         [5] Executar um ciclo  (Step)            ║\n");
        printf(" ║                                                  ║\n");
        printf(" ║         [6] Estatisticas                         ║\n");
        printf(" ║         [0] Sair                                 ║\n");
        printf(" ║                                                  ║\n");
        printf(" ╚══════════════════════════════════════════════════╝\n");
        printf("\n Escolha: ");
        scanf("%d", &opcao);
        getchar();


        switch (opcao) {
            case 1: {
                char nome[100];
                printf("Nome do arquivo .mem: ");
                scanf("%s", nome);
                getchar();
                memset(memoria, 0, sizeof(memoria));
                PC = 0;
                estado = BUSCA;
                inicializar_registradores(registradores);
                num_instrucoes = leitura_arquivo_mem(memoria, nome);
                break;
            }
            case 2:
                if (num_instrucoes == 0) { 
                printf("Carregue um arquivo .mem primeiro.\n"); 
                break; 
            }
                imprimir_memoria_instrucao(memoria, num_instrucoes);
                imprimir_memoria_dados(memoria, 128, 255);
                break;
            case 3:
                imprimir_registradores(registradores);
                break;
            case 4: {
                if (num_instrucoes == 0) { printf("Carregue um arquivo .mem primeiro.\n"); break; }
                printf("\n--- Iniciando Execucao ---\n");
                const char *nomes_estado[] = {
                    "BUSCA","DECODE","EXEC","WRITE","MEM_ADDR",
                    "MEM_READ","MEM_WRITEBACK","MEM_WRITE","BRANCH","JUMP"
                };
                while (estado != BUSCA || PC < num_instrucoes) {
                    char asm_str[64];
                    instrucao_para_asm(estado == BUSCA ? memoria[PC] : RI, asm_str);
                    printf("[%s] PC=%d | %s\n", nomes_estado[estado], PC, asm_str);
                    ciclo(memoria, registradores, &PC);
                    imprimir_estado_cpu(PC);
                    imprimir_registradores(registradores);
                }
                printf("--- Execucao concluida ---\n");
                imprimir_memoria_dados(memoria, 128, 255);
                break;
            }
            case 5: {
                if (num_instrucoes == 0) { printf("Carregue um arquivo .mem primeiro.\n"); break; }

                const char *nomes_estado[] = {
                    "BUSCA","DECODE","EXEC","WRITE","MEM_ADDR",
                    "MEM_READ","MEM_WRITEBACK","MEM_WRITE","BRANCH","JUMP"
                };

                char asm_str[64];
                instrucao_para_asm(estado == BUSCA ? memoria[PC] : RI, asm_str);

                printf("\n[STEP] PC=%d | Estado: %s | %s\n", PC, nomes_estado[estado], asm_str);
                ciclo(memoria, registradores, &PC);
                printf("[STEP] Proximo estado: %s\n", nomes_estado[estado]);
                imprimir_estado_cpu(PC);
                imprimir_registradores(registradores);
                break;
            }
            case 6:
                printf("Funcao nao implementada.\n");
                break;
            case 0:
                printf("Saindo...\n");
                break;
            default:
                printf("Opcao invalida. Tente novamente.\n");
        }
    } while (opcao != 0);

    return 0;
}
