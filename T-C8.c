// MADE IN 2016 BY MIPPI
// COMMENTS ARE WIP

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>

unsigned short opcode;          // - BUFFER - opcode to process
unsigned char memory[4096];
unsigned char V[16];            // 16 CPU registers
unsigned short I;               // Index register
unsigned short pc;              // Program Counter from 0x200 to 0xFFF
long opNb;
/*

The systems memory map:

0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
0x200-0xFFF - Program ROM and work RAM

*/
unsigned char gfx[64 * 32];
// 60Hz timers
unsigned char delay_timer;
unsigned char sound_timer;
// and opcode allow to make "jump" to address or subroutine, so there is 16 lvl of stack to remember the location before the jump.
unsigned short stack[16];
// and its stack pointer
unsigned short sp;
unsigned char key[16];
unsigned char buffer[2048];
short bufferSize;
unsigned char draw_flag = 0;

unsigned char chip8_fontset[80] =
{
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void emulateCycle();
void openFile();
void init();
void draw();

int main()
{
    opNb = 0;
    //system("mode con LINES=40 COLS=140");
    system("mode con LINES=34 COLS=132");
    openFile();
    Sleep(1500);
    init();
    system("cls");
    Sleep(1000);
    while(1){
        opNb++;
        emulateCycle();
        if(draw_flag == 1)  draw();
        if(delay_timer != 0){
            --delay_timer;
            Sleep(1);
        }
        if(pc > bufferSize + 0x200){
            printf("\n\nERROR\n");
            return 1;
        }
        if(kbhit()){
            short keyboard = getch();
            if(keyboard == 0x1B)
                return 0;
            if(keyboard > 0x30 && keyboard < 0x39){
                keyboard &= 0x0F;
                key[keyboard] = keyboard;
            }
            else if (keyboard > 0x41 && keyboard < 0x46){
                keyboard &= 0x0F;
                keyboard += 0x09;
                key[keyboard] = keyboard;
            }
            keyboard = 0;
        }
    }
}

void init(){
    opcode = 0;
    I = 0;
    pc = 0x200;
    sp = 0;
    for(short test = 0; test < 16; test++)
        stack[test] = 0x200;
    for(int i = 0; i < 80; ++i)
        memory[i] = chip8_fontset[i];
    printf("MEMORY:\n\n");
    for(int i = 0; i < bufferSize; ++i){
        memory[i + 0x200] = buffer[i];
        printf("|%3x|", memory[i + 0x200]);
        if((i + 1) % 8 == 0) printf("\n");
    }
}

void openFile(){
    system("cls");
    printf("CHIP-8 EMULATOR BY MIPPI - V 0 - \n\n");
    printf("\nROM PATH ? (should looks like './TETRIS')\n");
    //char s[128] = "./";
    char s[128];
    scanf("%s", &s);
    printf("Opening %s \n\nFILE:\n\n", s);
    FILE *prog = fopen(s, "rb");
    for(unsigned int i = 0; i < 2048; i++){
        buffer[i] = fgetc(prog);
        printf("|%3x|", buffer[i]);
        if((i + 1) % 8 == 0) printf("\n");
        if((buffer[i] == 255) && (buffer[i-1] == 255) && (buffer[i-2] == 255) && (buffer[i-3] == 255)){
            bufferSize = i;
            i = 65500;
        }
    }
    printf("\nBufferSize = %i \n\n", bufferSize);
    fclose(prog);
}

void emulateCycle(){
    // Fetch opcode / Decode / Execute
    //Sleep(500);
    opcode = memory[pc] << 8 | memory[pc + 1];
    //printf("%i - OPCODE %X: 0x%X\n", opNb, pc, opcode);
    switch(opcode & 0xF000){
    case 0x0000:
        switch(opcode & 0x000F)
        {
              case 0x0000: // 0x00E0: Clears the screen
                if((opcode & 0x00FF) == 0x00E0){
                    system("cls");
                    for(short x = 0; x < (64 * 32); x++){
                        gfx[x] = 0;
                    }
                    pc += 2;
                }
              break;
              /*case 0x0004: //
                  if(V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
                      V[0xF] = 1; //carry
                  else
                      V[0xF] = 0;
                      V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                      pc += 2;
              break;*/
              case 0x000E: // 0x00EE: Returns from subroutine
                // Execute opcode
                    switch(opcode & 0x00F0){
                    case 0x00E0:
                        pc = stack[sp] + 2;
                        --sp;
                    break;
                    default:
                        printf ("Unknown opcode [0x000E]: 0x%X\n", opcode);
                    }
              break;
              default:
                printf ("Unknown opcode [0x0000]: 0x%X\n", opcode);
        }
    break;
    case 0x1000:  // perso JP ADDR 1NNN ONLY JUMP TO CODE
        pc = opcode & 0xFFF;
    break;
    case 0x2000:  // Jump subroutine
        ++sp;
        stack[sp] = pc;
        pc = opcode & 0x0FFF;
    break;
    case 0x3000:     // 3xkk skip the next instruction if Vx == kk
        if(V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
                pc += 4;
        else    pc += 2;
    break;
    case 0x4000:     // 4xkk skip the next instruction if Vx != kk
        if(V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
                pc += 4;
        else    pc += 2;
    break;
    case 0x5000:
        if(V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
                pc += 4;
        else    pc += 2;
    break;
    case 0x6000:     // 6XKK put KK into register VX
        V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
        pc += 2;
    break;
    case 0x7000:     // 7XKK add KK into register VX
        V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] + (opcode & 0x00FF);
        pc += 2;
    break;
    case 0x8000: // 8XY0 -> VX = VY
    {
        switch(opcode & 0x000F){
        case 0x0000:
            V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
            pc += 2;
        break;
        case 0x0001:
            V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] | V[(opcode & 0x0F00) >> 8];
            pc += 2;
        break;
        case 0x0002:
            V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
            pc += 2;
        break;
        case 0x0003:
            V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
            pc += 2;
        break;
        case 0x0004:
            if((V[(opcode & 0x0F00) >> 8] + V[(opcode & 0x00F0) >> 4]) < 255)
                V[0xF] = 1;
                //V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
            else
                V[0xF] = 0;
            //V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x0F00) >> 8] + V[(opcode & 0x00F0) >> 4];
            V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
            pc += 2;
        break;
        case 0x0005:
            if(V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
                V[0xF] = 1;
            else
                V[0xF] = 0;
            V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
            pc += 2;
        break;
        case 0x0006:
            // 8xy6 - SHR Vx {, Vy}
            // Set Vx = Vx SHR 1.
            // If the least-significant bit of Vx is 1, then VF is
			// set to 1, otherwise 0. Then Vx is divided by 2.
            if((V[(opcode & 0x0F00) >> 8] & 0x01) == 0x01){
                V[0xF] = 1;
            }
            else V[0xF] = 0;
            V[(opcode & 0x0F00) >> 8] /= 2;
            pc += 2;
        break;
        case 0x0007:
            // Set Vx = Vy - Vx, set VF = NOT borrow.
            if(V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
                V[0xF] = 1;
            else
                V[0xF] = 0;
            V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
            pc += 2;
        break;
        case 0x000E:
            // Set Vx = Vx SHL 1.
            if(V[(opcode & 0x0F00) >> 8] & 0x80 == 0x80)
                V[0xF] = 1;
            else
                V[0xF] = 0;
            V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] * 2;
            pc += 2;
        break;
        default:
            printf ("Unknown opcode [0x8000]: 0x%X\n", opcode);
        }
    }
    break;
    case 0x9000:  // skip if vx != vy
        if(V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]){
            pc += 4;
        }
        else pc += 2;
    break;
    case 0xA000:    // ANNN: Sets I to the address NNN
        I = opcode & 0x0FFF;
        pc += 2;
    break;
    case 0xB000:// Jump to location nnn + V0.
        pc = (opcode & 0x0FFF) + V[0];
    break;
    case 0xC000: // Set Vx = random byte AND kk.
        V[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
        pc += 2;
    break;
    case 0xD000:  // print screen DXYN N HEIGHT FROM I TO I + HEIGHT
    {
        unsigned short x = V[(opcode & 0x0F00) >> 8];
        unsigned short y = V[(opcode & 0x00F0) >> 4];
        unsigned short height = opcode & 0x000F;
        unsigned short pixel;
        V[0xF] = 0;
        for (int yline = 0; yline < height; yline++)
        {
            pixel = memory[I + yline];
            for(int xline = 0; xline < 8; xline++)
            {
                  if((pixel & (0x80 >> xline)) != 0)
                  {
                    if(gfx[(x + xline + ((y + yline) * 64))] == 1)
                      V[0xF] = 1;
                    gfx[x + xline + ((y + yline) * 64)] ^= 1;
                  }
            }
        }
            draw_flag = 1;
            pc += 2;
        }

    break;
    case 0xE000:
    {
        if((opcode & 0x00FF) == 0x009E){
            if(key[V[(opcode & 0x0F00) >> 8]] == 1){
                pc += 4;
                key[V[(opcode & 0x0F00) >> 8]] = 0;
            }
            else
                key[V[(opcode & 0x0F00) >> 8]] = 0;
                pc += 2;
        }
        else if((opcode & 0x00FF) == 0x00A1){
            if(key[V[(opcode & 0x0F00) >> 8]] == 0){
                pc += 4;
                key[V[(opcode & 0x0F00) >> 8]] = 0;
            }
            else
                pc += 2;
                key[V[(opcode & 0x0F00) >> 8]] = 0;
        }
    }
    break;
    case 0xF000:
    {
        switch(opcode & 0x00FF){
        case 0x0007:  // FX07 VX = delay timer
            V[(opcode & 0x0F00) >> 8] = delay_timer;
            pc += 2;
        break;
        case 0x000A:  // FX0A W8 FOR KEY AND STORE KEY PRESSED IN VX
            for(short i = 0; i < 16; i++){
                if(key[i] != 0){
                    V[(opcode & 0x0F00) >> 8] = key[i];
                    key[i] = 0;
                    pc += 2;
                }
            }
        break;
        case 0x0015:  // FX15 SET DELAY FROM VX
            delay_timer = V[(opcode & 0x0F00) >> 8];
            pc += 2;
        break;
        case 0x0018:  // FX18 SET SOUND DELAY
            sound_timer = V[(opcode & 0x0F00) >> 8];
            pc += 2;
        break;
        case 0x001E:  // FX1E add I and VX AND STORE INTO I
            I = I + V[(opcode & 0x0F00) >> 8];
            pc += 2;
        break;
        case 0x0029: // FX29 PUT I LOCATION OF SPRITE FOR DIGIT VX
            I = V[(opcode & 0x0F00) >> 8];
            pc += 2;
        break;
        case 0x0033:
            memory[I]     = V[(opcode & 0x0F00) >> 8] / 100;
            memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
            memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
            pc += 2;
        break;
        case 0x0055:  // FX55 V0 -> VX into mem
            for(short i = 0; i < (opcode & 0x0F00) >> 8; i++){
                memory[I + i] = V[i];
            }
            pc += 2;
        break;
        case 0x0065:  // FX65 Fill V0 -> VX with mem starting at I
            for(short i = 0; i < (opcode & 0x0F00) >> 8; i++){
                V[0 + i] = memory[I + i];
            }
            I += ((opcode & 0x0F00) >> 8) + 1;
            pc += 2;
        break;
        default:
            printf ("Unknown opcode [0xF000]: 0x%X\n", opcode);
        }
    }
    break;
    default:
        printf("UNKNOWN OPCODE: 0x%X\n", opcode);
    }
}

void draw(){
    printf("\33[%d;%dH%s", 0, 0, "");   // WORKS ONLY ON WINDOWS 10
    fflush(stdout);
        for(short x = 0; x < 64 * 32; x++){
            if(gfx[x] == 0){
                printf("  ");
            }
            else{
                printf("%c", 0xDB);
                printf("%c", 0xDB);
            }
            if((x + 1) % 64 == 0)   printf("\n");
        }
    draw_flag = 0;
}
