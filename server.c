#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Estrutura para representar o tabuleiro
typedef struct {
    int board[4][4];
} GameBoard;

// Estrutura para representar uma ação do cliente
typedef struct {
    int type;
    int coordinates[2];
} ClientAction;

// Função para inicializar o tabuleiro a partir de um arquivo de texto
void initializeBoard(GameBoard *board, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            // Lê um valor do arquivo e o armazena na matriz do tabuleiro
            if (fscanf(file, "%d,", &board->board[i][j]) != 1) {
                fprintf(stderr, "Erro ao ler o arquivo\n");
                exit(1);
            }
        }
    }

    fclose(file);
}

// Função para imprimir o tabuleiro
void printBoard(GameBoard *board) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            // Converte os valores do tabuleiro em caracteres para impressão
            char cell;
            switch (board->board[i][j]) {
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
                    cell = '0' + board->board[i][j]; // Número de bombas na vizinhança
                    break;
            }
            printf("%c\t", cell);
        }
        printf("\n");
    }
}

// Função para verificar condições de vitória ou derrota
int checkGameStatus(GameBoard *board) {
    int unrevealedCells = 0;
    int bombRevealed = 0;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (board->board[i][j] == -2) {
                unrevealedCells++;
            } else if (board->board[i][j] == -1) {
                bombRevealed = 1;
            }
        }
    }

    if (unrevealedCells == 0 && bombRevealed) {
        return -1; // Todas as células não contendo bombas foram reveladas, mas uma bomba foi revelada (derrota)
    } else if (unrevealedCells == 0 && !bombRevealed) {
        return 1; // Todas as células não contendo bombas foram reveladas (vitória)
    } else {
        return 0; // O jogo continua
    }
}

// Função para processar ação do cliente e atualizar o tabuleiro
void processClientAction(GameBoard *board, ClientAction *action) {
    int row = action->coordinates[0];
    int col = action->coordinates[1];

    switch (action->type) {
        case 0: // Start
            // Inicializa o jogo
            break;
        case 1: // Reveal
            // Revela a célula na posição (row, col)
            board->board[row][col] = -2;
            if (board->board[row][col] == -1) {
                // O jogador perdeu
                send(clientSocket, &board, sizeof(GameBoard), 0);
                printf("Derrota! O jogo terminou.\n");
                exit(1);
            }
            break;
        case 2: // Flag
            // Adiciona uma flag na célula na posição (row, col)
            board->board[row][col] = -3;
            break;
        case 3: // State
            // Envia o estado atual do tabuleiro para o cliente
            send(clientSocket, &board, sizeof(GameBoard), 0);
            break;
        case 4: // Remove flag
            // Remove uma flag da célula na posição (row, col)
            board->board[row][col] = -2;
            break;
        case 5: // Reset
            // Reinicia o jogo
            initializeBoard(board, argv[1]);
            printBoard(board);
            break;
        case 6: // Win
            // O jogador ganhou
            printf("Vitória! O jogo terminou.\n");
            break;
        case 7: // Exit
            // O jogador desconectou
            break;
        case 8: // Game over
            // O jogador perdeu
            printf("Derrota! O jogo terminou.\n");
            break;
        default:
            // Ação inválida
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
       fprintf(stderr, "Uso: %s <nome_do_arquivo.txt>\n", argv[0]);
       return 1;
    }

    // Inicialize o tabuleiro
    GameBoard gameBoard;
    initializeBoard(&gameBoard, argv[1]);
    printBoard(&gameBoard);

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
    serverAddr.sin_port = htons(51511);
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
        ClientAction clientAction;
        recv(clientSocket, &clientAction, sizeof(ClientAction), 0);

        // Processar a ação do cliente e atualizar o tabuleiro
        processClientAction(&gameBoard, &clientAction);

        // Enviar o estado atual do tabuleiro de volta para o cliente
        send(clientSocket, &gameBoard, sizeof(GameBoard), 0);

        // Verificar condições de vitória ou derrota
        int gameStatus = checkGameStatus(&gameBoard);
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
