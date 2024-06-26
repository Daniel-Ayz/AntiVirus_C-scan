#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct virus{
    unsigned short SigSize;
    char virusName[16];
    unsigned char* sig;
} virus;

typedef struct link link;
struct link{
    link *nextVirus;
    virus *vir;
};


struct fun_desc {
    char *name;
    link* (*fun)(link*, FILE*);
};

void PrintHex(unsigned char* buffer, size_t length){
    for(size_t i=0;i<length;i++){
        printf("%02X ", buffer[i]);
    }
    printf("\n\n");
}

virus* readVirus(FILE* inputFile, int littleEndian){
    virus* virus = calloc(1, sizeof(struct virus));
    int firstRead = fread(virus, 1, 18, inputFile);
    if(firstRead == 0){
        free(virus);
        return NULL;
    }
    if (!littleEndian){
        virus->SigSize = be16toh(virus->SigSize);
    }
    virus->sig = calloc(1, virus->SigSize);
    int secondRead = fread(virus->sig, sizeof(char), virus->SigSize, inputFile);
    if(secondRead == 0){
        free(virus->sig);
        free(virus);
        return NULL;
    }
    return virus;
}

void printVirus(virus* virus, FILE* output){
    fprintf(output, "Virus name: %s\n",virus->virusName);
    fprintf(output, "Virus size: %d\n",virus->SigSize);
    fprintf(output, "signature:\n");
    PrintHex(virus->sig, virus->SigSize);
}

link* list_print(link* virus_list, FILE* outputFile){
    link* my_ptr = virus_list;
    while(my_ptr != NULL){
        printVirus(my_ptr->vir, outputFile);
        my_ptr = my_ptr->nextVirus;
    }
    return virus_list;
}

link* list_print_wrap(link* virus_list, FILE* file){
    return list_print(virus_list, stdout);
}

link* list_append(link* virus_list, virus* data){
    link* new_link = calloc(1, sizeof(struct link));
    new_link->vir = data;
    new_link->nextVirus = virus_list;
    return new_link;
}

void list_free(link* virus_list){
    if(virus_list != NULL){
        if(virus_list->nextVirus != NULL){
        list_free(virus_list->nextVirus);
        }
        free(virus_list->vir->sig);
        free(virus_list->vir);
        free(virus_list);
    }
}

link* load_signatures(link* virus_list, char* inputFileName){
    //define variables
    char magicNumber[4];
    char littleEndian = 0;
    // link* virus_list = NULL;

    //open input file
    FILE* inputFile = fopen(inputFileName, "rb");
    if(inputFile == NULL){
        printf("[-] Error. file not found: %s\n", inputFileName);
        return virus_list;
    }

    //check little/big endian
    fread(magicNumber, 1, 4, inputFile);
    if(strncmp(magicNumber, "VIRL", 4)==0){
        littleEndian = 1;
    }
    else if(strncmp(magicNumber, "VIRB", 4) != 0){
        printf("[-] Error. VIRL/VIRB not detected\n");
        fclose(inputFile);
        exit(1);
    }

    while(!feof(inputFile)){
        virus *virus = readVirus(inputFile, littleEndian);
        if (virus == NULL){
            break;
        }
        virus_list = list_append(virus_list, virus);
    }
    fclose(inputFile);
    return virus_list;
}

link* load_signatures_wrap(link* virus_list, FILE* file){
    //get input file name from user
    char inputFileName[100];
    fgets(inputFileName, sizeof(inputFileName), stdin);
    //the following line is from stackoverflow: https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
    inputFileName[strcspn(inputFileName, "\n")] = 0;
    return load_signatures(virus_list, inputFileName);
}


link* run_function(link* virus_list, FILE* file, link* (*func)(link*, FILE*)){
    return func(virus_list, file);
}

void detect_virus(char* buffer, unsigned int size, link* virus_list){
    link* ptr = virus_list;
    while(ptr != NULL){
        for(int i=0; i<size; i++){
            if(memcmp(buffer+i, ptr->vir->sig, ptr->vir->SigSize) == 0){
                printf("Virus detected at byte: %d\n", i);
                printf("Virus name: %s\n", ptr->vir->virusName);
                printf("Virus signature size: %d\n", ptr->vir->SigSize);
            }
        }
        ptr = ptr->nextVirus;
    }
}

