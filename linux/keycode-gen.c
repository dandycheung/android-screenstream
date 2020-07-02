#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include <input-debug.h>
#include "keycode-gen.h"

/**
 * Keycode generate from curent XConfig
 */
int main(int argc, char const *argv[])
{
    Display *display = XOpenDisplay(NULL);
    char name[5];
    char *tmp_name;
    int len, i, a, keycodes_len = 0;
    FILE *f;
    uint16_t found_key;

    if (display == NULL)
    {
        printf("Can't open display\n");
        return 1;
    }
    // https://stackoverflow.com/a/42584969/6682759
    XkbDescPtr KbDesc = XkbGetMap(display, 0, XkbUseCoreKbd);
    XkbGetNames(display, XkbKeyNamesMask, KbDesc);
    XkbGetNames(display, XkbGroupNamesMask, KbDesc);
    tmp_name = XGetAtomName(display, KbDesc->names->groups[0]);

    f = fopen("keycodes.h", "w");
    fprintf(f, "#pragma once\n"
               "/**\n"
               " * This file is auto generated using keycode-gen.c\n"
               " * Keyboard: %s\n"
               " */\n"
               "#include <linux/input-event-codes.h>\n\n"
               "const unsigned short keycodes[] = {",
            tmp_name);

    XFree(tmp_name);
    for (i = 0; i < KbDesc->max_key_code; i++)
    {
        memcpy(&name, KbDesc->names->keys[i].name, XkbKeyNameLength);
        name[XkbKeyNameLength] = 0;
        len = strlen(name);
        keycodes_len++;

        printf("%03d -> ", i);
        if (len == 0)
        {
            printf("\x1b[31mNULL\x1b[0m\n");
            fprintf(f, "\n    0,");
            continue;
        }
        printf("%-4s -> ", name);
        found_key = 0;

        for (a = 0; a < keycode_maps_count; a++)
        {
            if (strncmp(keycode_maps[a].name, name, len) == 0)
            {
                found_key = keycode_maps[a].value;
                break;
            }
        }
        if (found_key)
        {
            tmp_name = get_label(key_labels, found_key);
            printf("\x1b[32m%03d\x1b[33m %s\x1b[0m\n", found_key, tmp_name);
            fprintf(f, "\n    %-22s, // %-4s (%-3d) -> %d", tmp_name, name, i, found_key);
        }
        else
        {
            printf("\x1b[31mNULL\x1b[0m\n");
            fprintf(f, "\n    0,");
        }
    }
    fseek(f, -1, SEEK_CUR);
    fprintf(f, "};\n\n" \ 
    "const unsigned short keycodes_len = %d;\n",
            keycodes_len);
    fclose(f);

    XkbFreeNames(KbDesc, XkbGroupNamesMask, 1);
    XkbFreeNames(KbDesc, XkbKeyNamesMask, 1);
    XkbFreeClientMap(KbDesc, 0, 1);
    /* code */
    return 0;
}
