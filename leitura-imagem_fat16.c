// Trabalho por Arthur Angelo Cenci Silva e Lucas Ivanov Costa

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Define um tamanho padrão de setor (comum em FAT16)
#define TAMANHO_SETOR 512

// Lê 2 bytes little-endian e retorna em um inteiro 16 bits
uint16_t le16(FILE *fp) {
    uint8_t b[2];
    fread(b, 1, 2, fp);
    // little-endian => byte[0] é a parte baixa, byte[1] a alta
    return (b[1] << 8) | b[0];
}

// Lê 4 bytes little-endian e retorna em um inteiro 32 bits
uint32_t le32(FILE *fp) {
    uint8_t b[4];
    fread(b, 1, 4, fp);
    return ((uint32_t)b[3] << 24) | ((uint32_t)b[2] << 16) |
           ((uint32_t)b[1] << 8)  |  (uint32_t)b[0];
}

// Função para exibir um nome 8.3 (sem tratar LFN)
void imprimeNome83(uint8_t nome[8], uint8_t ext[3]) {
    char nomeC[9];
    char extC[4];
    memcpy(nomeC,  nome, 8);
    memcpy(extC,   ext, 3);
    nomeC[8] = '\0';
    extC[3]  = '\0';

    // Remover espaços ao final do nome
    for (int i = 7; i >= 0; i--) {
        if (nomeC[i] == ' ' || nomeC[i] == 0) {
            nomeC[i] = '\0';
        } else {
            break;
        }
    }
    // Remover espaços ao final da extensão
    for (int i = 2; i >= 0; i--) {
        if (extC[i] == ' ' || extC[i] == 0) {
            extC[i] = '\0';
        } else {
            break;
        }
    }

    if (extC[0] == '\0') {
        // sem extensão
        printf("%s", nomeC);
    } else {
        printf("%s.%s", nomeC, extC);
    }
}

// Lê o valor de 2 bytes (FAT16) para obter o próximo cluster
// Recebe a posição de início da FAT em bytes e o cluster atual
uint16_t leProximoCluster(FILE *fp, uint32_t inicioFatBytes,
                          uint16_t cluster, uint16_t bytesPorSetor)
{
    // Em FAT16, cada entrada na FAT tem 2 bytes
    // então o offset dentro da FAT é cluster * 2
    uint32_t offset = inicioFatBytes + (cluster * 2);

    fseek(fp, offset, SEEK_SET);

    uint8_t b[2];
    fread(b, 1, 2, fp);

    uint16_t valor = (b[1] << 8) | b[0];
    return valor;
}