link* detect_viruses(link* virus_list, FILE* file){
    if(file == NULL){
        printf("[*] File for scan should be specified in command line arguments\n");
        return virus_list;
    }
    else if(virus_list == NULL){
        printf("[*] No virus signatures were specified\n");
        return virus_list;
    }
    char * file_to_scan_buffer = calloc(10000, sizeof(char));
    size_t bytes_read = fread(file_to_scan_buffer, sizeof(char), 10000, file);
    detect_virus(file_to_scan_buffer, bytes_read, virus_list);
    free(file_to_scan_buffer);
    fseek(file, 0, SEEK_SET);
    return virus_list;
}

void neutralize_virus(FILE* file, int signatureOffset){
    char ret_opt = 0xC3;
    fseek(file, signatureOffset, SEEK_SET);
    fwrite(&ret_opt, 1, 1, file);
    fseek(file, 0, SEEK_SET);
}

int next_virus(char* buffer, unsigned int size, link* virus_list){
    link* ptr = virus_list;
    while(ptr != NULL){
        for(int i=0; i<size; i++){
            if(memcmp(buffer+i, ptr->vir->sig, ptr->vir->SigSize) == 0){
                return i;
            }
        }
        ptr = ptr->nextVirus;
    }
    return -1;
}

link* fix_file(link* virus_list, FILE* file){
    if(file == NULL){
        printf("[*] file for scan should be specified in command line arguments\n");
        return virus_list;
    }
    else if(virus_list == NULL){
        printf("[*] No virus signatures were specified\n");
        return virus_list;
    }
    
    char * file_to_scan_buffer = calloc(10000, sizeof(char));
    size_t bytes_read = fread(file_to_scan_buffer, sizeof(char), 10000, file);
    int nextVir = -1;
    while ((nextVir = next_virus(file_to_scan_buffer, bytes_read, virus_list)) != -1){
        neutralize_virus(file, nextVir);
        free(file_to_scan_buffer);
        fseek(file, 0, SEEK_SET);
        file_to_scan_buffer = calloc(10000, sizeof(char));
        bytes_read = fread(file_to_scan_buffer, sizeof(char), 10000, file);
    }
    free(file_to_scan_buffer);
    fseek(file, 0, SEEK_SET);

    return virus_list;
}

link* quit(link* virus_list, FILE* file){
    list_free(virus_list);
    if(file != NULL){
        fclose(file);
    }
    exit(1);
}

int main(int argc, char **argv){
    //vars
    char choise[3];
    link* virus_list = NULL;
    struct fun_desc menu[] = { { "Load signatures", load_signatures_wrap }, { "Print signatures", list_print_wrap }, { "Detect viruses", detect_viruses }, { "Fix file", fix_file }, { "Quit", quit },{ NULL, NULL } };
    FILE* file_to_scan = NULL;
    if(argc > 1){
        file_to_scan = fopen(argv[1], "r+b");
    }

    //find size of menu
    int i = 0;
    struct fun_desc p = menu[i];
    while(p.name != NULL){
        p = menu[++i];
    }
    char ascii_i = i + '0';

    //Main menu
    printf("Select operation from the following menu:\n");
    for(int option=0; option<i; option++){
        printf("%d) %s\n", option, menu[option].name);
    }
    printf("Option:");
    while(fgets(choise, 3, stdin) != NULL){
        int number = atoi(&choise[0]);
        // int number = atoi(&choise[0]);
        // if ( number < 0 || i <= number){
        //     printf("Not within bounds\n");
        //     exit(1);
        // }
        if(choise[0] < '0' || ascii_i <= choise[0]){
            printf("Not within bounds\n");
            exit(1);
        }
        printf("\nWithin bounds\n");
        virus_list = run_function(virus_list, file_to_scan, menu[number].fun);
        printf("Done.\n\n");

        printf("Select operation from the following menu:\n");
        for(int option=0; option<i; option++){
            printf("%d) %s\n", option, menu[option].name);
        }
        printf("Option:");
    }
}