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
        // Adicionar uma nova variável para armazenar o comando do usuário
        char command[10];

        // Solicitar o comando do usuário
        printf("Digite um comando: ");
        scanf("%s", command);

        // Configurar a ação do cliente de acordo com o comando
        struct action gameAction;
        if (strcmp(command, "start") == 0) {
            // Enviar o comando "start" para o servidor
            gameAction.type = 0;
        } else if (strcmp(command, "reveal") == 0) {
            // Enviar o comando "reveal" para o servidor, juntamente com as coordenadas da célula a ser revelada
            printf("Informe as coordenadas (linha coluna): ");
            scanf("%d %d", &gameAction.coordinates[0], &gameAction.coordinates[1]);
            gameAction.type = 1;
        } else if (strcmp(command, "flag") == 0) {
            // Enviar o comando "flag" para o servidor, juntamente com as coordenadas da célula a ser marcada com uma bandeira
            printf("Informe as coordenadas (linha coluna): ");
            scanf("%d %d", &gameAction.coordinates[0], &gameAction.coordinates[1]);
            gameAction.type = 2;
        } else if (strcmp(command, "remove_flag") == 0) {
            // Enviar o comando "remove_flag" para o servidor, juntamente com as coordenadas da célula a ter a bandeira removida
            printf("Informe as coordenadas (linha coluna): ");
            scanf("%d %d", &gameAction.coordinates[0], &gameAction.coordinates[1]);
            gameAction.type = 4;
        } else if (strcmp(command, "state") == 0) {
            // Enviar o comando "state" para o servidor
            gameAction.type = 5;
        } else if (strcmp(command, "reset") == 0) {
            // Enviar o comando "reset" para o servidor
            gameAction.type = 5;
        } else if (strcmp(command, "exit") == 0) {
            // Enviar o comando "exit" para o servidor
            gameAction.type = 7;
        } else {
            // Comando inválido
            printf("Comando inválido\n");
            continue;
        }

        // Enviar a ação ao servidor e receber o tabuleiro atualizado
        sendAction(clientSocket, &gameAction);
        recv(clientSocket, &gameAction, sizeof(struct action), 0);

        // Imprimir o tabuleiro atualizado
        printf("Tabuleiro Atualizado:\n");
        printBoard(gameAction.board);
    }

    // Fechar o socket do cliente
    close(clientSocket);

    // Sair do programa
    exit(0);
}
