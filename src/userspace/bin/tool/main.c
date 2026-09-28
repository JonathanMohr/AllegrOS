#include "syscall_nums.h"
#include <syscall.h>

#define SC_LSHIFT 0x2A
#define SC_RSHIFT 0x36

static const char ascii_lower[128] = {
    [0x02]='1', [0x03]='2', [0x04]='3', [0x05]='4', [0x06]='5', [0x07]='6',
    [0x08]='7', [0x09]='8', [0x0A]='9', [0x0B]='0',
    [0x0C]=0 /* ß */, [0x0D]=0 /* ´ */,
    [0x0E]='\b', [0x0F]='\t',
    [0x10]='q', [0x11]='w', [0x12]='e', [0x13]='r', [0x14]='t', [0x15]='z',
    [0x16]='u', [0x17]='i', [0x18]='o', [0x19]='p',
    [0x1A]=0 /* ü */, [0x1B]='+', [0x1C]='\n',
    [0x1E]='a', [0x1F]='s', [0x20]='d', [0x21]='f', [0x22]='g', [0x23]='h',
    [0x24]='j', [0x25]='k', [0x26]='l',
    [0x27]=0 /* ö */, [0x28]=0 /* ä */, [0x29]=0 /* ^ */,
    [0x2B]='#', [0x2C]='y', [0x2D]='x', [0x2E]='c', [0x2F]='v',
    [0x30]='b', [0x31]='n', [0x32]='m',
    [0x33]=',', [0x34]='.', [0x35]='-',
    [0x37]='*', [0x39]=' ',
    [0x56]='<',
};

static const char ascii_upper[128] = {
    [0x02]='!', [0x03]='"', [0x04]=0 /* § */, [0x05]='$', [0x06]='%', [0x07]='&',
    [0x08]='/', [0x09]='(', [0x0A]=')', [0x0B]='=',
    [0x0C]='?', [0x0D]='`',
    [0x0E]='\b', [0x0F]='\t',
    [0x10]='Q', [0x11]='W', [0x12]='E', [0x13]='R', [0x14]='T', [0x15]='Z',
    [0x16]='U', [0x17]='I', [0x18]='O', [0x19]='P',
    [0x1A]=0 /* Ü */, [0x1B]='*', [0x1C]='\n',
    [0x1E]='A', [0x1F]='S', [0x20]='D', [0x21]='F', [0x22]='G', [0x23]='H',
    [0x24]='J', [0x25]='K', [0x26]='L',
    [0x27]=0 /* Ö */, [0x28]=0 /* Ä */, [0x29]=0 /* ° */,
    [0x2B]='\'', [0x2C]='Y', [0x2D]='X', [0x2E]='C', [0x2F]='V',
    [0x30]='B', [0x31]='N', [0x32]='M',
    [0x33]=';', [0x34]=':', [0x35]='_',
    [0x37]='*', [0x39]=' ',
    [0x56]='>',
};

#define KEY_ARROW_UP    (0x48 | 0x80)
#define KEY_ARROW_DOWN  (0x50 | 0x80)
#define KEY_ARROW_LEFT  (0x4B | 0x80)
#define KEY_ARROW_RIGHT (0x4D | 0x80)
#define KEY_DELETE      (0x53 | 0x80)
#define KEY_HOME        (0x47 | 0x80)
#define KEY_END         (0x4F | 0x80)

#define KEY_ENTER       (0x1C)

#define MAX_PATH_CHARACTERS 1028
#define MAX_DATA_BYTES 8192

static unsigned char data_buffer[MAX_DATA_BYTES] = {0};
static unsigned long current_data_pos = 0;

static char buffer[MAX_PATH_CHARACTERS + 1] = {0};
static unsigned long current_pos = 0;

static const char err_msg[] = "\nERROR: TOO MANY CHARACTERS / BYTES\n";
static const unsigned long err_msg_len = sizeof(err_msg) - 1;

static const char file_err1[] = "\nERROR: COULD NOT OPEN FILE\n";
static const char file_err1_len = sizeof(file_err1) - 1;

static const char file_err2[] = "\nERROR: COULD NOT WRITE TO FILE\n";
static const char file_err2_len = sizeof(file_err2) - 1;

static const char file_err3[] = "\nERROR: COULD NOT OPEN DIRECTORY\n";
static const char file_err3_len = sizeof(file_err3) - 1;

static const char file_err4[] = "\nERROR: COULD NOT CREATE DIRECTORY\n";
static const char file_err4_len = sizeof(file_err4) - 1;

static const char file_err5[] = "\nERROR: COULD NOT MOVE ENTRY\n";
static const char file_err5_len = sizeof(file_err5) - 1;

