#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

// Variável global para controlar o estado do jogo
int gameStarted = 0;

// Estrutura para representar a ação do cliente e o estado do tabuleiro
struct action {
    int type;
    int coordinates[2];
    int board[4][4];
};

// Matriz para armazenar a versão de referência do tabuleiro
int referenceBoard[4][4];

// Função para copiar a versão de referência do tabuleiro para o tabuleiro de ação
void copyReferenceBoard(struct action *action) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            action->board[i][j] = referenceBoard[i][j];
        }
    }
}

// Função para inicializar o tabuleiro a partir de um arquivo de texto
void initializeBoard(int reference[4][4], int board[4][4], const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening the file");
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            // Lê um valor do arquivo e o armazena na matriz do tabuleiro de referência e do tabuleiro de ação
            if (fscanf(file, "%d,", &reference[i][j]) != 1) {
                fprintf(stderr, "Error reading the file\n");
                exit(1);
            }

            board[i][j] = -2;  // Inicialize o tabuleiro de ação com células ocultas
        }
    }

    fclose(file);
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

// Função para enviar o tabuleiro ao cliente
void sendBoard(int clientSocket, struct action *action) {
    send(clientSocket, action, sizeof(struct action), 0);
}

// Função para verificar condições de vitória ou derrota
int checkGameStatus(int board[4][4]) {
    int unrevealedCells = 0;
    int bombRevealed = 0;
    int bombNumber = 0;


    // Percorre todo o tabuleiro para checar o estado
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (board[i][j] == -2) {
                unrevealedCells++; // Conta as células não reveladas
            } else if (board[i][j] == -1) {
                bombRevealed = 1; // Verifica se há alguma bomba revelada
            }

            if (referenceBoard[i][j] == -1) {
                bombNumber++; // Conta a quantidade de bombas no tabuleiro de referência
            }
        }
    }

    if (bombRevealed) {
        return -1; // Uma bomba foi revelada (derrota)
    } else if (unrevealedCells == bombNumber && !bombRevealed ) {
        return 1; // Todas as células não contendo bombas foram reveladas (vitória)
    } else {
        return 0; // O jogo continua
    }
}

// Função para processar a ação do cliente e atualizar o tabuleiro
int processClientAction(int clientSocket, struct action *action, const char *filename) {
    int row = action->coordinates[0];
    int col = action->coordinates[1];
    int gameStatus = 0; // Inicializa o status do jogo

    switch (action->type) {
        case 0: // Start
            // Inicializa o jogo e imprime "starting new game" se o jogo ainda não começou
            if (gameStarted == 0) {
                gameStarted = 1;
                initializeBoard(referenceBoard, action->board, filename); // Inicializa o tabuleiro com base no arquivo fornecido

                printf("Starting new game\n");
            }

            break;
        case 1: // Reveal
            // Revela a célula na posição (row, col)
            if (gameStarted == 1) {
                action->board[row][col] = referenceBoard[row][col]; // Atualiza o tabuleiro de ação com base no tabuleiro de referência

                gameStatus = checkGameStatus(action->board); // Verifica o status do jogo
            }

            break;
        case 2: // Flag
            // Adiciona uma flag na célula na posição (row, col)
            if (gameStarted == 1) {
                action->board[row][col] = -3;

            }

            break;
        case 4: // Remove flag
            // Remove uma flag da célula na posição (row, col)
            if (gameStarted == 1) {
                if (action->board[row][col] <= -2) {
                    action->board[row][col] = -2;
                }
            }

            break;
        case 5: // Reset
            // Reinicia o jogo, imprime "starting new game" e reseta o estado do jogo
            gameStarted = 1;
            initializeBoard(referenceBoard, action->board, filename); // Inicializa o tabuleiro com base no arquivo fornecido

            printf("Starting new game\n");

            break;
        case 7: // Exit
            // O jogador desconectou, imprime "client disconnected"
            printf("Client disconnected\n");

            gameStatus = -2;
            gameStarted = 0;
            initializeBoard(referenceBoard, action->board, filename);

            break;
        default:
            // Ação inválida
            break;
    }

    return gameStatus; // Retorna o status do jogo
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <v4/v6> <port_number> -i <filename.txt>\n", argv[0]);
        return 1;
    }

    // Verifica se é IPv4 ou IPv6
    int isIPv6 = 0;
    if (strcmp(argv[1], "v6") == 0) {
        isIPv6 = 1;
    }

    // Obtenha o número de porta
    int port = atoi(argv[2]);

    // Inicia estruturas de conexão
    int serverSocket, clientSocket;
    struct sockaddr_storage serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    // Inicializa e exibe o tabuleiro
    struct action gameAction;
    initializeBoard(referenceBoard, gameAction.board, argv[4]);
    printBoard(referenceBoard);

    // Configuração do servidor
    serverSocket = socket(isIPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Error creating server socket");
        exit(1);
    }

    if (isIPv6) {
        struct sockaddr_in6 *serverAddrIPv6 = (struct sockaddr_in6 *)&serverAddr;
        serverAddrIPv6->sin6_family = AF_INET6;
        serverAddrIPv6->sin6_port = htons(port);
        serverAddrIPv6->sin6_addr = in6addr_any;
    } else {
        struct sockaddr_in *serverAddrIPv4 = (struct sockaddr_in *)&serverAddr;
        serverAddrIPv4->sin_family = AF_INET;
        serverAddrIPv4->sin_port = htons(port);
        serverAddrIPv4->sin_addr.s_addr = INADDR_ANY;
    }

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error binding socket");
        exit(1);
    }

    if (listen(serverSocket, 1) == -1) {
        perror("Error waiting for connections");
        exit(1);
    }

    while (1) {
        printf("Waiting for connections...\n");

        // Aguardando conexão de um cliente
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket == -1) {
            perror("Error accepting client connection");
            exit(1);
        }

        printf("Connected client\n");

        // Loop principal de comunicação
        int gameStatus = 0; // Status do jogo
        while (1) {
            // Receba uma ação do cliente
            recv(clientSocket, &gameAction, sizeof(struct action), 0);

            // Processar a ação do cliente e atualizar o tabuleiro
            gameStatus = processClientAction(clientSocket, &gameAction, argv[4]); // Obtém o status do jogo
            if (gameAction.type == 7) {
                // O cliente optou por "exit", então encerramos o loop de comunicação atual
                close(clientSocket); // Fecha a conexão com o cliente
                break;
            }

            // Enviar ação de acordo com o status do jogo
            if (gameStatus == 1) {
                // Vitória
                gameAction.type = 6;
                copyReferenceBoard(&gameAction);
            } else if (gameStatus == -1) {
                // Derrota
                gameAction.type = 8;
                copyReferenceBoard(&gameAction);
            } else {
                // Atualização do tabuleiro
                gameAction.type = 3;
            }

            sendBoard(clientSocket, &gameAction);

            if (gameStatus == 1 || gameStatus == -1) {
                printf("The game has ended.\n");
            }
        }
    }

    return 0;
}
