#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

// Estrutura para representar uma ação do cliente, incluindo o estado atual do tabuleiro
struct action {
    int type;
    int coordinates[2];
    int board[4][4];
};

// Função para enviar uma ação para o servidor
void sendAction(int socket, struct action *action) {
    send(socket, action, sizeof(struct action), 0);
}

// Função para imprimir o tabuleiro
void printBoard(int board[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            // Converte os valores do tabuleiro em caracteres para impressão
            char cell;
            switch (board[i][j]) {
                case -1:
                    cell = '*'; // Bomba
                    break;
                case -2:
                    cell = '-'; // Célula oculta
                    break;
                case -3:
                    cell = '>'; // Célula com flag
                    break;
                default:
                    cell = '0' + board[i][j]; // Número de bombas na vizinhança
                    break;
            }
            printf("%c\t", cell);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso correto: %s <endereço IP do servidor> <número de porta>\n", argv[0]);
        exit(1);
    }

    const char *serverIP = argv[1];
    int serverPort = atoi(argv[2]);

    // Configuração do cliente
    int clientSocket;
    struct sockaddr_in serverAddr;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Erro ao criar o socket do cliente");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0) {
        perror("Endereço IP inválido");
        exit(1);
    }

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Erro ao conectar ao servidor");
        exit(1);
    }

    printf("Conectado ao servidor\n");

    // Loop principal do cliente
    while (1) {
        // Estrutura para armazenar a ação de jogo
        struct action gameAction;

        // Variável para armazenar o comando do usuário
        char command[10];

        // Solicitar o comando do usuário
        printf("Digite um comando: ");
        scanf("%s", command);

        // Configurar a ação do cliente de acordo com o comando
        if (strcmp(command, "start") == 0) {
            // Enviar o comando "start" para o servidor
            gameAction.type = 0;
        } else if (strcmp(command, "reveal") == 0) {
            // Enviar o comando "reveal" para o servidor, juntamente com as coordenadas da célula a ser revelada
            printf("Informe as coordenadas (linha coluna): ");
            scanf("%d %d", &gameAction.coordinates[0], &gameAction.coordinates[1]);

            // **Erro: Célula fora do range do tabuleiro**
            if (gameAction.coordinates[0] < 0 || gameAction.coordinates[0] >= 4 || gameAction.coordinates[1] < 0 || gameAction.coordinates[1] >= 4) {
                printf("error: invalid cell\n");
                continue;
            }

            // **Erro: Revela uma célula já revelada**
            if (gameAction.board[gameAction.coordinates[0]][gameAction.coordinates[1]] != -2) {
                printf("error: cell already revealed\n");
                continue;
            }

            gameAction.type = 1;
        } else if (strcmp(command, "flag") == 0) {
            // Enviar o comando "flag" para o servidor, juntamente com as coordenadas da célula a ser marcada com uma bandeira
            printf("Informe as coordenadas (linha coluna): ");
            scanf("%d %d", &gameAction.coordinates[0], &gameAction.coordinates[1]);

            // **Erro: Flag em uma célula já marcada**
            if (gameAction.board[gameAction.coordinates[0]][gameAction.coordinates[1]] == -3) {
                printf("error: cell already has a flag\n");
                continue;
            }

            // **Erro: Flag em uma célula revelada**
            if (gameAction.board[gameAction.coordinates[0]][gameAction.coordinates[1]] != -2) {
                printf("error: cannot insert flag in revealed cell\n");
                continue;
            }

            gameAction.type = 2;
        } else if (strcmp(command, "remove_flag") == 0) {
            // Enviar o comando "remove_flag" para o servidor, juntamente com as coordenadas da célula a ter a bandeira removida
            printf("Informe as coordenadas (linha coluna): ");
            scanf("%d %d", &gameAction.coordinates[0], &gameAction.coordinates[1]);

            gameAction.type = 4;
        } else if (strcmp(command, "reset") == 0) {
            // Enviar o comando "reset" para o servidor
            gameAction.type = 5;
        } else if (strcmp(command, "exit") == 0) {
            // Enviar o comando "exit" para o servidor
            gameAction.type = 7;
        } else {
            // **Erro: Comando inválido**
            printf("error: command not found\n");
            continue;
        }

        int actionType = gameAction.type;
        if (actionType == 7) {
            // Enviar a ação ao servidor em caso de "exit"
            sendAction(clientSocket, &gameAction);
            // Fechar comunicação com o servidor
            close(clientSocket);

            break;
        } else {
            // Enviar a ação ao servidor em quaisquer outros casos
            sendAction(clientSocket, &gameAction);
            // Receber a ação do servidor
            recv(clientSocket, &gameAction, sizeof(struct action), 0);
        }

        // Processar a ação do servidor
        actionType = gameAction.type;
        if (actionType == 6) {
            // Ação do tipo 6: Vitória
            printf("Vitória! O jogo terminou.\n");
            printBoard(gameAction.board);
        } else if (actionType == 8) {
            // Ação do tipo 8: Derrota
            printf("Derrota! O jogo terminou.\n");
            printBoard(gameAction.board);
        } else {
            // Ação do tipo 9: Atualização do tabuleiro
            printf("Tabuleiro Atualizado:\n");
            printBoard(gameAction.board);
        }
    }

    // Sair do programa
    return 0;
}
