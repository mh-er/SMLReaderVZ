#include "smlDebug.h"
#include "unit.h"
/* ***
# Description smlDebug.cpp #
smlDebug.cpp implementing debug output of the SML file message structure

2023-01-27 mh
- rename file from debug to smlDebug due to name collision with framework include

2022-11-30   mh
- adapted for own use based on https://github.com/mruettgers/SMLReader
- modularization of code (own cpp instead of monolithic include of code into main.cpp)

(C) M. Herbert, 2022.
Licensed under the GNU General Public License v3.0

*** end change log *** */

void DEBUG_DUMP_BUFFER(byte *buf, int size)
{
#if (defined(SERIAL_DEBUG_VERBOSE) && SERIAL_DEBUG_VERBOSE)
    DEBUG("----DATA----");
    for (int i = 0; i < size; i++)
    {
        if (buf[i] < 16)
        {
            SERIAL_DEBUG_IMPL.print("0");
        }
        SERIAL_DEBUG_IMPL.print(buf[i], HEX);
        SERIAL_DEBUG_IMPL.print(" ");
        if (((i + 1) % 16) == 0)
        {
            SERIAL_DEBUG_IMPL.println();
        }
    }
    SERIAL_DEBUG_IMPL.println();
    DEBUG("---END OF DATA---");
#endif
}
void DEBUG_SML_FILE(sml_file *file)
{
#if (defined(SERIAL_DEBUG) && SERIAL_DEBUG)

    sml_file_print(file);

    // read here some values ...
    printf("OBIS data\n");
    for (int i = 0; i < file->messages_len; i++)
    {
        sml_message *message = file->messages[i];
        if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE)
        {
            sml_list *entry;
            sml_get_list_response *body;
            body = (sml_get_list_response *)message->message_body->data;
            for (entry = body->val_list; entry != NULL; entry = entry->next)
            {
                if (!entry->value)
                { // do not crash on null value
                    fprintf(stderr, "Error in data stream. entry->value should not be NULL. Skipping this.\n");
                    continue;
                }
                if (entry->value->type == SML_TYPE_OCTET_STRING)
                {
                    char *str;
                    printf("%d-%d:%d.%d.%d*%d#%s#\n",
                           entry->obj_name->str[0], entry->obj_name->str[1],
                           entry->obj_name->str[2], entry->obj_name->str[3],
                           entry->obj_name->str[4], entry->obj_name->str[5],
                           sml_value_to_strhex(entry->value, &str, true));
                    free(str);
                }
                else if (entry->value->type == SML_TYPE_BOOLEAN)
                {
                    printf("%d-%d:%d.%d.%d*%d#%s#\n",
                           entry->obj_name->str[0], entry->obj_name->str[1],
                           entry->obj_name->str[2], entry->obj_name->str[3],
                           entry->obj_name->str[4], entry->obj_name->str[5],
                           entry->value->data.boolean ? "true" : "false");
                }
                else if (((entry->value->type & SML_TYPE_FIELD) == SML_TYPE_INTEGER) ||
                         ((entry->value->type & SML_TYPE_FIELD) == SML_TYPE_UNSIGNED))
                {
                    double value = sml_value_to_double(entry->value);
                    int scaler = (entry->scaler) ? *entry->scaler : 0;
                    int prec = -scaler;
                    if (prec < 0)
                        prec = 0;
                    value = value * pow(10, scaler);
                    printf("%d-%d:%d.%d.%d*%d#%.*f#",
                           entry->obj_name->str[0], entry->obj_name->str[1],
                           entry->obj_name->str[2], entry->obj_name->str[3],
                           entry->obj_name->str[4], entry->obj_name->str[5], prec, value);
                    const char *unit = NULL;
                    if (entry->unit && // do not crash on null (unit is optional)
                        (unit = dlms_get_unit((unsigned char)*entry->unit)) != NULL)
                        printf("%s", unit);
                    printf("\n");
                    // flush the stdout puffer, that pipes work without waiting
                    fflush(stdout);
                }
            }
        }
    }
#endif
}