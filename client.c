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
            printf("%c\t\t", cell);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Correct usage: %s <server IP address> <port number>\n", argv[0]);
        exit(1);
    }

    const char *serverIP = argv[1];
    int serverPort = atoi(argv[2]);

    // Verifica se é IPv4 ou IPv6
    int isIPv6 = strchr(serverIP, ':') != NULL;

    // Configuração do cliente
    int clientSocket;
    struct sockaddr_storage serverAddr;

    clientSocket = socket(isIPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Error creating client socket");
        exit(1);
    }

    if (isIPv6) {
        struct sockaddr_in6 *serverAddrIPv6 = (struct sockaddr_in6 *)&serverAddr;
        serverAddrIPv6->sin6_family = AF_INET6;
        serverAddrIPv6->sin6_port = htons(serverPort);
        if (inet_pton(AF_INET6, serverIP, &serverAddrIPv6->sin6_addr) <= 0) {
            perror("Invalid IP address");
            exit(1);
        }
    } else {
        struct sockaddr_in *serverAddrIPv4 = (struct sockaddr_in *)&serverAddr;
        serverAddrIPv4->sin_family = AF_INET;
        serverAddrIPv4->sin_port = htons(serverPort);
        if (inet_pton(AF_INET, serverIP, &serverAddrIPv4->sin_addr) <= 0) {
            perror("Invalid IP address");
            exit(1);
        }
    }

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, isIPv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)) == -1) {
        perror("Error connecting to server");
        exit(1);
    }

    printf("Connected to the server\n");

    // Loop principal do cliente
    while (1) {
        // Estrutura para armazenar a ação de jogo
        struct action gameAction;

        // Variável para armazenar a entrada de comando do usuário
        char command[20];

        // Solicitar o comando do usuário
        printf("Enter a command: ");
        fgets(command, sizeof(command), stdin);

        // Remover o caractere de nova linha do final
        size_t len = strlen(command);
        if (len > 0 && command[len - 1] == '\n') {
            command[len - 1] = '\0';
        }

        // Extrair o comando e os argumentos
        char action[10];
        int arg1, arg2;
        int parsedArgs = sscanf(command, "%s %d,%d", action, &arg1, &arg2);

        // Configurar a ação do cliente de acordo com o comando
        if (strcmp(action, "start") == 0) {
            // Enviar o comando "start" para o servidor
            gameAction.type = 0;
        } else if (parsedArgs == 3 && strcmp(action, "reveal") == 0) {
            // Enviar o comando "reveal" para o servidor, juntamente com as coordenadas da célula a ser revelada

            // **Erro: Célula fora do range do tabuleiro**
            if (arg1 < 0 || arg1 >= 4 || arg2 < 0 || arg2 >= 4) {
                printf("error: invalid cell\n");
                continue;
            }

            // **Erro: Revela uma célula já revelada**
            if (gameAction.board[arg1][arg2] != -2 && gameAction.board[arg1][arg2] -3) {
                printf("error: cell already revealed\n");
                continue;
            }

            gameAction.coordinates[0] = arg1;
            gameAction.coordinates[1] = arg2;

            gameAction.type = 1;
        } else if (parsedArgs == 3 && strcmp(action, "flag") == 0) {
            // Enviar o comando "flag" para o servidor, juntamente com as coordenadas da célula a ser marcada com uma bandeira

            // **Erro: Flag em uma célula já marcada**
            if (gameAction.board[arg1][arg2] == -3) {
                printf("error: cell already has a flag\n");
                continue;
            }

            // **Erro: Flag em uma célula revelada**
            if (gameAction.board[arg1][arg2] != -2) {
                printf("error: cannot insert flag in revealed cell\n");
                continue;
            }

            gameAction.coordinates[0] = arg1;
            gameAction.coordinates[1] = arg2;

            gameAction.type = 2;
        } else if (parsedArgs == 3 && strcmp(action, "remove_flag") == 0) {
            // Enviar o comando "remove_flag" para o servidor, juntamente com as coordenadas da célula a ter a bandeira removida

            gameAction.coordinates[0] = arg1;
            gameAction.coordinates[1] = arg2;

            gameAction.type = 4;
        } else if (strcmp(action, "reset") == 0) {
            // Enviar o comando "reset" para o servidor
            gameAction.type = 5;
        } else if (strcmp(action, "exit") == 0) {
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
            printf("YOU WIN!\n");
            printBoard(gameAction.board);
        } else if (actionType == 8) {
            // Ação do tipo 8: Derrota
            printf("GAME OVER!\n");
            printBoard(gameAction.board);
        } else {
            // Ação do tipo 9: Atualização do tabuleiro
            printf("Updated Board:\n");
            printBoard(gameAction.board);
        }
    }

    // Sair do programa
    return 0;
}
