#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "cs402.h"
#include "my402list.h"

#define MAX_LINE_LEN 1024

typedef struct {
    char type;
    time_t timestamp;
    unsigned long amount_cents;
    char description[MAX_LINE_LEN];
} Transaction;

/* ----------------------- Error Handling ----------------------- */

static
void BailOut(const char *msg)
{
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(1);
}

/* ----------------------- Parsing ----------------------- */

static
void ParseLine(char *line, Transaction *txn)
{
    char *field1 = NULL, *field2 = NULL, *field3 = NULL, *field4 = NULL;
    char *tab1 = NULL, *tab2 = NULL, *tab3 = NULL;
    int tab_count = 0;
    char *p = NULL;

    /* count tabs to verify exactly 3 */
    for (p = line; *p != '\0'; p++) {
        if (*p == '\t') tab_count++;
    }
    if (tab_count != 3) {
        BailOut("malformed line (does not have exactly 3 tabs)");
    }

    /* remove trailing newline */
    p = strchr(line, '\n');
    if (p != NULL) *p = '\0';

    /* split into 4 fields by tabs */
    field1 = line;
    tab1 = strchr(field1, '\t');
    if (tab1 == NULL) BailOut("malformed line");
    *tab1 = '\0';

    field2 = tab1 + 1;
    tab2 = strchr(field2, '\t');
    if (tab2 == NULL) BailOut("malformed line");
    *tab2 = '\0';

    field3 = tab2 + 1;
    tab3 = strchr(field3, '\t');
    if (tab3 == NULL) BailOut("malformed line");
    *tab3 = '\0';

    field4 = tab3 + 1;

    /* Field 1: transaction type */
    if (strlen(field1) != 1 || (field1[0] != '+' && field1[0] != '-')) {
        BailOut("malformed line (invalid transaction type)");
    }
    txn->type = field1[0];

    /* Field 2: timestamp */
    {
        char *endptr = NULL;
        long ts = 0;

        if (strlen(field2) == 0) {
            BailOut("malformed line (empty timestamp)");
        }
        if (field2[0] == '0' && strlen(field2) > 1) {
            BailOut("malformed line (timestamp has leading zero)");
        }
        if (strlen(field2) >= 11) {
            BailOut("malformed line (timestamp too large)");
        }

        ts = strtol(field2, &endptr, 10);
        if (*endptr != '\0') {
            BailOut("malformed line (timestamp not a valid number)");
        }
        if (ts <= 0) {
            BailOut("malformed line (timestamp must be positive)");
        }
        if (ts >= (long)time(NULL)) {
            BailOut("malformed line (timestamp is in the future)");
        }
        txn->timestamp = (time_t)ts;
    }

    /* Field 3: amount */
    {
        char *dot = NULL;
        char *endptr = NULL;
        long dollars = 0;
        long cents = 0;

        if (strlen(field3) == 0) {
            BailOut("malformed line (empty amount)");
        }

        dot = strchr(field3, '.');
        if (dot == NULL) {
            BailOut("malformed line (amount missing decimal point)");
        }
        if (strlen(dot + 1) != 2) {
            BailOut("malformed line (amount must have exactly 2 digits after decimal)");
        }

        /* validate chars before dot are digits */
        {
            int num_digits = (int)(dot - field3);
            int i = 0;
            if (num_digits == 0) {
                BailOut("malformed line (no digits before decimal point)");
            }
            if (num_digits > 7) {
                BailOut("malformed line (amount too large, >7 digits before decimal)");
            }
            for (i = 0; i < num_digits; i++) {
                if (field3[i] < '0' || field3[i] > '9') {
                    BailOut("malformed line (invalid character in amount)");
                }
            }
            if (num_digits > 1 && field3[0] == '0') {
                BailOut("malformed line (amount has leading zero)");
            }
        }

        /* validate chars after dot are digits */
        if (dot[1] < '0' || dot[1] > '9' || dot[2] < '0' || dot[2] > '9') {
            BailOut("malformed line (invalid cents in amount)");
        }

        *dot = '\0';
        dollars = strtol(field3, &endptr, 10);
        if (*endptr != '\0') {
            BailOut("malformed line (invalid dollar amount)");
        }
        cents = strtol(dot + 1, &endptr, 10);
        if (*endptr != '\0') {
            BailOut("malformed line (invalid cents)");
        }

        txn->amount_cents = (unsigned long)(dollars * 100 + cents);
        if (txn->amount_cents == 0) {
            BailOut("malformed line (amount must be positive)");
        }
    }

    /* Field 4: description */
    {
        char *desc = field4;

        /* strip leading spaces */
        while (*desc == ' ') desc++;

        if (strlen(desc) == 0) {
            BailOut("malformed line (empty description)");
        }

        strncpy(txn->description, desc, sizeof(txn->description));
        txn->description[sizeof(txn->description) - 1] = '\0';
    }
}

