#include "SortedList.h"
#include <string.h>
#include <sched.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
    if (list == NULL || element == NULL)
    {
        return;
    }
    SortedListElement_t *curr = list -> next;
    while (curr != list)
    {
        if (strcmp(element->key, curr->key) <= 0)
        {
            break;
        }
        curr = curr -> next;
    }
    if (opt_yield & INSERT_YIELD)
    {
        sched_yield();
    }
    element -> next = curr;
    element -> prev = curr -> prev;
    curr->prev->next = element;
    curr->prev = element;
    
    
}

int SortedList_delete( SortedListElement_t *element)
{
    if(element == NULL)
    {
        return 1;
    }
    if (element->next->prev != element || element->prev->next != element)
    {    return 1;}
    if(opt_yield & DELETE_YIELD)
    {    sched_yield();}
    
    element->prev->next = element->next;
    element->next->prev = element->prev;
    
    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
    if (list == NULL || key == NULL)
    {
        return NULL;
    }
    SortedListElement_t* curr = list -> next;
    while (curr != list)
    {
        if(strcmp(curr->key, key) == 0)
        {
            return curr;
        }
        if (opt_yield & LOOKUP_YIELD)
        {
            sched_yield();
        }
        curr = curr->next;
    }
    return NULL;
}

int SortedList_length(SortedList_t *list){
    if(list == NULL)
    {
        return -1;
    }
    int count = 0;
    SortedListElement_t* curr = list->next;
    while(curr != list)
    {
        count++;
        if(opt_yield & LOOKUP_YIELD)
        {
            sched_yield();
        }
        curr = curr -> next;
    }
    return count;
}



