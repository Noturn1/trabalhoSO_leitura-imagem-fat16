# Leitura de Imagem FAT16

Este projeto foi desenvolvido por Arthur Angelo Cenci Silva e Lucas Ivanov Costa. Ele tem como objetivo ler e exibir informações de uma imagem de sistema de arquivos FAT16.

## Funcionalidades

- Leitura do Boot Record de uma imagem FAT16.
- Cálculo das posições das FATs, diretório raiz e área de dados.
- Exibição das entradas do diretório raiz no formato 8.3.
- Leitura e exibição do conteúdo de arquivos em formato hexadecimal.

## Compilação

Para compilar o programa, utilize um compilador C como `gcc`. Execute o seguinte comando no terminal:

```sh
gcc  leitura_fat16.c -o leitura-imagem_fat16
```

## Execução

Para executar o programa, forneça o caminho para a imagem FAT16 como argumento:

```sh
./leitura_fat16 <imagem_fat16>
```

Exemplo:

```sh
./leitura_fat16 imagem.img
```

## Estrutura do Código

- `le16(FILE *fp)`: Lê 2 bytes em formato little-endian e retorna um inteiro de 16 bits.
- `le32(FILE *fp)`: Lê 4 bytes em formato little-endian e retorna um inteiro de 32 bits.
- `imprimeNome83(uint8_t nome[8], uint8_t ext[3])`: Exibe um nome no formato 8.3.
- `leProximoCluster(FILE *fp, uint32_t inicioFatBytes, uint16_t cluster, uint16_t bytesPorSetor)`: Lê o próximo cluster na FAT.
- `exibeConteudoArquivo(FILE *fp, uint16_t primeiroCluster, uint32_t tamanho, uint32_t inicioAreaDadosBytes, uint8_t setoresPorCluster, uint16_t bytesPorSetor, uint32_t inicioFAT1Bytes)`: Exibe o conteúdo de um arquivo em formato hexadecimal.

## Observações

- O programa foi desenvolvido para fins educacionais e pode não cobrir todos os casos de uso de um sistema de arquivos FAT16.
- Certifique-se de fornecer uma imagem FAT16 válida para evitar comportamentos inesperados.
