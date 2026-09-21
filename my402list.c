#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cs402.h"
#include "my402list.h"

int My402ListInit(My402List *list)
{
    if (list == NULL) return FALSE;

    list->num_members = 0;
    list->anchor.obj = NULL;
    list->anchor.next = &(list->anchor);
    list->anchor.prev = &(list->anchor);

    return TRUE;
}

int My402ListLength(My402List *list)
{
    return list->num_members;
}

int My402ListEmpty(My402List *list)
{
    return (list->num_members == 0);
}

My402ListElem *My402ListFirst(My402List *list)
{
    if (list->num_members == 0) return NULL;
    return list->anchor.next;
}

My402ListElem *My402ListLast(My402List *list)
{
    if (list->num_members == 0) return NULL;
    return list->anchor.prev;
}

My402ListElem *My402ListNext(My402List *list, My402ListElem *cur)
{
    if (cur->next == &(list->anchor)) return NULL;
    return cur->next;
}

My402ListElem *My402ListPrev(My402List *list, My402ListElem *cur)
{
    if (cur->prev == &(list->anchor)) return NULL;
    return cur->prev;
}

My402ListElem *My402ListFind(My402List *list, void *obj)
{
    My402ListElem *elem = NULL;

    for (elem = My402ListFirst(list);
         elem != NULL;
         elem = My402ListNext(list, elem)) {
        if (elem->obj == obj) return elem;
    }
    return NULL;
}

int My402ListAppend(My402List *list, void *obj)
{
    My402ListElem *new_elem = (My402ListElem *)malloc(sizeof(My402ListElem));
    if (new_elem == NULL) return FALSE;

    new_elem->obj = obj;

    new_elem->prev = list->anchor.prev;
    new_elem->next = &(list->anchor);
    list->anchor.prev->next = new_elem;
    list->anchor.prev = new_elem;

    list->num_members++;
    return TRUE;
}

int My402ListPrepend(My402List *list, void *obj)
{
    My402ListElem *new_elem = (My402ListElem *)malloc(sizeof(My402ListElem));
    if (new_elem == NULL) return FALSE;

    new_elem->obj = obj;

    new_elem->next = list->anchor.next;
    new_elem->prev = &(list->anchor);
    list->anchor.next->prev = new_elem;
    list->anchor.next = new_elem;

    list->num_members++;
    return TRUE;
}

void My402ListUnlink(My402List *list, My402ListElem *elem)
{
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;

    list->num_members--;
    free(elem);
}

void My402ListUnlinkAll(My402List *list)
{
    My402ListElem *cur = list->anchor.next;

    while (cur != &(list->anchor)) {
        My402ListElem *next = cur->next;
        free(cur);
        cur = next;
    }
    list->anchor.next = &(list->anchor);
    list->anchor.prev = &(list->anchor);
    list->num_members = 0;
}

int My402ListInsertAfter(My402List *list, void *obj, My402ListElem *elem)
{
    if (elem == NULL) return My402ListAppend(list, obj);

    My402ListElem *new_elem = (My402ListElem *)malloc(sizeof(My402ListElem));
    if (new_elem == NULL) return FALSE;

    new_elem->obj = obj;

    new_elem->next = elem->next;
    new_elem->prev = elem;
    elem->next->prev = new_elem;
    elem->next = new_elem;

    list->num_members++;
    return TRUE;
}

int My402ListInsertBefore(My402List *list, void *obj, My402ListElem *elem)
{
    if (elem == NULL) return My402ListPrepend(list, obj);

    My402ListElem *new_elem = (My402ListElem *)malloc(sizeof(My402ListElem));
    if (new_elem == NULL) return FALSE;

    new_elem->obj = obj;

    new_elem->prev = elem->prev;
    new_elem->next = elem;
    elem->prev->next = new_elem;
    elem->prev = new_elem;

    list->num_members++;
    return TRUE;
}
