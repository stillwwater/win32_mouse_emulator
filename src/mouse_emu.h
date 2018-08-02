#ifndef MOUSE_EMU_H
#define MOUSE_EMU_H

#define WIN32_LEAN_AND_MEAN

typedef enum {
    RECORD,
    PLAY,
} Program_Mode;

typedef struct {
    char *path;
    Program_Mode mode;
    DWORD input_lag;
    DWORD speed;
} Config;

typedef struct {
    short position_x;
    short position_y;
    USHORT state;
} Record;

typedef struct {
    Record *items;
    size_t count;
    size_t size;
} Array;

void array_init(Array *a);
void array_add(Array *a, Record item);
void array_free(Array *a);

bool record_cmp(Record a, Record b);
bool point_in_window(RECT window, POINT p);

void save(const Array *records, const char *path);
void load(Array *records, const char *path);

void do_record(RECT win_rect, Array *records, DWORD polling_lag);
void do_play(HWND window, RECT win_rect, const Array *records, DWORD polling_lag, DWORD speed);

bool parse_args(Config *config, int argc, char **argv);

#endif