/* ----------------------- Sorted Insertion ----------------------- */

static
void InsertSorted(My402List *list, Transaction *txn)
{
    My402ListElem *elem = NULL;

    for (elem = My402ListFirst(list);
         elem != NULL;
         elem = My402ListNext(list, elem)) {
        Transaction *cur = (Transaction *)(elem->obj);
        if (cur->timestamp == txn->timestamp) {
            BailOut("duplicate timestamp found");
        }
        if (cur->timestamp > txn->timestamp) {
            (void)My402ListInsertBefore(list, txn, elem);
            return;
        }
    }
    (void)My402ListAppend(list, txn);
}

/* ----------------------- Output Formatting ----------------------- */

static
void FormatMoney(char *buf, long amount_cents, int is_negative)
{
    /*
     * Format |amount_cents| into a string like "1,234.56" or "(1,234.56)"
     * Field is 14 characters wide (positions are right-justified in that space).
     * For amounts >= 10,000,000.00 (1,000,000,000 cents), print ?,???,???.??
     */
    char num_buf[32];
    long abs_cents = amount_cents < 0 ? -amount_cents : amount_cents;
    long dollars = 0;
    long cents = 0;

    if (is_negative) abs_cents = -amount_cents;
    else abs_cents = amount_cents;

    if (abs_cents < 0) abs_cents = -abs_cents;

    dollars = abs_cents / 100;
    cents = abs_cents % 100;

    if (dollars >= 10000000) {
        if (is_negative) {
            memcpy(buf, "(", 1);
            memcpy(buf+1, "?,???,???.??", 12);
            buf[13] = ')';
            buf[14] = '\0';
        } else {
            buf[0] = ' ';
            memcpy(buf+1, "?,???,???.??", 12);
            buf[13] = ' ';
            buf[14] = '\0';
        }
        return;
    }

    /* Format dollars with commas */
    if (dollars >= 1000000) {
        snprintf(num_buf, sizeof(num_buf), "%ld,%03ld,%03ld.%02ld",
                 dollars / 1000000, (dollars / 1000) % 1000, dollars % 1000, cents);
    } else if (dollars >= 1000) {
        snprintf(num_buf, sizeof(num_buf), "%ld,%03ld.%02ld",
                 dollars / 1000, dollars % 1000, cents);
    } else {
        snprintf(num_buf, sizeof(num_buf), "%ld.%02ld", dollars, cents);
    }

    {
        int num_len = (int)strlen(num_buf);
        int i = 0;

        memset(buf, ' ', 14);
        buf[14] = '\0';

        if (is_negative) {
            buf[0] = '(';
            buf[13] = ')';
            for (i = 0; i < num_len; i++) {
                buf[13 - num_len + i] = num_buf[i];
            }
        } else {
            for (i = 0; i < num_len; i++) {
                buf[13 - num_len + i] = num_buf[i];
            }
        }
    }
}

static
void FormatDate(char *date_buf, time_t timestamp)
{
    char ctime_buf[26];
    char *ct = ctime(&timestamp);

    strncpy(ctime_buf, ct, sizeof(ctime_buf));
    ctime_buf[25] = '\0';

    /*
     * ctime format: "Thu Aug 21 12:00:00 2008\n"
     *                0123456789...
     * We want: "Thu Aug 21 2008" (15 chars)
     * = chars 0-10 (day-of-week, month, day) + chars 20-23 (year)
     */
    date_buf[0]  = ctime_buf[0];
    date_buf[1]  = ctime_buf[1];
    date_buf[2]  = ctime_buf[2];
    date_buf[3]  = ctime_buf[3];
    date_buf[4]  = ctime_buf[4];
    date_buf[5]  = ctime_buf[5];
    date_buf[6]  = ctime_buf[6];
    date_buf[7]  = ctime_buf[7];
    date_buf[8]  = ctime_buf[8];
    date_buf[9]  = ctime_buf[9];
    date_buf[10] = ' ';
    date_buf[11] = ctime_buf[20];
    date_buf[12] = ctime_buf[21];
    date_buf[13] = ctime_buf[22];
    date_buf[14] = ctime_buf[23];
    date_buf[15] = '\0';
}

