# Ransomware Dilma Locker

## Versão
1.4 (22/07/2017)

## Estado do projeto
Abandonado (atingimos a meta e não vamos dobrar a meta).

## Compilação
Baixe o [msys32](https://www.msys2.org) no site oficial.

#### Atualize os repositórios:

```bash
pacman -Syu
``` 

#### Instale o compilador:

```bash
pacman -S mingw-w64-i686-toolchain
```

#### Compilação do master.exe

```bash
gcc master.c -o ../bin/master.exe --std=c18 -Os -O1 -Wall -Wextra && strip --strip-all --discard-all ../bin/master.exe
```

#### Compilação do Dilma Locker:

```bash
gcc dilma.c -o ../bin/dilma.exe --std=c18 -lpthread -lshlwapi -static -mwindows -Os -O1 -Wall -Wextra && strip --strip-all --discard-all ../bin/dilma.exe

```

## Sobre
1.0 -> versão beta, tinha mais bugs que os sites do governo.
  * Foi usado RC4 ao invés de AES.

1.1 -> segunda versão, corrigido alguns bugs mas ainda faltava algo
  * Mudado a criptografia de RC4 para AES 256 bits.

1.4 -> terceira versão, reescrita do zero e corrigido alguns bugs. Foi lançada um mês após a primeira versão.
  * Refeito para deixar o código fonte mais bonito.


## Dever de casa
Perdemos o código do decrypter então você terá que coda-lo :)

Obs: Não nos responsabilizamos pelo uso inadequado desse programa, use apenas para estudos em maquinas virtuais.