static const char file_err6[] = "\nERROR: COULD NOT REMOVE ENTRY\n";
static const char file_err6_len = sizeof(file_err6) - 1;

static int add_to_data(unsigned char c)
{
    if (current_data_pos >= MAX_DATA_BYTES)
        return 0;
    data_buffer[current_data_pos++] = c;
    return 1;
}

static int validate_path(char c)
{
    if (c >= 'a' && c <= 'z')
        return 1;
    if (c >= 'A' && c <= 'Z')
        return 1;
    if (c >= '0' && c <= '9')
        return 1;
    if (c == '/' || c == ' ' || c == '_' || c == '-' || c == '.' || c == '\b')
        return 1;
    return 0;
}

static int validate_data(char c)
{
    if (validate_path(c))
        return 1;
    if (c == ',' || c == ';' || c == '.' || c == ':' || c == '<' || c == '>')
        return 1;
    if (c == '!' || c == '"' || c == '$' || c == '%' || c == '&' || c == '(' || c == ')' || c == '=' || c == '?')
        return 1;
    if (c == '+' || c == '*' || c == '#' || c == '\'')
        return 1;
    return 0;
}

static const char newline_char = '\n';
static const char inHex_char = '~';

static int shift_held = 0;
static struct syscall_keyboard_event ev;

static int enter_path_better(const char* msg, syscall_t len)
{
    current_pos = 0;

    syscall_write(0, msg, len);

    ev.released = 1;
    while (ev.keycode != KEY_ENTER || ev.released)
    {
        if (syscall_read_keyboard_event(&ev) != 0)
            continue;

        if (ev.keycode == SC_LSHIFT || ev.keycode == SC_RSHIFT)
        {
            shift_held = !ev.released;
            continue;
        }

        if (ev.released)
            continue;

        const char c = shift_held ? ascii_upper[ev.keycode] : ascii_lower[ev.keycode];
        if (!validate_path(c))
            continue;

        if (c == '\b')
        {
            if (current_pos > 0)
            {
                buffer[--current_pos] = '\0';
                syscall_write(0, &c, 1);
            }
            continue;
        }
        else
        {
            buffer[current_pos++] = c;
        }

        if (current_pos >= MAX_PATH_CHARACTERS)
        {
            syscall_write(0, err_msg, err_msg_len);
            return 0;
        }

        syscall_write(0, &c, 1);
    }
    buffer[current_pos] = '\0';

    syscall_write(0, &newline_char, 1);

    return 1;
}

static int enter_path(void)
{
    static const char path_msg[] = "Enter the path of the file [PATH]: ";
    return enter_path_better(path_msg, sizeof(path_msg) - 1);
}

static int enter_data(void)
{
    static const char data_msg[] = "Enter content of file [BIN]:\n";

    current_data_pos = 0;

    syscall_write(0, data_msg, sizeof(data_msg) - 1);

    int inHex = 0;
    while (1)
    {
        if (syscall_read_keyboard_event(&ev) != 0)
            continue;

        if (ev.keycode == SC_LSHIFT || ev.keycode == SC_RSHIFT)
        {
            shift_held = !ev.released;
            continue;
        }

        if (ev.released)
            continue;

        const char c = shift_held ? ascii_upper[ev.keycode] : ascii_lower[ev.keycode];
        if (!validate_data(c) && ev.keycode != KEY_ENTER)
            continue;

        if (c == '\b')
        {
            if (inHex)
            {
                static const char backspace = '\b';
                syscall_write(0, &backspace, 1);
                inHex = 0;
            }
            else
            {
                if (current_data_pos > 0)
                {
                    current_data_pos--;
                    syscall_write(0, &c, 1);
                }
            }
            continue;
        }

        if (inHex)
        {
            if (ev.keycode == KEY_ENTER && !ev.released)
                break;

            if (c == '$')
            {
                static const char dollar[] = "\b$";
                syscall_write(0, dollar, sizeof(dollar) - 1);
                inHex = 0;

                if (!add_to_data((unsigned char)'$'))
                {
                    syscall_write(0, err_msg, err_msg_len);
                    return 0;
                }
            }

            inHex = 0;

            continue;
        }

        if (c == '$')
        {
            syscall_write(0, &inHex_char, 1);
            inHex = 1;
            continue;
        }

        if (!add_to_data((unsigned char)c))
        {
            syscall_write(0, err_msg, err_msg_len);
            return 0;
        }

        syscall_write(0, &c, 1);
    }

    syscall_write(0, &newline_char, 1);

    return 1;
}


