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
        perror("Erro ao abrir o arquivo");
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            // Lê um valor do arquivo e o armazena na matriz do tabuleiro de referência e do tabuleiro de ação
            if (fscanf(file, "%d,", &reference[i][j]) != 1) {
                fprintf(stderr, "Erro ao ler o arquivo\n");
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
            printf("%c\t", cell);
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

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (board[i][j] == -2) {
                unrevealedCells++;
            } else if (board[i][j] == -1) {
                bombRevealed = 1;
            }
        }
    }

    if (bombRevealed) {
        return -1; // Uma bomba foi revelada (derrota)
    } else if (unrevealedCells == 0 && !bombRevealed) {
        return 1; // Todas as células não contendo bombas foram reveladas (vitória)
    } else {
        return 0; // O jogo continua
    }
}

// Função para processar a ação do cliente e atualizar o tabuleiro
void processClientAction(int clientSocket, struct action *action, const char *filename) {
    int row = action->coordinates[0];
    int col = action->coordinates[1];

    switch (action->type) {
        case 0: // Start
            // Inicializa o jogo e imprime "starting new game" se o jogo ainda não começou
            if (gameStarted == 0) {
                gameStarted = 1;
                initializeBoard(referenceBoard, action->board, filename); // Inicializa o tabuleiro com base no arquivo fornecido
                printf("Starting new game\n");
                sendBoard(clientSocket, action); // Envia o tabuleiro ao cliente
            }
            break;
        case 1: // Reveal
            // Revela a célula na posição (row, col)
            if (gameStarted == 1) {
                action->board[row][col] = referenceBoard[row][col]; // Atualiza o tabuleiro de ação com base no tabuleiro de referência
                sendBoard(clientSocket, action); // Envia o tabuleiro ao cliente
            }
            break;
        case 2: // Flag
            // Adiciona uma flag na célula na posição (row, col)
            if (gameStarted == 1) {
                action->board[row][col] = -3;
                sendBoard(clientSocket, action); // Envia o tabuleiro ao cliente
            }
            break;
        case 3: // Remove flag
            // Remove uma flag da célula na posição (row, col)
            if (gameStarted == 1) {
                action->board[row][col] = -2;
                sendBoard(clientSocket, action); // Envia o tabuleiro ao cliente
            }
            break;
        case 4: // Reset
            // Reinicia o jogo, imprime "starting new game" e reseta o estado do jogo
            gameStarted = 0;
            initializeBoard(referenceBoard, action->board, filename); // Inicializa o tabuleiro com base no arquivo fornecido
            printf("Starting new game\n");
            sendBoard(clientSocket, action); // Envia o tabuleiro ao cliente
            break;
        case 7: // Exit
            // O jogador desconectou, imprime "client disconnected"
            printf("Client disconnected\n");
            break;
        default:
            // Ação inválida
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <v4/v6> <número_de_porta> -i <nome_do_arquivo.txt>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "v4") != 0 && strcmp(argv[1], "v6") != 0) {
        fprintf(stderr, "Use 'v4' for IPv4 or 'v6' for IPv6\n");
        return 1;
    }

    // Obtenha o número de porta
    int port = atoi(argv[2]);

    // Inicialize o tabuleiro
    struct action gameAction;
    initializeBoard(referenceBoard, gameAction.board, argv[4]);
    printBoard(referenceBoard);

    // Configuração do servidor
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Erro ao criar o socket do servidor");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Erro ao vincular o socket");
        exit(1);
    }

    if (listen(serverSocket, 1) == -1) {
        perror("Erro ao aguardar por conexões");
        exit(1);
    }

    printf("Aguardando conexões...\n");

    // Aguardando conexão de um cliente
    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientSocket == -1) {
        perror("Erro ao aceitar conexão do cliente");
        exit(1);
    }

    printf("Cliente conectado\n");

    // Loop principal de comunicação
    while (1) {
        // Receba uma ação do cliente
        recv(clientSocket, &gameAction, sizeof(struct action), 0);

        // Processar a ação do cliente e atualizar o tabuleiro
        processClientAction(clientSocket, &gameAction, argv[4]); // Passa o nome do arquivo como argumento

        // Verificar condições de vitória ou derrota
        int gameStatus = checkGameStatus(gameAction.board);
        if (gameStatus == 1) {
            printf("Vitória! O jogo terminou.\n");
            break; // Encerrar o jogo
        } else if (gameStatus == -1) {
            printf("Derrota! O jogo terminou.\n");
            break; // Encerrar o jogo
        }
    }

    close(clientSocket);
    close(serverSocket);

    return 0;
}
