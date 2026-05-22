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
    printf("\n");
    printf("         ---------------------------------------\n");
    printf("              SIMULADOR MIPS MULTICICLO\n");
    printf("         ---------------------------------------\n");

    inicializar_registradores(registradores);
    do {
        printf("\n");
        printf(" ----------------------------------------------------\n");
        printf(" |           Menu do Simulador MiniMips             |\n");
        printf(" |--------------------------------------------------|\n");
        printf(" |                                                  |\n");
        printf(" |         [1] Carregar memoria (.mem)              |\n");
        printf(" |         [2] Imprimir memorias                    |\n");
        printf(" |         [3] Imprimir registradores               |\n");
        printf(" |                                                  |\n");
        printf(" |         [4] Executar programa  (Run)             |\n");
        printf(" |         [5] Executar um ciclo  (Step)            |\n");
        printf(" |         [6] Voltar um ciclo (Stepback)           |\n");
        printf(" |                                                  |\n");
        printf(" |         [7] Estatisticas                         |\n");
        printf(" |         [8] Mostrar Assembly                     |\n");
        printf(" |         [0] Sair                                 |\n");
        printf(" |                                                  |\n");
        printf(" ----------------------------------------------------\n");
        printf("\n Escolha: ");
        scanf("%d", &opcao);
        getchar();


        int estado_antes = estado;
        switch (opcao) {
            case 1: {
                char nome[100];
                printf("Nome do arquivo .mem: ");
                scanf("%s", nome);
                printf("Arquivo carregado: %s\n", nome);
                getchar();
                memset(memoria, 0, sizeof(memoria));
                PC = 0;
                estado = BUSCA;
                inicializar_registradores(registradores);
                limpar_historico();
                num_instrucoes = leitura_arquivo_mem(memoria, nome);
                break;
            }
            case 2:
                if (num_instrucoes == 0) { 
                printf("Carregue um arquivo .mem primeiro.\n"); 
                break; 
            }
                imprimir_memoria_ID(memoria, num_instrucoes);
                break;
            case 3:
                imprimir_registradores(registradores);
                break;
            case 4: {
                if (num_instrucoes == 0) { printf("Carregue um arquivo .mem primeiro.\n"); break; }
                printf("\n--- Iniciando Execucao ---\n");
                run(memoria, registradores, &PC, num_instrucoes);
                printf("\n--- Execucao concluida ---\n");
                break;
            }
            case 5: {
                if (num_instrucoes == 0) { printf("Carregue um arquivo .mem primeiro.\n"); break; }

                step(memoria, registradores, &PC, num_instrucoes);
                break;
            }
            case 6: {
                if (num_instrucoes == 0) { printf("Carregue um arquivo .mem primeiro.\n"); break; }

                stepback(memoria, registradores, &PC);
                break;
            }
            case 7:
                imprimir_estatisticas(PC, num_instrucoes);
                break;
            case 8: {
            if (num_instrucoes == 0) { printf("Carregue um arquivo .mem primeiro.\n"); break; }
                printf("\n=== Assembly ===\n");
                    for (int i = 0; i < num_instrucoes; i++) {
                            char asm_str[64];
                instrucao_para_asm(memoria[i], asm_str);
                printf("mem[%d]: %s\n", i, asm_str);
                }
    break;
}
            case 0:
                printf("Saindo...\n");
                break;
            default:
                printf("Opcao invalida. Tente novamente.\n");
        }

        if (opcao != 0) {
            imprimir_estado_cpu(PC, registradores);
            // stepback restaura o estado anterior: usa estado atual (ja restaurado)
            // outras opcoes: usam estado antes da acao para mostrar sinais do ciclo executado
            int est_display = (opcao == 6) ? estado : estado_antes;
            imprimir_sinais_atuais(est_display);
        }
    } while (opcao != 0);

    return 0;
}
