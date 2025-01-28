// Trabalho por Arthur Angelo Cenci Silva e Lucas Ivanov Costa

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Define um tamanho padrão de setor (comum em FAT16)
#define TAMANHO_SETOR 512

// Lê 2 bytes little-endian e retorna inteiro de 16 bits
uint16_t little_endian_to_int16(FILE *fp) {
    uint8_t b[2];
    fread(b, 1, 2, fp);     
    // byte[0] é a parte baixa, byte[1] a alta
    return (b[1] << 8) | b[0];
}

// Lê 4 bytes little-endian e retorna inteiro de 32 bits
uint32_t little_endian_to_int32(FILE *fp) {
    uint8_t b[4];
    fread(b, 1, 4, fp);
    return ((uint32_t)b[3] << 24) | ((uint32_t)b[2] << 16) |
           ((uint32_t)b[1] << 8)  |  (uint32_t)b[0];
}

void imprime_nome_83(uint8_t nome[8], uint8_t ext[3]) {
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
}

// Recebe a posição de início da FAT em bytes e o cluster atual
uint16_t le_proximo_cluster(FILE *fp, uint32_t inicioFatBytes,
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
void exibe_conteudo_arquivo(FILE *fp,
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
        clusterAtual = le_proximo_cluster(fp, inicioFAT1Bytes, clusterAtual, bytesPorSetor);

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
    //Ler boot record
    uint8_t jumpBoot[3];
    fread(jumpBoot, 1, 3, fp);

    uint8_t oemName[8];
    fread(oemName, 1, 8, fp);

    uint16_t bytesPorSetor = little_endian_to_int16(fp);

    uint8_t setoresPorCluster;
    fread(&setoresPorCluster, 1, 1, fp);

    uint16_t setoresReservados = little_endian_to_int16(fp);

    uint8_t numeroFATs;
    fread(&numeroFATs, 1, 1, fp);

    uint16_t entradasRoot = little_endian_to_int16(fp);

    uint16_t totalSetores16 = little_endian_to_int16(fp);

    uint8_t midia;
    fread(&midia, 1, 1, fp);

    uint16_t setoresPorFAT = little_endian_to_int16(fp);

    uint16_t setoresPorTrilha = little_endian_to_int16(fp);

    uint16_t numCabecas = little_endian_to_int16(fp);

    uint32_t setoresOcultos = little_endian_to_int32(fp);

    uint32_t totalSetores32 = little_endian_to_int32(fp);

    uint32_t totalSetores = (totalSetores16 != 0) ? totalSetores16 : totalSetores32;
    // Calcula FAT1
    uint32_t fat1Inicio = setoresReservados * (uint32_t)bytesPorSetor;

    // FAT2 após FAT1
    uint32_t fat2Inicio = fat1Inicio + (setoresPorFAT * (uint32_t)bytesPorSetor);

    // Diretório raiz após FATs
    uint32_t rootDirInicio = fat1Inicio + (numeroFATs * setoresPorFAT * (uint32_t)bytesPorSetor);

    uint32_t tamanhoRootDir = entradasRoot * 32;

    // Área de dados após diretório raiz
    uint32_t areaDadosInicio = rootDirInicio + tamanhoRootDir;


    // Exibe informações específicas
    printf("Numero de FATs             : %u\n", numeroFATs);
    printf("Posicao da FAT1 (em bytes) : 0x%X\n", fat1Inicio);
    printf("Posicao da FAT2 (em bytes) : 0x%X\n", fat2Inicio);
    printf("Posicao Root Dir (em bytes): 0x%X\n", rootDirInicio);
    printf("Posicao Area Dados (bytes) : 0x%X\n\n", areaDadosInicio);

    // 3) Ler o diretório raiz
    fseek(fp, rootDirInicio, SEEK_SET);

    uint8_t *rootDirBuffer = (uint8_t*)malloc(tamanhoRootDir);
    if (!rootDirBuffer) {
        printf("Falha ao alocar buffer do root dir.\n");
        fclose(fp);
        return 1;
    }
    fread(rootDirBuffer, 1, tamanhoRootDir, fp);

    printf("===== Entradas 8.3 no Root Dir =====\n");
    // Percorre cada entrada de 32 bytes
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
        // Nome e extensão 8.3
        uint8_t nome[8];
        memcpy(nome, &entry[0], 8);

        uint8_t ext[3];
        memcpy(ext, &entry[8], 3);

        // Primeiro cluster 
        uint16_t firstCluster = (entry[27] << 8) | entry[26];

        // Tamanho do arquivo 
        uint32_t tamanhoArquivo = 
            (entry[31] << 24) |
            (entry[30] << 16) |
            (entry[29] <<  8) |
             entry[28];

        // Atributo 0x10 => DIR, 0x20 => ARQ
        if (atributo == 0x10) {
            // Diretório
            printf("[DIR] ");
        } else if (atributo == 0x20) {
            // Arquivo
            printf("[ARQ] ");
        } else {
            // Outros atributos 
            printf("[OUTRO] ");
        }

        imprime_nome_83(nome, ext);
        printf(" | FirstClus: %u | Tamanho: %u\n", firstCluster, tamanhoArquivo);

      
    }

    free(rootDirBuffer);

    fclose(fp);
    return 0;
}