void write(void)
{
    static const char start_msg[] = "Write:\n";
    static const char end_msg[] = "Finished writing to file!\n";

    syscall_write(0, start_msg, sizeof(start_msg) - 1);

    if (!enter_path())
        return;
    
    if (!enter_data())
        return;

    syscall_t file = syscall_open_file(buffer, 1);
    if (!file)
    {
        syscall_write(0, file_err1, file_err1_len);
        return;
    }
    if (syscall_write(file, data_buffer, current_data_pos) != current_data_pos)
    {
        syscall_write(0, file_err2, file_err2_len);
        return;
    }
    syscall_close_file(file);

    syscall_write(0, end_msg, sizeof(end_msg) - 1);
}

void read(void)
{
    static const char start_msg[] = "Read:\n";
    static const char end_msg[] = "~\n";

    syscall_write(0, start_msg, sizeof(start_msg) - 1);

    if (!enter_path())
        return;

    syscall_t file = syscall_open_file(buffer, 0);
    if (!file)
    {
        syscall_write(0, file_err1, file_err1_len);
        return;
    }
    syscall_t read;
    while ((read = syscall_read(file, data_buffer, MAX_DATA_BYTES)))
    {
        for (unsigned long i = 0; i < read; i++)
        {
            unsigned char c = data_buffer[i];
            if (validate_data((char)c) && c != '\b')
            {
                syscall_write(0, &c, 1);
            }
            else
            {
                syscall_write(0, &inHex_char, 1);
                // TODO
            }
        }
    }
    syscall_close_file(file);

    syscall_write(0, end_msg, sizeof(end_msg) - 1);
}


void list(void)
{
    static const char start_msg[] = "List:\n";
    // static const char end_msg[] = "";

    syscall_write(0, start_msg, sizeof(start_msg) - 1);

    if (!enter_path())
        return;

    syscall_t dir = syscall_open_dir(buffer);
    if (!dir)
    {
        syscall_write(0, file_err3, file_err3_len);
        return;
    }
    struct syscall_entry entry;
    while (syscall_readdir(dir, &entry) == 0)
    {
        static const char file_str[] = "FIL [";
        static const char directory_str[] = "DIR [";

        static const char after_size_str[] = "] (";

        static const char not_readonly_str[] = "-- | ";
        static const char readonly_str[] =     "ro | ";
        static const char not_executable_str[] = "---- | ";
        static const char executable_str[] =     "exec | ";
        static const char not_hidden_str[] = "--- | ";
        static const char hidden_str[] =     "hid | ";
        static const char not_system_str[] = "--- | ";
        static const char system_str[] =     "sys | ";

        static const char no_name_str[] = "?";

        switch (entry.type)
        {
            case SYSCALL_ENTRY_FILE:
                syscall_write(0, file_str, sizeof(file_str) - 1);
                break;

            case SYSCALL_ENTRY_DIRECTORY:
                syscall_write(0, directory_str, sizeof(directory_str) - 1);
                break;
            
            default:
                continue;
        }

        // TODO: Size
        syscall_write(0, after_size_str, sizeof(after_size_str) - 1);

        if (entry.attributes & SYSCALL_ENTRY_READONLY)
            syscall_write(0, readonly_str, sizeof(readonly_str) - 1);
        else
            syscall_write(0, not_readonly_str, sizeof(not_readonly_str) - 1);

        if (entry.attributes & SYSCALL_ENTRY_EXECUTABLE)
            syscall_write(0, executable_str, sizeof(executable_str) - 1);
        else
            syscall_write(0, not_executable_str, sizeof(not_executable_str) - 1);

        if (entry.attributes & SYSCALL_ENTRY_HIDDEN)
            syscall_write(0, hidden_str, sizeof(hidden_str) - 1);
        else
            syscall_write(0, not_hidden_str, sizeof(not_hidden_str) - 1);

        if (entry.attributes & SYSCALL_ENTRY_SYSTEM)
            syscall_write(0, system_str, sizeof(system_str) - 1);
        else
            syscall_write(0, not_system_str, sizeof(not_system_str) - 1);

        const char* namePtr = entry.name;
        while (*namePtr)
            namePtr++;

        if ((syscall_t)(namePtr - entry.name))
            syscall_write(0, entry.name, (syscall_t)(namePtr - entry.name));
        else
            syscall_write(0, no_name_str, sizeof(no_name_str) - 1);

        syscall_write(0, &newline_char, 1);
    }
    syscall_close_dir(dir);

    // syscall_write(0, end_msg, sizeof(end_msg) - 1);
}

