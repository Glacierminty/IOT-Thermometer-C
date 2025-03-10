/*
 * Display messages thread.  Modules can use the helper function
 * to send information to any displays connected to the thing.
 *
 */
#include "mbed.h"
#include "config.h"
#include "display.h"
#include <string.h>
#include "vt100.h"

Semaphore displayUse(1);

/* clang has a problem with strcpy so implemented our own function */
int stringcpy(char* b, char* a) {
    int i = 0;
    while (a[i] != NULL) {
        b[i] = a[i];
        i++;
    }
    return i;
}

static MemoryPool<message_t, 32> mpool;
static Queue<message_t, 32> queue;

void displayMessage(message_t msg){
    while (displayUse.try_acquire() == false) {
        ThisThread::sleep_for(1);
    }
    message_t *message = mpool.alloc();
    if(message) {
        stringcpy ( message->buffer, msg.buffer);
        message->displayType = msg.displayType;
        queue.put(message);
    }
    displayUse.release();
}    
void displayPanel() {
    while (displayUse.try_acquire() == false) {
        ThisThread::sleep_for(1);
    }
    CLS;
    ThisThread::sleep_for(10);
    HOME;

    printf("┌───────────────────────────────────────────────────────────────────────────┐\n");
    printf("│                           City1082 Telemetry                              │\n");
    printf("├───────────────────────────┬─────────┬───────────────────────────┬─────────┤\n");
    printf("│ Temperature Reading       │         │ Light Level               │         │\n");
    printf("├───────────────────────────┼─────────┼───────────────────────────┼─────────┤\n");
    printf("│ Temperature Setting       │         │ Light Level Setting       │         │\n");
    printf("├───────────────────────────┼─────────┼───────────────────────────┼─────────┤\n");
    printf("│ Heater State              │         │ Light State               │         │\n");
    printf("├───────────────────────────┴─────────┴───────────────────────────┴─────────┤\n");
    printf("│                                                                           │\n");
    printf("└───────────────────────────────────────────────────────────────────────────┘\n");
    displayUse.release();

}
void displayTask() {
    //RIS; // reset terminal
    //ThisThread::sleep_for(1000);
    CLS; // clear the vt100 terminal screen
    ThisThread::sleep_for(10);
    BLUE_BOLD;
    HIDE_CURSOR;
    displayPanel();
//    printf("\033[2;25HCity1082 Telemetry"); //Title at top middle
    NORMAL;
    while (true) {
        osEvent evt = queue.get();
        if (evt.status == osEventMessage) {
            message_t *message = (message_t*)evt.value.p;
            switch(message->displayType) {
                case TEMPERATURE_READING: {
                    printf("\033[4;31H%s", message->buffer);
                    break;
                }
                case TEMPERATURE_SETTING: {
                    printf("\033[6;31H%s", message->buffer);
                    break;
                }
                case HEATER_STATE: {
                    printf("\033[8;32H%s", message->buffer);
                    break;
                }
                case LIGHT_READING: {
                    printf("\033[4;69H%s", message->buffer);
                    break;
                }
                case LIGHT_SETTING: {
                    printf("\033[6;69H%s", message->buffer);
                    break;
                }
                 case LIGHT_STATE: {
                    printf("\033[8;70H%s", message->buffer);
                    break;
                }
               case STATUS_DISPLAY: {
                    printf("\033[10;3H%s", message->buffer);
                    break;
                }
                default: {
                    printf("\033[20;3HNo definition detected %d %s\n", message->displayType, message->buffer);
                }
            }

            mpool.free(message);
        }
        ThisThread::sleep_for(10);
    }
}