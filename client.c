#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// Estrutura para representar uma ação do cliente
typedef struct {
    int type;
    int coordinates[2];
} ClientAction;

// Função para enviar uma ação para o servidor
void sendAction(int socket, ClientAction *action) {
    send(socket, action, sizeof(ClientAction), 0);
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
        printf("Opções:\n");
        printf("1. Revelar célula\n");
        printf("2. Marcar com flag\n");
        printf("3. Remover flag\n");
        printf("4. Sair\n");

        int choice;
        scanf("%d", &choice);

        ClientAction action;
        action.type = choice;

        if (choice >= 1 && choice <= 3) {
            printf("Informe as coordenadas (linha coluna): ");
            scanf("%d %d", &action.coordinates[0], &action.coordinates[1]);
        } else if (choice == 4) {
            action.coordinates[0] = -1;
            action.coordinates[1] = -1;
        } else {
            printf("Escolha inválida\n");
            continue;
        }

        // Enviar a ação para o servidor
        sendAction(clientSocket, &action);

        // Receber a resposta do servidor (atualização do tabuleiro)
        int gameResult;
        recv(clientSocket, &gameResult, sizeof(int), 0);

        if (gameResult == 1) {
            printf("Você venceu!\n");
            // Realize ações adicionais após a vitória
        } else if (gameResult == -1) {
            printf("Você perdeu!\n");
            // Realize ações adicionais após a derrota
        }
    }

    close(clientSocket);

    return 0;
}