void dir(void)
{
    static const char start_msg[] = "Dir:\n";
    static const char end_msg[] = "Created directory!\n";

    syscall_write(0, start_msg, sizeof(start_msg) - 1);

    if (!enter_path())
        return;

    if (syscall_makedir(buffer) != 0)
    {
        syscall_write(0, file_err4, file_err4_len);
        return;
    }

    syscall_write(0, end_msg, sizeof(end_msg) - 1);
}

static char move_buffer[MAX_PATH_CHARACTERS + 1] = {0};

void move(void)
{
    static const char move_msg1[] = "Enter the source path: ";
    static const char move_msg2[] = "Enter the destination path: ";

    static const char start_msg[] = "Move:\n";
    static const char end_msg[] = "Moved entry!\n";

    syscall_write(0, start_msg, sizeof(start_msg) - 1);

    if (!enter_path_better(move_msg1, sizeof(move_msg1) - 1))
        return;

    for (unsigned long i = 0; i < MAX_PATH_CHARACTERS; i++)
        move_buffer[i] = buffer[i];

    if (!enter_path_better(move_msg2, sizeof(move_msg2) - 1))
        return;

    if (syscall_move(move_buffer, buffer) != 0)
    {
        syscall_write(0, file_err5, file_err5_len);
        return;
    }

    syscall_write(0, end_msg, sizeof(end_msg) - 1);
}

void remove(void)
{
    static const char start_msg[] = "Remove:\n";
    static const char end_msg[] = "Removed entry!\n";

    syscall_write(0, start_msg, sizeof(start_msg) - 1);

    if (!enter_path())
        return;

    if (syscall_remove(buffer) != 0)
    {
        syscall_write(0, file_err6, file_err6_len);
        return;
    }

    syscall_write(0, end_msg, sizeof(end_msg) - 1);
}


int main(void)
{
    static const char clear_msg[] =
        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
        "\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r"
        "\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r";

    static const char info_msg[] =
        "Info:\n"
        "  This is the first program of the OS!\n"
        "  You can manipulate the filesystem with this program.\n"
        "\n"
        "  To enter commands press the corresponding key.\n"
        "  To see all commands with descriptions, press the key 'h'.\n";

    static const char help_msg[] =
        "Help:\n"
        "  i        -- Info     - Show the info message\n"
        "  h        -- Help     - Show this message\n"
        "\n"
        "  c        -- Clear    - Clear the screen\n"
        "\n"
        "  w        -- Write    - Write content to a file\n"
        "  r        -- Read     - Read content from a file\n"
        "  l        -- List     - List the entries of a directory\n"
        "  d        -- Dir      - Create a new directory\n"
        "  m        -- Move     - Move an entry\n"
        "  e        -- Remove   - Remove an entry\n"
        "\n"
        "\n"
        "  You can start with 'l' and set the path to either '', '.' or '/' to see\n"
        "  every entry of the root directory.\n";

    syscall_write(0, clear_msg, sizeof(clear_msg) - 1);
    syscall_write(0, info_msg, sizeof(info_msg) - 1);

    while (1)
    {
        if (syscall_read_keyboard_event(&ev) != 0)
            continue;

        if (ev.keycode == SC_LSHIFT || ev.keycode == SC_RSHIFT)
        {
            shift_held = !ev.released;
            continue;
        }

        if (ev.released)
            continue;

        switch (ev.keycode)
        {
            case KEY_ARROW_UP:    /* handle_arrow_up();    */ continue;
            case KEY_ARROW_DOWN:  /* handle_arrow_down();  */ continue;
            case KEY_ARROW_LEFT:  /* handle_arrow_left();  */ continue;
            case KEY_ARROW_RIGHT: /* handle_arrow_right(); */ continue;
            case KEY_DELETE:      /* handle_delete();      */ continue;
            case KEY_HOME:        /* handle_home();        */ continue;
            case KEY_END:         /* handle_end();         */ continue;
            default: break;
        }

        char c = shift_held ? ascii_upper[ev.keycode] : ascii_lower[ev.keycode];

        switch (c)
        {
            case 'i': case 'I':
                syscall_write(0, info_msg, sizeof(info_msg) - 1);
                break;

            case 'h': case 'H':
                syscall_write(0, help_msg, sizeof(help_msg) - 1);
                break;


            case 'c': case 'C':
                syscall_write(0, clear_msg, sizeof(clear_msg) - 1);
                break;


            case 'w': case 'W':
                write();
                break;

            case 'r': case 'R':
                read();
                break;

            case 'l': case 'L':
                list();
                break;

            case 'd': case 'D':
                dir();
                break;

            case 'm': case 'M':
                move();
                break;

            case 'e': case 'E':
                remove();
                break;
        }
    }

    return 0;
}