// Recebe o primeiro cluster, tamanho do arquivo, e informações sobre a área de dados
void exibeConteudoArquivo(FILE *fp,
                          uint16_t primeiroCluster,
                          uint32_t tamanho,
                          uint32_t inicioAreaDadosBytes,
                          uint8_t  setoresPorCluster,
                          uint16_t bytesPorSetor,
                          uint32_t inicioFAT1Bytes)
{
    printf("\n--- Exibindo conteudo do arquivo (hex) ---\n");

    uint32_t bytesRestantes = tamanho;
    uint16_t clusterAtual = primeiroCluster;
    uint32_t tamanhoCluster = setoresPorCluster * bytesPorSetor;

    // Aloca buffer para ler 1 cluster
    uint8_t *buffer = (uint8_t*)malloc(tamanhoCluster);
    if (!buffer) {
        printf("Erro ao alocar memoria.\n");
        return;
    }

    while (1) {
        // Calcula posição do cluster na área de dados (em bytes)
        // cluster2 => base, cluster N => base + (N-2)*tamanho_cluster
        uint32_t offsetCluster =
            inicioAreaDadosBytes + (clusterAtual - 2) * (uint32_t)(tamanhoCluster);

        fseek(fp, offsetCluster, SEEK_SET);
        // lê um cluster inteiro
        fread(buffer, 1, tamanhoCluster, fp);

        // quantos bytes deste cluster pertencem ao arquivo
        uint32_t bytesParaLer = (bytesRestantes < tamanhoCluster) ? bytesRestantes : tamanhoCluster;

        // Exibe em hexadecimal (poderia exibir como texto se fosse ASCII)
        for (uint32_t i = 0; i < bytesParaLer; i++) {
            printf("%02X ", buffer[i]);
            if ((i+1) % 16 == 0) printf("\n");
        }
        if (bytesParaLer % 16 != 0) printf("\n");

        bytesRestantes -= bytesParaLer;
        if (bytesRestantes == 0) {
            // chegou no fim do arquivo
            break;
        }

        // Lê próximo cluster na FAT
        clusterAtual = leProximoCluster(fp, inicioFAT1Bytes, clusterAtual, bytesPorSetor);

        // Valores típicos de fim de arquivo em FAT16 >= 0xFFF8
        if (clusterAtual >= 0xFFF8) {
            break; 
        }
    }

    free(buffer);
    printf("--- Fim do conteudo ---\n\n");
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Uso: %s <imagem_fat16>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Erro ao abrir imagem");
        return 1;
    }

    // ----------------------------------------
    // 1) Ler o Boot Record de acordo com as notas
    // ----------------------------------------
    // Lê primeiros 3 bytes (código de jump - não precisamos interpretá-lo para a leitura)
    uint8_t jumpBoot[3];
    fread(jumpBoot, 1, 3, fp);

    // Lê os 8 bytes do "OEM Name"
    uint8_t oemName[8];
    fread(oemName, 1, 8, fp);

    // Lê "bytes por setor" (2 bytes, LE)
    uint16_t bytesPorSetor = le16(fp);

    // Lê "setores por cluster" (1 byte)
    uint8_t setoresPorCluster;
    fread(&setoresPorCluster, 1, 1, fp);

    // Lê "setores reservados" (2 bytes, LE)
    uint16_t setoresReservados = le16(fp);

    // Lê "numero de FATs" (1 byte)
    uint8_t numeroFATs;
    fread(&numeroFATs, 1, 1, fp);

    // Lê "entradas do root" (2 bytes, LE)
    uint16_t entradasRoot = le16(fp);

    // Lê "total de setores (16 bits)" (2 bytes, LE)
    uint16_t totalSetores16 = le16(fp);

    // Lê "descritor de midia" (1 byte)
    uint8_t midia;
    fread(&midia, 1, 1, fp);

    // Lê "setores por FAT16" (2 bytes, LE)
    uint16_t setoresPorFAT = le16(fp);

    // Lê "setores por trilha" (2 bytes, LE) - não necessariamente usado
    uint16_t setoresPorTrilha = le16(fp);

    // Lê "numero de cabeças" (2 bytes, LE) - não necessariamente usado
    uint16_t numCabecas = le16(fp);

    // Lê "setores ocultos" (4 bytes, LE) - pode ser 0
    uint32_t setoresOcultos = le32(fp);

    // Lê "total de setores 32 bits" (4 bytes, LE) - se totalSetores16=0 usar este
    uint32_t totalSetores32 = le32(fp);

    // Poderíamos precisar pular o restante do Boot Sector até 512 bytes, 
    // mas para a leitura básica do FAT16, isto é suficiente.

    // Determinar quantos setores totais de fato
    uint32_t totalSetores = (totalSetores16 != 0) ? totalSetores16 : totalSetores32;

    // ----------------------------------------
    // 2) Calcular posições em BYTES no disco
    // ----------------------------------------
    // - Cada setor tem 'bytesPorSetor' bytes.
    // - FAT1 começa após os 'setoresReservados'.
    //   Posição da FAT1 em bytes = setoresReservados * bytesPorSetor
    uint32_t fat1Inicio = setoresReservados * (uint32_t)bytesPorSetor;

    // - FAT2 começa logo após FAT1
    //   Posição da FAT2 = fat1Inicio + (setoresPorFAT * bytesPorSetor)
    uint32_t fat2Inicio = fat1Inicio + (setoresPorFAT * (uint32_t)bytesPorSetor);

    // - Diretório raiz começa após FAT2 (ou após todas as FATs, se forem mais FATs)
    //   Mas no FAT16 “padrão” são 2 FATs apenas.
    //   Tamanho total das FATs = numeroFATs * setoresPorFAT * bytesPorSetor
    uint32_t rootDirInicio = fat1Inicio + (numeroFATs * setoresPorFAT * (uint32_t)bytesPorSetor);

    // Tamanho do diretório raiz (em bytes) = entradasRoot * 32
    // (cada entrada de 32 bytes)
    uint32_t tamanhoRootDir = entradasRoot * 32;

    // - Área de dados começa após o diretório raiz
    uint32_t areaDadosInicio = rootDirInicio + tamanhoRootDir;

    // ----------------------------------------
    // Exibe informações gerais
    // ----------------------------------------
    printf("===== Informacoes (Boot Record) =====\n");
    printf("JumpBoot (3 bytes)        : %02X %02X %02X\n", jumpBoot[0], jumpBoot[1], jumpBoot[2]);
    printf("OEM Name                  : %.8s\n", oemName);
    printf("Bytes por setor           : %u\n", bytesPorSetor);
    printf("Setores por cluster       : %u\n", setoresPorCluster);
    printf("Setores reservados        : %u\n", setoresReservados);
    printf("Numero de FATs            : %u\n", numeroFATs);
    printf("Entradas no root dir      : %u\n", entradasRoot);
    printf("Total setores (16/32)     : %u\n", totalSetores);
    printf("Midia descriptor (0xF8?)  : 0x%02X\n", midia);
    printf("Setores por FAT           : %u\n", setoresPorFAT);
    printf("Setores por trilha        : %u\n", setoresPorTrilha);
    printf("Numero de cabecas         : %u\n", numCabecas);
    printf("Setores ocultos           : %u\n", setoresOcultos);
    printf("=====================================\n\n");

    printf("Posicao da FAT1 (em bytes): 0x%X\n", fat1Inicio);
    printf("Posicao da FAT2 (em bytes): 0x%X\n", fat2Inicio);
    printf("Posicao Root Dir (em bytes): 0x%X\n", rootDirInicio);
    printf("Posicao Area Dados (bytes): 0x%X\n\n", areaDadosInicio);

    // ----------------------------------------
    // 3) Ler o diretório raiz
    // ----------------------------------------
    // Vamos alocar memória para as entradas do root dir
    // (entradasRoot * 32 bytes)
    fseek(fp, rootDirInicio, SEEK_SET);

    uint8_t *rootDirBuffer = (uint8_t*)malloc(tamanhoRootDir);
    if (!rootDirBuffer) {
        printf("Falha ao alocar buffer do root dir.\n");
        fclose(fp);
        return 1;
    }
    fread(rootDirBuffer, 1, tamanhoRootDir, fp);

    printf("===== Entradas 8.3 no Root Dir =====\n");
    // Percorrer cada entrada de 32 bytes
    for (int i = 0; i < entradasRoot; i++) {
        uint8_t *entry = &rootDirBuffer[i * 32];

        // Primeiro byte do nome
        uint8_t firstByte = entry[0];

        // Se for 0x00 => não há mais entradas
        if (firstByte == 0x00) {
            break;
        }
        // Se for 0xE5 => arquivo deletado
        if (firstByte == 0xE5) {
            continue;
        }

        // Byte de atributo (posição 11)
        uint8_t atributo = entry[11];

        // Se for 0x0F => LFN, ignorar
        if (atributo == 0x0F) {
            continue;
        }

        // Nome (8 bytes) e Extensão (3 bytes)
        uint8_t nome[8];
        memcpy(nome, &entry[0], 8);

        uint8_t ext[3];
        memcpy(ext, &entry[8], 3);

        // Primeiro cluster (2 bytes a partir do offset 0x1A)
        // little-endian
        uint16_t firstCluster = (entry[27] << 8) | entry[26];

        // Tamanho do arquivo (4 bytes no offset 0x1C, LE)
        uint32_t tamanhoArquivo = 
            (entry[31] << 24) |
            (entry[30] << 16) |
            (entry[29] <<  8) |
             entry[28];

        // Exibir somente se for arquivo normal (0x20) ou diretório (0x10)
        // Atributo 0x10 => DIR, 0x20 => ARQ
        if (atributo == 0x10) {
            // Diretório
            printf("[DIR] ");
        } else if (atributo == 0x20) {
            // Arquivo
            printf("[ARQ] ");
        } else {
            // Outros atributos (volume label, system, etc.), podemos exibir se quiser
            printf("[OUTRO] ");
        }

        imprimeNome83(nome, ext);
        printf(" | Atributo: 0x%02X | FirstClus: %u | Tamanho: %u\n",
               atributo, firstCluster, tamanhoArquivo);
    }

    free(rootDirBuffer);

    // (Opcional) Caso queira, poderíamos voltar e ler a FAT e exibir
    // o conteúdo de um dos arquivos listados acima.

    fclose(fp);
    return 0;
}