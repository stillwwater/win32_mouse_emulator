/* ==============================================================
    Record: record output_file_path [input_lag (ms)]
    Play:   play input_file_path [input_lag (ms)] [speed]
   ============================================================= */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "mouse_emu.h"

int main(int argc, char **argv) {
    RECT win_rect;
    Config config;
    Array records;

    if (!parse_args(&config, argc, argv)) return -1;
    array_init(&records);

    printf("--- select a window and press F9 to begin ---\n");
    while (!GetKeyState(VK_F9)) Sleep(1); // Wait for keypress
    Sleep(200);

    HWND window = GetForegroundWindow();
    GetWindowRect(window, &win_rect);

    if (config.mode == PLAY) {
        load(&records, config.path);
        do_play(window, win_rect, &records, config.input_lag, config.speed);
        printf(" done.\n");
    } else {
        do_record(win_rect, &records, config.input_lag);
        printf(" recorded %d point", records.count);

        if (records.count == 1) {
            printf(".\n");
        } else {
            printf("s.\n");
        }
        save(&records, config.path);
    }
    array_free(&records);
    return 0;
}

void do_record(RECT win_rect, Array *records, DWORD polling_lag) {
    POINT cursor;
    printf("\nrecording (F9 to stop)...");

    while (1) {
        SHORT exit_press = GetAsyncKeyState(VK_F9);
        if ((1 << 15) & exit_press) break;

        if (GetCursorPos(&cursor) && point_in_window(win_rect, cursor)) {
            Record input;
            // Get relative position of cursor
            input.position_x = (short)(cursor.x - win_rect.left);
            input.position_y = (short)(cursor.y - win_rect.top);

            if ((GetKeyState(VK_LBUTTON) & 0x100) != 0) {
                input.state = WM_LBUTTONDOWN; // Left click
            } else if ((GetKeyState(VK_RBUTTON) & 0x100) != 0) {
                input.state = WM_RBUTTONDOWN; // Right click
            } else {
                input.state = 0;
            }

            if (records->count == 0 ) {
                array_add(records, input);
            } else if (!record_cmp(input, records->items[records->count - 1])) {
                // Only record if there was a change in the input
                array_add(records, input);
            }
        }
        Sleep(polling_lag);
    }
}

void do_play(HWND window, RECT win_rect, const Array *records, DWORD polling_lag, DWORD speed) {
    int button_down = 0;
    printf("\nplaying...");

    for (int i = 0; i < records->count; i++) {
        Record r = records->items[i];
        SetCursorPos(r.position_x + win_rect.left, r.position_y + win_rect.top);

        if (!button_down && r.state) {
            button_down = r.state;
            // Send mouse down
            SendMessage(window, r.state, MK_LBUTTON, MAKELPARAM(r.position_x, r.position_y));
        } else if (button_down && !r.state) {
            // Send mouse up
            SendMessage(window, button_down + 1, MK_LBUTTON, MAKELPARAM(r.position_x, r.position_y));
            button_down = 0;
        }
        Sleep(polling_lag / speed);
    }
}

void save(const Array *records, const char *path) {
    FILE *file = fopen(path, "wb");

    if (file == NULL) {
        printf("Error: Unable to open file\n");
        return;
    }

    fwrite(records->items, sizeof(Record), records->count, file);
    fclose(file);
}

void load(Array *records, const char *path) {
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        printf("Error: Unable to open file\n");
        return;
    }

    Record tmp;

    while (fread(&tmp, sizeof(Record), 1, file)) {
        array_add(records, tmp);
    }
}

void array_init(Array *a) {
    a->items = malloc(sizeof(Record));
    a->count = 0;
    a->size  = 1;
}

void array_add(Array *a, Record item) {
    if (a->count >= a->size) {
        a->size *= 2;
        a->items = realloc(a->items, a->size * sizeof(Record));
    }
    a->items[a->count] = item;
    a->count++;
}

void array_free(Array *a) {
    free(a->items);
    a->items  = NULL;
    a->count  = 0;
    a->size   = 0;
}

bool record_cmp(Record a, Record b) {
    return (a.position_x == b.position_x && a.position_y == b.position_y && a.state == b.state);
}

bool point_in_window(RECT window, POINT p) {
    return (p.x >= window.left && p.x <= window.right && p.y >= window.top && p.y <= window.bottom);
}

bool parse_args(Config *config, int argc, char **argv) {
    if (argc <= 1) {
        printf("--- Win32 Mouse Emulator ---\n\n");
        printf("Record mouse input to a file, and play it back in Windows.\n\n");
        printf("Usage:\n\n");
        printf("mouse_emu record output_path [input_lag (ms)]\n");
        printf("mouse_emu play input_path [input_lag (ms)] [speed]\n");
        return false;
    }

    if (argc <= 2) {
        printf("error: Missing file path for recording.");
        return false;
    }

    if (strcmp(argv[1], "record") == 0) {
        config->mode = RECORD;
    } else if (strcmp(argv[1], "play") == 0) {
        config->mode = PLAY;
    } else {
        printf("error: Unknown program mode (must be one of record|play).");
        return false;
    }

    config->path = argv[2];
    config->input_lag = argc > 3 ? atoi(argv[3]) : 1;
    config->speed     = argc > 4 ? atoi(argv[4]) : 1;

    if (config->speed < 1) {
        printf("warning: Playback speed must be at least 1.");
        config->speed = 1;
    }

    return true;
}
