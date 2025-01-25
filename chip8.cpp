#include <SDL.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>

int main(int arg, char** args)
{
    if (arg < 2)
    {
        std::cout << "Must supply ROM\n";
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        return -1;
    }

    srand(time(NULL));

    const int RES_X = 64;
    const int RES_Y = 32;
    const int WINDOW_SCALE = 20;
    const int REFRESH = 60;

    SDL_Window* window = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, RES_X * WINDOW_SCALE, RES_Y * WINDOW_SCALE, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    uint8_t memory[4096];
    uint8_t registers[16];
    uint16_t iregister = 0;
    uint16_t programCounter = 512;
    uint8_t delayRegister = 0;
    uint8_t soundRegister = 0;
    uint8_t stackPointer = 0;
    uint16_t stack[16];

    const uint16_t displayPtr = 80; // Memory 80 - 336 used for display, with 1 bit per pixel
    const uint16_t keyboardPtr = 337; // Memory 337 - 352 used for keyboard
    const uint16_t waitKeyboardPtr = 360; // Whether waiting for key press
    const uint16_t waitKeyboardRegisterPtr = 361; // If waiting for key press, which register to store key value in

    std::fill(memory, memory + 4095, 0);
    std::fill(registers, registers + 15, 0);
    std::fill(stack, stack + 15, 0);

    uint8_t font[80] = {
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

    std::copy(font, font + 80, memory);

    std::ifstream input(args[1], std::ios::binary);

    std::vector<char> bytes(
        (std::istreambuf_iterator<char>(input)),
        (std::istreambuf_iterator<char>()));

    input.close();

    for (int i = 0; i < bytes.size(); i++)
    {
        memory[512 + i] = bytes[i];
    }

    bool running = true;

    const uint8_t* keyboardState = SDL_GetKeyboardState(NULL);

    Uint64 currentTick = SDL_GetTicks64();
    Uint64 lastTick = currentTick;

    while (running)
    {
        currentTick = SDL_GetTicks64();
        int tickDiff = currentTick - lastTick;
        lastTick = currentTick;
        int ticksToDelay = 1000 / REFRESH - tickDiff;
        if (ticksToDelay > 0)
        {
            SDL_Delay(ticksToDelay);
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EventType::SDL_QUIT)
            {
                running = false;
            }
        }

        // Poll keyboard
        memory[keyboardPtr] = keyboardState[SDL_Scancode::SDL_SCANCODE_X]; // 0;
        memory[keyboardPtr + 1] = keyboardState[SDL_Scancode::SDL_SCANCODE_1]; // 1;
        memory[keyboardPtr + 2] = keyboardState[SDL_Scancode::SDL_SCANCODE_2]; // 2;
        memory[keyboardPtr + 3] = keyboardState[SDL_Scancode::SDL_SCANCODE_3]; // 3;
        memory[keyboardPtr + 4] = keyboardState[SDL_Scancode::SDL_SCANCODE_Q]; // 4;
        memory[keyboardPtr + 5] = keyboardState[SDL_Scancode::SDL_SCANCODE_W]; // 5;
        memory[keyboardPtr + 6] = keyboardState[SDL_Scancode::SDL_SCANCODE_E]; // 6;
        memory[keyboardPtr + 7] = keyboardState[SDL_Scancode::SDL_SCANCODE_A]; // 7;
        memory[keyboardPtr + 8] = keyboardState[SDL_Scancode::SDL_SCANCODE_S]; // 8;
        memory[keyboardPtr + 9] = keyboardState[SDL_Scancode::SDL_SCANCODE_D]; // 9;
        memory[keyboardPtr + 10] = keyboardState[SDL_Scancode::SDL_SCANCODE_Z]; // A;
        memory[keyboardPtr + 11] = keyboardState[SDL_Scancode::SDL_SCANCODE_C]; // B;
        memory[keyboardPtr + 12] = keyboardState[SDL_Scancode::SDL_SCANCODE_4]; // C;
        memory[keyboardPtr + 13] = keyboardState[SDL_Scancode::SDL_SCANCODE_R]; // D;
        memory[keyboardPtr + 14] = keyboardState[SDL_Scancode::SDL_SCANCODE_F]; // E;
        memory[keyboardPtr + 15] = keyboardState[SDL_Scancode::SDL_SCANCODE_V]; // F;


        if (memory[waitKeyboardPtr])
        {
            // Check for key presses
            for (int i = 0; i < 16; i++)
            {
                if (memory[keyboardPtr + i])
                {
                    memory[waitKeyboardPtr] = false;
                    registers[memory[waitKeyboardRegisterPtr]] = i;
                    memory[waitKeyboardRegisterPtr] = 0;
                    break;
                }
            }
        }
        else
        {
            delayRegister = std::max(delayRegister - 1, 0);
            soundRegister = std::max(soundRegister - 1, 0);

            uint16_t instruction = ((static_cast<uint16_t>(memory[programCounter]) << 8) | static_cast<uint16_t>(memory[programCounter + 1]));
            
            programCounter += 2;

            if (instruction == 0x00E0) // Clear screen
            {
                for (int i = displayPtr; i < displayPtr + (RES_X * RES_Y) / 8; i++)
                {
                    memory[i] = 0x00;
                }
            }
            else if (instruction == 0x00EE) // Return from subroutine
            {
                programCounter = stack[stackPointer];
                stackPointer--;
            }
            else if ((instruction & 0xF000) == 0x1000) // Jump to address
            {
                programCounter = (instruction & 0x0FFF);
            }
            else if ((instruction & 0xF000) == 0x2000) // Call subroutine
            {
                stackPointer++;
                stack[stackPointer] = programCounter;
                programCounter = (instruction & 0x0FFF);
            }
            else if ((instruction & 0xF000) == 0x3000) // Skip next instruction if register == value
            {
                if ((instruction & 0x00FF) == registers[(instruction & 0x0F00) >> 8])
                {
                    programCounter += 2;
                }
            }
            else if ((instruction & 0xF000) == 0x4000) // Skip next instruction if register != value
            {
                if ((instruction & 0x00FF) != registers[(instruction & 0x0F00) >> 8])
                {
                    programCounter += 2;
                }
            }
            else if ((instruction & 0xF00F) == 0x5000) // Skip next instruction if register == register
            {
                if (registers[(instruction & 0x00F0) >> 4] == registers[(instruction & 0x0F00) >> 8])
                {
                    programCounter += 2;
                }
            }
            else if ((instruction & 0xF000) == 0x6000) // Set register = value
            {
                registers[(instruction & 0x0F00) >> 8] = (instruction & 0x00FF);
            }
            else if ((instruction & 0xF000) == 0x7000) // Add value to register
            {
                registers[(instruction & 0x0F00) >> 8] += (instruction & 0x00FF);
            }
            else if ((instruction & 0xF00F) == 0x8000) // Copy register to another register
            {
                registers[(instruction & 0x0F00) >> 8] = registers[(instruction & 0x00F0) >> 4];
            }
            else if ((instruction & 0xF00F) == 0x8001) // Set register to bitwise OR of itself and another register
            {
                registers[(instruction & 0x0F00) >> 8] |= registers[(instruction & 0x00F0) >> 4];
            }
            else if ((instruction & 0xF00F) == 0x8002) // Set register to bitwise AND of itself and another register
            {
                registers[(instruction & 0x0F00) >> 8] &= registers[(instruction & 0x00F0) >> 4];
            }
            else if ((instruction & 0xF00F) == 0x8003) // Set register to bitwise XOR of itself and another register
            {
                registers[(instruction & 0x0F00) >> 8] ^= registers[(instruction & 0x00F0) >> 4];
            }
            else if ((instruction & 0xF00F) == 0x8004) // Add value to register from another register and set register 15 to carry
            {
                uint16_t result = static_cast<uint16_t>(registers[(instruction & 0x0F00) >> 8]) + static_cast<uint16_t>(registers[(instruction & 0x00F0) >> 4]);
                registers[(instruction & 0x0F00) >> 8] = static_cast<uint8_t>(result & 0x00FF);
                if ((result & 0xFF00) == 0)
                {
                    registers[15] = 0;
                }
                else
                {
                    registers[15] = 1;
                }
            }
            else if ((instruction & 0xF00F) == 0x8005) // Subtract value from register from another register and set register 15 to NOT borrow flag
            {
                if (registers[(instruction & 0x0F00) >> 8] > registers[(instruction & 0x00F0) >> 4])
                {
                    registers[15] = 1;
                }
                else
                {
                    registers[15] = 0;
                }
                registers[(instruction & 0x0F00) >> 8] -= registers[(instruction & 0x00F0) >> 4];
            }
            else if ((instruction & 0xF00F) == 0x8006) // Shift register right and set register 15 to 1 if underflow
            {
                registers[15] = (registers[(instruction & 0x0F00) >> 8] & 0x01);
                registers[(instruction & 0x0F00) >> 8] >> 1;
            }
            else if ((instruction & 0xF00F) == 0x8007) // Subtract value from register from another register (inverted) and set register 15 to NOT borrow flag
            {
                if (registers[(instruction & 0x00F0) >> 4] > registers[(instruction & 0x0F00) >> 8])
                {
                    registers[15] = 1;
                }
                else
                {
                    registers[15] = 0;
                }
                registers[(instruction & 0x00F0) >> 4] -= registers[(instruction & 0x0F00) >> 8];
            }
            else if ((instruction & 0xF00F) == 0x800E) // Shift register left and set register 15 to 1 if overflow
            {
                registers[15] = (registers[(instruction & 0x0F00) >> 8] & 0x80);
                registers[(instruction & 0x0F00) >> 8] << 1;
            }
            else if ((instruction & 0xF00F) == 0x9000) // Skip next instruction if register does not equal another
            {
                if (registers[(instruction & 0x0F00) >> 8] != registers[(instruction & 0x00F0) >> 4])
                {
                    programCounter += 2;
                }
            }
            else if ((instruction & 0xF000) == 0xA000) // Set i register value
            {
                iregister = (instruction & 0x0FFF);
            }
            else if ((instruction & 0xF000) == 0xB000) // Jump to value + value in register 0
            {
                programCounter = registers[0] + (instruction & 0x0FFF);
            }
            else if ((instruction & 0xF000) == 0xC000) // Set register to value and bitwise AND with random value
            {
                uint8_t random = rand() % 0xFF;
                registers[(instruction & 0x0F00) >> 8] = (((instruction & 0x00FF)) & random);
            }
            else if ((instruction & 0xF000) == 0xD000) // Draw sprite to screen
            {
                int spriteHeight = (instruction & 0x000F);
                int xDraw = registers[(instruction & 0x0F00) >> 8];
                int yDraw = registers[(instruction & 0x00F0) >> 4];

                bool erased = false;
                for (int i = 0; i < spriteHeight; i++)
                {
                    uint64_t spriteLine = static_cast<uint64_t>(memory[iregister + i]) << (7 * 8);
                    uint16_t displayLinePtr = displayPtr + ((yDraw + i) % RES_Y) * RES_X / 8;
                    uint64_t displayLine = 0;

                    for (int lineChar = 0; lineChar < RES_X / 8; lineChar++)
                    {
                        displayLine = (displayLine << 8) | memory[displayLinePtr + lineChar];
                    }

                    uint64_t displayLineDrawn = displayLine ^ (spriteLine >> (xDraw % RES_X));
                    
                    for (int lineChar = 0; lineChar < RES_X / 8; lineChar++)
                    {
                        memory[displayLinePtr + lineChar] = ((displayLineDrawn >> (8 * (RES_X / 8 - 1 - lineChar))) & 0xFF);
                    }

                    if (displayLine & ~displayLineDrawn)
                    {
                        erased = true;
                    }
                }

                registers[15] = erased;
            }
            else if ((instruction & 0xF0FF) == 0xE09E) // Skip next instruction if key is pressed
            {
                if (memory[keyboardPtr + registers[(instruction & 0x0F00) >> 8]])
                {
                    programCounter += 2;
                }
            }
            else if ((instruction & 0xF0FF) == 0xE0A1) // Skip next instruction if key is NOT pressed
            {
                if (!memory[keyboardPtr + registers[(instruction & 0x0F00) >> 8]])
                {
                    programCounter += 2;
                }
            }
            else if ((instruction & 0xF0FF) == 0xF007) // Set register to delay register value
            {
                registers[(instruction & 0x0F00) >> 8] = delayRegister;
            }
            else if ((instruction & 0xF0FF) == 0xF00A) // Wait for key press, then store key value in register
            {
                memory[waitKeyboardPtr] = true;
                memory[waitKeyboardRegisterPtr] = (instruction & 0x0F00) >> 8;
            }
            else if ((instruction & 0xF0FF) == 0xF015) // Set delay register to register value
            {
                delayRegister = registers[(instruction & 0x0F00) >> 8];
            }
            else if ((instruction & 0xF0FF) == 0xF018) // Set sound register to register value
            {
                soundRegister = registers[(instruction & 0x0F00) >> 8];
            }
            else if ((instruction & 0xF0FF) == 0xF01E) // Add register value to i register
            {
                iregister += registers[(instruction & 0x0F00) >> 8];
            }
            else if ((instruction & 0xF0FF) == 0xF029) // Set i register to memory location containing font bitmap of register value
            {
                iregister = 5 * registers[(instruction & 0x0F00) >> 8];
            }
            else if ((instruction & 0xF0FF) == 0xF033) // Store BCD of register in 3 memory locations, starting with location in i register
            {
                int value = registers[(instruction & 0x0F00) >> 8];
                memory[iregister] = value / 100;
                memory[iregister + 1] = (value / 10) % 10;
                memory[iregister + 2] = value % 10;
            }
            else if ((instruction & 0xF0FF) == 0xF055) // Copy n registers into memory, starting with location in i register
            {
                int count = (instruction & 0x0F00) >> 8;
                for (int i = 0; i <= count; i++)
                {
                    memory[iregister + i] = registers[i];
                }
            }
            else if ((instruction & 0xF0FF) == 0xF065) // Read from memory into n registers, starting with location in i register
            {
                int count = (instruction & 0x0F00) >> 8;
                for (int i = 0; i <= count; i++)
                {
                    registers[i] = memory[iregister + i];
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Display screen
        for (int y = 0; y < RES_Y; y++)
        {
            uint64_t displayLine = 0;
            for (int lineChar = 0; lineChar < RES_X / 8; lineChar++)
            {
                displayLine = (displayLine << 8) | memory[displayPtr + y * RES_X / 8 + lineChar];
            }
            for (int x = 0; x < RES_X; x++)
            {
                int colour = ((displayLine >> (RES_X - 1 - x)) & 0x1) ? 255 : 0;
                SDL_SetRenderDrawColor(renderer, colour, colour, colour, 255);

                SDL_Rect rect{x * WINDOW_SCALE, y * WINDOW_SCALE, WINDOW_SCALE, WINDOW_SCALE};
                SDL_RenderFillRect(renderer, &rect);
            }
        }

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();

    return 0;
}