static
void PrintLine()
{
    printf("+-----------------+--------------------------+----------------+----------------+\n");
}

static
void PrintHeader()
{
    PrintLine();
    printf("|       Date      | Description              |         Amount |        Balance |\n");
    PrintLine();
}

static
void PrintRow(Transaction *txn, long balance_cents)
{
    char date_buf[16];
    char desc_buf[25];
    char amount_buf[15];
    char balance_buf[15];
    int is_withdrawal = 0;
    int balance_negative = 0;

    memset(date_buf, 0, sizeof(date_buf));
    memset(desc_buf, 0, sizeof(desc_buf));
    memset(amount_buf, 0, sizeof(amount_buf));
    memset(balance_buf, 0, sizeof(balance_buf));

    /* Format date */
    FormatDate(date_buf, txn->timestamp);

    /* Format description (truncate to 24 chars, pad with spaces) */
    {
        int desc_len = (int)strlen(txn->description);
        if (desc_len > 24) desc_len = 24;
        strncpy(desc_buf, txn->description, (size_t)desc_len);
        desc_buf[desc_len] = '\0';
    }

    /* Format amount */
    is_withdrawal = (txn->type == '-');
    FormatMoney(amount_buf, (long)txn->amount_cents, is_withdrawal);

    /* Format balance */
    balance_negative = (balance_cents < 0);
    FormatMoney(balance_buf, balance_cents, balance_negative);

    printf("| %s | %-24s | %s | %s |\n",
           date_buf, desc_buf, amount_buf, balance_buf);
}

/* ----------------------- Process ----------------------- */

static
void ProcessFile(FILE *fp)
{
    My402List list;
    char line[MAX_LINE_LEN + 2];
    long balance_cents = 0;
    My402ListElem *elem = NULL;
    int line_count = 0;

    memset(&list, 0, sizeof(My402List));
    (void)My402ListInit(&list);

    while (fgets(line, sizeof(line), fp) != NULL) {
        Transaction *txn = NULL;
        int len = (int)strlen(line);

        if (len > MAX_LINE_LEN) {
            BailOut("input line too long (exceeds 1024 characters)");
        }

        txn = (Transaction *)malloc(sizeof(Transaction));
        if (txn == NULL) {
            BailOut("malloc failed");
        }
        memset(txn, 0, sizeof(Transaction));

        ParseLine(line, txn);
        InsertSorted(&list, txn);
        line_count++;
    }

    if (line_count == 0) {
        BailOut("empty input file (must contain at least one transaction)");
    }

    PrintHeader();

    for (elem = My402ListFirst(&list);
         elem != NULL;
         elem = My402ListNext(&list, elem)) {
        Transaction *txn = (Transaction *)(elem->obj);

        if (txn->type == '+') {
            balance_cents += (long)txn->amount_cents;
        } else {
            balance_cents -= (long)txn->amount_cents;
        }
        PrintRow(txn, balance_cents);
    }

    PrintLine();

    /* cleanup */
    for (elem = My402ListFirst(&list);
         elem != NULL;
         elem = My402ListNext(&list, elem)) {
        free(elem->obj);
    }
    My402ListUnlinkAll(&list);
}

/* ----------------------- main ----------------------- */

int main(int argc, char *argv[])
{
    FILE *fp = NULL;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: warmup1 sort [tfile]\n");
        exit(1);
    }

    if (strcmp(argv[1], "sort") != 0) {
        fprintf(stderr, "ERROR: unknown command '%s'\n", argv[1]);
        fprintf(stderr, "usage: warmup1 sort [tfile]\n");
        exit(1);
    }

    if (argc == 3) {
        struct stat st;

        if (stat(argv[2], &st) != 0) {
            fprintf(stderr, "ERROR: cannot access file '%s'\n", argv[2]);
            exit(1);
        }
        if (S_ISDIR(st.st_mode)) {
            fprintf(stderr, "ERROR: '%s' is a directory, not a file\n", argv[2]);
            exit(1);
        }

        fp = fopen(argv[2], "r");
        if (fp == NULL) {
            fprintf(stderr, "ERROR: cannot open file '%s'\n", argv[2]);
            exit(1);
        }
    } else {
        fp = stdin;
    }

    ProcessFile(fp);

    if (fp != stdin) {
        fclose(fp);
    }

    return 0;
}
